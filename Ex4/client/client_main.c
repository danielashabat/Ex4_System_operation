/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
/*Authors -Daniela Shabat 316415645, Anat Sinai 312578149.
Project � Cows and Bulls (exersice 4 in System Operation course)
Description �  this file implement the client main

 */
 /*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

// Includes --------------------------------------------------------------------
#include "message.h"
#include "ClientFunctions.h"


#define USER_LEN 20




int main() {
	//char msg[] = "client:daniela;or";
	//char *params[2];
	//get_params(msg,2,params);
	//printf("%s\n", params[0]);
	//printf("%s\n", params[1]);
	//return;



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
	char username[] = "daniela";
	int message_type = 0;
	char* send_params[MAX_PARAMS] = { NULL };
	char* recieve_params[MAX_PARAMS] = { NULL };
	DWORD ret_val = 0;
	int k = 0;

	while (message_type!= CLIENT_DISCONNECT) {

		switch (message_type) {
		case CLIENT_REQUEST:
			send_params[0] = (char*)malloc(sizeof(char) * USER_LEN);
			if (send_params[0] != NULL)
			strcpy_s(send_params[0], USER_LEN, username);
			ret_val = SendMsg(client_socket, CLIENT_REQUEST, send_params);
			IS_FAIL(ret_val);
			break;

		case SERVER_APPROVED:
			break;
		case SERVER_MAIN_MENU:
			print_main_menu();
			int user_chose; 
			scanf_s("%d", &user_chose);
			if (user_chose == 1) {
				ret_val = SendMsg(client_socket, CLIENT_VERSUS, NULL);
				IS_FAIL(ret_val);
			}
			if (user_chose ==2) {
				ret_val = SendMsg(client_socket, CLIENT_DISCONNECT, NULL);
				IS_FAIL(ret_val);
				message_type = CLIENT_DISCONNECT;
				continue;
			}
			break;


		}
		free_params(recieve_params);
		ret_val = RecieveMsg(client_socket, &message_type, recieve_params);//recieve server respond
		IS_FAIL(ret_val);

	}


	free_params(recieve_params);
	closesocket(client_socket);

	WSACleanup();

	return 0;
	//DANIELA END
}


