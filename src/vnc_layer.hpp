// (c) Yasuhiro Fujii <http://mimosa-pudica.net>, under MIT License.
#pragma once
#include <optional>
#include "vnc_thread.hpp"


namespace std {
	template<>
	struct default_delete<ovrTextureSwapChain> {
		void operator()( ovrTextureSwapChain* self ) const {
			vrapi_DestroyTextureSwapChain( self );
		}
	};
}

struct vnc_layer_t {
	void run( std::string host, int const port, std::string password, bool lossy ) {
		_thread.run( std::move( host ), port, std::move( password ), lossy );
	}

	void update() {
		region_t const region = _thread.get_update_region();
		if( region.buf == nullptr ) {
			return;
		}
		if( _screen_w != region.w || _screen_h != region.h ) {
			_screen_w = region.w;
			_screen_h = region.h;
			_screen = std::unique_ptr<ovrTextureSwapChain>(
				vrapi_CreateTextureSwapChain( VRAPI_TEXTURE_TYPE_2D, VRAPI_TEXTURE_FORMAT_8888_sRGB, region.w, region.h, 1, false )
			);
			glBindTexture( GL_TEXTURE_2D, vrapi_GetTextureSwapChainHandle( _screen.get(), 0 ) );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
			GLfloat borderColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
			glTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			//glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 2.0f );
		}
		if( region.x0 < region.x1 && region.y0 < region.y1 ) {
			uint32_t const* const buf = region.buf->data() + region.w * region.y0 + region.x0;
			int const w = region.x1 - region.x0;
			int const h = region.y1 - region.y0;
			glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
			glPixelStorei( GL_UNPACK_ROW_LENGTH, region.w );
			glBindTexture( GL_TEXTURE_2D, vrapi_GetTextureSwapChainHandle( _screen.get(), 0 ) );
			glTexSubImage2D( GL_TEXTURE_2D, 0, region.x0, region.y0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf );
		}
		glBindTexture( GL_TEXTURE_2D, 0 );
	}

	void handle_pointer( ovrTracking const& tracking, uint32_t const buttons ) {
		OVR::Matrix4f const m = transform.Inverted() * ovrMatrix4f_CreateFromQuaternion( &tracking.HeadPose.Pose.Orientation );
		float const x = m.M[0][2];
		float const y = m.M[1][2];
		float const z = m.M[2][2];
		float const u = std::atan2( x, z );
		float const v = y / std::hypot( x, z );
		int const iu = int( std::floor( float( -resolution / M_PI ) * u + 0.5f * float( _screen_w ) ) );
		int const iv = int( std::floor( float( +resolution / M_PI ) * v + 0.5f * float( _screen_h ) ) );
		if( 0 <= iu && iu < _screen_w && 0 <= iv && iv < _screen_h ) {
			bool button_0 = (buttons & ovrButton_A    ) != 0;
			bool button_1 = (buttons & ovrButton_Enter) != 0;
			_thread.push_mouse_event( iu, iv, button_0, button_1 );
		}
	}

	std::optional<ovrLayerCylinder2> layer( OVR::ovrFrameInput const& frame ) const {
		if( _screen == nullptr ) {
			return std::nullopt;
		}

		// the shape of cylinder is hard-coded to 180 deg around and 60 deg vertical FOV in SDK.
		float const sx = resolution / float( _screen_w );
		float const fy = float( std::sqrt( 3.0 ) * M_PI / 2.0 ) * float( _screen_h ) / resolution;
		OVR::Matrix4f const m_m = transform * OVR::Matrix4f::Scaling( radius, radius * fy, radius );

		ovrLayerCylinder2 layer = vrapi_DefaultLayerCylinder2();
		layer.Header.SrcBlend = VRAPI_FRAME_LAYER_BLEND_ONE;
		layer.Header.DstBlend = VRAPI_FRAME_LAYER_BLEND_SRC_ALPHA;
		layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;
		layer.HeadPose = frame.Tracking.HeadPose;
		for( size_t eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; ++eye ) {
			layer.Textures[eye].ColorSwapChain = _screen.get();
			layer.Textures[eye].SwapChainIndex = 0;

			layer.Textures[eye].TexCoordsFromTanAngles = (OVR::Matrix4f( frame.Tracking.Eye[eye].ViewMatrix ) * m_m).Inverted();
			layer.Textures[eye].TextureMatrix.M[0][0] = sx;
			layer.Textures[eye].TextureMatrix.M[0][2] = -0.5f * sx + 0.5f;
		}
		return layer;
	}

	double        resolution =   0.0f;
	double        radius     = 100.0f;
	OVR::Matrix4f transform;

private:
	std::unique_ptr<ovrTextureSwapChain> _screen;
	int                                  _screen_w = 0;
	int                                  _screen_h = 0;
	vnc_thread_t                         _thread;
};
