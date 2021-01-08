
/*this module include all the messages types for the send/recieve functions*/

// Includes --------------------------------------------------------------------
#include "message.h"

// Function Implementation -------------------------------------------------------

// Server messages --------------------------------------------------------------------

// Client messages --------------------------------------------------------------------

DWORD SendMsg(SOCKET socket, int message_type, char* params[]) {
	TransferResult_t SendRes;
	char msg[MSG_LEN];
	//messages send in this frame: "<message_type>:<param_list>\n"

	switch (message_type)
	{

	case CLIENT_REQUEST:
		//send user name
		sprintf_s(msg, MSG_LEN, "CLIENT_REQUEST:%s\n", params[0]);
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
		printf("-client info- succeed sent messeage: %s ", msg);
		return 0;
	}
}


//typedef enum { CLIENT_REQUEST,CLIENT_VERSUS,CLIENT_SETUP,CLIENT_PLAYER_MOVE,CLIENT_DISCONNECT,SERVER_MAIN_MENU, SERVER_APPROVED,SERVER_DENIED,} message_type;
DWORD RecieveMsg(SOCKET socket, int *message_type, char **params) {
	char* AcceptedStr = NULL;
	TransferResult_t RecvRes;
	int inputs = 0;
	int* indexes = NULL;
	int *lens = NULL;
	int i = 0;

	if (params != NULL) {
		printf("ERROR: params neet to be intialized to NULL!\n");
		return TRNS_FAILED;
	}

	RecvRes = ReceiveString(&AcceptedStr, socket);
	
	//recieve the message
	if (TRNS_SUCCEEDED == RecvRes) {
		printf("RecieveString from server succeed the message is: %s", AcceptedStr);
	}
	else {
		printf("RecieveString from server failed\n");
		return RecvRes;
	}

	//find the message type
	if (check_if_message_type_instr_message(AcceptedStr, "SERVER_MAIN_MENU") ){
		*message_type = SERVER_MAIN_MENU ;
	}
	else if (check_if_message_type_instr_message(AcceptedStr, "SERVER_APPROVED")) {
		*message_type = SERVER_APPROVED;
	}
	else if (check_if_message_type_instr_message(AcceptedStr, "SERVER_DENIED")) {
		*message_type = SERVER_DENIED;
		inputs = 1;
	}
	else {
		printf("ERROR: message type is invalid");
		return TRNS_FAILED;
	}

	//if paramters sent in the msg, allocate it in params
	if (inputs > 1) {
		indexes = (int*)malloc(inputs * sizeof(int));
		lens = (int*)malloc(inputs * sizeof(int));
		if ((indexes == NULL) || (lens == NULL)) { printf("ERROR: ALLOCATION FAILED"); return  TRNS_FAILED; }
		get_param_index_and_len( indexes, lens, AcceptedStr, strlen(AcceptedStr));

		//create array of strings
		params = (char**)malloc(inputs * sizeof(char*));

		//create every string in params array 
		for (i = 0; i < inputs; i++) {
			params[i] = (char*)malloc((lens[i]+1) * sizeof(char));
			if (params[i]==NULL) { printf("ERROR: ALLOCATION FAILED"); return  TRNS_FAILED; }
			copy_param_from_message(params[i], AcceptedStr, indexes[i], lens[i]);
		}
		
	}

	free(indexes);
	free(lens);
	free(AcceptedStr);
	return TRNS_SUCCEEDED;

}
/*
* dest - pointer to the destination of the string
msg - pointer to the recieved message
index - is the index the parameter is placed into msg
len - is the len of the parameter (not include \0)
*/
void copy_param_from_message(char *dest,char *msg, int index, int len) {
	int i = 0;
	for (i = 0; i < len; i++) {
		dest[i] = msg[index + i];
	}
	dest[i] = '\0';//end for string 
}