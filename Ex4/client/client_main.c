/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
/*Authors -Daniela Shabat 316415645, Anat Sinai 312578149.
Project – Cows and Bulls (exersice 4 in System Operation course)
Description –  this file implement the client main

 */
 /*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

// Includes --------------------------------------------------------------------
#include "message.h"
#include "ClientFunctions.h"

#define END_PROGRAM 16

int main() {

	////BEGIN ANAT
	//char str[16] = "hello:anat;gil\n";
	//int indexes[2];
	//int lens[2];

	//get_param_index_and_len_2_param(&indexes, &lens, &str);
	////END ANAT

	//DANIELA BEGIN
	SOCKET client_socket;
	SOCKADDR_IN clientService;

	// Initialize Winsock.
	WSADATA wsaData; //Create a WSADATA object called wsaData.
	//The WSADATA structure contains information about the Windows Sockets implementation.

	//Call WSAStartup and check for errors.
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		printf("Error at WSAStartup()\n");
	// Create a socket.
	client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check for errors to ensure that the socket is a valid socket.
	if (client_socket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return;
	}


	//Create a sockaddr_in object clientService and set  values.
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(SERVER_ADDRESS_STR); //Setting the IP address to connect to
	clientService.sin_port = htons(SERVER_PORT); //Setting the port to connect to.

	/*
		AF_INET is the Internet address family.
	*/
	// Call the connect function, passing the created socket and the sockaddr_in structure as parameters. 
// Check for general errors.
	if (connect(client_socket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR) {
		printf("Failed to connect.\n");
		closesocket(client_socket);
		WSACleanup();
		return;
	}
	int state = CLIENT_REQUEST;
	int message_type = 0;
	char** params = NULL;


	//while true
	switch (state) {
	case CLIENT_REQUEST:
		//client requst
		RecieveMsg(client_socket, &message_type, params);
		if (message_type == SERVER_APPROVED) state = SERVER_MAIN_MENU;
		else state = END_PROGRAM;//server denied 
		break;

	case SERVER_MAIN_MENU:
		//main menue
		break;

	case END_PROGRAM:
		break;


	}


	//char user_name[] = "daniela";
	//char* params[1] = {user_name};
	//DWORD ret_val = 0;
	//ret_val=SendMsg(client_socket, CLIENT_REQUEST, params);
	//if (ret_val != 0) {
	//	printf("ERROR:sendMSG failed\n");
	//	//end program
	//}

	//
	
	//ret_val = RecieveMsg(client_socket, CLIENT_REQUEST, params);
	//if (ret_val != 0) {
	//	printf("ERROR:sendMSG failed\n");
	//	//end program
	//}


	closesocket(client_socket);

	WSACleanup();

	return 0;
	//DANIELA END
}


