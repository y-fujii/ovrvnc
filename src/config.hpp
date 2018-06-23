// (c) Yasuhiro Fujii <http://mimosa-pudica.net>, under MIT License.
#pragma once

#include <string>
#define CPPTOML_NO_RTTI
#include <cpptoml.h>


struct config_t {
	float       resolution = 2560.0f;
	std::string host;
	int         port;
	std::string password;
	float       latitude    = 0.0f;
	float       longitude   = 0.0f;
	float       color_bg[3] = { 0.0f, 0.0f, 0.0f };
};

inline config_t config_load( std::string const& fn ) {
	auto config = cpptoml::parse_file( fn );

	config_t result;
	result.host     = config->get_as<std::string>( "host" ).value_or( {} );
	result.port     = config->get_as<int>( "port" ).value_or( 5900 );
	result.password = config->get_as<std::string>( "password" ).value_or( {} );
	result.latitude  = float( config->get_as<double>( "latitude"  ).value_or( 0.0 ) );
	result.longitude = float( config->get_as<double>( "longitude" ).value_or( 0.0 ) );
	if( auto const color_bg = config->get_array_of<double>( "color_bg" ) ) {
		if( color_bg->size() == 3 ) {
			result.color_bg[0] = float( color_bg->at( 0 ) );
			result.color_bg[1] = float( color_bg->at( 1 ) );
			result.color_bg[2] = float( color_bg->at( 2 ) );
		}
	}

	return result;
}
