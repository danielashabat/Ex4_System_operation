/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
/*Authors -Daniela Shabat 316415645, Anat Sinai 312578149.
Project – Cows and Bulls (exersice 4 in System Operation course)
Description –  this file implement the client main

 */
 /*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/


/*------------Includes-------------*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <conio.h>

#include "server_main_header.h"


/*------------functions declaration--------*/
/*this function find a free slot in ThreadHandles
inputs: ptr_Ind- pointer to int
return values:
FALSE - if an error occured during the function, returns FALSE ->need to terminate server
otherwise it returns TRUE
*/
static BOOL FindFirstUnusedThreadSlot(int* ptr_Ind);

/*this function checks if any of the threads terminated with a failure
if fail found the function returns TRUE, otherwise returns FALSE*/
static BOOL service_thread_failed();

/*check if there is non signaled threads running in program */
static int open_threads();

/*
*create Thread data struct and fill the paramers needed for the thread function
*Accept:
*socket : pointer to socket
ptr_to_thread_data : pointer to ThreadData
*/
static BOOL Create_Thread_data(SOCKET* socket, int num_of_thread, ThreadData** ptr_to_thread_data);
/*------------Globals vars--------*/
HANDLE ThreadHandles[NUM_OF_WORKER_THREADS];
SOCKET ThreadInputs[NUM_OF_WORKER_THREADS];
/* -------------main------------------*/
int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("ERROR:there is %d arguments, need to be 2!\n", argc);
        return 1;
    }

    int port = 0;
    port = atoi(argv[1]);
    
    int Ind;
    SOCKET MainSocket = INVALID_SOCKET;
    unsigned long Address;
    SOCKADDR_IN service;
    int bindRes;
    int ListenRes;
    ThreadData* ptr_to_thread = NULL;
    // Initialize Winsock.
    WSADATA wsaData;
    int StartupRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
   
    BOOL ret_val;
    SOCKET AcceptSocket;
    int program_running = 1;
    int i = 0;
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
        goto server_main_cleanup_1;
    }

    //create a sockaddr_in 

    Address = inet_addr(SERVER_ADDRESS_STR);
    if (Address == INADDR_NONE)
    {
        printf("The string \"%s\" cannot be converted into an ip address. ending program.\n",
            SERVER_ADDRESS_STR);
        goto server_main_cleanup_2;
    }

    service.sin_family = AF_INET;
    service.sin_addr.s_addr = Address;
    service.sin_port = htons(port);
    /*bind*/

    bindRes = bind(MainSocket, (SOCKADDR*)&service, sizeof(service));
    if (bindRes == SOCKET_ERROR)
    {
        printf("bind( ) failed with error %ld. Ending program\n", WSAGetLastError());
        goto server_main_cleanup_2;
    }

    // Listen on the Socket.
    ListenRes = listen(MainSocket, SOMAXCONN);
    if (ListenRes == SOCKET_ERROR)
    {
        printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
        goto server_main_cleanup_2;
    }

    // Initialize all thread handles to NULL, to mark that they have not been initialized
    for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
        ThreadHandles[Ind] = NULL;
    for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
        ThreadInputs[Ind] = NULL;
    ret_val = create_mutexs_and_events();
    if (!ret_val) {
        goto server_main_cleanup_2;
    }

   

    while (program_running)
    {
        /*check for new connection*/
        fd_set set;
        struct timeval time_out;
        FD_ZERO(&set); /* clear the set */
        FD_SET(MainSocket, &set); /* add our file descriptor to the set */
        time_out.tv_sec = 0;//do polling
        time_out.tv_usec = 0;
        int rv = select(MainSocket + 1, &set, NULL, NULL, &time_out);
        if (rv == SOCKET_ERROR)
        {
            DEBUG("ERROR: Socket connection disconnected\n");
            return SOCKET_ERROR;
        }
        /*if new connection found, create new ServiceThread*/
        if (FD_ISSET(MainSocket, &set))
        {
            AcceptSocket = accept(MainSocket, NULL, NULL);

            if (AcceptSocket == INVALID_SOCKET)
            {
                printf("Accepting connection with client failed, error %ld\n", WSAGetLastError());
                goto server_main_cleanup_3;

            }

            printf("Client Connected.\n");



            if (!FindFirstUnusedThreadSlot(&Ind)) {
                printf("ERROR: FindFirstUnusedThreadSlot failed!\n");
                goto server_main_cleanup_final;
            }


            if (Ind == NUM_OF_WORKER_THREADS) //no slot is available
            {
                /////****************Send client message
                printf("No slots available for client, dropping the connection.\n");
                closesocket(AcceptSocket); //Closing the socket, dropping the connection.
            }
            else
            {
                ThreadInputs[Ind] = AcceptSocket;
                ret_val = Create_Thread_data(&(ThreadInputs[Ind]), Ind, &ptr_to_thread);
                if (!ret_val) {
                    goto server_main_cleanup_3;
                }
                ret_val = CreateThreadSimple((LPTHREAD_START_ROUTINE)ServiceThread, ptr_to_thread, ThreadHandles + Ind);
                if (!ret_val) {
                    printf("ERRROR:create thread failed!\n");
                    goto server_main_cleanup_final;
                }
            }
        }//end creating new service thread

        if (service_thread_failed()) {//check for failures
            printf("ERROR:server detected error in one of the threads!\n");
            goto server_main_cleanup_final;
        }

        /*terminate after 'exit'*/
        char c;
        char exit_arr[4] = { 'e','x','i','t' };

        if (_kbhit()) {
            c = _getch();
            if (c == exit_arr[i])
                i++;
            else
                i = 0;
        }
        if (i == 4)
            program_running = 0;
    }
    //terminate proccess after exit 

    goto server_main_cleanup_final;


/*---------------goto cleanup-------------------*/
server_main_cleanup_1:
    if (WSACleanup() == SOCKET_ERROR)
        printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());

server_main_cleanup_2:
    if (closesocket(MainSocket) == SOCKET_ERROR)
        printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());
    if (WSACleanup() == SOCKET_ERROR)
        printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());

server_main_cleanup_3:

    close_thread_and_sockets();
    if (WSACleanup() == SOCKET_ERROR)
        printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
    if (ptr_to_thread != NULL) {
        free(ptr_to_thread);
    }
    close_event_and_mutex();
    RemoveDirectoryA("GameSession.txt");
    return 0;

server_main_cleanup_final:
    printf("Server ends program! Bye!\n");
    if (open_threads())
        printf("Oh No! there is open clients! please close all clients\n");
    
    for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
    {
        if (ThreadHandles[Ind] != NULL)
        {
            DWORD Res = WaitForSingleObject(ThreadHandles[Ind], 15000);

            if (Res == WAIT_OBJECT_0)
            {
                closesocket(ThreadInputs[Ind]);
                CloseHandle(ThreadHandles[Ind]);
                ThreadHandles[Ind] = NULL;
                break;
            }
            else
            {
                printf("Waiting for thread failed. Terminate Thread\n");
                ret_val = TerminateThread(ThreadHandles[Ind], 0x55);
                if (FALSE == ret_val)
                {
                    printf("Error when terminating\n");
                }
                return;
            }
        }
    }
    if (ptr_to_thread != NULL) {
        free(ptr_to_thread);
    }
    
    close_event_and_mutex();
    if (WSACleanup() == SOCKET_ERROR)
        printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
    RemoveDirectoryA("GameSession.txt");
    return 0;
}

/*check if there is non signaled threads running in program */
int open_threads()
{
    DWORD Res = 0;
    for (int Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++) {
        if (ThreadHandles[Ind] != NULL) {
            Res = WaitForSingleObject(ThreadHandles[Ind], 0);
            if (Res != WAIT_OBJECT_0)
                return TRUE;
        }
    }
    return FALSE;
}


BOOL service_thread_failed() {
    DWORD Res = 0;
    for (int Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++) {
        if (ThreadHandles[Ind] != NULL) {
            Res = WaitForSingleObject(ThreadHandles[Ind], 0);
            if (Res == WAIT_OBJECT_0) {
                DWORD exit_code = 0;
                BOOL ret_exit_code = TRUE;
                ret_exit_code = GetExitCodeThread(ThreadHandles[Ind], &exit_code);
                if (ret_exit_code) {
                    if (exit_code == 0) {//thread function failed 
                        return TRUE;
                    }
                }
                else {
                    printf("GetExitCodeThread failed\n");
                    return TRUE;//end program
                }

            }
        }

    }
    return FALSE;
}


void close_thread_and_sockets() {
    for ( int Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
    {
        if (ThreadHandles[Ind] != NULL)
        {
            CloseHandle(ThreadHandles[Ind]);
        }
        if (ThreadInputs[Ind] != NULL) {
            closesocket(ThreadInputs[Ind]);
        }
    }
    
}


/*this function find a free slot in ThreadHandles
inputs: ptr_Ind- pointer to int
return values:
FALSE - if an error occured during the function, returns FALSE ->need to terminate server
otherwise it returns TRUE 
*/
static BOOL FindFirstUnusedThreadSlot(int *ptr_Ind){

    int Ind = 0;

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
                if (service_thread_failed()) { //checks for a failure
                    return FALSE;
                }
                CloseHandle(ThreadHandles[Ind]);
                ThreadHandles[Ind] = NULL;
                break;
            }
            else if ((Res == WAIT_ABANDONED) || (Res == WAIT_FAILED)) {
                return FALSE;//the function failed, need to end server
            }
        }
    }

    *ptr_Ind = Ind;
    return TRUE;//function succeed
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


BOOL Create_Thread_data(SOCKET* socket, int num_of_thread, ThreadData** ptr_to_thread_data ) {
    (*ptr_to_thread_data) = (ThreadData*)malloc(sizeof(ThreadData));
    if (*ptr_to_thread_data == NULL) {
        printf("ERROR: allocation failed!\n");
        return FALSE;
    }
    (*ptr_to_thread_data)->thread_number = num_of_thread;
    (*ptr_to_thread_data)->p_socket = socket;
    //(*ptr_to_thread_data)->file_mutex = file_handle_mutex;
   
    return TRUE;
}

