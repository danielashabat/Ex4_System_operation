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

void print_main_menu();
int print_reconnect_menu(char  IP[], int port);
#endif // CLIENT_FUNCTIONS_H