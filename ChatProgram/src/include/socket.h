#pragma once
#include "stdafx.h"
#include "constants.h"
#include "packets.h"

namespace chatprogram
{
	enum class ESocketType
	{
		SERVER,
		CLIENT
	};

	enum class EProtocolType
	{
		TCP,
		UDP
	};

	struct SSocketDesc
	{
		ESocketType socketType;
		EProtocolType protocolType;
		USHORT port;
		in_addr destIP;
		bool isBlocking;
		bool isBroadcast;
		int backlog;

		sockaddr_in destAddr;
		bool useDestAddr;

		SSocketDesc()
		{
			isBlocking = true;
			isBroadcast = false;
			backlog = 5;
			useDestAddr = false;
		}
	};

	class CSocket
	{
	private:
		SOCKET m_socket;
		sockaddr_in m_service;
		SSocketDesc m_socketDesc;

	public:
		CSocket(const SSocketDesc& sockDesc);
		virtual ~CSocket();

		int Send(const char* data, const int& dataLen) const;
		int Receive(char* data, const int& dataLen, sockaddr_in* senderAddr) const;

		int Listen() const;
		int Connect() const;
		SOCKET Accept(sockaddr_in& clientAddr) const;

		inline SOCKET GetHandle() const { return m_socket; }
		inline sockaddr_in GetService() const { return m_service; }
		inline int GetServiceLen() const { return sizeof(m_service); }
		inline SSocketDesc GetDescription() const { return m_socketDesc; }

	private:
		int Init();
	};
}