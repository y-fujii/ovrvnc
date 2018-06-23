// (c) Yasuhiro Fujii <http://mimosa-pudica.net>, under MIT License.
#pragma once

#include <memory>
#include <vector>
#include <queue>
#include <string>
#include <thread>
#include <mutex>
#include <poll.h>
#include <rfb/rfbclient.h>


struct vnc_thread_t {
	struct region_t {
		std::shared_ptr<std::vector<uint32_t>> buf;
		int w  = 0;
		int h  = 0;
		int x0 = 0;
		int y0 = 0;
		int x1 = 0;
		int y1 = 0;
	};

	~vnc_thread_t() {
		close( _event_pipe[1] );
		close( _event_pipe[0] );
	}

	vnc_thread_t() {
		if( pipe2( _event_pipe, O_CLOEXEC | O_NONBLOCK ) < 0 ) {
			throw std::runtime_error( "pipe" );
		}
	}

	void run( std::string host, int const port, std::string pass ) {
		_password = std::move( pass );
		_thread = std::thread( &vnc_thread_t::_process, this, std::move( host ), port );
	}

	void detach() {
		_thread.detach();
	}

	void join() {
		_thread.join();
	}

	region_t get_update_region() {
		region_t tmp;
		{
			std::lock_guard<std::mutex> lock( _region_mutex );
			tmp = _region;
			_region.x0 = _region.w;
			_region.y0 = _region.h;
			_region.x1 = 0;
			_region.y1 = 0;
		}
		return tmp;
	}

	void push_mouse_event( uint16_t x, uint16_t y, bool b0, bool b1 ) {
		_pointer_event_t ev{ x, y, (b0 ? rfbButton1Mask : 0u) | (b1 ? rfbButton3Mask : 0u) };
		// writing data less than PIPE_BUF to a pipe is atomic.
		write( _event_pipe[1], &ev, sizeof( ev ) );
	}

private:
	struct _pointer_event_t {
		uint16_t x;
		uint16_t y;
		uint32_t button;
	};

	static rfbBool _on_resize( rfbClient* rfb ) {
		auto const self = reinterpret_cast<vnc_thread_t*>( rfbClientGetClientData( rfb, nullptr ) );
		{
			auto& region = self->_region;
			std::lock_guard<std::mutex> lock( self->_region_mutex );
			region.buf = std::make_shared<std::vector<uint32_t>>( rfb->width * rfb->height, ~0u );
			region.w   = rfb->width;
			region.h   = rfb->height;
			region.x0  = 0;
			region.y0  = 0;
			region.x1  = rfb->width;
			region.y1  = rfb->height;
			rfb->frameBuffer = reinterpret_cast<uint8_t*>( region.buf->data() );
		}
		return TRUE;
	}

	static void _on_update( rfbClient* rfb, int x0, int y0, int w, int h ) {
		auto const self = reinterpret_cast<vnc_thread_t*>( rfbClientGetClientData( rfb, nullptr ) );
		// XXX
		for( int y = y0; y < y0 + h; ++y ) {
			for( int x = x0; x < x0 + w; ++x ) {
				reinterpret_cast<uint32_t*>( rfb->frameBuffer )[rfb->width * y + x] &= 0x00ffffff;
			}
		}
		{
			auto& region = self->_region;
			std::lock_guard<std::mutex> lock( self->_region_mutex );
			region.x0 = std::min( region.x0, x0 );
			region.y0 = std::min( region.y0, y0 );
			region.x1 = std::min( std::max( region.x1, x0 + w ), region.w );
			region.y1 = std::min( std::max( region.y1, y0 + h ), region.h );
		}
	}

	static char* _on_password( rfbClient* rfb ) {
		auto const self = reinterpret_cast<vnc_thread_t*>( rfbClientGetClientData( rfb, nullptr ) );
		return strdup( self->_password.c_str() );
	}

	// XXX: separating sender/receiver threads is possible with libvncserver?
	bool _process_event( rfbClient* const rfb ) {
		pollfd fds[2];
		fds[0].fd = rfb->sock;
		fds[0].events = POLLIN;
		fds[1].fd = _event_pipe[0];
		fds[1].events = POLLIN;
		if( poll( fds, 2, -1 ) < 0 ) {
			__android_log_print( ANDROID_LOG_ERROR, "ovrvnc", "poll" );
			return false;
		}

		if( (fds[1].revents & POLLIN) != 0 ) {
			pollfd fds[1];
			fds[0].fd = rfb->sock;
			fds[0].events = POLLOUT;
			if( poll( fds, 1, 0 ) < 0 ) {
				__android_log_print( ANDROID_LOG_ERROR, "ovrvnc", "poll" );
				return false;
			}
			if( (fds[0].revents & POLLOUT) != 0 ) {
				_pointer_event_t ev;
				ssize_t n = read( _event_pipe[0], &ev, sizeof( ev ) );
				// should have read atomically.
				if( n != sizeof( ev ) ) {
					__android_log_print( ANDROID_LOG_ERROR, "ovrvnc", "read" );
					return false;
				}
				if( !SendPointerEvent( rfb, ev.x, ev.y, ev.button ) ) {
					__android_log_print( ANDROID_LOG_ERROR, "ovrvnc", "SendPointerEvent" );
					return false;
				}
			}
		}

		if( (fds[0].revents & POLLIN) != 0 ) {
			if( !HandleRFBServerMessage( rfb ) ) {
				__android_log_print( ANDROID_LOG_ERROR, "ovrvnc", "HandleRFBServerMessage" );
				return false;
			}
		}

		return true;
	}

	void _process( std::string host, int const port ) {
		while( true ) {
			rfbClient* const rfb = rfbGetClient( 8, 3, 4 );
			rfb->MallocFrameBuffer     = _on_resize;
			rfb->GotFrameBufferUpdate  = _on_update;
			rfb->GetPassword           = _on_password;
			rfb->canHandleNewFBSize    = TRUE;
			rfb->serverHost            = strdup( host.c_str() );
			rfb->serverPort            = port;
			rfb->appData.enableJPEG    = false;
			rfb->appData.compressLevel = 1;
			rfbClientSetClientData( rfb, nullptr, this );

			if( !rfbInitClient( rfb, nullptr, nullptr ) ) {
				// rfbInitClient() calls rfbClientCleanup() on failure.
				__android_log_print( ANDROID_LOG_ERROR, "ovrvnc", "rfbInitClient" );
				sleep( 1 );
				continue;
			}

			while( _process_event( rfb ) );

			rfbClientCleanup( rfb );
		}
	}

	region_t                     _region;
	std::mutex                   _region_mutex;
	int                          _event_pipe[2];
	std::thread                  _thread;
	std::string                  _password;
};
