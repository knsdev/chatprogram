#pragma once
#include "stdafx.h"
#include "constants.h"
#include "packets.h"

namespace chatprogram
{
	class CChat
	{
	private:
		char m_msgBuffer[g_chatBufferSize];
		int m_index;
		std::function<void(const char*)> m_callback;

	public:
		CChat();
		virtual ~CChat();

		void QueryInput();

		// If one Line was read completely, this callback can be used to send the data
		void SetCallback(std::function<void(const char*)> callback);

	private:
		void Reset();
	};
}