#include <windows.h>
#include <vector>

#include "..\kernel\memory.h"
#include "..\kernel\virtual_memory.h"
#include "..\common.h"

#define OFFSET_GAME_MANAGER						0x5225280
#define OFFSET_GAME_MANAGER_ENTITY_COUNT		0x1D0
#define OFFSET_GAME_MANAGER_ENTITY_LIST2		0x1c8

#define OFFSET_STATUS_MANAGER					0x5225220
#define OFFSET_STATUS_MANAGER_CONTAINER			0x370		// de-referenced twice
#define OFFSET_STATUS_MANAGER_LOCALENTITY		0x28		// converts from + 0x1D8 entity to the + 0x1C8 entity

#define OFFSET_ENTITY_REPLICATION				0xC8
#define OFFSET_ENTITY_REPLICATION_TEAM			0x13C

#define OFFSET_ENTITY_COMPONENT					0x28		//
#define OFFSET_ENTITY_COMPONENT_LIST			0xD8		//

#define ENTITY_MARKER_VT_OFFSET					0x37EC998
#define ENTITY_MARKER_ENABLED_OFFSET			0x530

unsigned long get_player_count( HANDLE driver, uintptr_t pml4, uintptr_t base );
uintptr_t get_player_by_id( HANDLE driver, uintptr_t pml4, uintptr_t base, unsigned int i );
unsigned short get_player_team( HANDLE driver, uintptr_t pml4, uintptr_t player );
uintptr_t get_local_player( HANDLE driver, uintptr_t pml4, uintptr_t base );
bool get_enemy_players( HANDLE driver, uintptr_t pml4, uintptr_t base, std::vector<uintptr_t>& players );
uintptr_t get_spotted_marker( HANDLE driver, uintptr_t pml4, uintptr_t base, uintptr_t player );
void game_dump_test( HANDLE driver, uintptr_t pml4, uintptr_t module_base );