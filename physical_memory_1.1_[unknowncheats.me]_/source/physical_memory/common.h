#pragma once

#include <windows.h>
#include <fstream>
#include <iostream>
#include <string>
#include <random>

char generate_character( );
std::string random_string( size_t length );
bool write_binary_file( const char* file, void* data, size_t size );