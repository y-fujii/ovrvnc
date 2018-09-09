// (c) Yasuhiro Fujii <http://mimosa-pudica.net>, under MIT License.
#pragma once

#include <vector>
#include <string>
#define CPPTOML_NO_RTTI
#include <cpptoml.h>


struct config_t {
	struct screen_t {
		std::string host;
		int         port        = 5900;
		std::string password;
		float       latitude    = 0.0f;
		float       longitude   = 0.0f;
		float       radius      = 1.0f;
		bool        lossy       = true;
		bool        use_pointer = true;
	};

	float                 resolution  = 2560.0f;
	std::vector<screen_t> screens;
	float                 bg_color[3] = { 0.0f, 0.0f, 0.0f };
	std::string           bg_image;
};

inline config_t config_load( std::string const& fn ) {
	config_t result;
	auto config = cpptoml::parse_file( fn );

	if( auto const bg = config->get_table( "background" ) ) {
		if( auto const color = bg->get_array_of<double>( "color" ) ) {
			if( color->size() == 3 ) {
				result.bg_color[0] = float( color->at( 0 ) );
				result.bg_color[1] = float( color->at( 1 ) );
				result.bg_color[2] = float( color->at( 2 ) );
			}
		}
		if( auto img = bg->get_as<std::string>( "image" ) ) {
			result.bg_image = std::move( *img );
		}
	}

	if( auto const screens = config->get_table_array( "screens" ) ) {
		config_t::screen_t const d;
		for( auto const& screen: *screens ) {
			result.screens.push_back( {
				screen->get_as<std::string>( "host" ).value_or( d.host ),
				screen->get_as<int>( "port" ).value_or( d.port ),
				screen->get_as<std::string>( "password" ).value_or( d.password ),
				float( screen->get_as<double>( "latitude"  ).value_or( d.latitude ) ),
				float( screen->get_as<double>( "longitude" ).value_or( d.longitude ) ),
				float( screen->get_as<double>( "radius"  ).value_or( d.radius ) ),
				screen->get_as<bool>( "lossy" ).value_or( d.lossy ),
				screen->get_as<bool>( "use_pointer" ).value_or( d.use_pointer )
			} );
		}
	}

	return result;
}
