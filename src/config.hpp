// (c) Yasuhiro Fujii <http://mimosa-pudica.net>, under MIT License.
#pragma once

#include <string>
#define CPPTOML_NO_RTTI
#include <cpptoml.h>


struct config_t {
	float         resolution = 2560.0f;
	std::string   host;
	int           port;
	std::string   password;
	OVR::Vector3f color_bg;
	std::string   model_path;
	OVR::Vector3f model_translation;
	OVR::Vector3f model_euler_zyx;
	OVR::Vector3f model_scaling = OVR::Vector3f( 1.0f );
};

inline config_t config_load( std::string const& fn ) {
	auto config = cpptoml::parse_file( fn );

	config_t result;
	result.host     = config->get_as<std::string>( "host" ).value_or( {} );
	result.port     = config->get_as<int>( "port" ).value_or( 5900 );
	result.password = config->get_as<std::string>( "password" ).value_or( {} );
	auto const color_bg = config->get_array_of<double>( "color_bg" );
	if( color_bg && color_bg->size() == 3 ) {
		result.color_bg.x = float( color_bg->at( 0 ) );
		result.color_bg.y = float( color_bg->at( 1 ) );
		result.color_bg.z = float( color_bg->at( 2 ) );
	}
	if( auto const model = config->get_table( "model" ) ) {
		result.model_path = model->get_as<std::string>( "path" ).value_or( {} );
		auto const t = model->get_array_of<double>( "translation" );
		if( t && t->size() == 3 ) {
			result.model_translation.x = float( t->at( 0 ) );
			result.model_translation.y = float( t->at( 1 ) );
			result.model_translation.z = float( t->at( 2 ) );
		}
		auto const r = model->get_array_of<double>( "rotation" );
		if( r && r->size() == 3 ) {
			result.model_euler_zyx.x = float( r->at( 0 ) );
			result.model_euler_zyx.y = float( r->at( 1 ) );
			result.model_euler_zyx.z = float( r->at( 2 ) );
		}
		auto const s = model->get_array_of<double>( "scaling" );
		if( s && s->size() == 3 ) {
			result.model_scaling.x = float( s->at( 0 ) );
			result.model_scaling.y = float( s->at( 1 ) );
			result.model_scaling.z = float( s->at( 2 ) );
		}
	}

	return result;
}
