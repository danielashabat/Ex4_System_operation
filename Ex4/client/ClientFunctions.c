#include "ClientFunctions.h"
#include "message.h"


//Sending message to the server
 DWORD SendExample(SOCKET socket, char string[])
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


/* *Returns:
 *------ -
	 *TRNS_SUCCEEDED - if receivingand memory allocation succeeded
	 * TRNS_DISCONNECTED - if the socket was disconnected
	 * TRNS_FAILED - otherwise
	 * /*/
 DWORD client_request(SOCKET socket, char* username,int *new_state) {
	 int message_type=0;

	 char* send_params[1] = { username };
	 DWORD ret_val = 0;
	 ret_val = SendMsg(socket, CLIENT_REQUEST, send_params);
	 if (ret_val != 0) {
		 printf("ERROR:sendMsg failed\n");
		 return ret_val;
	 }

	 char** recieve_params = NULL;
	 ret_val = RecieveMsg(socket, &message_type, &recieve_params);//recieve server respond
	 if (ret_val != TRNS_SUCCEEDED) {
		 printf("%s line[%d] ERROR:RecieveMsg failed\n", __func__, __LINE__);
		 return ret_val;
	 }
	 if (message_type == SERVER_APPROVED) 
		 *new_state = SERVER_APPROVED;
	 else 
		 //server denied need to read the respond
		 *new_state = END_PROGRAM;//server denied 

	 free(&recieve_params);
	 return TRNS_SUCCEEDED;
 }