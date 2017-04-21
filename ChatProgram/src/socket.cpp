#include "include\stdafx.h"
#include "include\socket.h"

namespace chatprogram
{
	CSocket::CSocket(const SSocketDesc& sockDesc)
	{
		m_socketDesc = sockDesc;
		m_socket = INVALID_SOCKET;

		if (Init() != 0)
		{
			std::cout << "CSocket::Init() failed!" << std::endl;
		}
	}

	CSocket::~CSocket()
	{
		closesocket(m_socket);
	}

	int CSocket::Init()
	{
		int result = 0;

		m_service.sin_family = AF_INET;
		m_service.sin_port = htons(m_socketDesc.port);

		switch (m_socketDesc.protocolType)
		{
		case EProtocolType::TCP:
			m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			break;

		case EProtocolType::UDP:
			m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			break;
		}

		switch (m_socketDesc.socketType)
		{
		case ESocketType::CLIENT:
			if (m_socketDesc.isBroadcast)
			{
				m_service.sin_addr.s_addr = htonl(INADDR_BROADCAST);
			}
			else
			{
				m_service.sin_addr = m_socketDesc.destIP;
			}
			break;
		case ESocketType::SERVER:
			m_service.sin_addr.s_addr = htonl(INADDR_ANY);
			break;
		}

		if (m_socketDesc.useDestAddr)
		{
			m_service = m_socketDesc.destAddr;
		}

		// Set Broadcast Option
		if (m_socketDesc.isBroadcast)
		{
			int optVal = TRUE;
			int bOptLen = sizeof(BOOL);
			result = setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, (char*)&optVal, bOptLen);
			result = setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&optVal, bOptLen);

			if (result != 0)
				return result;
		}

		// Set (Non-)Blocking
		u_long iMode = m_socketDesc.isBlocking ? 0 : 1;
		ioctlsocket(m_socket, FIONBIO, &iMode);

		switch (m_socketDesc.socketType)
		{
		case ESocketType::CLIENT:

			break;
		case ESocketType::SERVER:
			// Bind
			result = bind(m_socket, (SOCKADDR*)&m_service, sizeof(m_service));
			break;
		}

		return result;
	}

	int CSocket::Send(const char* data, const int& dataLen) const
	{
		int result = 0;

		switch (m_socketDesc.protocolType)
		{
		case EProtocolType::TCP:
			result = send(m_socket, data, dataLen, NULL);
			break;
		case EProtocolType::UDP:
			result = sendto(m_socket, data, dataLen, NULL, (SOCKADDR*)&m_service, GetServiceLen());
			break;
		}

		return result;
	}

	int CSocket::Receive(char* data, const int& dataLen, sockaddr_in* senderAddr) const
	{
		int result = 0;

		switch (m_socketDesc.protocolType)
		{
		case EProtocolType::TCP:
			result = recv(m_socket, data, dataLen, NULL);
			break;
		case EProtocolType::UDP:
			int senderAddrLen = sizeof(sockaddr_in);
			result = recvfrom(m_socket, data, dataLen, NULL, (SOCKADDR*)senderAddr, &senderAddrLen);
			break;
		}

		return result;
	}

	int CSocket::Listen() const
	{
		int result = -1;

		if (m_socketDesc.protocolType == EProtocolType::TCP)
		{
			// Listen
			result = listen(m_socket, m_socketDesc.backlog);
		}

		return result;
	}

	int CSocket::Connect() const
	{
		int result = 0;

		if (m_socketDesc.protocolType != EProtocolType::TCP)
		{
			std::cout << "CSocket::Connect() => Error Socket is not TCP!" << std::endl;
			return -1;
		}

		result = connect(m_socket, (SOCKADDR*)&m_service, sizeof(m_service));

		return result;
	}

	SOCKET CSocket::Accept(sockaddr_in& clientAddr) const
	{
		if (m_socketDesc.protocolType != EProtocolType::TCP)
		{
			std::cout << "CSocket::Accept(...) => Error Socket is not TCP!" << std::endl;
			return INVALID_SOCKET;
		}

		SOCKET acceptedSocket = INVALID_SOCKET;

		int clientAddrLen = sizeof(sockaddr_in);

		acceptedSocket = accept(m_socket, (SOCKADDR*)&clientAddr, &clientAddrLen);

		return acceptedSocket;
	}
}