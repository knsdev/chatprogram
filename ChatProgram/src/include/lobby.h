#pragma once
#include "stdafx.h"
#include "lobbyDef.h"
#include "constants.h"
#include "packets.h"
#include "chat.h"
#include "socket.h"

namespace chatprogram
{
	class CLobby
	{
	private:
		static CLobby* m_instance;
		CChat m_chat;
		std::vector<SLobbyInfo> m_lobbyList;
		SLobbyData m_data;
		ELobbyState m_state;

		bool m_isClient;

		CSocket* m_clientSocketUDP;
		CSocket* m_clientSocketTCP;

		sockaddr_in m_ownAddr;

		const DWORD m_pingSendTimeInterval = 1;
		const DWORD m_timeOut = 1000;
		DWORD m_timeLastPingSent;
		DWORD m_timeLastPongRecv;

		const char* m_username;

	public:
		virtual ~CLobby();

		static void CreateInstance() { m_instance = new CLobby(); }
		static void DestroyInstance() { delete m_instance; }
		inline static CLobby* GetInstance() { return m_instance; }

		void RunServer(const char* lobbyName, const char* username);
		void RunClient(const sockaddr_in* newHostAddr, const char* username);

		void Search();
		void JoinSelectedLobby(int lobbyIndex);
		void Join(const sockaddr_in& hostAddr);

		void SendChatMsg(const char* msg, const char* username);
		void SendLobbyMemberJoined(const SLobbyMember& member);
		void SendLobbyMemberLeft(const SLobbyMember& member);
		void SendPing();

		int SendGetOwnAddr(const sockaddr_in& addr, const SOCKET& destClient);

		int SendData(const char* data, const int& dataLen, const SOCKET* exceptClient = nullptr);
		int SendDataToClient(const SOCKET& socket, const char* data, const int& dataLen);

		void OnServerConnectionInterrupted();

		inline bool IsClient() { return m_isClient; }
		inline ELobbyState GetState() { return m_state; }
		inline const char* GetUsername() { return m_username; }

	private:
		CLobby();
		void Init();
		void HandleUDPRequests(const CSocket& socket);
		void HandleTCPPacket(const char* data, SLobbyMember* member = nullptr);
		bool AddLobbyMember(const SOCKET& socket, const sockaddr_in& addr);
		bool RemoveLobbyMember(const SOCKET& socketHandle);
		void OnClientDisconnected(SLobbyMember& member, fd_set& selectSockets);
		void ResetLobbyMembersArray();
		sockaddr_in GetLowestAddrInLobby();
		bool AreAddrEqual(const sockaddr_in& left, const sockaddr_in& right);
	};

	// Callback
	void OnChatLineRead(const char* msg);
}

