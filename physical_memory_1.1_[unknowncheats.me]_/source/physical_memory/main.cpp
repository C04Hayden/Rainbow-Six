#include <windows.h>
#include <filesystem>
#include <stdio.h>

#include "main.h"
#include "common.h"
#include "service.h"

#include "game\game_test.h"
#include "kernel\kprocess.h"

int main( void )
{
	SetConsoleTitle( "" );
	SetConsoleCtrlHandler( control_handler, TRUE );
	
	if ( load_driver( ) )
	{
		poc_test( );
		control_handler( 0 );
	}

	printf( "\n[*] press f12 to exit\n" );

	while ( ( GetAsyncKeyState( VK_F12 ) & 1 ) == 0 )
		Sleep( 100 );
}

/* Purpose: locates the game's running process
			enumerates players with different team id
			enables spotted marker in their components list
*/


bool esp( uintptr_t pml4, uintptr_t base_address )
{
	std::vector<uintptr_t> enemy_players;
	get_enemy_players( g_driver, pml4, base_address, enemy_players );
	printf( "[+] players found: %llu\n", enemy_players.size( ) );

	std::vector<uintptr_t> enemy_marker_components;

	for ( uintptr_t player : enemy_players )
	{
		printf( "    %llx\n", player );
		uintptr_t marker_component = get_spotted_marker( g_driver, pml4, base_address, player );

		if ( marker_component )
			enemy_marker_components.push_back( marker_component );
	}

	for ( uintptr_t marker : enemy_marker_components )
	{
		unsigned int enable = 65793;
		write_virtual_memory( g_driver, pml4, marker + ENTITY_MARKER_ENABLED_OFFSET, &enable, sizeof( unsigned int ) );
	}

	return true;
}

bool no_recoil( uintptr_t pml4, uintptr_t base_address, uintptr_t local_player )
{
	uintptr_t lpVisualCompUnk = read_virtual_memory<uintptr_t>( g_driver, pml4, local_player + 0x98 );

	if ( !lpVisualCompUnk )
		return false;

	uintptr_t lpWeapon = read_virtual_memory<uintptr_t>( g_driver, pml4, lpVisualCompUnk + 0xC8 );

	if ( !lpWeapon )
		return false;

	uintptr_t lpCurrentDisplayWeapon = read_virtual_memory<uintptr_t>( g_driver, pml4, lpWeapon + 0x208 );

	if ( !lpCurrentDisplayWeapon )
		return false;

	float almost_zero = 0.002f;
	write_virtual_memory( g_driver, pml4, lpCurrentDisplayWeapon + 0x50, &almost_zero, sizeof( float ) );
	write_virtual_memory( g_driver, pml4, lpCurrentDisplayWeapon + 0xA0, &almost_zero, sizeof( float ) );

	return true;
}


bool poc_test( )
{
	uintptr_t size = 0;
	uintptr_t pml4 = 0;
	uintptr_t base_address = find_kprocess( g_driver, "RainbowSix.exe", size, pml4 );

	if ( !base_address )
	{
		printf( "[-] game is not running\n" );
		return false;
	}

	// check if you're in the game

	uintptr_t local_player = get_local_player( g_driver, pml4, base_address );

	if ( !local_player )
	{
		printf( "[-] you do not have a valid local player\n" );
		return false;
	}

	// enable markers

	esp( pml4, base_address );
	no_recoil( pml4, base_address, local_player );

	//std::vector<uintptr_t> enemy_players;
	//get_enemy_players( g_driver, pml4, base_address, enemy_players );

	//printf( "[+] players found: %llu\n", enemy_players.size( ) );

	//for ( uintptr_t player : enemy_players )
	//{
	//	printf( "    %llx\n", player );
	//	enable_spotted_marker( g_driver, pml4, base_address, player );
	//}

	//printf( "[*] markers enabled\n" );

	return true;
}

/* Purpose: creates a copy of the gigabyte driver with a temporary name
			creates and registers a new service
			gets the gigabyte driver handle
*/

bool load_driver( )
{
	printf( "[+] physical_memory\n\n" );

	g_temp_name			= random_string( 6 );
	g_temp_driver_path	= std::filesystem::absolute( "driver/" + g_temp_name + ".sys" );

	// create a temporary copy of the driver, to prevent our original from being listed in
	// the services journal and PiDDBCacheTable

	std::error_code error;

	if ( !std::filesystem::copy_file( std::filesystem::absolute( g_driver_path ), g_temp_driver_path, error ) )
	{
		printf( "[-] copy failed: %s\n", error.message( ).c_str( ) );
		return false;
	}

	printf( "[*] using: %ws\n", g_temp_driver_path.c_str( ) );

	if ( !start_service( g_temp_name, g_temp_driver_path.string( ) ) )
	{
		printf( "[-] start_service failed, no admin rights?\n" );
		return false;
	}

	// set the global driver handle that is used for reading, writing memory

	g_driver = get_driver_handle( );

	if ( !g_driver )
	{
		printf( "[-] get_driver_handle failed, last_error=%d\n", GetLastError( ) );
		return false;
	}

	printf( "[+] driver loaded, handle: 0x%llx\n", reinterpret_cast<uintptr_t>( g_driver ) );
	return true;
}

/* Purpose: stops and removes the service
			unregisters the service
			deletes temporary driver file
*/

void unload_driver( )
{
	if ( g_driver )
		CloseHandle( g_driver );

	if ( !stop_service( g_temp_name ) )
		printf( "[-] stop_service failed\n" );

	std::error_code error;

	if ( !std::filesystem::remove( g_temp_driver_path, error ) )
		printf( "[-] remove failed: %s\n", error.message( ).c_str( ) );
}

/* Purpose: catches users accidentally closing the console window
			unloads driver for safety
*/

BOOL WINAPI control_handler( DWORD fdwCtrlType )
{
	switch ( fdwCtrlType )
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_BREAK_EVENT:
		unload_driver( );
		break;
	}

	return TRUE;
}