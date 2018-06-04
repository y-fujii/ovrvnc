// (c) Yasuhiro Fujii <http://mimosa-pudica.net>, under MIT License.
#pragma once

#include <string>
#include <cpptoml.h>


struct config_t {
	float       resolution = 2560.0f;
	std::string host;
	int         port;
};

inline config_t config_load( std::string const& fn ) {
	auto config = cpptoml::parse_file( fn );

	config_t result;
	result.host = config->get_as<std::string>( "host" ).value_or( {} );
	result.port = config->get_as<int>( "port" ).value_or( 5900 );

	return result;
}
