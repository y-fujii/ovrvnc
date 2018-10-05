// (c) Yasuhiro Fujii <http://mimosa-pudica.net>, under MIT License.
#include <cassert>
#include <cmath>
#include <memory>
#include <vector>
#include <unistd.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra-semi"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wgnu-anonymous-struct"
#include "VrApi_Input.h"
#include "App.h"
#include "SceneView.h"
#pragma GCC diagnostic pop
#include "vnc_layer.hpp"
#include "equirect_layer.hpp"
#include "config.hpp"


struct application_t: OVR::VrAppInterface {
	application_t( std::string const& ext_path ) {
		_config = config_load( ext_path + "/ovrvnc.toml" );
	}

	virtual void Configure( OVR::ovrSettings& settings ) override {
		settings.UseSrgbFramebuffer = true;
		settings.RenderMode         = OVR::RENDERMODE_MULTIVIEW;
		settings.TrackingTransform  = VRAPI_TRACKING_TRANSFORM_SYSTEM_CENTER_EYE_LEVEL;
		settings.CpuLevel           = 3;
		settings.GpuLevel           = 0;
	}

	virtual void EnteredVrMode( OVR::ovrIntentType const intent_type, char const*, char const*, char const* ) override {
		if( intent_type == OVR::INTENT_LAUNCH ) {
			vrapi_SetPropertyInt( app->GetJava(), VRAPI_REORIENT_HMD_ON_CONTROLLER_RECENTER, 1 );
			vrapi_SetDisplayRefreshRate( app->GetOvrMobile(), 72.0f );

			for( auto const& screen: _config.screens ) {
				auto vnc = std::make_unique<vnc_layer_t>();
				vnc->resolution = _config.resolution / screen.pixel_scaling;
				vnc->transform =
					OVR::Matrix4f::RotationY( float( M_PI / 180.0 ) * screen.longitude ) *
					OVR::Matrix4f::RotationX( float( M_PI / 180.0 ) * screen.latitude  ) *
					OVR::Matrix4f::Scaling( 10.0f );
				vnc->use_pointer = screen.use_pointer;
				vnc->run( screen.host, screen.port, screen.password, screen.lossy );
				_vnc_layers.push_back( std::move( vnc ) );
			}
			if( !_config.bg_image.empty() ) {
				try {
					_background = equirect_layer_t::load( _config.bg_image );
				}
				catch( std::exception const & ) {
				}
			}
		}
	}

	virtual OVR::ovrFrameResult Frame( OVR::ovrFrameInput const& frame ) override {
		_scene.Frame( frame );

		OVR::ovrFrameResult res;
		_scene.GetFrameMatrices( frame.FovX, frame.FovY, res.FrameMatrices );
		_scene.GenerateFrameSurfaceList( res.FrameMatrices, res.Surfaces );

		res.FrameIndex   = frame.FrameNumber;
		res.DisplayTime  = frame.PredictedDisplayTimeInSeconds;
		res.SwapInterval = app->GetSwapInterval();

		ovrTracking tracking;
		uint32_t    buttons;
		if( _get_pointer( frame.PredictedDisplayTimeInSeconds, tracking, buttons ) ) {
			for( auto const& vnc: _vnc_layers ) {
				vnc->handle_pointer( tracking, buttons );
			}
		}

		if( _background ) {
			res.Layers[res.LayerCount++].Equirect = _background.layer( frame.Tracking );
		}
		else {
			res.ClearColorBuffer = true;
			res.ClearColor       = OVR::Vector4f( _config.bg_color[0], _config.bg_color[1], _config.bg_color[2], 1.0f );

			ovrLayerProjection2& layer = res.Layers[res.LayerCount++].Projection;
			layer = vrapi_DefaultLayerProjection2();
			layer.HeadPose = frame.Tracking.HeadPose;
			for( size_t eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; ++eye ) {
				layer.Textures[eye].ColorSwapChain         = frame.ColorTextureSwapChain[eye];
				layer.Textures[eye].SwapChainIndex         = frame.TextureSwapChainIndex;
				layer.Textures[eye].TexCoordsFromTanAngles = frame.TexCoordsFromTanAngles;
			}
		}

		for( auto const& vnc: _vnc_layers ) {
			vnc->update();
			if( auto layer = vnc->layer( frame.Tracking ) ) {
				res.Layers[res.LayerCount++].Cylinder = *layer;
			}
		}

		return res;
	}

private:
	bool _get_pointer( double const time, ovrTracking& tracking, uint32_t& buttons ) {
		ovrMobile* const mobile = app->GetOvrMobile();
		for( uint32_t i = 0; true; ++i ) {
			ovrInputCapabilityHeader caps_header;
			if( vrapi_EnumerateInputDevices( mobile, i, &caps_header ) < 0 ) {
				return false;
			}

			if( (caps_header.Type & ovrControllerType_TrackedRemote) != 0 ) {
				ovrInputTrackedRemoteCapabilities caps;
				caps.Header = caps_header;
				if( vrapi_GetInputDeviceCapabilities( mobile, &caps.Header ) != ovrSuccess ) {
					continue;
				}
				if( (caps.ControllerCapabilities & ovrControllerCaps_HasOrientationTracking) == 0 ||
					(caps.ButtonCapabilities & ovrButton_A) == 0 ) {
					continue;
				}
				ovrInputStateTrackedRemote state;
				state.Header.ControllerType = ovrControllerType_TrackedRemote;
				if( vrapi_GetCurrentInputState( mobile, caps_header.DeviceID, &state.Header ) != ovrSuccess ) {
					continue;
				}
				buttons = state.Buttons;
			}
			else if( (caps_header.Type & ovrControllerType_Headset) != 0 ) {
				ovrInputHeadsetCapabilities caps;
				caps.Header = caps_header;
				if( vrapi_GetInputDeviceCapabilities( mobile, &caps.Header ) != ovrSuccess ) {
					continue;
				}
				if( (caps.ControllerCapabilities & ovrControllerCaps_HasOrientationTracking) == 0 ||
					(caps.ButtonCapabilities & ovrButton_A) == 0 ) {
					continue;
				}
				ovrInputStateHeadset state;
				state.Header.ControllerType = ovrControllerType_Headset;
				if( vrapi_GetCurrentInputState( mobile, caps_header.DeviceID, &state.Header ) != ovrSuccess ) {
					continue;
				}
				buttons = state.Buttons;
			}
			else {
				continue;
			}

			if( vrapi_GetInputTrackingState( mobile, caps_header.DeviceID, time, &tracking ) != ovrSuccess ) {
				continue;
			}
			if( (tracking.Status & VRAPI_TRACKING_STATUS_ORIENTATION_TRACKED) == 0 ) {
				continue;
			}

			return true;
		}
	}

	config_t                                  _config;
	OVR::OvrSceneView                         _scene;
	std::vector<std::unique_ptr<vnc_layer_t>> _vnc_layers;
	equirect_layer_t                          _background;
};

#if defined( OVR_OS_ANDROID )
extern "C" jlong Java_net_mimosa_1pudica_ovrvnc_MainActivity_nativeSetAppInterface(
	JNIEnv* jni, jclass clazz, jobject activity, jstring package, jstring command, jstring uri, jstring ext_path
) {
	char const* const ext_path_c = jni->GetStringUTFChars( ext_path, nullptr );
	application_t* const app = new application_t( ext_path_c );
	jni->ReleaseStringUTFChars( ext_path, ext_path_c );
	return app->SetActivity( jni, clazz, activity, package, command, uri );
}
#endif
