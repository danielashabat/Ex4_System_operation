#include "SocketSendRecvTools.h"


typedef struct ThreadData {
	SOCKET* p_socket;// the path of the input file
	int thread_number;
	HANDLE file_mutex;
} ThreadData;

//typedef enum { SUCCEEDED, SOCKET_ERROR, SUCCEEDED = TRNS_SUCCEEDED }server_main_exit;



//for get_round_result function
typedef enum { NO_WINNER, USER_WIN, OPPONENT_WIN, TIE } round_result;

int other_thread_ind(int Ind, char* user_name, char* oppsite_user_name);
BOOL  CreateThreadSimple(LPTHREAD_START_ROUTINE p_start_routine,
	LPVOID p_thread_parameters, HANDLE* thread_handle);
BOOL Create_Thread_data(SOCKET* socket, int num_of_thread,HANDLE file_mutex, ThreadData** ptr_to_thread_data);
BOOL file_handle( HANDLE file_mutex);
static DWORD ServiceThread(LPVOID lpParam);
BOOL create_mutexs_and_events();
BOOL game_session(int Ind, char* message_to_file, char* message_from_file, BOOL users_name_flag, char* user_title, char* user_opposite_title, int oppsite_ind, HANDLE file_mutex);
BOOL clien_versus(char* user_title, char* user_name, char* user_opposite_title, int oppsite_ind, int Ind, char* oppsite_user_name);
void get_bulls_and_cows(char* guess, char* opponent_digits, int* cows, int* bulls);
BOOL close_file_handle(HANDLE file_mutex);
BOOL get_round_result(int Ind, char* user_title, char* user_opposite_title, int oppsite_ind,HANDLE file_mutex, char* bulls, int* round_result, int* message_type_to_send);
BOOL disconnect_client();
BOOL wait_for_another_client(int Ind, int oppsite_ind, int* oponnent_alive);