#pragma once
#include "stdafx.h"
#include "constants.h"

namespace chatprogram
{
	struct SLobbyMember
	{
		SOCKET socket;
		sockaddr_in addr;
		DWORD lastPongRecv;
	};

	struct SLobbyData
	{
		SLobbyMember members[g_lobbyMaxMembers];
		char name[g_lobbyNameLength];
	};

	struct SLobbyInfo
	{
		char lobbyName[g_lobbyNameLength];
		sockaddr_in addr;
	};

	enum class ELobbyState
	{
		NOT_INIT,
		INIT,
		SEARCH_CHOOSE_LOBBY,
		IN_LOBBY,
		HOST_MIGRATION,
	};
}
