#include "stdafx.h"
#include "CCommandCollectNetworkPlayersForTheMission.h"

void CCommandCollectNetworkPlayersForTheMission::Process(CRunningScript* script)
{
	constexpr int MISSION_NETWORK_PLAYERS = 7;  // host + 7 = 8 players in missions
	uint8_t i = 0;

	memset(ScriptParams, 0, MISSION_NETWORK_PLAYERS * sizeof(int));

	for (auto networkPlayer : CNetworkPlayerManager::m_pPlayers)
	{
		if (auto player = networkPlayer->m_pPed)
		{
			ScriptParams[i] = CPools::GetPedRef(networkPlayer->m_pPed);
		}

		if (++i >= MISSION_NETWORK_PLAYERS)
			break;
	}

	script->StoreParameters(MISSION_NETWORK_PLAYERS);
}