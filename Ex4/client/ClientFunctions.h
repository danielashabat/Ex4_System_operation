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


// Function Declarations -------------------------------------------------------

DWORD SendMsg(SOCKET socket, char string[]);
#endif // CLIENT_FUNCTIONS_H