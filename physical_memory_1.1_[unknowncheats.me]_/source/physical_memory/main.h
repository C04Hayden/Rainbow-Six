#pragma once

#include <filesystem>

const std::filesystem::path g_driver_path = "driver/gdrv.sys";

std::string g_temp_name;
std::filesystem::path g_temp_driver_path;

HANDLE g_driver = nullptr;

int main( void );
bool poc_test( );
bool load_driver( );
void unload_driver( );

BOOL WINAPI control_handler( DWORD fdwCtrlType );