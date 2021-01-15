#include "SocketSendRecvTools.h"


typedef struct ThreadData {
	SOCKET* p_socket;// the path of the input file
	int thread_number;
	HANDLE file_mutex;
} ThreadData;

//typedef enum { SUCCEEDED, SOCKET_ERROR, SUCCEEDED = TRNS_SUCCEEDED }server_main_exit;



//for get_round_result function
typedef enum { NO_WINNER, USER_WIN, OPPONENT_WIN, TIE } round_result;

BOOL get_server_responed(int* server_denied_or_approved);

BOOL disconnect_client();
/*
*this function create the thred and check if it's failed  
*Accept:
*p_start_routine : the thread function name
*p_thread_parameters: ThreadData variable
thread_handle : the handle of specific  thread
*/
BOOL  CreateThreadSimple(LPTHREAD_START_ROUTINE p_start_routine,
	LPVOID p_thread_parameters, HANDLE* thread_handle);

/*
*create Thread data struct and fill the paramers needed for the thread function
*Accept:
*socket : pointer to socket
*file _handle_mutex: get the handle tomutex that we created in main
ptr_to_thread_data : pointer to ThreadData
*/
BOOL Create_Thread_data(SOCKET* socket, int num_of_thread, HANDLE file_handle_mutex, ThreadData** ptr_to_thread_data);


/*
*create all relevant events and mutex for the thread use
*/
BOOL create_mutexs_and_events();

/*
*this function check if the file is  exist or assigned as NULL, if not-> we create new file . 
If it is exits - we set event that both users checked the file. all of this protected with mutex.
Accept
file_mutex : handle to the mutex that protect files
*/
BOOL file_handle(HANDLE file_mutex);

/*
Each thread get index 0 or 1 
if your index is 0 the opponnet is 1 and backward.
evert user get his title in the file and the titleof the opponent
Accept
Ind : user ind
user_title : pointer to char 
oppsite_user_name : pointer to char the contain the opponent title

*/
int other_thread_ind(int Ind, char* user_title, char* oppsite_title);

/*
functionality: each thread write to a shared file. after they both finish writing, each thread in his turn read the opponent message.
Ind- the user ind 
message_to_file- the message to write in the file
message_from_file - the message the thread read from file.(opponent message)
users_name_flag - if we want to get user name
user_title - title of the user to put in fille
user_opposite_title - tilleof theopponent user to file
oppsite_ind - the index of the opponent to use for event -to know that the opponent also fininhes
*/
BOOL game_session(int Ind, char* message_to_file, char* message_from_file, BOOL users_name_flag, char* user_title, char* user_opposite_title, int oppsite_ind, HANDLE file_mutex, int* opponent_alive);

//
BOOL wait_for_another_client(int Ind, int oppsite_ind, int* oponnent_alive);

/*
if we failed at connention in one thread, so we want to continue in the opponet one
*/
BOOL reset_game(SOCKET socket);


/*
read lines from file from the start to the end of file, 
bufffer- poinet to string to save the result
offet - where to start
*/
BOOL read_from_pointer_in_file(int offset, char* buffer);

/*
this function find the opponent user name by  writing and reading from file
Accept:
### all this params willbe used in more function sowewill explain them here ##
Ind- the user ind
user_title - title of the user to put in fille
user_opposite_title - tilleof theopponent user to file
user_name - your user name

oppsite_ind - the index of the opponent to use for event -to know that the opponent also fininhes
file mutew - handle to mutex to protect file 
opponent alive - pointer to value - 0 if the opponet failed , 1 if not
##end##

for this function 
oppsite_user_name  - pointer to char to save the oppsit user name
*/
BOOL clien_versus(char* user_title, char* user_name, char* user_opposite_title, int oppsite_ind, int Ind, char* oppsite_user_name, HANDLE file_mutex, int* opponent_alive);


/*
In this function we manage the guesses 
first , write both guesses to the file  and read the opponnet guess
then we calculate how much bulls and cows the opponent has with our number
we write the bulls and cows to the file and reade ours
we check all the ti,e if opponet is still in the game

opponent_guess - pointer to string tosave opponet guess to send client
bulls,cows - pointer to string tosave the number
*/
BOOL client_move(int Ind, char* your_guess, char* message_from_file, BOOL users_name_flag,
	char* user_title, char* user_opposite_title, int oppsite_ind, char* your_number
	, HANDLE file_mutex, char* bulls, char* cows, char* opponent_guess, int* opponent_alive);


/*
After we knows the bulls and cows we knows the winner
we write to the file if we have 4 bulls- which means we won
then we read result from file , and send the client  if we won or lose

round_result - pointer to string  to save the result of this round
*/
BOOL get_round_result(int Ind, char* user_title, char* user_opposite_title, int oppsite_ind,
	HANDLE file_mutex, char* bulls, int* round_result, int* message_type_to_send, int* opponent_alive);


/*
We read from file two lines sepratewith \n - one for each user with the user title
In this function, each user read what is under the opponet title 
Accept:
string_from_file - the 2lines that we got from file
param - pointer to sting to save params
start_point - the length of the title
user_opposite_title - the opponent title - sowe whould know where to start looking
*/
BOOL get_param_from_file(char* string_from_file, char* param, int start_point, char* user_opposite_title);

/*
*get_bulls_and_cows assigns to bullsand cows the results
*
* Accepts:
*------ -
*guess - pointer to string
* opponent_digits - pointer to string
* bulls - pointer to int
* cows - pointer to int
*/
void get_bulls_and_cows(char* guess, char* opponent_digits, int* cows, int* bulls);

/*
Every time we finish on game session - means that we wrote our info and get the opponnet info
we close the handle to file and assign NULL if we are the first one
if we are the second we set the event that we both finished

file_mutex - handle to mutex to protect file

*/
BOOL close_file_handle(HANDLE file_mutex);


/*
We get the number of the opponent user to by write and read to the file
*/
BOOL get_opponent_number(int Ind, char* user_title, char* user_opposite_title, int oppsite_ind,
	HANDLE file_mutex, char* opponent_number, char* your_number, int* opponent_alive);


/*
In sevreal place we want towait for event for the second client 
we use wait for multiple object because we want to wait to the event the the othr user succesed or the event the the othe user disconncet
this way we  won't be stuck in waiting for event
 if the user disconneted we put 0 in opponent_alive
 chhose event - we choose the event that we are waiting for

*/
BOOL wait_for_client_answer(int* opponent_alive, int choose_event, int opponnent_ind);


/* The threadfncion - we manage  the communcation with this function */
static DWORD ServiceThread(LPVOID lpParam);


/* 
we find the first abailbe qslot and return it's index to send the thread function
*/
static BOOL FindFirstUnusedThreadSlot(int* ptr_Ind);
/*
close all the thread and socket that we have opened if the server got exit 
*/
void close_thread_and_sockets();

/*
close all mutex and event - in every case 
*/
void close_event_and_mutex(HANDLE file_mutex);



/*this function checks if any of the threads terminated with a failure
if fail found the function returns TRUE, otherwise returns FALSE*/
BOOL service_thread_failed();

int open_threads();
