/*------------Includes-------------*/

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <winsock2.h>

#include "SocketShared.h"
#include "SocketSendRecvTools.h"
#include "server_main_header.h"
#include "message.h"
/*-------------Define Varibles--------*/
#define NUM_OF_WORKER_THREADS 2
#define MAX_LOOPS 2
#define USER_LEN 20
#define USER_NAME_MSG 30
#define SEND_STR_SIZE 100
#define USER_TITLE 7
#define GUESS 4

#define CHECK_CONNECTION(RESULT) if (RESULT != TRNS_SUCCEEDED) connected_to_client=0;

HANDLE event_thread[NUM_OF_WORKER_THREADS];
HANDLE event_file= NULL;
HANDLE file = NULL;
HANDLE ThreadHandles[NUM_OF_WORKER_THREADS];
SOCKET ThreadInputs[NUM_OF_WORKER_THREADS];
//SOCKET AcceptSocket;

/* -------------main------------------*/
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
    printf("waiting for clients to connect\n");
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
    create_mutexs_and_events();

    file_mutex = CreateMutex(
        NULL,	/* default security attributes */
        FALSE,	/* initially not owned */
        NULL);	/* unnamed mutex */
    if (NULL == file_mutex)
    {
        printf("Error when creating mutex: %d\n", GetLastError());
        return FALSE;
    }
    for (Loop = 0; Loop < MAX_LOOPS; Loop++)
    {
        SOCKET AcceptSocket = accept(MainSocket, NULL, NULL);
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

            printf("creating thread\n");
            Create_Thread_data(&AcceptSocket, Ind,file_mutex,&ptr_to_thread);
            CreateThreadSimple((LPTHREAD_START_ROUTINE)ServiceThread, ptr_to_thread, ThreadHandles+Ind);
           // ThreadInputs[Ind] = AcceptSocket; // shallow copy: don't close 
                                              // AcceptSocket, instead close 
                                              // ThreadInputs[Ind] when the
                                              // time comes.
            
        }


    }

    for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
    {
        if (ThreadHandles[Ind] != NULL)
        {
            // poll to check if thread finished running:
            DWORD Res = WaitForSingleObject(ThreadHandles[Ind], INFINITE);

            if (Res == WAIT_OBJECT_0)
            {
                closesocket(ThreadInputs[Ind]);
                CloseHandle(ThreadHandles[Ind]);
                ThreadHandles[Ind] = NULL;
                break;
            }
            else
            {
                printf("Waiting for thread failed. Ending program\n");
                return;
            }
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

static DWORD ServiceThread(LPVOID lpParam) {

    printf("enter to thread\n");

    //declare
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
    char session_result[USER_NAME_MSG] = { 0 };
    char user_title[USER_TITLE] = { 0 };
    char user_opposite_title[USER_TITLE] = { 0 };
    char* user_opposite_pointer;
    char initial_number[4];
    char* AcceptedStr = NULL;
    char user_name[USER_LEN] = { 0 };
    char oppsite_user_name[USER_NAME_MSG]= { 0 };
    int cows;
    int bulls;
    int state = CLIENT_REQUEST;

    int message_type = CLIENT_REQUEST;
    char* send_params[MAX_PARAMS] = { NULL };
    char* recieve_params[MAX_PARAMS] = { NULL };
    int connected_to_client = 1;
    int ret_val;


    //get thread params
    ThreadData* p_params;
    p_params = (ThreadData*)lpParam;
    t_socket = p_params->p_socket;
    Ind = p_params->thread_number;
    file_mutex = p_params->file_mutex;

    int oppsite_ind = other_thread_ind(Ind, user_title, user_opposite_title);

    while (connected_to_client) {
        RecvRes = RecieveMsg(*t_socket, &message_type, recieve_params, DEFUALT_TIMEOUT);
        if (!check_recv(RecvRes)) {
            printf("recieve string failed\n");
            return FALSE;
        }
        switch (message_type)
        {
        case CLIENT_REQUEST://get user name from client
            strcpy_s(user_name, USER_LEN, recieve_params[0]);
            SendRes = SendMsg(*t_socket, SERVER_APPROVED, NULL);
            IS_FAIL(SendRes,"SendMsg Failed\n");
            SendRes = SendMsg(*t_socket, SERVER_MAIN_MENU, NULL);
            IS_FAIL(SendRes, "SendMsg Failed\n");
            break;

        case CLIENT_VERSUS:
            //search for another client 
            //if found send SERVER_INVITE
            clien_versus(user_title, user_name, user_opposite_title, oppsite_ind, Ind, oppsite_user_name,file_mutex);
            send_params[0] = oppsite_user_name;
            ret_val = SendMsg(*t_socket, SERVER_INVITE, send_params);
            CHECK_CONNECTION(ret_val);
            //printf("%s", SendStr);
            //SendRes = SendString(SendStr, *t_socket);


            ret_val = SendMsg(*t_socket, SERVER_SETUP_REQUEST, NULL);
            CHECK_CONNECTION(ret_val);
            //printf("%s", SendStr);
            //SendRes = SendString(SendStr, *t_socket);
            break;

        case CLIENT_SETUP:
            strcpy_s(user_number, GUESS, recieve_params[0]);
            ret_val = SendMsg(*t_socket, SERVER_PLAYER_MOVE_REQUEST, NULL);
            CHECK_CONNECTION(ret_val);
            break;

       // case CLIENT_PLAYER_MOVE:
            //strcpy_s(opponent_guess, GUESS, recieve_params[0]);
            //client_move(Ind, opponent_guess, session_result, FALSE, user_title,user_opposite_title, oppsite_ind, user_number, file_mutex);



        case CLIENT_DISCONNECT:
            connected_to_client = 0;
            //free stuff and end program
            break;

        default:
            break;
        }

        free_params(recieve_params);
    }
    return;
   


    
    

    

    




    
    

  
    


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
        TEXT("Second_thread")  // object name
    );

    if (event_thread[1] == NULL)
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


    return TRUE;
}

BOOL file_handle(  HANDLE file_mutex) {
    // check if need to open file - protected by mutex;
    DWORD wait_code;
    BOOL ret_val;
    char file_path[74] = "C:\\Users\\Anat\\source\\repos\\Ex4_System_operation\\Ex4\\Debug\\GameSession.txt";
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
        file = CreateFileA((LPCSTR)file_path,// file name 
            GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file == INVALID_HANDLE_VALUE)
        {
            return FALSE;
            printf("file is open\n");
        }
    }
    else {
        printf("seconf_player_enter");
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
    }
   
    ret_val = ReleaseMutex(file_mutex);
    if (FALSE == ret_val)
    {
        printf("-ERROR: %d - release mutex failed !\n", GetLastError());
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
    printf("oppsinte name %s\n", oppsite_user_name);
    close_file_handle(file_mutex);
}

BOOL client_move(int Ind, char* opponent_guess, char* message_from_file, BOOL users_name_flag,
    char* user_title, char* user_opposite_title, int oppsite_ind,char* your_number
    , HANDLE file_mutex) {
    int cows;
    int bulls;
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
    char guess_message_to_file[USER_NAME_MSG] = { 0 };
    char cows_and_bulls[USER_NAME_MSG] = { 0 };
    char cows_and_bulls_message_to_file[USER_NAME_MSG] = { 0 };
    sprintf_s(guess_message_to_file, USER_NAME_MSG, "%s%s\n\r", user_title, opponent_guess);
    game_session(Ind, guess_message_to_file, message_from_file, FALSE, user_title, user_opposite_title, oppsite_ind, file_mutex);
    ResetEvent(event_thread[Ind]);
    close_file_handle(file_mutex);
    get_bulls_and_cows(your_number, message_from_file, &cows, &bulls);
    sprintf_s(cows_and_bulls, USER_NAME_MSG, "%d,%d", bulls, cows);
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
    ResetEvent(event_thread[Ind]);
    close_file_handle(file_mutex);
    

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
        CloseHandle(file);
        file = NULL;
    }
    else {
        
        ResetEvent(event_file);
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

