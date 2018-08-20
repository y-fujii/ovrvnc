// (c) Yasuhiro Fujii <http://mimosa-pudica.net>, under MIT License.
#pragma once

#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <sys/ioctl.h>
#include <network/TcpSocket.h>
#include <rfb/Exception.h>
#include <rfb/CConnection.h>
#include <rfb/CMsgWriter.h>
#include <rfb/PixelBuffer.h>
#include <rfb/PixelFormat.h>
#include <rfb/CSecurity.h>
#include <rfb/fenceTypes.h>

using std::swap;


struct region_t {
	std::shared_ptr<std::vector<uint32_t>> buf;
	int w  = 0;
	int h  = 0;
	int x0 = 0;
	int y0 = 0;
	int x1 = 0;
	int y1 = 0;
};

struct pixel_buffer_t: rfb::FullFramePixelBuffer {
	pixel_buffer_t( int const w, int const h ):
		rfb::FullFramePixelBuffer( { 32, 24, false, true, 255, 255, 255, 0, 8, 16 }, w, h, nullptr, w ),
		_damaged( INT_MAX, INT_MAX, 0, 0 )
	{
		buffer = std::make_shared<std::vector<uint32_t>>( w * h );
		data = reinterpret_cast<uint8_t*>( buffer->data() );
	}

	virtual void commitBufferRW( rfb::Rect const& rect ) override {
		_damaged = _damaged.union_boundary( rect );
	}

	rfb::Rect damaged() {
		rfb::Rect tmp( INT_MAX, INT_MAX, 0, 0 );
		swap( tmp, _damaged );
		return tmp;
	}

	std::shared_ptr<std::vector<uint32_t>> buffer;

private:
	rfb::Rect _damaged;
};

struct user_password_getter_t: rfb::UserPasswdGetter {
	inline static thread_local std::string pass;

	user_password_getter_t() {
		rfb::CSecurity::upg = this;
		rfb::SecurityClient::setDefaults();
	}

	virtual void getUserPasswd( bool, char**, char** const pass_ ) override {
		*pass_ = strdup( pass.c_str() );
	}
};

struct client_connection_t: rfb::CConnection {
	inline static user_password_getter_t user_password_getter;

	client_connection_t( std::string const& host, int const port, std::string pass, bool const lossy ):
		socket( host.c_str(), port ),
		_damaged( INT_MAX, INT_MAX, 0, 0 )
	{
		user_password_getter_t::pass = std::move( pass );
		cp.compressLevel = 1;
		cp.qualityLevel  = lossy ? 8 : -1;
		setStreams( &socket.inStream(), &socket.outStream() );
		initialiseProtocol();
	}

	region_t get_update_region() {
		region_t region;
		{
			std::lock_guard<std::mutex> lock( _damaged_mutex );
			if( auto fb = static_cast<pixel_buffer_t*>( getFramebuffer() ) ) {
				region.buf = fb->buffer;
				region.w   = fb->width();
				region.h   = fb->height();
				region.x0  = _damaged.tl.x;
				region.y0  = _damaged.tl.y;
				region.x1  = _damaged.br.x;
				region.y1  = _damaged.br.y;
				_damaged = { INT_MAX, INT_MAX, 0, 0 };
			}
		}
		return region;
	}

	virtual void serverInit() override {
		CConnection::serverInit();
		writer()->writeSetEncodings( rfb::encodingTight, true );
		writer()->writeFramebufferUpdateRequest( { 0, 0, cp.width, cp.height }, false );

		{
			std::lock_guard<std::mutex> lock( writer_mutex );
			writer_mt = std::unique_ptr<rfb::CMsgWriter>( writer() );
		}
		setWriter( nullptr );
	}

	// note: also called on serverInit message.
	virtual void setDesktopSize( int const w, int const h ) override {
		CConnection::setDesktopSize( w, h );
		_resize();
	}

	virtual void setExtendedDesktopSize( unsigned const reason, unsigned const result, int const w, int const h, rfb::ScreenSet const& layout ) override {
		CConnection::setExtendedDesktopSize( reason, result, w, h, layout );
		_resize();
	}

	virtual void endOfContinuousUpdates() override {
		CConnection::endOfContinuousUpdates(); // cp.supportsContinuousUpdates = true.

		std::lock_guard<std::mutex> lock( writer_mutex );
		writer_mt->writeEnableContinuousUpdates( true, 0, 0, cp.width, cp.height );
	}

	virtual void framebufferUpdateStart() override {
		CConnection::framebufferUpdateStart();

		if( !cp.supportsContinuousUpdates ) {
			std::lock_guard<std::mutex> lock( writer_mutex );
			writer_mt->writeFramebufferUpdateRequest( { 0, 0, cp.width, cp.height }, true );
		}
	}

	virtual void framebufferUpdateEnd() override {
		CConnection::framebufferUpdateEnd();

		rfb::Rect new_damaged = static_cast<pixel_buffer_t*>( getFramebuffer() )->damaged();
		{
			std::lock_guard<std::mutex> lock( _damaged_mutex );
			_damaged = _damaged.union_boundary( new_damaged );
		}
	}

	// XXX
	virtual void fence( rdr::U32 const flags, unsigned const len, char const* const data ) override {
		CMsgHandler::fence( flags, len, data );

		if( flags & rfb::fenceFlagRequest ) {
			std::lock_guard<std::mutex> lock( writer_mutex );
			writer_mt->writeFence( flags & ~rfb::fenceFlagRequest, len, data );
		}
	}

	virtual void setColourMapEntries( int, int, rdr::U16* ) override {}
	virtual void bell() override {}
	virtual void serverCutText( char const*, rdr::U32 ) override {}
	virtual void setCursor( int, int, const rfb::Point&, rdr::U8 const* ) override {}

	network::TcpSocket               socket;
	std::mutex                       writer_mutex;
	std::unique_ptr<rfb::CMsgWriter> writer_mt;

private:
	void _resize() {
		{
			auto pb = std::make_unique<pixel_buffer_t>( cp.width, cp.height );
			std::lock_guard<std::mutex> lock( _damaged_mutex );
			setFramebuffer( pb.release() );
			_damaged = { INT_MAX, INT_MAX, 0, 0 };
		}

		if( cp.supportsContinuousUpdates ) {
			assert( state() == RFBSTATE_NORMAL );
			std::lock_guard<std::mutex> lock( writer_mutex );
			writer_mt->writeEnableContinuousUpdates( true, 0, 0, cp.width, cp.height );
		}
	}

	std::mutex _damaged_mutex;
	rfb::Rect  _damaged;
};

struct vnc_thread_t {
	region_t get_update_region() {
		if( std::shared_ptr<client_connection_t> c = _connection ) {
			return c->get_update_region();
		}
		else {
			return {};
		}
	}

	void run( std::string host, int const port, std::string pass, bool const lossy ) {
		std::thread( &vnc_thread_t::_process, this, std::move( host ), port, std::move( pass ), lossy ).detach();
	}

	void push_mouse_event( uint16_t const x, uint16_t const y, bool const b0, bool const b1 ) {
		if( std::shared_ptr<client_connection_t> c = _connection ) {
			std::lock_guard<std::mutex> lock( c->writer_mutex );
			if( c->writer_mt != nullptr ) {
				int remaining = 0;
				if( ioctl( c->socket.getFd(), TIOCOUTQ, &remaining ) < 0 || remaining >= 2048 ) {
					__android_log_print( ANDROID_LOG_INFO, "ovrvnc", "ioctl()" );
					return;
				}
				try {
					c->writer_mt->writePointerEvent( { x, y }, (b0 ? 1 : 0) | (b1 ? 4 : 0) );
				}
				catch( rfb::Exception const& e ) {
					__android_log_print( ANDROID_LOG_INFO, "ovrvnc", "%s", e.str() );
				}
			}
		}
	}

private:
	void _process( std::string const host, int const port, std::string const pass, bool const lossy ) {
		while( true ) {
			try {
				_connection = std::make_shared<client_connection_t>( host, port, pass, lossy );
				while( true ) {
					_connection->processMsg();
				}
			}
			catch( rfb::Exception const& e ) {
				__android_log_print( ANDROID_LOG_INFO, "ovrvnc", "%s", e.str() );
			}

			_connection = nullptr;
			sleep( 1 );
		}
	}

	std::shared_ptr<client_connection_t> _connection;
};
