#pragma once
#include "constants.h"
#include "lobbyDef.h"

namespace chatprogram
{
	enum class EPacketType : BYTE
	{
		NONE,
		GET_INFO,
		CHAT_MSG,
		LOBBY_MEMBER_JOINED,
		LOBBY_MEMBER_LEFT,
		LOBBY_MEMBERS_TOTAL,
		GET_OWN_ADDR,
		PING,
		PONG,
	};

	struct PacketHeader
	{
		const char* id = "CP123";
		EPacketType type = EPacketType::NONE;
	};

	struct PacketInfo : public PacketHeader
	{
		char lobbyName[g_lobbyNameLength];

		PacketInfo()
		{
			type = EPacketType::GET_INFO;
		}
	};

	struct PacketChatMsg : public PacketHeader
	{
		char msg[g_chatBufferSize];
		char username[g_lobbyNameLength];

		PacketChatMsg()
		{
			type = EPacketType::CHAT_MSG;
		}
	};

	struct PacketLobbyMemberJoined : public PacketHeader
	{
		SLobbyMember member;

		PacketLobbyMemberJoined()
		{
			type = EPacketType::LOBBY_MEMBER_JOINED;
		}
	};

	struct PacketLobbyMemberLeft : public PacketHeader
	{
		SOCKET socketHandle;

		PacketLobbyMemberLeft()
		{
			type = EPacketType::LOBBY_MEMBER_LEFT;
		}
	};

	struct PacketLobbyMembersTotal : public PacketHeader
	{
		SLobbyMember members[g_lobbyMaxMembers];
		char lobbyName[g_lobbyNameLength];

		PacketLobbyMembersTotal()
		{
			type = EPacketType::LOBBY_MEMBERS_TOTAL;
		}
	};

	struct PacketGetOwnAddr : public PacketHeader
	{
		sockaddr_in addr;

		PacketGetOwnAddr()
		{
			type = EPacketType::GET_OWN_ADDR;
		}
	};

	struct PacketPing : public PacketHeader
	{
		DWORD timeSent;

		PacketPing()
		{
			type = EPacketType::PING;
		}
	};

	struct PacketPong : public PacketHeader
	{
		DWORD timeSent;

		PacketPong()
		{
			type = EPacketType::PONG;
		}
	};
}