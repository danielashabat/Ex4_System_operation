#ifndef CLIENT_FUNCTIONS_H
#define CLIENT_FUNCTIONS_H

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// Includes --------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <Windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include "SocketShared.h"
#include "SocketSendRecvTools.h"

#pragma comment(lib,"Ws2_32.lib")

// Macros --------------------------------------------------------------------
#define END_PROGRAM 16

// Function Declarations -------------------------------------------------------

typedef enum { FAILED= TRNS_FAILED,DISCONNECTED = TRNS_DISCONNECTED, SUCCEEDED = TRNS_SUCCEEDED,DENIED }return_values;
//* TRNS_SUCCEEDED - if receivingand memory allocation succeeded
//* TRNS_DISCONNECTED - if the socket was disconnected
//* TRNS_FAILED - otherwise

void print_main_menu();
int print_reconnect_menu(char  IP[], int port);
int print_server_denied_menu(char  IP[], int port);
void print_result(char* bulls, char* cows, char* opponent_username, char* opponent_move);
#endif // CLIENT_FUNCTIONS_H