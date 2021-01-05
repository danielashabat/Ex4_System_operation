/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
/*Authors -Daniela Shabat 316415645, Anat Sinai 312578149.
Project – Cows and Bulls (exersice 4 in System Operation course)
Description –  this file implement the client main

 */
 /*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

#include "ClientFunctions.h"

int main() {

	//BEGIN ANAT
	char str[16] = "hello:anat;gil\n";
	int indexes[2];
	int lens[2];

	get_param_index_and_len_2_param(&indexes, &lens, &str);
	//END ANAT

	////DANIELA BEGIN
	//SOCKET client_socket;
	//SOCKADDR_IN clientService;
	//HANDLE hThread[2];

	//// Initialize Winsock.
	//WSADATA wsaData; //Create a WSADATA object called wsaData.
	////The WSADATA structure contains information about the Windows Sockets implementation.

	////Call WSAStartup and check for errors.
	//int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	//if (iResult != NO_ERROR)
	//	printf("Error at WSAStartup()\n");


	//return 0;
	////DANIELA END
}


