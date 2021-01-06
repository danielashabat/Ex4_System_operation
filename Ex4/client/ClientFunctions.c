#include "ClientFunctions.h"


//Sending message to the server
 DWORD SendMsg(SOCKET socket, char string[])
{
	TransferResult_t SendRes;

		SendRes = SendString(string, socket);

		if (SendRes == TRNS_FAILED)
		{
			printf("Socket error while trying to write data to socket\n");
			return 0x555;
		}
		else {
			printf("sent to server string:%s SUCCEED!\n",string);
			return 0;
		}
	
}