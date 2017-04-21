#include "include\stdafx.h"
#include "include\chat.h"

namespace chatprogram
{
	int getch_noblock()
	{
		if (_kbhit())
			return _getch();
		else
			return -1;
	}

	CChat::CChat()
	{
		Reset();
	}

	CChat::~CChat()
	{

	}

	void CChat::QueryInput()
	{
		char curChar = (char)getch_noblock();
		if (curChar == -1)
			return;

		if (curChar == '\r')
		{
			m_callback(m_msgBuffer);
			Reset();
		}
		else
		{
			putchar((int)curChar);

			if (m_index + 1 < g_chatBufferSize)
			{
				m_msgBuffer[m_index] = curChar;
				m_index++;
			}
		}
	}

	void CChat::Reset()
	{
		memset(m_msgBuffer, 0, g_chatBufferSize);
		m_index = 0;
	}

	void CChat::SetCallback(std::function<void(const char*)> callback)
	{
		m_callback = callback;
	}
}