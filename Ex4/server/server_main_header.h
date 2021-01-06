#include "SocketSendRecvTools.h"


typedef struct ThreadData {
	SOCKET* p_socket;// the path of the input file
	HANDLE Event_first_thread;
	HANDLE Event_second_thread;
	int thread_number;
} ThreadData;

BOOL  CreateThreadSimple(LPTHREAD_START_ROUTINE p_start_routine,
	LPVOID p_thread_parameters,
	LPDWORD p_thread_id, HANDLE* thread_handle);
BOOL Create_Thread_data(SOCKET* ptr_socket, HANDLE event_first_thread, HANDLE event_second_thread, int num_of_thread, ThreadData** ptr_to_thread_data);