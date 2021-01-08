#ifndef MESSAGE_H
#define MESSAGE_H

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

typedef enum { CLIENT_REQUEST } message_type;
#define MSG_LEN 100//ask anat for the right length
// Function Declarations -------------------------------------------------------

DWORD SendMsg(SOCKET socket, int message_type, char* params[]);
#endif //  MESSAGE_H