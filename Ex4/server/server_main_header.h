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

BOOL  CreateThreadSimple(LPTHREAD_START_ROUTINE p_start_routine,
	LPVOID p_thread_parameters, HANDLE* thread_handle);

BOOL Create_Thread_data(SOCKET* socket, int num_of_thread, HANDLE file_handle_mutex, ThreadData** ptr_to_thread_data);

BOOL create_mutexs_and_events();

BOOL file_handle(HANDLE file_mutex);

int other_thread_ind(int Ind, char* user_name, char* oppsite_user_name);

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