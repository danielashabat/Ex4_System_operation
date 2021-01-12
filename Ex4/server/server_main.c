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


#include "SocketShared.h"
#include "SocketSendRecvTools.h"
#include "server_main_header.h"
#include "message.h"
/*-------------Define Varibles--------*/
#define NUM_OF_WORKER_THREADS 3
#define USER_LEN 21
#define USER_NAME_MSG 30
#define SEND_STR_SIZE 100
#define USER_TITLE 7
#define GUESS 5//need to include '\0'

#define CHECK_CONNECTION(RESULT) if (RESULT != TRNS_SUCCEEDED) {connected_to_client=0; break;}
#define IS_FALSE(RESULT,MSG) if (RESULT != TRUE)\
{\
    printf("function:[%s line:%d] %s\n", __func__,__LINE__,MSG);\
    connected_to_client=0; break;\
}

HANDLE event_thread[NUM_OF_WORKER_THREADS];
HANDLE event_thread_end_reading[NUM_OF_WORKER_THREADS];
HANDLE event_file= NULL;
HANDLE close_file_event = NULL;
HANDLE file = NULL;
HANDLE connected_clients_mutex = NULL;
HANDLE ThreadHandles[NUM_OF_WORKER_THREADS];
SOCKET ThreadInputs[NUM_OF_WORKER_THREADS];
//SOCKET AcceptSocket;

/* -------------main------------------*/
int connected_clients;
int other_thread_ind(int Ind, char* user_name, char* oppsite_user_name);
BOOL  CreateThreadSimple(LPTHREAD_START_ROUTINE p_start_routine,
    LPVOID p_thread_parameters, HANDLE* thread_handle);
BOOL Create_Thread_data(SOCKET socket, int num_of_thread, ThreadData** ptr_to_thread_data);
BOOL file_handle(char* user_name, int Ind);
static DWORD ServiceThread(LPVOID lpParam);
BOOL create_mutexs_and_events();
BOOL game_session(int Ind, char* message_to_file, char** message_from_file);




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
    HANDLE file_mutex = NULL;
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
    service.sin_port = htons(SERVER_PORT);
    /*bind*/

    bindRes = bind(MainSocket, (SOCKADDR*)&service, sizeof(service));
    if (bindRes == SOCKET_ERROR)
    {
        printf("bind( ) failed with error %ld. Ending program\n", WSAGetLastError());
        goto server_main_cleanup_2;
    }

    // Listen on the Socket.
    ListenRes = listen(MainSocket, SOMAXCONN);
    printf("waiting for clients to connect\n");
    if (ListenRes == SOCKET_ERROR)
    {
        printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
        goto server_main_cleanup_2;
    }
    //Initalize all event to NULL
    for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
        event_thread[Ind] = NULL;
    // Initialize all thread handles to NULL, to mark that they have not been initialized
    for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
        ThreadHandles[Ind] = NULL;
    ret_val = create_mutexs_and_events();
    //anat
    file_mutex = CreateMutex(
        NULL,	/* default security attributes */
        FALSE,	/* initially not owned */
        NULL);	/* unnamed mutex */
    if (NULL == file_mutex)
    {
        printf("Error when creating mutex: %d\n", GetLastError());
        goto server_main_cleanup_2;
    }
    connected_clients = 0;

    while (program_running)
    {
       //char c;
       //char exit_arr[4] = { 'e','x','i','t' };

       //if (_kbhit()) {
       //    c = _getch();
       //    if (c == exit_arr[i])
       //        i++;
       //    else
       //        i = 0;
       //    }
       //if (i == 4)
       //    program_running = 0;
       //
       //if (!program_running) break;//end program 

        /*check for new connection*/
        fd_set set;
        struct timeval time_out;
        FD_ZERO(&set); /* clear the set */
        FD_SET(MainSocket, &set); /* add our file descriptor to the set */
        time_out.tv_sec = 0;//do polling
        time_out.tv_usec = 0;
        int rv = select(MainSocket  + 1, &set, NULL, NULL, &time_out);
        if (rv == SOCKET_ERROR )
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



            Ind = FindFirstUnusedThreadSlot();

            if (Ind == NUM_OF_WORKER_THREADS) //no slot is available
            {
                /////****************Send client message
                printf("No slots available for client, dropping the connection.\n");
                closesocket(AcceptSocket); //Closing the socket, dropping the connection.
            }
            else
            {
                ThreadInputs[Ind] = AcceptSocket;
                ret_val = Create_Thread_data(&(ThreadInputs[Ind]), Ind, file_mutex, &ptr_to_thread);
                if (!ret_val) {
                    goto server_main_cleanup_3;
                }
                ret_val = CreateThreadSimple((LPTHREAD_START_ROUTINE)ServiceThread, ptr_to_thread, ThreadHandles + Ind);
                if (!ret_val) {
                    printf("ERRROR:create thread failed!\n");
                    goto server_main_cleanup_3;
                }
            }
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



server_main_cleanup_final:
    printf("Server ends program! Bye!\n");
    //printf("closing all open threads\n");
    for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
    {
        if (ThreadHandles[Ind] != NULL)
        {
            // poll to check if thread finished running:
            DWORD Res = WaitForSingleObject(ThreadHandles[Ind], 0);
            if (Res != WAIT_OBJECT_0) {
                printf("Oh No! there is open clients! please close all clients\n");
            }
             Res = WaitForSingleObject(ThreadHandles[Ind], 15000);

            if (Res == WAIT_OBJECT_0)
            {
                closesocket(ThreadInputs[Ind]);
                CloseHandle(ThreadHandles[Ind]);
                ThreadHandles[Ind] = NULL;
                break;
            }
            else
            {
                printf("Waiting for thread failed. Terminate Threads\n");
                ret_val = TerminateThread(ThreadHandles[Ind],0x55);
                if (FALSE == ret_val)
                {
                    printf("Error when terminating\n");
                }
                return;
            }
        }
    }
    if (event_file != NULL) {
        CloseHandle(event_file);
    }
    if (event_thread[0] != NULL) {
        CloseHandle(event_thread[0]);
    }
    if (event_thread[1] != NULL) {
        CloseHandle(event_thread[1]);
    }
    if (file_mutex != NULL) {
        CloseHandle(file_mutex);
    }
    if (WSACleanup() == SOCKET_ERROR)
        printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());

    return 0;
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
    for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
    {
        if (ThreadHandles[Ind] != NULL)
        {
            CloseHandle(ThreadHandles[Ind]);
        }
        if (ThreadInputs[Ind] != NULL) {
            closesocket(ThreadInputs[Ind]);
        }
    }
    if (event_file != NULL) {
        CloseHandle(event_file);
    }
    if (event_thread[0] != NULL) {
        CloseHandle(event_thread[0]);
    }
    if (event_thread[1] != NULL) {
        CloseHandle(event_thread[1]);
    }
    if (file_mutex != NULL) {
        CloseHandle(file_mutex);
    }
    if (WSACleanup() == SOCKET_ERROR)
        printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
    if (file != NULL) {
        CloseHandle(file);
    }

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

static DWORD ServiceThread(LPVOID lpParam) {
    char SendStr[SEND_STR_SIZE];
    char ParamStr[SEND_STR_SIZE];
    int num_of_param;
    BOOL Done = FALSE;
    TransferResult_t SendRes;
    TransferResult_t RecvRes;
    BOOL retval = FALSE;
    SOCKET* t_socket = NULL;
    HANDLE file_mutex = NULL;
    BOOL read_users_flag;
    int Ind;
    char user_number[GUESS] = { 0 };
    char opponent_guess[GUESS] = { 0 };
    char your_guess[GUESS] = { 0 };
    char session_result[USER_NAME_MSG] = { 0 };
    char user_title[USER_TITLE] = { 0 };
    char user_opposite_title[USER_TITLE] = { 0 };
    char* user_opposite_pointer;
    char initial_number[4];
    char* AcceptedStr = NULL;
    char user_name[USER_LEN] = { 0 };
    char oppsite_user_name[USER_NAME_MSG]= { 0 };
    char cows[GUESS] = { 0 };
    char bulls[GUESS] = { 0 };
    int state = CLIENT_REQUEST;

    int message_type = CLIENT_REQUEST;
    int message_type_to_send = CLIENT_REQUEST;

    char* send_params[MAX_PARAMS] = { NULL };
    char* recieve_params[MAX_PARAMS] = { NULL };
    int connected_to_client = 1;
    int ret_val;
    char oppsite_user_name_with_title[30] = {0};
    int round_result = 0;
    int timeout = DEFUALT_TIMEOUT;
    int oponnent_alive = 0;
    //get thread params
    ThreadData* p_params;
    p_params = (ThreadData*)lpParam;
    t_socket = p_params->p_socket;
    Ind = p_params->thread_number;
    file_mutex = p_params->file_mutex;

    int oppsite_ind = other_thread_ind(Ind, user_title, user_opposite_title);
    if (INVALID_SOCKET == *t_socket) {
        printf("ERRROR:socket is invalid!\n");
        return 1;
    }

    while (connected_to_client) {
        //printf("waiting to recieve message\n");
        RecvRes = RecieveMsg(*t_socket, &message_type, recieve_params, timeout);
        timeout = TEN_MIN;
        if (!check_recv(RecvRes)) {
            printf("recieve string failed\n");
            ret_val = disconnect_client();
            IS_FALSE(ret_val, "disconnect_client failed\n");
            return FALSE;
        }
        switch (message_type)
        {
        case CLIENT_REQUEST://get user name from client
            strcpy_s(user_name, USER_LEN, recieve_params[0]);
            ret_val=get_server_responed(&message_type_to_send);
            IS_FALSE(ret_val,"get_server_responed failed\n");
            send_params[0] = "server is full of clients";//in case sending SERVER_DENIED
            ret_val = SendMsg(*t_socket, message_type_to_send, send_params);
            CHECK_CONNECTION(ret_val);
            if (message_type_to_send == SERVER_DENIED) {
                connected_to_client = 0;
                break;
            }

            ret_val = SendMsg(*t_socket, SERVER_MAIN_MENU, NULL);
            CHECK_CONNECTION(ret_val);
            break;

        case CLIENT_VERSUS:
            //search for another client
            ret_val=wait_for_another_client(Ind, oppsite_ind, &oponnent_alive);
            IS_FALSE(ret_val, "wait_for_another_client failed\n");
            if (oponnent_alive) {
                clien_versus(user_title, user_name, user_opposite_title, oppsite_ind, Ind, oppsite_user_name, file_mutex);
                send_params[0] = oppsite_user_name;
                ret_val = SendMsg(*t_socket, SERVER_INVITE, send_params);
                CHECK_CONNECTION(ret_val);

                ret_val = SendMsg(*t_socket, SERVER_SETUP_REQUEST, NULL);
                CHECK_CONNECTION(ret_val);
            }
            else {
                ret_val = SendMsg(*t_socket, SERVER_NO_OPPONENTS, send_params);
                CHECK_CONNECTION(ret_val);
            }
            break;

        case CLIENT_SETUP:
            ret_val = wait_for_another_client(Ind, oppsite_ind, &oponnent_alive);
            IS_FALSE(ret_val, "wait_for_another_client failed\n");
            if (!oponnent_alive) { reset_game(*t_socket); IS_FALSE(ret_val, "reset_game failed\n") break; }

            strcpy_s(user_number, GUESS, recieve_params[0]);
            ret_val = SendMsg(*t_socket, SERVER_PLAYER_MOVE_REQUEST, NULL);
            CHECK_CONNECTION(ret_val);
            break;

        case CLIENT_PLAYER_MOVE:
            ret_val = wait_for_another_client(Ind, oppsite_ind, &oponnent_alive);
            IS_FALSE(ret_val, "wait_for_another_client failed\n");
            if (!oponnent_alive) { reset_game(*t_socket); IS_FALSE(ret_val, "reset_game failed\n") break; }

            strcpy_s(your_guess, GUESS, recieve_params[0]);
            //calculate moves
            client_move(Ind, your_guess, session_result, FALSE, user_title,user_opposite_title, oppsite_ind, user_number, file_mutex , bulls,cows ,opponent_guess);
            send_params[0] = bulls; send_params[1] = cows; send_params[2] = oppsite_user_name; send_params[3] = opponent_guess;
            ret_val = SendMsg(*t_socket, SERVER_GAME_RESULTS, send_params);
            CHECK_CONNECTION(ret_val);

            ret_val=get_round_result(Ind, user_title, user_opposite_title, oppsite_ind,file_mutex, bulls, &round_result,&message_type_to_send);
            IS_FALSE(ret_val, "get_round_result failed\n");

            if (round_result == USER_WIN)
                send_params[0] = user_name;
            else
                send_params[0] = oppsite_user_name;
            send_params[1] = "####";//opponent 4 digits
            ret_val = SendMsg(*t_socket, message_type_to_send, send_params);
            CHECK_CONNECTION(ret_val);

            if (round_result != NO_WINNER) {
                ret_val = SendMsg(*t_socket, SERVER_MAIN_MENU, NULL);
                CHECK_CONNECTION(ret_val);
            }
            break;


        case CLIENT_DISCONNECT:
            ret_val=disconnect_client();
            IS_FALSE(ret_val, "disconnect_client failed\n");
            connected_to_client = 0;
            //free stuff and end program
            break;

        default:
            printf("ERROR: this case don't exist in server\n");
            break;
        }

        free_params(recieve_params);
    }


    return 0;
   

}

BOOL get_server_responed(int *server_denied_or_approved) {
    DWORD wait_code;
    BOOL ret_val;
    //check how many clients connented to server
    wait_code = WaitForSingleObject(connected_clients_mutex, 5000);
    if (WAIT_OBJECT_0 != wait_code)
    {
        printf("-ERROR: %d - WaitForSingleObject failed !\n", GetLastError());
        return FALSE;
    }
    //critical section
    if (connected_clients < 2) {
        connected_clients++;
        *server_denied_or_approved = SERVER_APPROVED;
    }
    else {
        *server_denied_or_approved = SERVER_DENIED;
    }


    //end critical section
    ret_val = ReleaseMutex(connected_clients_mutex);
    if (FALSE == ret_val)
    {
        printf("-ERROR: %d - release semaphore failed !\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}
BOOL disconnect_client() {
    DWORD wait_code;
    BOOL ret_val;
    //decrease connected_clients
    wait_code = WaitForSingleObject(connected_clients_mutex, 5000);
    if (WAIT_OBJECT_0 != wait_code)
    {
        printf("-ERROR: %d - WaitForSingleObject failed !\n", GetLastError());
        return FALSE;
    }
    //critical section
    connected_clients--;
    //end critical section
    ret_val = ReleaseMutex(connected_clients_mutex);
    if (FALSE == ret_val)
    {
        printf("-ERROR: %d - release semaphore failed !\n", GetLastError());
        return FALSE;
    }

    return TRUE;
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


BOOL Create_Thread_data(SOCKET* socket, int num_of_thread,HANDLE file_handle_mutex, ThreadData** ptr_to_thread_data ) {
    (*ptr_to_thread_data) = (ThreadData*)malloc(sizeof(ThreadData));
    if (*ptr_to_thread_data == NULL) {
        printf("ERROR: allocation failed!\n");
        return FALSE;
    }
    (*ptr_to_thread_data)->thread_number = num_of_thread;
    (*ptr_to_thread_data)->p_socket = socket;
    (*ptr_to_thread_data)->file_mutex = file_handle_mutex;
    // (*ptr_to_thread_data)->Event_first_thread = event_first_thread;
    // (*ptr_to_thread_data)->Event_second_thread = event_second_thread;
    return TRUE;
}

BOOL create_mutexs_and_events() {

    event_thread[0] = CreateEvent(
        NULL,               // default security attributes
        TRUE,               // manual-reset event
        FALSE,              // initial state is nonsignaled
        TEXT("First_threadWrite")  // object name
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
        TEXT("Second_threadWrite")  // object name
    );

    if (event_thread[1] == NULL)
    {
        printf("CreateEvent failed (%d)\n", GetLastError());
        return FALSE;
    }

    event_thread_end_reading[0] = CreateEvent(
        NULL,               // default security attributes
        TRUE,               // manual-reset event
        FALSE,              // initial state is nonsignaled
        TEXT("First_threadRead")  // object name
    );

    if (event_thread_end_reading[0] == NULL)
    {
        printf("CreateEvent failed (%d)\n", GetLastError());
        return FALSE;
    }

    event_thread_end_reading[1] = CreateEvent(
        NULL,               // default security attributes
        TRUE,               // manual-reset event
        FALSE,              // initial state is nonsignaled
        TEXT("Second_threadRead")  // object name
    );

    if (event_thread_end_reading[1] == NULL)
    {
        printf("CreateEvent failed (%d)\n", GetLastError());
        return FALSE;
    }
    event_file = CreateEvent(
        NULL,               // default security attributes
        TRUE,               // manual-reset event
        FALSE,              // initial state is nonsignaled
        TEXT("File_Event")  // object name
    );

    if (event_file == NULL)
    {
        printf("CreateEvent failed (%d)\n", GetLastError());
        return FALSE;
    }
    
    close_file_event = CreateEvent(
            NULL,               // default security attributes
            TRUE,               // manual-reset event
            FALSE,              // initial state is nonsignaled
            TEXT("Close_Event")  // object name
        );

    if (close_file_event == NULL)
    {
        printf("CreateEvent failed (%d)\n", GetLastError());
        return FALSE;
    }
    

    connected_clients_mutex = CreateMutex(
        NULL,	/* default security attributes */
        FALSE,	/* initially not owned */
        NULL);	/* unnamed mutex */
    
    if (NULL == connected_clients_mutex)
    {
        printf("Error when creating mutex: %d\n", GetLastError());
       return FALSE;
    }

    return TRUE;
}

/*the first thread get in this function need to open a new file, the other thread release event_file (mutex) */
BOOL file_handle(  HANDLE file_mutex) {
    // check if need to open file - protected by mutex;
    DWORD wait_code;
    BOOL ret_val;
    char file_path[74] = "GameSession.txt";//daniela changed the path to relative path
    BOOL bErrorFlag = FALSE;
    DWORD lpNumberOfBytesWritten;
    LPDWORD* end_of_file_offset;
    printf("start file handle\n");
   /* Create the mutex that will be used to synchronize access to queue */
    wait_code = WaitForSingleObject(file_mutex, INFINITE);
    if (WAIT_OBJECT_0 != wait_code)
    {
        printf("-ERROR: %d - WaitForSingleObject failed !\n", GetLastError());
        return FALSE;
    }
    printf("enter mutex \n");
    //critical section- check if queue is not empty and pop the mission 
    if (file == NULL) {
        ResetEvent(close_file_event);
        file = CreateFileA((LPCSTR)file_path,// file name 
            GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file == INVALID_HANDLE_VALUE)
        {   
            printf("ERROR:create file failed!\n");
            return FALSE;
            
        }
    }
    else {
        printf("second_player_enter");
        //GetFileSize(file, end_of_file_offset);
        //SetFilePointer(file, end_of_file_offset, NULL, FILE_BEGIN); 
        //WriteFile(file, user_name, strlen(user_name), &lpNumberOfBytesWritten, NULL);
        SetEvent(event_file);
       
    }
    //end of critical section 
    //*Release queue mutex
    
    ret_val = ReleaseMutex(file_mutex);
    if (FALSE == ret_val)
    {
        printf("-ERROR: %d - release semaphore failed !\n", GetLastError());
        return FALSE;
    }
    else {
        printf(" release mutex file handle succeed !\n");
    }
    return TRUE;
    
    
    }


int other_thread_ind(int Ind,char* user_name, char* oppsite_user_name) {

    if (Ind == 0) {
        strcpy_s(user_name, USER_TITLE, "user0:");
        strcpy_s(oppsite_user_name, USER_TITLE, "user1:");
        return 1;
    }
    if (Ind == 1) {
        strcpy_s(user_name, 7, "user1:");
        strcpy_s(oppsite_user_name, USER_TITLE, "user0:");
        
        return 0;
    }
}

/*
functionality: each thread write to a shared file. after they both finish writing, each thread in his turn read the opponent message.
message_to_file- the message to write in the file
message_from_file - the message the thread read from file.(opponent message)
*/
BOOL game_session(int Ind , char* message_to_file, char* message_from_file, BOOL users_name_flag,char* user_title, char* user_opposite_title , int oppsite_ind, HANDLE file_mutex) {
    DWORD wait_code;
    BOOL bErrorFlag = FALSE;
    DWORD lpNumberOfBytesWritten;
    BOOL ret_val;
    DWORD dwWaitResultOtherClient;
    char string_from_file[50] = {0};
    char* user_opposite_pointer;
    DWORD offset_end_a;
    printf("start game session \n");
    wait_code = WaitForSingleObject(file_mutex, INFINITE);
    if (WAIT_OBJECT_0 != wait_code)
    {
        printf("-ERROR: %d - WaitForSingleObject failed !\n", GetLastError());
        return FALSE;
    }

    //critical 
    
    
   
    SetFilePointer(file, 0, NULL, FILE_END); //set pointer to end of file
    WriteFile(file, message_to_file, strlen(message_to_file), &lpNumberOfBytesWritten, NULL);
    SetEvent(event_thread[Ind]);
    printf("end writing \n");
    ret_val = ReleaseMutex(file_mutex);
    if (FALSE == ret_val)
    {
        printf("-ERROR: %d - release semaphore failed !\n", GetLastError());
        return FALSE;
    }
    
    //Wait for the second thread to write
    dwWaitResultOtherClient = WaitForSingleObject(
    event_thread[oppsite_ind], // event handle
    INFINITE);
    if (WAIT_OBJECT_0 != dwWaitResultOtherClient)
    {
        printf("-ERROR: %d - WaitForSingleObject failed !\n", GetLastError());
        return FALSE;
    }    // indefinite wait
    printf("can start reading \n");
    
    
    //printf("file size from begin %10d \n", offset_end_a);
    printf("startreading \n");
    if (dwWaitResultOtherClient != WAIT_OBJECT_0) {
        printf("Wait error (%d)\n", GetLastError());
        return FALSE;
    }
    //in your turn read the other user play 
    
    wait_code = WaitForSingleObject(file_mutex, INFINITE);
    if (WAIT_OBJECT_0 != wait_code)
    {
        printf("-ERROR: %d - WaitForSingleObject failed !\n", GetLastError());
        return FALSE;
    }
    SetFilePointer(file, 0, NULL, FILE_BEGIN);
    ret_val = read_from_pointer_in_file(0, string_from_file);
    printf("string_from_file %s \n", string_from_file);
    if (ret_val) {
        char param[20] = { 0 };

        get_param_from_file(string_from_file, param, USER_TITLE - 1,user_opposite_title);
        printf("get_param_succseed\n");
        sprintf_s(message_from_file, USER_NAME_MSG, "%s", param);
        printf("oppsinte name %s\n", message_from_file);
        SetEvent(event_thread_end_reading[Ind]);
    }
   
    ret_val = ReleaseMutex(file_mutex);
    if (FALSE == ret_val)
    {
        printf("-ERROR: %d - release mutex failed !\n", GetLastError());
        return FALSE;
    }
    dwWaitResultOtherClient = WaitForSingleObject(
        event_thread_end_reading[oppsite_ind], // event handle
        INFINITE);
    if (WAIT_OBJECT_0 != dwWaitResultOtherClient)
    {
        printf("-ERROR: %d - WaitForSingleObject failed !\n", GetLastError());
        return FALSE;
    }    // indefinite wait
    close_file_handle(file_mutex);
    dwWaitResultOtherClient = WaitForSingleObject(
        close_file_event, // event handle
        INFINITE);
    if (WAIT_OBJECT_0 != dwWaitResultOtherClient)
    {
        printf("-ERROR: %d - WaitForSingleObject failed !\n", GetLastError());
        return FALSE;
    }    // indefinite wait
}

/*if oponnent_alive=1, the oponnent still in the game. otherwise there is no opponent or he quit*/
BOOL wait_for_another_client(int Ind, int oppsite_ind,int *oponnent_alive) {
    DWORD dwWaitResultOtherClient;
    BOOL ret_val = FALSE;
    ret_val = SetEvent(event_thread[Ind]);
    if (ret_val == FALSE) {
        return FALSE;
    }

    dwWaitResultOtherClient = WaitForSingleObject(
        event_thread[oppsite_ind], // event handle
        15000);//15 sec
    if (WAIT_OBJECT_0 != dwWaitResultOtherClient)
    {
        printf("there is no other client to play :(\n", GetLastError());
        ResetEvent(event_thread[Ind]);
        *oponnent_alive = 0;
        return TRUE;
    }    // indefinite wait
    ResetEvent(event_thread[oppsite_ind]);
    *oponnent_alive = 1;
    return TRUE;

}
BOOL reset_game(SOCKET socket) {
    DWORD ret_val = 0;
    ret_val = SendMsg(socket, SERVER_OPPONENT_QUIT, NULL);
    if (ret_val != TRNS_SUCCEEDED) {
        return FALSE;
    }
    ret_val = SendMsg(socket, SERVER_MAIN_MENU, NULL);
    if (ret_val != TRNS_SUCCEEDED) {
        return FALSE;
    }

}




BOOL read_from_pointer_in_file(int offset, char* buffer) {

   
    DWORD nBytesToRead;
    DWORD dwBytesRead;
    BOOL bResult = FALSE;
    int dw_pointer;
    DWORD end_of_file_offset;
    char* buffer_to_read;
    
    //dw_pointer = SetFilePointer(file, offset, NULL, FILE_BEGIN);//set poniter to offset
    end_of_file_offset = GetFileSize(file, NULL);
    if (end_of_file_offset == INVALID_FILE_SIZE){
        printf("get size error (%d)\n", GetLastError());
        return FALSE;
    }
    printf("file size %10d\n", end_of_file_offset);
    nBytesToRead = end_of_file_offset - offset;
    buffer_to_read = (char*)malloc((nBytesToRead+1) * sizeof(char));
    bResult = ReadFile(file, buffer_to_read, nBytesToRead, &dwBytesRead, NULL);
    if (!bResult) {
        printf("READ from tasks file NOT success!");
        return FALSE;
    }
    buffer_to_read[dwBytesRead] = '\0';
    sprintf_s(buffer, 50,"%s", buffer_to_read);
    free(buffer_to_read);
    printf("READ from tasks file success!");
    return TRUE;
}

BOOL clien_versus(char* user_title,char* user_name, char* user_opposite_title,int oppsite_ind, int Ind, char* oppsite_user_name, HANDLE file_mutex) {

    char message_to_file[USER_NAME_MSG] = {0};
    int offest_end_file = 0;
    DWORD retval;
    DWORD dwWaitResultFile;

    retval = file_handle(file_mutex);
    printf("waiting for other playar\n");
    dwWaitResultFile = WaitForSingleObject(
        event_file, // event handle
        INFINITE);    // indefinite wait

    if (dwWaitResultFile != WAIT_OBJECT_0) {
        printf("Wait error (%d)\n", GetLastError());
        return FALSE;
    }
    printf("file event set\n");
    strcpy_s(message_to_file, USER_NAME_MSG, user_title);
    strcat_s(message_to_file, USER_NAME_MSG, user_name);
    strcat_s(message_to_file, USER_NAME_MSG, "\n\r");
    game_session(Ind, message_to_file, oppsite_user_name, TRUE, user_title, user_opposite_title, oppsite_ind, file_mutex);
    ResetEvent(event_thread[Ind]);
    ResetEvent(event_thread_end_reading[Ind]);
    printf("oppsinte name %s%s \n", oppsite_user_name, user_title);
   
}

BOOL client_move(int Ind, char* your_guess, char* message_from_file, BOOL users_name_flag,
    char* user_title, char* user_opposite_title, int oppsite_ind,char* your_number
    , HANDLE file_mutex , char* bulls, char* cows , char* opponent_guess) {
    int cows_calculate;
    int bulls_calculate;
    DWORD retval;
    DWORD dwWaitResultFile;

    retval = file_handle(file_mutex);
    printf("waiting for other player\n");
    dwWaitResultFile = WaitForSingleObject(
        event_file, // event handle
        INFINITE);    // indefinite wait

    if (dwWaitResultFile != WAIT_OBJECT_0) {
        printf("Wait error (%d)\n", GetLastError());
        return FALSE;
    }
    char guess_message_to_file[USER_NAME_MSG] = { 0 };
    char cows_and_bulls[USER_NAME_MSG] = { 0 };
    char cows_and_bulls_message_to_file[USER_NAME_MSG] = { 0 };
    sprintf_s(guess_message_to_file, USER_NAME_MSG, "%s%s\n\r", user_title, your_guess);
    game_session(Ind, guess_message_to_file, message_from_file, FALSE, user_title, user_opposite_title, oppsite_ind, file_mutex);
    sprintf_s(opponent_guess, GUESS, "%s", message_from_file);
    ResetEvent(event_thread[Ind]);
    ResetEvent(event_thread_end_reading[Ind]);
    
    get_bulls_and_cows(your_number, message_from_file, &cows_calculate, &bulls_calculate);
    sprintf_s(cows_and_bulls, USER_NAME_MSG, "%d,%d", bulls_calculate, cows_calculate);
    sprintf_s(cows_and_bulls_message_to_file, USER_NAME_MSG, "%s%s\n\r", user_title, cows_and_bulls);

    retval = file_handle(file_mutex);
    printf("waiting for other playar\n");
    dwWaitResultFile = WaitForSingleObject(
        event_file, // event handle
        INFINITE);    // indefinite wait

    if (dwWaitResultFile != WAIT_OBJECT_0) {
        printf("Wait error (%d)\n", GetLastError());
        return FALSE;
    }
    game_session(Ind, cows_and_bulls_message_to_file, message_from_file, FALSE, user_title, user_opposite_title, oppsite_ind, file_mutex);
    sprintf_s(bulls, 5, "%c", message_from_file[0]);
    sprintf_s(cows, 5, "%c", message_from_file[2]);
    
    ResetEvent(event_thread[Ind]);
    ResetEvent(event_thread_end_reading[Ind]);
    

}

//result values: no winner= 0, user won=1, opponent won=2, tie =3
BOOL get_round_result(int Ind, char* user_title, char* user_opposite_title, int oppsite_ind,
     HANDLE file_mutex, char* bulls, int* round_result, int * message_type_to_send) {

    DWORD retval;
    DWORD dwWaitResultFile;
    char message_to_file[USER_NAME_MSG] = { 0 };
    char message_from_file[USER_NAME_MSG] = { 0 };
    char opponent_bulls[USER_NAME_MSG] = { 0 };

    retval = file_handle(file_mutex);
    printf("waiting for other playar\n");
    dwWaitResultFile = WaitForSingleObject(
        event_file, // event handle
        INFINITE);    // indefinite wait

    if (dwWaitResultFile != WAIT_OBJECT_0) {
        printf("Wait error (%d)\n", GetLastError());
        return FALSE;
    }

    *round_result = NO_WINNER;
    *message_type_to_send = SERVER_PLAYER_MOVE_REQUEST;
    //check if the user won in this round (winning=4 bulls)
    if (strcmp("4", bulls) == 0) {//if 0, the strings are indetical
        *round_result = USER_WIN;
        *message_type_to_send = SERVER_WIN;
    }
    sprintf_s(message_to_file, USER_NAME_MSG, "%s%s\n\r", user_title, bulls);


    game_session(Ind, message_to_file, message_from_file, FALSE, user_title, user_opposite_title, oppsite_ind, file_mutex);
    sprintf_s(opponent_bulls, USER_NAME_MSG, "%c", message_from_file[0]);//get opponent result
    ResetEvent(event_thread[Ind]);
    ResetEvent(event_thread_end_reading[Ind]);

    //check user result and update the result 
    if (strcmp("4", opponent_bulls) == 0) {
        if (*round_result == NO_WINNER) {
            *round_result = OPPONENT_WIN;
            *message_type_to_send = SERVER_WIN;
        }
        if (*round_result == USER_WIN) {
            *round_result = TIE;
            *message_type_to_send = SERVER_DRAW;
        }
    }

    return TRUE;
}



BOOL get_param_from_file(char* string_from_file, char* param, int start_point, char* user_opposite_title) {
    char* user_opposite_pointer;
    char check_param[100] = { 0 };
    user_opposite_pointer = strstr(string_from_file, user_opposite_title);
    strcpy_s(check_param, 100, user_opposite_pointer);
    int j = 0;
    for (int i = start_point; i < USER_NAME_MSG; i++)
        if (check_param[i] != '\n') {
            param[j] = check_param[i];
            j++;
        }
        else {
            break;
        }
    
        

}

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


BOOL close_file_handle(HANDLE file_mutex) {
    // check if need to open file - protected by mutex;
    DWORD wait_code;
    BOOL ret_val;
    char file_path[74] = { 0 };
    
    
       
    
    /* Create the mutex that will be used to synchronize access to queue */
    wait_code = WaitForSingleObject(file_mutex, INFINITE);
    if (WAIT_OBJECT_0 != wait_code)
    {
        printf("-ERROR: %d - WaitForSingleObject failed !\n", GetLastError());
        return FALSE;
    }
    printf("enter mutex \n");
    //critical section- check if queue is not empty and pop the mission 
    if (file != NULL) {
        sprintf_s(file_path, 74, "%s", "GameSession.txt");
        
        CloseHandle(file);
        ResetEvent(event_file);
        file = NULL;
        printf("file is null");
    }
    else {
        SetEvent(close_file_event);
        
    }
    //end of critical section 
    //*Release queue mutex

    ret_val = ReleaseMutex(file_mutex);
    if (FALSE == ret_val)
    {
        printf("-ERROR: %d - release semaphore failed !\n", GetLastError());
        return FALSE;
    }
    return TRUE;


}

