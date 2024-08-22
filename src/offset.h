#pragma once
#include <cstdint>

namespace offsets
{
	constexpr std::uintptr_t gameMode = 0x18ABF8; // int, game mode (check enums)

	constexpr std::uintptr_t totalPlayer = 0x18AC0C; // int, total player in a match

	constexpr std::uintptr_t viewMatrix = 0x17DFD0; // float[16], view matrix

	constexpr std::uintptr_t playersEntityList = 0x18AC04; // uintptr_t, start from 1
	constexpr std::uintptr_t playerEntitySize = 0x4; // size 0x4 for each player

	constexpr std::uintptr_t itemsEntityList = 0x19086C; // uintptr_t, start from 0 (dont dereference this, use with offset directly)
	constexpr std::uintptr_t itemEntitySize = 0x18; // size 0x18 for each item
	constexpr std::uintptr_t itemPosition = 0x0; // Vector3ItemEntity, short data type, size 6 bytes
	constexpr std::uintptr_t itemType = 0x8; // int, enum item type
	constexpr std::uintptr_t itemAttr2 = 0x9; // int, for CTF_FLAG, attr2 = red/blue (0/1)

	constexpr std::uintptr_t totalItemPickups = 0x17F2B4; // uintptr_t, crosshair entity
	constexpr std::uintptr_t totalEntity = 0x1829B0; // int, on a map (exclude player entities)
	constexpr std::uintptr_t localPlayerEntity = 0x18AC00; // uintptr_t, local player entity

	constexpr std::uintptr_t name = 0x205; // char[16], player name
	constexpr std::uintptr_t head = 0x4; // vec3, player head
	constexpr std::uintptr_t foot = 0x28; // vec3, player feet
	constexpr std::uintptr_t viewAngles = 0x34; // vec3

	constexpr std::uintptr_t health = 0xEC; // int, 0 - 100
	constexpr std::uintptr_t armor = 0xF0; // int, 0 - 100
	constexpr std::uintptr_t teamSide = 0x30C; // 0: CLA, 1: RVSF, 2: CLA-SPEC, 3: RVSF-SPEC, 4: SPECTATOR
	constexpr std::uintptr_t isDead = 0x318; // 0: alive, 1: dead

	constexpr std::uintptr_t currentWeaponObject = 0x364;
	constexpr std::uintptr_t ammoReserve = 0x10; // uintptr_t, ammo reserve
	constexpr std::uintptr_t ammoLoaded = 0x14; // uintptr_t, ammo loaded
	constexpr std::uintptr_t noRecoil = 0xC8BA0; // uintptr_t, ammo loaded

	constexpr std::uintptr_t radarMap		= 0x5D2C4;
	constexpr std::uintptr_t radarMiniMap	= 0x5C4D6;

	//ac_client.exe+5D2C4 - 0F8D CC000000         - jnl ac_client.exe+5D396 radar hack map
	//ac_client.exe+5D3E6 - F3 0F7E 46 04         - movq xmm0,[esi+04] change to this

	//ac_client.exe+5C4D6 - 0F8D D5000000         - jnl ac_client.exe+5C5B1 radar hack minimap
	//ac_client.exe+5C606 - F3 0F7E 46 04         - movq xmm0,[esi+04]

}