
/*this module include all the messages types for the send/recieve functions*/

// Includes --------------------------------------------------------------------
#include "message.h"

// Function Implementation -------------------------------------------------------

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
		printf("-client info- succeed sent messeage: %s ",msg);
		return 0;
	}
}



// Server messages --------------------------------------------------------------------

// Client messages --------------------------------------------------------------------
