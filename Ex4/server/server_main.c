/*------------Includes-------------*/

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <winsock2.h>

#include "SocketShared.h"
#include "SocketSendRecvTools.h"
#include "server_main_header.h"
/*-------------Define Varibles--------*/
#define NUM_OF_WORKER_THREADS 2

#define MAX_LOOPS 3

#define SEND_STR_SIZE 100
HANDLE event_thread[NUM_OF_WORKER_THREADS];

HANDLE ThreadHandles[NUM_OF_WORKER_THREADS];
SOCKET ThreadInputs[NUM_OF_WORKER_THREADS];
SOCKET AcceptSocket;
HANDLE create_file_mutex = NULL;
/* -------------main------------------*/
int main() {
    int Ind;
    int Loop;
    SOCKET MainSocket = INVALID_SOCKET;
    unsigned long Address;
    SOCKADDR_IN service;
    int bindRes;
    int ListenRes;
    ThreadData* ptr_to_thread = NULL;
    // Initialize Winsock.
    WSADATA wsaData;
    int StartupRes = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (StartupRes != NO_ERROR)
    {
        printf("error %ld at WSAStartup( ), ending program.\n", WSAGetLastError());
        // Tell the user that we could not find a usable WinSock DLL.                                  
        return;
    }
    
    MainSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (MainSocket == INVALID_SOCKET)
    {
        printf("Error at socket( ): %ld\n", WSAGetLastError());
        goto server_cleanup_1;
    }
    
    //create a sockaddr_in 

    Address = inet_addr(SERVER_ADDRESS_STR);
    if (Address == INADDR_NONE)
    {
        printf("The string \"%s\" cannot be converted into an ip address. ending program.\n",
            SERVER_ADDRESS_STR);
        goto server_cleanup_2;
    }

    service.sin_family = AF_INET;
    service.sin_addr.s_addr = Address;
    service.sin_port = htons(SERVER_PORT);
    /*bind*/

    bindRes = bind(MainSocket, (SOCKADDR*)&service, sizeof(service));
    if (bindRes == SOCKET_ERROR)
    {
        printf("bind( ) failed with error %ld. Ending program\n", WSAGetLastError());
        goto server_cleanup_2;
    }

    // Listen on the Socket.
    ListenRes = listen(MainSocket, SOMAXCONN);
    if (ListenRes == SOCKET_ERROR)
    {
        printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
        goto server_cleanup_2;
    }
    //Initalize all event to NULL
    for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
        event_thread[Ind] = NULL;
    // Initialize all thread handles to NULL, to mark that they have not been initialized
    for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
        ThreadHandles[Ind] = NULL;

    for (Loop = 0; Loop < MAX_LOOPS; Loop++)
    {
        AcceptSocket = accept(MainSocket, NULL, NULL);
        if (AcceptSocket == INVALID_SOCKET)
        {
            printf("Accepting connection with client failed, error %ld\n", WSAGetLastError());
            //goto server_cleanup_3;
        }

        printf("Client Connected.\n");

        Ind = FindFirstUnusedThreadSlot();

        if (Ind == NUM_OF_WORKER_THREADS) //no slot is available
        {
            /////****************Send client message
            printf("No slots available for client, dropping the connection.\n");
            closesocket(AcceptSocket); //Closing the socket, dropping the connection.
        }
        else
        {
            Create_Thread_data(AcceptSocket, Ind,&ptr_to_thread);
            CreateThreadSimple(ServiceThread, ptr_to_thread, ThreadHandles+Ind);
           //ThreadInputs[Ind] = AcceptSocket; // shallow copy: don't close 
                                              // AcceptSocket, instead close 
                                              // ThreadInputs[Ind] when the
                                              // time comes.
            
        }
    }
/*---------------goto cleanup-------------------*/
server_cleanup_1:
    if (WSACleanup() == SOCKET_ERROR)
        printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());

server_cleanup_2:
    if (closesocket(MainSocket) == SOCKET_ERROR)
        printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());
    if (WSACleanup() == SOCKET_ERROR)
        printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
}


/* The WinSock DLL is acceptable. Proceed. */

static int FindFirstUnusedThreadSlot(){

    int Ind;

    for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
    {
        if (ThreadHandles[Ind] == NULL)
            break;
        else
        {
            // poll to check if thread finished running:
            DWORD Res = WaitForSingleObject(ThreadHandles[Ind], 0);

            if (Res == WAIT_OBJECT_0) // this thread finished running
            {
                CloseHandle(ThreadHandles[Ind]);
                ThreadHandles[Ind] = NULL;
                break;
            }
        }
    }

    return Ind;
}

static DWORD ServiceThread(SOCKET* t_socket) {

    char SendStr[SEND_STR_SIZE];
    char ParamStr[SEND_STR_SIZE];
    int num_of_param;
    BOOL Done = FALSE;
    TransferResult_t SendRes;
    TransferResult_t RecvRes;

  
    char* AcceptedStr = NULL;
    RecvRes = ReceiveString(&AcceptedStr, *t_socket);
    if (RecvRes == TRNS_FAILED)
    {
        printf("Service socket error while reading, closing thread.\n");
        closesocket(*t_socket);
        return 1;
    }
    char user_name[20];
    if (check_if_message_type_instr_message(AcceptedStr, "CLIENT_REQUEST")) {
        int indexes[1];
        int lens[1];
        get_param_index_and_len(&indexes, &lens, AcceptedStr, strlen(AcceptedStr));
        strncpy_s(user_name, lens[0], *(AcceptedStr + indexes[0]), strlen(user_name));
        strcpy_s(SendStr, strlen("SERVER_APPROVED"), "SERVER_APPROVED");
        SendRes = SendString(SendStr, *t_socket);
        strcpy_s(SendStr, strlen("SERVER_MAIN_MENU"), "SERVER_MAIN_MENU");
        SendRes = SendString(SendStr, *t_socket);
        
    }
    if (check_if_message_type_instr_message(AcceptedStr, "CLIENT_DISCONNECT")) {
        Done == TRUE;
        
    }
    


}


BOOL  CreateThreadSimple(LPTHREAD_START_ROUTINE p_start_routine,
    LPVOID p_thread_parameters, HANDLE* thread_handle)
{


    if (NULL == p_start_routine)
    {
        printf("Error when creating a thread");
        printf("Received null pointer");
        return FALSE;
    }

    

    *thread_handle = CreateThread(
        NULL,                /*  default security attributes */
        0,                   /*  use default stack size */
        p_start_routine,     /*  thread function */
        p_thread_parameters, /*  argument to thread function */
        0,                   /*  use default creation flags */
        NULL);        /*  returns the thread identifier */

    if (NULL == *thread_handle)
    {
        printf("Couldn't create thread\n");
        return FALSE;
    }

    return TRUE;
}


BOOL Create_Thread_data(SOCKET socket, int num_of_thread, ThreadData** ptr_to_thread_data) {
    (*ptr_to_thread_data) = (ThreadData*)malloc(sizeof(ThreadData));
    if (*ptr_to_thread_data == NULL) {
        printf("ERROR: allocation failed!\n");
        return FALSE;
    }
    (*ptr_to_thread_data)->thread_number = num_of_thread;
    (*ptr_to_thread_data)->p_socket = &socket;
    // (*ptr_to_thread_data)->Event_first_thread = event_first_thread;
    // (*ptr_to_thread_data)->Event_second_thread = event_second_thread;
    return TRUE;
}
BOOL create_mutexs_and_events() {
    
    event_thread[0] = CreateEvent(
        NULL,               // default security attributes
        TRUE,               // manual-reset event
        FALSE,              // initial state is nonsignaled
        TEXT("First_thread")  // object name
    );

    if (event_thread[0] == NULL)
    {
        printf("CreateEvent failed (%d)\n", GetLastError());
        return FALSE;
    }

    event_thread[1] = CreateEvent(
        NULL,               // default security attributes
        TRUE,               // manual-reset event
        FALSE,              // initial state is nonsignaled
        TEXT("First_thread")  // object name
    );

    if (event_thread[1] == NULL)
    {
        printf("CreateEvent failed (%d)\n", GetLastError());
        return FALSE;
    }
    create_file_mutex = CreateMutex(
        NULL,	/* default security attributes */
        FALSE,	/* initially not owned */
        NULL);	/* unnamed mutex */
    if (NULL == create_file_mutex)
    {
        printf("Error when creating mutex: %d\n", GetLastError());
        return FALSE;
    }

    return TRUE;
