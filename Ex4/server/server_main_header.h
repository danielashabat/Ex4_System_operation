#include "SocketSendRecvTools.h"


typedef struct ThreadData {
	SOCKET* p_socket;// the path of the input file
	int thread_number;
	HANDLE file_mutex;
} ThreadData;
int other_thread_ind(int Ind, char* user_name, char* oppsite_user_name);
BOOL  CreateThreadSimple(LPTHREAD_START_ROUTINE p_start_routine,
	LPVOID p_thread_parameters, HANDLE* thread_handle);
BOOL Create_Thread_data(SOCKET* socket, int num_of_thread,HANDLE file_mutex, ThreadData** ptr_to_thread_data);
BOOL file_handle(char* user_name, int Ind, HANDLE file_mutex);
static DWORD ServiceThread(LPVOID lpParam);
BOOL create_mutexs_and_events();
BOOL game_session(int Ind, char* message_to_file, char* message_from_file, BOOL users_name_flag, int offset_first, int* offset_end, char* user_title, char* user_opposite_title, int oppsite_ind, HANDLE file_mutex);
BOOL clien_versus(char* user_title, char* user_name, char* user_opposite_title, int oppsite_ind, int Ind, char* oppsite_user_name);