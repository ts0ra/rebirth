#pragma once
#include <enums.h>

namespace hack
{
	inline bool toggleAimbot;
	inline bool toggleNoRecoil;
	inline bool toggleUnlimitedAmmo;
	inline bool toggleUnlimitedArmor;
	inline bool toggleUnlimitedHealth;
	inline bool toggleRadar;

	inline bool toggleESP;
    inline bool toggleBoxESP;
    inline bool toggleNameESP;
    inline bool toggleHealthESP;
    inline bool toggleArmorESP;
    inline bool toggleDistaneESP;

    inline bool isTeamGameMode(int gameMode)
    {
        if (gameMode == gameModes::TEAMDEATHMATCH ||
            gameMode == gameModes::TEAMSURVIVOR ||
            gameMode == gameModes::CTF ||
            gameMode == gameModes::BOTTEAMDEATHMATCH ||
            gameMode == gameModes::TEAMONESHOTONEKILL ||
            gameMode == gameModes::HUNTTHEFLAG ||
            gameMode == gameModes::TEAMKEEPTHEFLAG ||
            gameMode == gameModes::TEAMPF ||
            gameMode == gameModes::TEAMLSS ||
            gameMode == gameModes::BOTTEAMSURVIVOR ||
            gameMode == gameModes::BOTTEAMONESHOTONKILL)
        {
            return true;
        }
        return false;
    }
}

namespace data
{
	inline std::uintptr_t baseAddress{};

	inline std::uintptr_t playersEntityList{};
	inline std::uintptr_t itemsEntityList{};

	inline std::uintptr_t localPlayerEntity{};
	inline D3DXVECTOR3 localPlayerHeadPos{};
	inline D3DXVECTOR3 viewAngle{};
    inline int localTeamSide{};
    inline bool localIsDead{};

    inline int gameMode{};
	
	inline int totalPlayers{};


	inline D3DMATRIX viewMatrix{};
}