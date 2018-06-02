// (c) Yasuhiro Fujii <http://mimosa-pudica.net>, under MIT License.
#include <cassert>
#include <cmath>
#include <memory>
#include <vector>
#include <unistd.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra-semi"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "App.h"
#include "SceneView.h"
#include "GuiSys.h"
#include "OVR_Locale.h"
#pragma GCC diagnostic pop
#include "vnc_thread.hpp"


namespace std {
	template<>
	struct default_delete<ovrTextureSwapChain> {
		void operator()( ovrTextureSwapChain* self ) const {
			vrapi_DestroyTextureSwapChain( self );
		}
	};

	template<>
	struct default_delete<OVR::OvrGuiSys> {
		void operator()( OVR::OvrGuiSys* self ) const {
			OVR::OvrGuiSys::Destroy( self );
		}
	};
};

struct application_t: OVR::VrAppInterface {
	inline static std::string const host = "192.168.179.3";
	inline static int const         port = 5900;

	application_t() {
		_gui = std::unique_ptr<OVR::OvrGuiSys>( OVR::OvrGuiSys::Create() );
	}

	virtual void Configure( OVR::ovrSettings& settings ) override {
		settings.UseSrgbFramebuffer = true;
	}

	virtual void EnteredVrMode( OVR::ovrIntentType const intent_type, char const*, char const*, char const* ) override {
		if( intent_type == OVR::INTENT_LAUNCH ) {
			vrapi_SetDisplayRefreshRate( app->GetOvrMobile(), 72.0f );
			_init_gui();
			_vnc_thread.run( host, port );
		}
	}

	virtual OVR::ovrFrameResult Frame( OVR::ovrFrameInput const& frame ) override {
		for( int i = 0; i < frame.Input.NumKeyEvents; ++i ) {
			auto const& ev = frame.Input.KeyEvents[i];
			if( _gui->OnKeyEvent( ev.KeyCode, ev.RepeatCount, ev.EventType ) ) {
				continue;
			}
			if( ev.KeyCode == OVR::OVR_KEY_BACK && ev.EventType == OVR::KEY_EVENT_SHORT_PRESS ) {
				app->ShowSystemUI( VRAPI_SYS_UI_CONFIRM_QUIT_MENU );
				continue;
			}
		}

		_scene.Frame( frame );

		OVR::ovrFrameResult res;
		_scene.GetFrameMatrices( frame.FovX, frame.FovY, res.FrameMatrices );
		_scene.GenerateFrameSurfaceList( res.FrameMatrices, res.Surfaces );

		_gui->Frame( frame, res.FrameMatrices.CenterView );
		_gui->AppendSurfaceList( res.FrameMatrices.CenterView, &res.Surfaces );

		res.FrameIndex   = frame.FrameNumber;
		res.DisplayTime  = frame.PredictedDisplayTimeInSeconds;
		res.SwapInterval = app->GetSwapInterval();

		_sync_screen();

		/* projection layer (currently unused). */ {
			ovrLayerProjection2& layer = res.Layers[res.LayerCount++].Projection;
			layer = vrapi_DefaultLayerProjection2();
			layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;
			layer.HeadPose = frame.Tracking.HeadPose;
			for( size_t eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; ++eye ) {
				layer.Textures[eye].ColorSwapChain         = frame.ColorTextureSwapChain[eye];
				layer.Textures[eye].SwapChainIndex         = frame.TextureSwapChainIndex;
				layer.Textures[eye].TexCoordsFromTanAngles = frame.TexCoordsFromTanAngles;
			}
		}

		if( _screen != nullptr ) {
			ovrMatrix4f const m_m = ovrMatrix4f_CreateScale( 100.0f, 100.0f, 100.0f );

			ovrLayerCylinder2& layer = res.Layers[res.LayerCount++].Cylinder;
			layer = vrapi_DefaultLayerCylinder2();
			layer.Header.SrcBlend = VRAPI_FRAME_LAYER_BLEND_ONE;
			layer.Header.DstBlend = VRAPI_FRAME_LAYER_BLEND_ONE_MINUS_SRC_ALPHA;
			layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;
			layer.HeadPose = frame.Tracking.HeadPose;
			for( size_t eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; ++eye ) {
				layer.Textures[eye].ColorSwapChain         = _screen.get();
				layer.Textures[eye].SwapChainIndex         = 0;

				ovrMatrix4f const m_mv = ovrMatrix4f_Multiply( &frame.Tracking.Eye[eye].ViewMatrix, &m_m );
				layer.Textures[eye].TexCoordsFromTanAngles = ovrMatrix4f_Inverse( &m_mv );

				// the shape of cylinder is hard-coded to 180 deg around and 60 deg vertical FOV in SDK.
				float const sx = 2560.0f / float( _screen_w );
				float const sy = float( 2560.0 * 2.0 / (std::sqrt( 3.0 ) * M_PI) ) / float( _screen_h );
				layer.Textures[eye].TextureMatrix.M[0][0] = sx;
				layer.Textures[eye].TextureMatrix.M[1][1] = sy;
				layer.Textures[eye].TextureMatrix.M[0][2] = -0.5f * sx + 0.5f;
				layer.Textures[eye].TextureMatrix.M[1][2] = -0.5f * sy + 0.5f;
			}
		}

		return res;
	}

private:
	void _init_gui() {
		ovrJava const* const java = app->GetJava();
		OVR::ovrLocale* locale = OVR::ovrLocale::Create( *java->Env, java->ActivityObject, "default" );
		OVR::String font_name;
		locale->GetString( "@string/font_name", "efigs.fnt", font_name );
		OVR::ovrLocale::Destroy( locale );

		_gui->Init( app, _sound_player, font_name.ToCStr(), &app->GetDebugLines() );
	}

	void _sync_screen() {
		vnc_thread_t::region_t const region = _vnc_thread.get_update_region();
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
			GLfloat borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
			glTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f );
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

	OVR::OvrGuiSys::ovrDummySoundEffectPlayer _sound_player;
	std::unique_ptr<OVR::OvrGuiSys>           _gui;
	OVR::OvrSceneView                         _scene;
	std::unique_ptr<ovrTextureSwapChain>      _screen;
	int                                       _screen_w = 0;
	int                                       _screen_h = 0;
	vnc_thread_t                              _vnc_thread;
};

#if defined( OVR_OS_ANDROID )
extern "C" jlong Java_net_mimosa_1pudica_ovr_1vnc_MainActivity_nativeSetAppInterface(
	JNIEnv* jni, jclass clazz, jobject activity, jstring package, jstring command, jstring uri
) {
	return (new application_t())->SetActivity( jni, clazz, activity, package, command, uri );
}
#endif
