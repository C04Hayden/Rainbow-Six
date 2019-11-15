#include "common.h"

char generate_character( )
{
	std::random_device random_device;
	std::mt19937 mersenne_generator( random_device( ) );
	std::uniform_int_distribution<> distribution( 97, 122 );

	return static_cast< unsigned char >( distribution( mersenne_generator ) );
}

std::string random_string( size_t length )
{
	std::string str( length, 0 );
	std::generate_n( str.begin( ), length, generate_character );
	return str;
}

bool write_binary_file( const char* file, void* data, size_t size )
{
	std::ofstream fout( file, std::ios::out | std::ios::binary );

	if ( !fout.is_open( ) )
		return false;

	fout.write( reinterpret_cast<char*>( data ), size );
	fout.close( );

	return true;
}