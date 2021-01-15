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
*/
BOOL game_session(int Ind, char* message_to_file, char* message_from_file, BOOL users_name_flag, char* user_title, char* user_opposite_title, int oppsite_ind, HANDLE file_mutex, int* opponent_alive);


BOOL wait_for_another_client(int Ind, int oppsite_ind, int* oponnent_alive);


BOOL reset_game(SOCKET socket);

BOOL read_from_pointer_in_file(int offset, char* buffer);

BOOL clien_versus(char* user_title, char* user_name, char* user_opposite_title, int oppsite_ind, int Ind, char* oppsite_user_name, HANDLE file_mutex, int* opponent_alive);

BOOL client_move(int Ind, char* your_guess, char* message_from_file, BOOL users_name_flag,
	char* user_title, char* user_opposite_title, int oppsite_ind, char* your_number
	, HANDLE file_mutex, char* bulls, char* cows, char* opponent_guess, int* opponent_alive);

BOOL get_round_result(int Ind, char* user_title, char* user_opposite_title, int oppsite_ind,
	HANDLE file_mutex, char* bulls, int* round_result, int* message_type_to_send, int* opponent_alive);

BOOL get_param_from_file(char* string_from_file, char* param, int start_point, char* user_opposite_title);

void get_bulls_and_cows(char* guess, char* opponent_digits, int* cows, int* bulls);

BOOL close_file_handle(HANDLE file_mutex);

BOOL get_opponent_number(int Ind, char* user_title, char* user_opposite_title, int oppsite_ind,
	HANDLE file_mutex, char* opponent_number, char* your_number, int* opponent_alive);

BOOL wait_for_client_answer(int* opponent_alive, int choose_event, int opponnent_ind);

static DWORD ServiceThread(LPVOID lpParam);

static int FindFirstUnusedThreadSlot();
void close_thread_and_sockets();

void close_event_and_mutex(HANDLE file_mutex);
static DWORD ServiceThread(LPVOID lpParam);