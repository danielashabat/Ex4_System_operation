/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
/*Authors -Daniela Shabat 316415645, Anat Sinai 312578149.
Project – Cows and Bulls (exersice 4 in System Operation course)
Description –  this file implement the client main

 */
 /*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

// Includes --------------------------------------------------------------------
#include "message.h"
#include "ClientFunctions.h"
#include "client_main.h"


#define USER_LEN 20
#define EXIT 2

#define CHECK_CONNECTION(RESULT) if (RESULT != TRNS_SUCCEEDED)\
{\
	connected_to_server=0;\
    client_exit_status= RESULT;\
}

int main() {
	char username[] = "daniela";
	char IP[] = SERVER_ADDRESS_STR;
	int port = SERVER_PORT;
	int status = 0;

	while (1) {
		status = client(IP, port, username);

		if (status == SUCCEEDED) {
			break;
		}
		else if (status == DENIED) {
			if (print_server_denied_menu(IP, port) == EXIT)
				break;
		}

		else {//other fail
			if (print_reconnect_menu(IP, port) == EXIT) {//user chose exit 
				break;
			}
		}
	}

	return 0;
}

int client(char  IP[10], int port, char  username[])
{
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
		return TRNS_FAILED;
	}


	//Create a sockaddr_in object clientService and set  values.
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(IP); //Setting the IP address to connect to
	clientService.sin_port = htons(port); //Setting the port to connect to.

										  /*
										  AF_INET is the Internet address family.
										  */
										  // Call the connect function, passing the created socket and the sockaddr_in structure as parameters. 
										  // Check for general errors.

	if (SOCKET_ERROR == connect(client_socket, (SOCKADDR*)&clientService, sizeof(clientService))) {
		closesocket(client_socket);
		WSACleanup();
		return TRNS_FAILED;
	}

	printf("Connected to server on %s:%d\n", IP, port);


	int message_type = 0;
	char* send_params[MAX_PARAMS] = { NULL };
	char* recieve_params[MAX_PARAMS] = { NULL };
	DWORD ret_val = 0;
	int k = 0;
	int client_exit_status = SUCCEEDED;
	int connected_to_server = 1;
	int timeout = DEFUALT_TIMEOUT;
	int user_chose=0;
	char user_move[5] = { 0 };
	while (connected_to_server) {

		switch (message_type) {
		case CLIENT_REQUEST:
			send_params[0] = username;
			ret_val = SendMsg(client_socket, CLIENT_REQUEST, send_params);
			CHECK_CONNECTION(ret_val);
			break;

		case SERVER_APPROVED:
			break;

		case SERVER_DENIED:
			connected_to_server = 0;
			client_exit_status = DENIED;
			break;

		case SERVER_MAIN_MENU:
			print_main_menu();
			scanf_s("%d", &user_chose);
			if (user_chose == 1) {//chose Play against another client
				ret_val = SendMsg(client_socket, CLIENT_VERSUS, NULL);
				CHECK_CONNECTION(ret_val);
				timeout = INVITE_TIMEOUT;
			}
			if (user_chose == 2) {//client chose to quit
				ret_val = SendMsg(client_socket, CLIENT_DISCONNECT, NULL);
				CHECK_CONNECTION(ret_val);
				connected_to_server = 0;
			}
			break;
		case SERVER_INVITE:
			printf("Game is on!\n");
			break;

		case SERVER_NO_OPPONENTS:
			print_main_menu();
			scanf_s("%d", &user_chose);
			if (user_chose == 1) {//chose Play against another client
				ret_val = SendMsg(client_socket, CLIENT_VERSUS, NULL);
				CHECK_CONNECTION(ret_val);
				timeout = INVITE_TIMEOUT;
			}
			if (user_chose == 2) {//client chose to quit
				ret_val = SendMsg(client_socket, CLIENT_DISCONNECT, NULL);
				CHECK_CONNECTION(ret_val);
				connected_to_server = 0;
			}
			break;
		case CLIENT_SETUP:
			printf("Choose your guess:\n");
			scanf_s("%4s", user_move, (rsize_t)sizeof(user_move));
			send_params[0] = user_move;
			ret_val = SendMsg(client_socket, CLIENT_PLAYER_MOVE, send_params);
			CHECK_CONNECTION(ret_val);
			break;

		}
		if (connected_to_server) {
			free_params(recieve_params);
			ret_val = RecieveMsg(client_socket, &message_type, recieve_params, timeout);//recieve server respond
			timeout = DEFUALT_TIMEOUT;
			CHECK_CONNECTION(ret_val);
		}
	}


	free_params(recieve_params);
	closesocket(client_socket);

	WSACleanup();

	return client_exit_status;
}




