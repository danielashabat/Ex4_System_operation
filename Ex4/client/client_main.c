/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
/*Authors -Daniela Shabat 316415645, Anat Sinai 312578149.
Project – Cows and Bulls (exersice 4 in System Operation course)
Description –  this file implement the client main

 */
 /*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

// Includes --------------------------------------------------------------------
#include "message.h"
#include "ClientFunctions.h"


#define USER_LEN 20
#define MAX_PARAMS 4



int main() {
	//char msg[] = "client:daniela;or";
	//char *params[2];
	//get_params(msg,2,params);
	//printf("%s\n", params[0]);
	//printf("%s\n", params[1]);
	//return;

	char username[USER_LEN] = "daniela";

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
	int message_type = 0;
	char* send_params[MAX_PARAMS];
	char** recieve_params = NULL;
	DWORD ret_val = 0;


	//while true
	ret_val = RecieveMsg(socket, &message_type, &recieve_params);//recieve server respond
	IS_FAIL(ret_val);
	switch (message_type) {
	case CLIENT_REQUEST:
		strcpy_s(send_params[0], USER_LEN, username);
		ret_val = SendMsg(socket, CLIENT_REQUEST, send_params);
		IS_FAIL(ret_val);
		//free(&recieve_params); and set to null
		break;

	case SERVER_APPROVED:
		break;
	case SERVER_MAIN_MENU:
		//main menue
		break;

	case END_PROGRAM:
		break;

	}




	closesocket(client_socket);

	WSACleanup();

	return 0;
	//DANIELA END
}


