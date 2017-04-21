#include "include\stdafx.h"
#include "include\socket.h"
#include "include\lobby.h"

int main(int argc, char** argv)
{
	using namespace chatprogram;

	// Initialize Winsock
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		wprintf(L"Error at WSAStartup()\n");
		return 1;
	}

	CLobby::CreateInstance();

	std::cout << "User Name: ";
	char username[g_lobbyNameLength];
	std::cin >> username;

	std::cout << "[s] Search for ChatRooms" << std::endl;
	std::cout << "[h] Host ChatRoom" << std::endl;

	char inputChar;
	std::cin >> inputChar;

	switch (inputChar)
	{
	case 's':
		CLobby::GetInstance()->RunClient(nullptr, username);
		break;
	case 'h':
		std::cout << "Lobby Name: ";
		char lobbyName[g_lobbyNameLength];
		std::cin >> lobbyName;

		CLobby::GetInstance()->RunServer(lobbyName, username);
		break;
	}

	system("pause");

	CLobby::DestroyInstance();
	WSACleanup();
	return 0;
}