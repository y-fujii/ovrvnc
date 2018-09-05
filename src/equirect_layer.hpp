// (c) Yasuhiro Fujii <http://mimosa-pudica.net>, under MIT License.
#pragma once
#include <stb_image.h>


struct equirect_layer_t {
	static equirect_layer_t load( std::string const& path ) {
		int w, h;
		uint8_t* img = stbi_load( path.c_str(), &w, &h, nullptr, 4 );
		if( img == nullptr ) {
			throw std::runtime_error( "" );
		}

		equirect_layer_t layer;
		layer._image = std::unique_ptr<ovrTextureSwapChain>( vrapi_CreateTextureSwapChain3(
			VRAPI_TEXTURE_TYPE_2D, GL_SRGB8_ALPHA8, w, h, 1, 1
		) );
		glBindTexture( GL_TEXTURE_2D, vrapi_GetTextureSwapChainHandle( layer._image.get(), 0 ) );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
		glPixelStorei( GL_UNPACK_ROW_LENGTH, w );
		glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, img );
		glBindTexture( GL_TEXTURE_2D, 0 );

		stbi_image_free( img );
		return layer;
	}

	operator bool() const {
		return _image != nullptr;
	}

	ovrLayerEquirect2 layer( ovrTracking2 const& tracking ) const {
		ovrLayerEquirect2 layer = vrapi_DefaultLayerEquirect2();
		layer.HeadPose.Pose.Position    = { 0.0f, 0.0f, 0.0f };
		layer.HeadPose.Pose.Orientation = { 0.0f, 0.0f, 0.0f, 1.0f };
		layer.TexCoordsFromTanAngles    = ovrMatrix4f_CreateIdentity();
		for( size_t eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; ++eye ) {
			layer.Textures[eye].ColorSwapChain = _image.get();
			layer.Textures[eye].SwapChainIndex = 0;
		}

		return layer;
	}

private:
	std::unique_ptr<ovrTextureSwapChain> _image;
};
