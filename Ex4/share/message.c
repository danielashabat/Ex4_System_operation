#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
//#include <Windows.h>
#include <winsock2.h>
#include "SocketShared.h"
#include "SocketSendRecvTools.h"

/*this module include all the messages types for the send/recieve functions*/

typedef enum { CLIENT_REQUEST } message_type;
#define MSG_LEN 100//ask anat for the right length

DWORD SendMsg(SOCKET socket, int message_type, char *params[]) {
	TransferResult_t SendRes;
	char msg[MSG_LEN];
	//messages send in this frame: "<message_type>:<param_list>\n"

	switch (message_type)
	{

	case CLIENT_REQUEST:
		//send user name
		sprintf_s(msg, MSG_LEN,"CLIENT_REQUEST:%s\n",params[0]);
		SendRes = SendString(msg, socket);
		break;


	default:
		printf("ERROR:The message type is not valid!\n");
		return 1;
		break;
	}
	if (SendRes == TRNS_FAILED)
	{
		printf("Socket error while trying to write data to socket\n");
		return 0x555;
	}
	else {
		printf("sent to server SUCCEED!\n");
		return 0;
	}
}



// Server messages --------------------------------------------------------------------

// Client messages --------------------------------------------------------------------
