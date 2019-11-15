#pragma once

#include <windows.h>
#include <string>

bool start_service( const std::string& driver_name, const std::string& driver_path );
bool stop_service( const std::string& driver_name );
HANDLE get_driver_handle( );