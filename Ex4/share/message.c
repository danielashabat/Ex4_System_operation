
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

	//printf("message_type:%d\n", message_type);

	switch (message_type)
	{

	case CLIENT_REQUEST:
		//send user name
		sprintf_s(msg, MSG_LEN, "CLIENT_REQUEST:%s\n", params[0]);
		break;

	case SERVER_APPROVED:
		sprintf_s(msg, MSG_LEN, "SERVER_APPROVED\n");//send to client approved
		break;

	case SERVER_DENIED:
		sprintf_s(msg, MSG_LEN, "SERVER_DENIED:%s\n", params[0]);
		break;

	case SERVER_MAIN_MENU:
		sprintf_s(msg, MSG_LEN, "SERVER_MAIN_MENU\n");
		break;

	case CLIENT_VERSUS:
		sprintf_s(msg, MSG_LEN, "CLIENT_VERSUS\n");
		break;

	case CLIENT_DISCONNECT:
		sprintf_s(msg, MSG_LEN, "CLIENT_DISCONNECT\n");
		break;

	default:
		printf("ERROR:The message type is not valid!\n");
		return 1;
		break;
	}
	//send message
	SendRes = SendString(msg, socket);
	IS_FAIL(SendRes, "Socket error while trying to write data to socket\n");
	printf("-client info- succeed sent messeage: %s ", msg);
	return TRNS_SUCCEEDED;
}


//typedef enum { CLIENT_REQUEST,CLIENT_VERSUS,CLIENT_SETUP,CLIENT_PLAYER_MOVE,CLIENT_DISCONNECT,SERVER_MAIN_MENU, SERVER_APPROVED,SERVER_DENIED,} message_type;
DWORD RecieveMsg(SOCKET socket, int *message_type, char ** params, int timeout) {
	char* AcceptedStr = NULL;
	TransferResult_t RecvRes;
	int inputs = 0;
	int* indexes = NULL;
	int *lens = NULL;
	int i = 0;

	fd_set set;
	struct timeval time_out;
	FD_ZERO(&set); /* clear the set */
	FD_SET(socket, &set); /* add our file descriptor to the set */
	time_out.tv_sec = timeout;
	time_out.tv_usec = 0;
	int rv = select(socket+1, &set, NULL, NULL, &time_out);
	if (rv == SOCKET_ERROR||rv==0)
	{
		IS_FAIL(TRNS_FAILED, "ERROR: Socket connection d\n")
	}

	RecvRes = ReceiveString(&AcceptedStr, socket);
	
	//recieve the message
	IS_FAIL(RecvRes, "RecieveString from server failed\n");
	printf("RecieveString from server succeed the message is: %s", AcceptedStr);

	//find the message type
	if (check_if_message_type_instr_message(AcceptedStr, "CLIENT_REQUEST")) {
		*message_type = CLIENT_REQUEST;
		inputs = 1;
		printf("message type is: CLIENT_REQUEST\n");
	}

	else if (check_if_message_type_instr_message(AcceptedStr, "SERVER_MAIN_MENU") ){
		*message_type = SERVER_MAIN_MENU ;
	}
	else if (check_if_message_type_instr_message(AcceptedStr, "SERVER_APPROVED")) {
		*message_type = SERVER_APPROVED;
	}
	else if (check_if_message_type_instr_message(AcceptedStr, "SERVER_DENIED")) {
		*message_type = SERVER_DENIED;
		inputs = 1;
	}
	else if (check_if_message_type_instr_message(AcceptedStr, "CLIENT_VERSUS")) {
		*message_type = CLIENT_VERSUS;
	}
	else if (check_if_message_type_instr_message(AcceptedStr, "CLIENT_DISCONNECT")) {
		*message_type = CLIENT_DISCONNECT;
	}
	else {
		IS_FAIL(TRNS_FAILED, "ERROR: message type is invalid\n")
	}

	//if paramters sent in the msg, allocate it in params
	if (inputs > 0) {
		printf("inputs is: %d\n",inputs);
		get_params(AcceptedStr,inputs,params);
		printf("param[0]= %s\n",params[0]);
		
	}

	free(AcceptedStr);
	return TRNS_SUCCEEDED;

}

void free_params(char** params) {
	for (int i = 0; i < MAX_PARAMS; i++) {
		if (params[i] != NULL) {
			free(params[i]);
			params[i]=NULL;
		}
	}
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

void get_params(char msg[], int inputs, char** params) {
	char* next_token = NULL;
	char* temp;
	char* temp_token = NULL;
	int i = 0;

	char* msg_type = strtok_s(msg, ":", &next_token);
	char* msg_inputs = strtok_s(NULL, ":", &next_token);

	while (i < inputs) {
		params[i] = NULL;
		temp = strtok_s(msg_inputs, ";", &next_token);//get rest of input
		temp = strtok_s(temp, "\n", &temp_token);//clean \n
		params[i] = (char*)malloc(sizeof(char) * (strlen(temp) + 1));
		if (params[i] == NULL) {
		printf("ERROR: allocation failed\n");
		return;
		}
		strcpy_s(params[i], (strlen(temp) + 1),temp);//copy param to params array
		
		msg_inputs = NULL;
		i++;
	}

}