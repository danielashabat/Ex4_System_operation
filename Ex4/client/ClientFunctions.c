#include "ClientFunctions.h"
#include "message.h"


void print_main_menu() {
	printf("Choose what to do next:\n");
	printf("1. Play against another client\n");
	printf("2. Quit\n");
}


/*print menu to client and return its result*/
int print_reconnect_menu(char  IP[], int port)
{
	int user_chose = 0;
	do {
	printf("Failed connecting to server on %s:%d.\n", IP, port);
	printf("Choose what to do next:\n");
	printf("1. Try to reconnect\n");
	printf("2. Exit\n");
	scanf_s("%d", &user_chose);
	} while (user_chose != 1 && user_chose != 2);//check if chose is valid

	return user_chose;
}

int print_server_denied_menu(char  IP[], int port)
{
	int user_chose = 0;
	do {
		printf("server on %s:%d denied the connection request.\n", IP, port);
		printf("Choose what to do next:\n");
		printf("1. Try to reconnect\n");
		printf("2. Exit\n");
		scanf_s("%d", &user_chose);
	} while (user_chose != 1 && user_chose != 2);//check if chose is valid

	return user_chose;
}

void print_result(char *bulls,char *cows, char* opponent_username, char* opponent_move) {
	printf("Bulls: %s\n",bulls);
	printf("Cows: %s\n",cows);
	printf("%s played: %s\n", opponent_username, opponent_move);
}


