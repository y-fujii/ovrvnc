// (c) Yasuhiro Fujii <http://mimosa-pudica.net>, under MIT License.
#pragma once

#include <memory>
#include <vector>
#include <thread>
#include <mutex>
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

	void run( std::string const& host, int const port ) {
		_thread = std::thread( &vnc_thread_t::_process, this, host, port );
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
			std::lock_guard<std::mutex> lock( _mutex );
			tmp = _region;
			_region.x0 = _region.w;
			_region.y0 = _region.h;
			_region.x1 = 0;
			_region.y1 = 0;
		}
		return tmp;
	}

private:
	static rfbBool _on_resize( rfbClient* rfb ) {
		auto const self = reinterpret_cast<vnc_thread_t*>( rfbClientGetClientData( rfb, nullptr ) );
		{
			auto& region = self->_region;
			std::lock_guard<std::mutex> lock( self->_mutex );
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
				reinterpret_cast<uint32_t*>( rfb->frameBuffer )[rfb->width * y + x] |= 0xff000000;
			}
		}
		{
			auto& region = self->_region;
			std::lock_guard<std::mutex> lock( self->_mutex );
			region.x0 = std::min( region.x0, x0 );
			region.y0 = std::min( region.y0, y0 );
			region.x1 = std::min( std::max( region.x1, x0 + w ), region.w );
			region.y1 = std::min( std::max( region.y1, y0 + h ), region.h );
		}
	}

	void _process( std::string host, int const port ) {
		while( true ) {
			rfbClient* const rfb = rfbGetClient( 8, 3, 4 );
			rfb->MallocFrameBuffer    = _on_resize;
			rfb->GotFrameBufferUpdate = _on_update;
			rfb->canHandleNewFBSize   = TRUE;
			rfb->serverHost           = strdup( host.c_str() );
			rfb->serverPort           = port;
			rfbClientSetClientData( rfb, nullptr, this );

			if( !rfbInitClient( rfb, nullptr, nullptr ) ) {
				// rfbInitClient() calls rfbClientCleanup() on failure.
				__android_log_print( ANDROID_LOG_ERROR, "ovr_vnc", "rfbInitClient" );
				sleep( 1 );
				continue;
			}

			while( true ) {
				int const i = WaitForMessage( rfb, 100'000 );
				if( i < 0 ) {
					__android_log_print( ANDROID_LOG_ERROR, "ovr_vnc", "WaitForMessage" );
					break;
				}
				if( i == 0 ) {
					continue;
				}
				if( !HandleRFBServerMessage( rfb ) ) {
					__android_log_print( ANDROID_LOG_ERROR, "ovr_vnc", "HandleRFBServerMessage" );
					break;
				}
			}
			rfbClientCleanup( rfb );
		}
	}

	region_t    _region;
	std::mutex  _mutex;
	std::thread _thread;
};
