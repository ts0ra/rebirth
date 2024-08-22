#pragma once
#include <enums.h>

namespace hack
{
    inline bool toggleAimbot{ false };
	inline bool toggleNoRecoil{ false };
	inline bool toggleUnlimitedAmmo{ false };
	inline bool toggleUnlimitedArmor{ false };
	inline bool toggleUnlimitedHealth{ false };
	inline bool toggleRadar{ false };

	inline bool toggleESP{ false };
    inline bool toggleBoxESP{ false };
    inline bool toggleNameESP{ false };
    inline bool toggleHealthESP{ false };
    inline bool toggleArmorESP{ false };
    inline bool toggleDistaneESP{ false };
    inline bool toggleDrawFOV{ false };

	inline float aimbotFOV{ 10.0f };
	inline float aimbotSmoothing{ 1.0f };
	inline bool toggleSmoothAim{ true };
    inline int fovType{ 0 };
	inline float fovColor[3]{ 255.0f, 255.0f, 255.0f };

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