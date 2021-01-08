#ifndef CLIENT_FUNCTIONS_H
#define CLIENT_FUNCTIONS_H

#define _WINSOCK_DEPRECATED_NO_WARNINGS
// Includes --------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
//#include <Windows.h>
#include <winsock2.h>
#include "SocketShared.h"
#include "SocketSendRecvTools.h"

// Macros --------------------------------------------------------------------
#define END_PROGRAM 16

// Function Declarations -------------------------------------------------------

DWORD SendExample(SOCKET socket, char string[]);

DWORD client_request(SOCKET socket, char* username, int* new_state);
#endif // CLIENT_FUNCTIONS_H