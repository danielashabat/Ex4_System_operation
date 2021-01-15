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
#define IS_FAIL(RESULT,MSG) if (RESULT != TRNS_SUCCEEDED)\
{\
    printf("function:[%s line:%d] %s\n", __func__,__LINE__,MSG);\
    return RESULT;\
}



#define DEBUG(MSG) printf("function:[%s line:%d] %s\n", __func__, __LINE__,MSG)

typedef enum{ CLIENT_REQUEST,CLIENT_VERSUS,CLIENT_SETUP,CLIENT_PLAYER_MOVE,
                CLIENT_DISCONNECT,SERVER_MAIN_MENU, SERVER_APPROVED,SERVER_DENIED,
                SERVER_INVITE,SERVER_SETUP_REQUEST,SERVER_PLAYER_MOVE_REQUEST,SERVER_GAME_RESULTS,
                SERVER_WIN,SERVER_DRAW,SERVER_NO_OPPONENTS,SERVER_OPPONENT_QUIT} message_type;

#define MSG_LEN 100
#define MAX_PARAMS 4
#define DEFUALT_TIMEOUT 15//15 sec
#define INVITE_TIMEOUT 30//30 sec
#define TEN_MIN 600000//10 minutes
// Function Declarations -------------------------------------------------------

/**
 * SendMsg() sends message to the socket by message type with the given parametrs 
 *
 * Accepts:
 * -------
 * message_type - int that represt the message type to send (see message_type enum)
 * params -pointer to array of strings that contains the paramaters to send. (each index has 1 paramter)
 * socket - the socket used for communication.
 *
 * Returns:
 * -------
 * TRNS_SUCCEEDED - if receiving and memory allocation succeeded
 * TRNS_DISCONNECTED - if the socket was disconnected
 * TRNS_FAILED - otherwise
 */
DWORD SendMsg(SOCKET socket, int message_type, char* params[]);


/*free_params free all the elements in params array and set every element to NULL
* params- array the be free
*/
void free_params(char** params);
/**
 * ReceiveMsg() recievs message from the socket and divide the message to message type and parametrs
 *
 * Accepts:
 * -------
 * message_type - pointer to int (will be assigned in the function)
 * params -pointer to array of strings that need to be pointed to NULL. for example : char *params[]={NULL}. 
           in the end of the function thr array will we set to the recieved paramters from the message
 * timeout- the maximum waiting time for rec function
 * socket - the socket used for communication.
 *
 * Returns:
 * -------
 * TRNS_SUCCEEDED - if receiving and memory allocation succeeded
 * TRNS_DISCONNECTED - if the socket was disconnected
 * TRNS_FAILED - otherwise
 */
DWORD RecieveMsg(SOCKET socket, int* message_type, char** params, int timeout);




#endif //  MESSAGE_H