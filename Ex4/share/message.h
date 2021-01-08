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

typedef enum { CLIENT_REQUEST,CLIENT_VERSUS,CLIENT_SETUP,CLIENT_PLAYER_MOVE,CLIENT_DISCONNECT,SERVER_MAIN_MENU, SERVER_APPROVED,SERVER_DENIED,} message_type;
#define MSG_LEN 100//ask anat for the right length
// Function Declarations -------------------------------------------------------

DWORD SendMsg(SOCKET socket, int message_type, char* params[]);

/**
 * ReceiveMsg() recievs message from the server and divide the message to message type and parametrs
 *
 * Accepts:
 * -------
 * message_type - pointer to int
 * params -pointer to array of strings that need to be pointed to NULL. for example : char **params=NULL
 *
 * socket - the socket used for communication.
 *
 * Returns:
 * -------
 * TRNS_SUCCEEDED - if receiving and memory allocation succeeded
 * TRNS_DISCONNECTED - if the socket was disconnected
 * TRNS_FAILED - otherwise
 */
DWORD RecieveMsg(SOCKET socket, int* message_type, char** params);

/*copy_param_from_message
* dest - pointer to the destination of the string
msg - pointer to the recieved message
index - is the index the parameter is placed into msg
len - is the len of the parameter (not include \0)
*/
void copy_param_from_message(char* dest, char* msg, int index, int len);
#endif //  MESSAGE_H