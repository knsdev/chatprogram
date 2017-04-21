#include "include\stdafx.h"
#include "include\lobby.h"

namespace chatprogram
{
	CLobby* CLobby::m_instance;

	CLobby::CLobby()
	{
		m_instance = this;
		m_clientSocketUDP = nullptr;
		m_state = ELobbyState::NOT_INIT;
		Init();
	}

	CLobby::~CLobby()
	{
		delete m_clientSocketUDP;
		delete m_clientSocketTCP;
	}

	void CLobby::Init()
	{
		m_chat.SetCallback(OnChatLineRead);

		m_state = ELobbyState::INIT;

		ResetLobbyMembersArray();
	}

	void CLobby::RunServer(const char* lobbyName, const char* username)
	{
		m_isClient = false;
		m_state = ELobbyState::IN_LOBBY;
		std::cout << "CLobby::RunServer()" << std::endl;

		strcpy_s(m_data.name, lobbyName);
		m_username = username;

		int result = 0;

		// UDP
		SSocketDesc descUDP = {};
		descUDP.socketType = ESocketType::SERVER;
		descUDP.protocolType = EProtocolType::UDP;
		descUDP.port = g_portUDPFindServer;
		descUDP.isBlocking = false;

		CSocket socketUDP(descUDP);

		// TCP
		SSocketDesc descTCP = {};
		descTCP.socketType = ESocketType::SERVER;
		descTCP.protocolType = EProtocolType::TCP;
		descTCP.port = g_portTCP;
		descTCP.isBlocking = true;
		descTCP.backlog = 10;

		CSocket listenSocket(descTCP);

		TIMEVAL selectTimeout = { 0, 150 };

		m_timeLastPingSent = timeGetTime();

		if (listenSocket.Listen() != -1)
		{
			fd_set selectSockets;

			int maxClientSocket = 0;

			for (;;)
			{
				m_chat.QueryInput();
				HandleUDPRequests(socketUDP);

				if (timeGetTime() - m_timeLastPingSent > m_pingSendTimeInterval)
				{
					SendPing();
					m_timeLastPingSent = timeGetTime();
				}

				FD_ZERO(&selectSockets);
				FD_SET((int)listenSocket.GetHandle(), &selectSockets);
				for (SLobbyMember client : m_data.members)
				{
					if (client.socket != -1)
					{
						FD_SET((int)client.socket, &selectSockets);
						maxClientSocket = max((int)maxClientSocket, (int)client.socket);
					}
				}

				// Query which sockets in the SOCKET Array are currently readable
				if (select(maxClientSocket + 1, &selectSockets, nullptr, &selectSockets, &selectTimeout) != -1)
				{
					// Accept TCP Clients via ListenSocket
					if (FD_ISSET((int)listenSocket.GetHandle(), &selectSockets))
					{
						sockaddr_in tempAddr;
						SOCKET tempSocket;
						tempSocket = listenSocket.Accept(tempAddr);
						AddLobbyMember(tempSocket, tempAddr);
					}
				}
				else
				{
					std::cout << "select(...) error!" << std::endl;
				}

				// Process those Client-Sockets who currently have data available
				for (auto& client : m_data.members)
				{
					if (client.socket == -1)
						continue;

					if ((timeGetTime() - client.lastPongRecv) / 2 > m_timeOut)
					{
						OnClientDisconnected(client, selectSockets);
						continue;
					}

					if (FD_ISSET((int)client.socket, &selectSockets))
					{
						// recv
						char recvBuffer[g_bufferSize];
						int resultTemp = recv(client.socket, recvBuffer, g_bufferSize, NULL);

						if (resultTemp > 0)
						{
							HandleTCPPacket(recvBuffer, &client);
						}
						else
						{
							//OnClientDisconnected(client, selectSockets);
						}
					}
				}
			}
		}
	}

	void CLobby::OnClientDisconnected(SLobbyMember& member, fd_set& selectSockets)
	{
		std::cout << "Client (" << inet_ntoa(member.addr.sin_addr) << ":" << member.addr.sin_port << ") has left." << std::endl;

		SendLobbyMemberLeft(member);

		for (int i = 0; i < g_lobbyMaxMembers; ++i)
		{
			if (selectSockets.fd_array[i] == member.socket)
			{
				SOCKET* ptr = selectSockets.fd_array;
				memset(ptr + i, -1, sizeof(SOCKET));
			}
		}

		memset(&member, -1, sizeof(member));
		closesocket(member.socket);
	}

	void CLobby::HandleUDPRequests(const CSocket& socket)
	{
		int result = 0;

		sockaddr_in senderAddr;

		char bufferUDP[g_bufferSize];
		result = socket.Receive(bufferUDP, sizeof(bufferUDP), &senderAddr);

		if (result > 0)
		{
			PacketInfo* packet = (PacketInfo*)&bufferUDP;

			if (packet->id == "CP123")
			{
				// Send Server Info back
				strcpy_s(packet->lobbyName, m_data.name);
				result = sendto(socket.GetHandle(), (const char*)packet, sizeof(PacketInfo), 0, (SOCKADDR *)& senderAddr, sizeof(senderAddr));
			}
		}
	}

	void CLobby::RunClient(const sockaddr_in* newHostAddr, const char* username)
	{
		std::cout << "CLobby::RunClient()" << std::endl;
		m_isClient = true;

		m_username = username;

		if (newHostAddr == nullptr)
		{
			m_state = ELobbyState::SEARCH_CHOOSE_LOBBY;

			SSocketDesc desc = {};
			desc.socketType = ESocketType::CLIENT;
			desc.protocolType = EProtocolType::UDP;
			desc.port = g_portUDPFindServer;
			desc.isBlocking = false;
			desc.isBroadcast = true;

			m_clientSocketUDP = new CSocket(desc);

			int result = 0;

			Search();

			char bufferUDPRecv[g_bufferSize];
			int bufferUDPRecvLen = g_bufferSize;
			while (m_state == ELobbyState::SEARCH_CHOOSE_LOBBY)
			{
				m_chat.QueryInput();

				sockaddr_in senderAddr;
				result = m_clientSocketUDP->Receive(bufferUDPRecv, bufferUDPRecvLen, &senderAddr);

				if (result > 0)
				{
					PacketInfo* packetRecv = (PacketInfo*)bufferUDPRecv;
					SLobbyInfo lobbyInfo = {};
					strcpy_s(lobbyInfo.lobbyName, packetRecv->lobbyName);
					lobbyInfo.addr = senderAddr;
					m_lobbyList.push_back(lobbyInfo);
					int lobbyIndex = m_lobbyList.size() - 1;

					std::cout << "[" << lobbyIndex << "] " << lobbyInfo.lobbyName << " | " << inet_ntoa(lobbyInfo.addr.sin_addr) << std::endl;
				}
			}

			if (m_clientSocketUDP)
				delete m_clientSocketUDP;
		}
		else
		{
			// Host Migration
			Join(*newHostAddr);
		}

		//-------------------------------
		// ELobbyState::IN_LOBBY
		//-------------------------------

		fd_set selectSockets;
		TIMEVAL selectTimeout = { 0, 150 };

		bool lostHost = false;
		m_timeLastPongRecv = m_timeLastPingSent = timeGetTime();

		while (m_state == ELobbyState::IN_LOBBY && !lostHost)
		{
			m_chat.QueryInput();

			DWORD currentTime = timeGetTime();

			if (currentTime - m_timeLastPingSent > m_pingSendTimeInterval)
			{
				SendPing();
				m_timeLastPingSent = timeGetTime();
			}

			if ((currentTime - m_timeLastPongRecv) > m_timeOut)
			{
				std::cout << "Timeout => Host did not respond!" << std::endl;
				lostHost = true;
			}

			if (!lostHost)
			{
				FD_ZERO(&selectSockets);
				FD_SET(m_clientSocketTCP->GetHandle(), &selectSockets);

				if (select(1, &selectSockets, NULL, NULL, &selectTimeout) != -1)
				{
					if (FD_ISSET(m_clientSocketTCP->GetHandle(), &selectSockets))
					{
						// Receive TCP Packets
						char recvBuffer[g_bufferSize];
						int resultTemp = m_clientSocketTCP->Receive(recvBuffer, g_bufferSize, nullptr);

						if (resultTemp > 0)
						{
							HandleTCPPacket(recvBuffer);
						}
						else
						{
							//std::cout << "Error in Client TCP Recv" << std::endl;
							//lostHost = true;
							//break;
						}
					}
				}
				else
				{
					std::cout << "CLobby::RunClient() => select(...) error" << std::endl;
				}
			}
		}

		if (m_clientSocketTCP)
			delete m_clientSocketTCP;

		if (lostHost)
			OnServerConnectionInterrupted();
	}

	void CLobby::HandleTCPPacket(const char* data, SLobbyMember* member)
	{
		PacketHeader* header = (PacketHeader*)data;

		if (strcmp(header->id, "CP123") != 0)
		{
			std::cout << "CLobby::HandleTCPPacket(...) => Error wrong Packet-ID!" << std::endl;
			return;
		}

		switch (header->type)
		{
		case EPacketType::CHAT_MSG:
		{
			PacketChatMsg* packet = (PacketChatMsg*)data;
			std::cout << packet->username << ": " << packet->msg << std::endl;

			if (!m_isClient)
			{
				SendChatMsg(packet->msg, packet->username);
			}
		}
		break;
		case EPacketType::LOBBY_MEMBER_JOINED:
		{
			PacketLobbyMemberJoined* packet = (PacketLobbyMemberJoined*)data;
			AddLobbyMember(packet->member.socket, packet->member.addr);
		}
		break;
		case EPacketType::LOBBY_MEMBERS_TOTAL:
		{
			PacketLobbyMembersTotal* packet = (PacketLobbyMembersTotal*)data;
			memcpy(m_data.members, packet->members, sizeof(SLobbyMember) * g_lobbyMaxMembers);
			std::cout << "Received all LobbyMembers!" << std::endl;
			memcpy(m_data.name, packet->lobbyName, g_lobbyNameLength);
			std::cout << "Received LobbyName: " << m_data.name << std::endl;
		}
		break;
		case EPacketType::GET_OWN_ADDR:
		{
			PacketGetOwnAddr* packet = (PacketGetOwnAddr*)data;
			m_ownAddr = packet->addr;
			std::cout << "Received Own Address: " << inet_ntoa(m_ownAddr.sin_addr) << ":" << m_ownAddr.sin_port << std::endl;
		}
		break;
		case EPacketType::LOBBY_MEMBER_LEFT:
		{
			PacketLobbyMemberLeft* packet = (PacketLobbyMemberLeft*)data;
			RemoveLobbyMember(packet->socketHandle);
		}
		break;
		case EPacketType::PING:
		{
			PacketPing* packetPing = (PacketPing*)data;

			PacketPong packetPong;
			packetPong.timeSent = packetPing->timeSent;

			if (m_isClient)
			{
				SendData((char*)&packetPong, sizeof(PacketPong));
			}
			else
			{
				SendDataToClient(member->socket, (char*)&packetPong, sizeof(PacketPong));
			}
		}
		break;
		case EPacketType::PONG:
		{
			PacketPong* packetPong = (PacketPong*)data;

			DWORD pingTime = timeGetTime() - packetPong->timeSent;

			if (m_isClient)
			{
#ifdef __CHAT_SHOW_PING_DEBUG_MESSAGES
				std::cout << "Ping: " << pingTime << " ms" << std::endl;
#endif
				m_timeLastPongRecv = timeGetTime();
			}
			else
			{
#ifdef __CHAT_SHOW_PING_DEBUG_MESSAGES
				std::cout << "Ping (" << member->addr.sin_port << "): " << pingTime << " ms" << std::endl;
#endif
				member->lastPongRecv = timeGetTime();
			}
		}
		break;
		}
	}

	void CLobby::Search()
	{
		if (m_state != ELobbyState::SEARCH_CHOOSE_LOBBY)
			return;

		int result = 0;

		m_lobbyList.clear();

		PacketInfo packet;
		const char* bufferUDPSend = (const char*)&packet;
		int bufferUDPSendLen = sizeof(PacketInfo);

		result = m_clientSocketUDP->Send(bufferUDPSend, bufferUDPSendLen);
	}

	void CLobby::JoinSelectedLobby(int lobbyIndex)
	{
		if (lobbyIndex < 0 || lobbyIndex >= (int)m_lobbyList.size())
		{
			std::cout << "CLobby::Join(int lobbyIndex) => invalid lobbyIndex!" << std::endl;
			return;
		}

		int result = 0;

		SLobbyInfo lobbyInfo = m_lobbyList[lobbyIndex];
		std::cout << "Joining Lobby: " << lobbyInfo.lobbyName << std::endl;

		Join(lobbyInfo.addr);
	}

	void CLobby::Join(const sockaddr_in& hostAddr)
	{
		int result = 0;

		SSocketDesc desc;
		desc.socketType = ESocketType::CLIENT;
		desc.protocolType = EProtocolType::TCP;
		desc.destIP = hostAddr.sin_addr;
		desc.port = g_portTCP;
		desc.isBlocking = true;
		desc.isBroadcast = false;

		m_clientSocketTCP = new CSocket(desc);

		result = m_clientSocketTCP->Connect();

		if (result == 0)
		{
			m_state = ELobbyState::IN_LOBBY;
			std::cout << "Joined Lobby successfully!" << std::endl;
		}
	}

	void CLobby::SendChatMsg(const char* msg, const char* username)
	{
		if (m_state != ELobbyState::IN_LOBBY)
			return;

		int result = 0;

		PacketChatMsg packet = {};
		strcpy_s(packet.msg, msg);
		strcpy_s(packet.username, username);

		result = SendData((char*)&packet, sizeof(PacketChatMsg));
	}

	void CLobby::SendLobbyMemberJoined(const SLobbyMember& member)
	{
		int result = 0;

		PacketLobbyMemberJoined packet;
		packet.member = member;

		// Send the information about the new Member to all Clients except the newly connected one
		result = SendData((char*)&packet, sizeof(PacketLobbyMemberJoined), &member.socket);

		PacketLobbyMembersTotal packetTotal;
		memset(packetTotal.members, -1, sizeof(SLobbyMember) * g_lobbyMaxMembers);
		// Find all LobbyMembers except the newly connected one
		for (int i = 0; i < g_lobbyMaxMembers; ++i)
		{
			if (m_data.members[i].socket != -1 && m_data.members[i].socket != member.socket)
			{
				packetTotal.members[i] = m_data.members[i];
			}
		}
		strcpy_s(packetTotal.lobbyName, m_data.name);
		// Send to the newly connected Client
		result = SendDataToClient(member.socket, (char*)&packetTotal, sizeof(PacketLobbyMembersTotal));

		// Tell the client his own addr
		result = SendGetOwnAddr(member.addr, member.socket);
	}

	void CLobby::SendLobbyMemberLeft(const SLobbyMember& member)
	{
		int result = 0;

		PacketLobbyMemberLeft packet;
		packet.socketHandle = member.socket;

		result = SendData((char*)&packet, sizeof(PacketLobbyMemberLeft), &member.socket);
	}

	int CLobby::SendGetOwnAddr(const sockaddr_in& addr, const SOCKET& destClient)
	{
		int result = 0;

		PacketGetOwnAddr packet;
		packet.addr = addr;

		result = SendDataToClient(destClient, (char*)&packet, sizeof(PacketGetOwnAddr));

		return result;
	}

	void CLobby::SendPing()
	{
		int result = 0;

		PacketPing packet;
		packet.timeSent = timeGetTime();

		if (m_isClient)
		{
			result = SendData((char*)&packet, sizeof(PacketPing));
		}
		else
		{
			for (SLobbyMember& client : m_data.members)
			{
				if (client.socket != -1)
				{
					packet.timeSent = timeGetTime();
					result = SendDataToClient(client.socket, (char*)&packet, sizeof(PacketPing));
				}
			}
		}
	}

	int CLobby::SendData(const char* data, const int& dataLen, const SOCKET* exceptClient)
	{
		int result = 0;

		if (m_isClient)
		{
			result = m_clientSocketTCP->Send(data, dataLen);
			//if (result <= 0)
			//std::cout << "[CLIENT] CLobby::SendData(...) => Error" << std::endl;
		}
		else
		{
			// Server sends to all Clients (not to himself!)
			for (SLobbyMember& client : m_data.members)
			{
				if (client.socket != -1)
				{
					if (exceptClient != nullptr && client.socket == *exceptClient)
						continue;

					result = SendDataToClient(client.socket, data, dataLen);
				}
			}
		}

		return result;
	}

	int CLobby::SendDataToClient(const SOCKET& socket, const char* data, const int& dataLen)
	{
		int result = send(socket, data, dataLen, NULL);
		//if (result <= 0)
		//std::cout << "[SERVER] CLobby::SendData(...) => Error" << std::endl;

		return result;
	}

	void OnChatLineRead(const char* msg)
	{
		auto lobbyInstance = CLobby::GetInstance();

		printf("\n");

		if (lobbyInstance->GetState() == ELobbyState::IN_LOBBY)
		{
			lobbyInstance->SendChatMsg(msg, lobbyInstance->GetUsername());
		}
		else if (lobbyInstance->IsClient() && lobbyInstance->GetState() == ELobbyState::SEARCH_CHOOSE_LOBBY)
		{
			switch (msg[0])
			{
			case 's':
				lobbyInstance->Search();
				break;
			case '0':
				lobbyInstance->JoinSelectedLobby(0);
				break;
			case '1':
				lobbyInstance->JoinSelectedLobby(1);
				break;
			case '2':
				lobbyInstance->JoinSelectedLobby(2);
				break;
			case '3':
				lobbyInstance->JoinSelectedLobby(3);
				break;
			case '4':
				lobbyInstance->JoinSelectedLobby(4);
				break;
			case '5':
				lobbyInstance->JoinSelectedLobby(5);
				break;
			case '6':
				lobbyInstance->JoinSelectedLobby(6);
				break;
			case '7':
				lobbyInstance->JoinSelectedLobby(7);
				break;
			case '8':
				lobbyInstance->JoinSelectedLobby(8);
				break;
			case '9':
				lobbyInstance->JoinSelectedLobby(9);
				break;
			}
		}

	}

	bool CLobby::AddLobbyMember(const SOCKET& socket, const sockaddr_in& addr)
	{
		// Find free slot (-1) in SOCKET Array
		for (auto& client : m_data.members)
		{
			if (client.socket == -1)
			{
				client.socket = socket;
				client.addr = addr;
				std::cout << "Added LobbyMember: SOCKET = " << (int)client.socket
					<< " | IP = " << inet_ntoa(addr.sin_addr)
					<< ":" << addr.sin_port
					<< std::endl;
				client.lastPongRecv = timeGetTime();

				// Send LobbyData Changes so that the Clients add the new LobbyMember too
				if (!m_isClient)
				{
					SendLobbyMemberJoined(client);
				}

				return true;
			}
		}
		return false;
	}

	bool CLobby::RemoveLobbyMember(const SOCKET& socketHandle)
	{
		// Find LobbyMember by SOCKET Handle
		for (auto& client : m_data.members)
		{
			if (client.socket == socketHandle)
			{
				std::cout << "Client (" << inet_ntoa(client.addr.sin_addr) << ":" << client.addr.sin_port << ") has left." << std::endl;
				memset(&client, -1, sizeof(client));
				return true;
			}
		}

		return false;
	}


	void CLobby::OnServerConnectionInterrupted()
	{
		m_state = ELobbyState::HOST_MIGRATION;

		sockaddr_in lowestAddr = GetLowestAddrInLobby();

		ResetLobbyMembersArray();

		if (AreAddrEqual(m_ownAddr, lowestAddr))
		{
			// Become Host
			RunServer(m_data.name, m_username);
		}
		else
		{
			// Run Client again (skip UDP Lobby Search) and connect to the new Host
			RunClient(&lowestAddr, m_username);
		}
	}

	void CLobby::ResetLobbyMembersArray()
	{
		memset(m_data.members, -1, sizeof(SLobbyMember) * g_lobbyMaxMembers);
	}

	sockaddr_in CLobby::GetLowestAddrInLobby()
	{
		sockaddr_in* lowestAddr = &m_ownAddr;

		for (auto& client : m_data.members)
		{
			if (client.socket != -1)
			{
				// Lower IP?
				if (client.addr.sin_addr.s_addr < lowestAddr->sin_addr.s_addr)
				{
					lowestAddr = &(client.addr);
				}
				// Same IP?
				else if (client.addr.sin_addr.s_addr == lowestAddr->sin_addr.s_addr)
				{
					// Lower Port?
					if (client.addr.sin_port < lowestAddr->sin_port)
					{
						lowestAddr = &(client.addr);
					}
				}
			}
		}

		return *lowestAddr;
	}

	bool CLobby::AreAddrEqual(const sockaddr_in& left, const sockaddr_in& right)
	{
		if (left.sin_addr.s_addr == right.sin_addr.s_addr &&
			left.sin_port == right.sin_port)
			return true;
		return false;
	}
}