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


/*
 * get_bulls_and_cows assigns to bulls and cows the results
 *
 * Accepts:
 * -------
 * guess -pointer to string
 * opponent_digits -pointer to string
 * bulls -pointer to int
 * cows -pointer to int
*/
void get_bulls_and_cows(char* guess, char* opponent_digits, int* cows, int* bulls) {
	int digits[10] = { 0 };
	int i = 0;
	int index = 0;
	*cows = 0;
	*bulls = 0;
	//get bulls
	//bulls= the number of digits that exist in guess and in opponent_digits
	for (i = 0; (guess[i] != '\0') || (opponent_digits[i] != '\0'); i++) {
		if ((guess[i] >= '0') && (opponent_digits[i]) >= '0') {
			digits[(int)guess[i] - 48]++;
			digits[(int)opponent_digits[i] - 48]++;
		}
		else printf("ERROR: inputs for function are not digits");
	}

	for (i = 0; i < 10; i++) {
		if (digits[i] == 2)
			(*cows)++;
	}
	//get cows
	//cows=number of exactly the same poistion and number in guess and in opponent_digits
	for (i = 0; (guess[i] != '\0') || (opponent_digits[i] != '\0'); i++) {
		if (guess[i] == opponent_digits[i]) {
			(*bulls)++;
			index = (int)guess[i] - 48;
			if (0 <= index && index < 10) {
				if (digits[index] == 2) {
					(*cows)--;
				}
			}
		}
	}
	printf("number of bulls:%d; number of cows:%d\n", *bulls, *cows);
	return;

}