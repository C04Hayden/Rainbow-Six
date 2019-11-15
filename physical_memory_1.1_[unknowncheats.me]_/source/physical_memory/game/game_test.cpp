#include "game_test.h"

unsigned long get_player_count( HANDLE driver, uintptr_t pml4, uintptr_t base )
{
	uintptr_t game_manager = read_virtual_memory<uintptr_t>( driver, pml4, base + OFFSET_GAME_MANAGER );

	if ( !game_manager )
		return NULL;

	return read_virtual_memory<unsigned long>( driver, pml4, game_manager + OFFSET_GAME_MANAGER_ENTITY_COUNT );
}

uintptr_t get_player_by_id( HANDLE driver, uintptr_t pml4, uintptr_t base, unsigned int i )
{
	uintptr_t game_manager = read_virtual_memory<uintptr_t>( driver, pml4, base + OFFSET_GAME_MANAGER );

	if ( !game_manager )
		return NULL;

	uintptr_t entity_list = read_virtual_memory<uintptr_t>( driver, pml4, game_manager + OFFSET_GAME_MANAGER_ENTITY_LIST2 );

	if ( !entity_list )
		return NULL;

	if ( i > get_player_count( driver, pml4, base ) )
		return NULL;

	uintptr_t entity = read_virtual_memory<uintptr_t>( driver, pml4, entity_list + ( sizeof( PVOID ) * i ) );

	if ( !entity )
		return NULL;

	uintptr_t entity1C8 = read_virtual_memory<uintptr_t>( driver, pml4, entity + OFFSET_STATUS_MANAGER_LOCALENTITY );
	return entity1C8;
}

unsigned short get_player_team( HANDLE driver, uintptr_t pml4, uintptr_t player )
{
	if ( !player )
		return 0xFF;

	uintptr_t replication = read_virtual_memory<uintptr_t>( driver, pml4, player + OFFSET_ENTITY_REPLICATION );

	if ( !replication )
		return 0xFF;

	unsigned long online_team_id = read_virtual_memory<unsigned long>( driver, pml4, replication + OFFSET_ENTITY_REPLICATION_TEAM );
	return LOWORD( online_team_id );
}

uintptr_t get_local_player( HANDLE driver, uintptr_t pml4, uintptr_t base )
{
	uintptr_t status_manager = read_virtual_memory<uintptr_t>( driver, pml4, base + OFFSET_STATUS_MANAGER );

	if ( !status_manager )
		return NULL;

	// iur entity but in 1D8 form is at + 0x00, de-ref twice

	uintptr_t entity_container = read_virtual_memory<uintptr_t>( driver, pml4, status_manager + OFFSET_STATUS_MANAGER_CONTAINER );

	if ( !entity_container )
		return NULL;

	entity_container = read_virtual_memory<uintptr_t>( driver, pml4, entity_container );

	if ( !entity_container )
		return NULL;

	// read the 1C8 entity type

	uintptr_t entity = read_virtual_memory<uintptr_t>( driver, pml4, entity_container + OFFSET_STATUS_MANAGER_LOCALENTITY );
	return entity;
}

bool get_enemy_players( HANDLE driver, uintptr_t pml4, uintptr_t base, std::vector<uintptr_t>& players )
{
	uintptr_t local_player = get_local_player( driver, pml4, base );

	if ( !local_player )
		return FALSE;

	unsigned short local_team = get_player_team( driver, pml4, local_player );

	unsigned int count = get_player_count( driver, pml4, base );

	if ( count > 255 )
		return false;

	for ( unsigned int i = 0; i < count; i++ )
	{
		uintptr_t target_player = get_player_by_id( driver, pml4, base, i );

		if ( !target_player )
			continue;

		if ( !local_player )
			continue;

		if ( target_player == local_player )
			continue;

		if ( get_player_team( driver, pml4, target_player ) == local_team )
			continue;

		players.push_back( target_player );
	}

	return true;
}

uintptr_t get_spotted_marker( HANDLE driver, uintptr_t pml4, uintptr_t base, uintptr_t player )
{
	if ( !player )
		return FALSE;

	uintptr_t component_chain = read_virtual_memory<uintptr_t>( driver, pml4, player + OFFSET_ENTITY_COMPONENT ); //

	if ( !component_chain )
		return NULL;

	uintptr_t component_list = read_virtual_memory<uintptr_t>( driver, pml4, component_chain + OFFSET_ENTITY_COMPONENT_LIST );

	if ( !component_list )
		return NULL;

	// loop through components
	// look for PlayerMarkerComponent object

	// uintptr_t marker_component = NULL;

	for ( unsigned int i = 15; i < 22; i++ )
	{
		uintptr_t component = read_virtual_memory<uintptr_t>( driver, pml4, component_list + i * sizeof( uintptr_t ) );

		if ( !component )
			continue;

		const uintptr_t vt_marker = ENTITY_MARKER_VT_OFFSET;

		uintptr_t vt_table = read_virtual_memory<uintptr_t>( driver, pml4, component );
		uintptr_t vt_offset = vt_table - base;

		if ( vt_offset == vt_marker )
			return component;

		//marker_component = component;
	}

	//if ( marker_component )
	//{
	//	unsigned int enable = 65793;
	//	write_virtual_memory( driver, pml4, marker_component + ENTITY_MARKER_ENABLED_OFFSET, &enable, sizeof( unsigned int ) );
	//}

	return NULL;
}

void game_dump_test( HANDLE driver, uintptr_t pml4, uintptr_t module_base )
{
	IMAGE_DOS_HEADER image_dos_header = {0};
	read_virtual_memory( driver, pml4, module_base, &image_dos_header, sizeof( IMAGE_DOS_HEADER ) );

	if ( image_dos_header.e_magic != IMAGE_DOS_SIGNATURE )
	{
		printf( "[-] dump: bad dos signature\n" );
		return;
	}

	IMAGE_NT_HEADERS64 nt_headers = {0};
	read_virtual_memory( driver, pml4, module_base + image_dos_header.e_lfanew, &nt_headers, sizeof( IMAGE_NT_HEADERS64 ) );

	if ( nt_headers.Signature != IMAGE_NT_SIGNATURE )
	{
		printf( "[-] dump: bad nt signature\n" );
		return;
	}

	unsigned long image_size = nt_headers.OptionalHeader.SizeOfImage;

	printf( "[*] game_dump_test: %lx bytes\n", image_size );

	void* image_buffer = malloc( image_size );
	RtlSecureZeroMemory( image_buffer, image_size );

	if ( !image_buffer )
		return;

	uintptr_t offset = 0;

	for ( uintptr_t offset = 0; offset < image_size; offset += 0x1000 )
	{
		uintptr_t image_buffer_offset = reinterpret_cast< uintptr_t >( image_buffer ) + offset;
		read_virtual_memory( driver, pml4, module_base + offset, reinterpret_cast< void* >( image_buffer_offset ), 0x1000 );
	}

	write_binary_file( "E:\\dump.exe", image_buffer, image_size );
	free( image_buffer );
}