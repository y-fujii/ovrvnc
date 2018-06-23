// (c) Yasuhiro Fujii <http://mimosa-pudica.net>, under MIT License.
#pragma once

#include <vector>
#include <string>
#define CPPTOML_NO_RTTI
#include <cpptoml.h>


struct config_t {
	struct screen_t {
		std::string host;
		int         port;
		std::string password;
		float       latitude  = 0.0f;
		float       longitude = 0.0f;
	};

	float                 resolution  = 2560.0f;
	float                 color_bg[3] = { 0.0f, 0.0f, 0.0f };
	std::vector<screen_t> screens;
};

inline config_t config_load( std::string const& fn ) {
	config_t result;
	auto config = cpptoml::parse_file( fn );

	if( auto const color_bg = config->get_array_of<double>( "color_bg" ) ) {
		if( color_bg->size() == 3 ) {
			result.color_bg[0] = float( color_bg->at( 0 ) );
			result.color_bg[1] = float( color_bg->at( 1 ) );
			result.color_bg[2] = float( color_bg->at( 2 ) );
		}
	}

	if( auto const screens = config->get_table_array( "screens" ) ) {
		for( auto const& screen: *screens ) {
			result.screens.push_back( {
				screen->get_as<std::string>( "host" ).value_or( {} ),
				screen->get_as<int>( "port" ).value_or( 5900 ),
				screen->get_as<std::string>( "password" ).value_or( {} ),
				float( screen->get_as<double>( "latitude"  ).value_or( 0.0 ) ),
				float( screen->get_as<double>( "longitude" ).value_or( 0.0 ) )
			} );
		}
	}

	return result;
}
