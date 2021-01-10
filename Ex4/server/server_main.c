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
#define MAX_LOOPS 1
#define USER_LEN 20

#define SEND_STR_SIZE 100
HANDLE event_thread[NUM_OF_WORKER_THREADS];
HANDLE event_file= NULL;
HANDLE file = NULL;
HANDLE ThreadHandles[NUM_OF_WORKER_THREADS];
SOCKET ThreadInputs[NUM_OF_WORKER_THREADS];
SOCKET AcceptSocket;
HANDLE file_mutex = NULL;
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

            printf("creating thread\n");
            Create_Thread_data(&AcceptSocket, Ind,&ptr_to_thread);
            CreateThreadSimple((LPTHREAD_START_ROUTINE)ServiceThread, ptr_to_thread, ThreadHandles+Ind);
            ThreadInputs[Ind] = AcceptSocket; // shallow copy: don't close 
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
    char SendStr[SEND_STR_SIZE];
    char ParamStr[SEND_STR_SIZE];
    int num_of_param;
    BOOL Done = FALSE;
    TransferResult_t SendRes;
    TransferResult_t RecvRes;
    BOOL retval = FALSE;
    SOCKET* t_socket = NULL;
    BOOL read_users_flag;
    int Ind;
    ThreadData* p_params;
    p_params = (ThreadData*)lpParam;
    DWORD dwWaitResultFile;
    t_socket = p_params->p_socket;
    Ind = p_params->thread_number;
    char user_title[7];
    char user_opposite_title[7];
    char* user_opposite_pointer;
    int oppsite_ind = other_thread_ind(Ind, user_title, user_opposite_title);
    char initial_number[4];
    char* AcceptedStr = NULL;
    char user_name[USER_LEN];

    int message_type = CLIENT_REQUEST;
    char* send_params[MAX_PARAMS] = { NULL };
    char* recieve_params[MAX_PARAMS] = { NULL };
    int connected_to_client = 1;

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
            break;

        case CLIENT_DISCONNECT:
            connected_to_client = 0;
            //free stuff and end program
            break;

        default:
            break;
        }

        free_params(recieve_params);
    }

    RecvRes = ReceiveString(&AcceptedStr, *t_socket);


    else {
       printf("recieve string succeed: %s", AcceptedStr);
    }
    return 0;//end

    ////get user name from client
    //if (check_if_message_type_instr_message(AcceptedStr, "CLIENT_REQUEST")) {
    //    printf("client request\n");

    //    //get_param_index_and_len(&indexes, &lens, AcceptedStr, strlen(AcceptedStr));
    //    //printf("get params succeed\n");
    //    strncpy_s(user_name, strlen(params[0]), params[0], strlen(user_name));//get user name
    //    //strcpy_s(SendStr, strlen("SERVER_APPROVED"), "SERVER_APPROVED");
    //    //printf("sending string: %s", SendStr);

    //    SendRes = SendMsg(*t_socket, SERVER_APPROVED, NULL);

    //    //SendRes = SendString(SendStr, *t_socket);//send SERVER_APPROVED to client
    //    if (!check_send(SendRes)) {
    //        printf("send string failed\n");
    //        return FALSE;
    //    }
    //    else {
    //        printf("send string succeed\n");
    //    }
    //    return 0;
    //    strcpy_s(SendStr, strlen("SERVER_MAIN_MENU"), "SERVER_MAIN_MENU");// send main menu  to client
    //    SendRes = SendString(SendStr, *t_socket);
    //    if (!check_send(SendRes)) return FALSE;
    //    
    //}
    
    //danielas code
    //strncpy_s(user_name, strlen(params[0]), params[0], strlen(user_name));//get user name



    //end daniela code

    //wait for answer from client for menue
    RecvRes = ReceiveString(&AcceptedStr, *t_socket);
    if (!check_recv(RecvRes)) return FALSE;
    if (check_if_message_type_instr_message(AcceptedStr, "CLIENT_DISCONNECT")) {
        Done == TRUE;///TODO : need to close thread##########!!!!!!!!
        
    }
    else if (check_if_message_type_instr_message(AcceptedStr, "CLIENT_VERSUS")) {
        retval = file_handle(&user_name, Ind);

        dwWaitResultFile = WaitForSingleObject(
            event_file, // event handle
            INFINITE);    // indefinite wait

        if (dwWaitResultFile != WAIT_OBJECT_0) {
            printf("Wait error (%d)\n", GetLastError());
            return FALSE;
        }
        //first  send user name 
        char message_to_file[27];
        char oppsite_user_name[27];
        int offest_end_file = 0 ;
        strcpy_s(message_to_file,25, user_title);

        strcat_s(message_to_file, 25, user_name);
        game_session(Ind, message_to_file, oppsite_user_name, TRUE ,offest_end_file, &offest_end_file, user_title, user_opposite_title, oppsite_ind);
        ResetEvent(event_thread[Ind]);

        sprintf_s(SendStr, SEND_STR_SIZE, "SERVER_INVITE:%s\n", oppsite_user_name);
        SendRes = SendString(SendStr, *t_socket);

        ///get initial number
        RecvRes = ReceiveString(&AcceptedStr, *t_socket);
        if (check_if_message_type_instr_message(AcceptedStr,"CLIENT_SETUP")) {
            int indexes[1];
            int lens[1];
            get_param_index_and_len(&indexes, &lens, AcceptedStr, strlen(AcceptedStr));
            strncpy_s(initial_number, lens[0], *(AcceptedStr + indexes[0]), strlen(initial_number));
        }


        
        //a
        //game session
        //Until some one wins
        //recive the number to guess 
        while (!Done) {
            char your_guess[4];
            char oppsite_guess[4];
            strcpy_s(SendStr, strlen("SERVER_PLAYER_MOVE_REQUEST"), "SERVER_PLAYER_MOVE_REQUEST");// ask for guess
            SendRes = SendString(SendStr, *t_socket);
            RecvRes = ReceiveString(&AcceptedStr, *t_socket);
            if (check_if_message_type_instr_message(AcceptedStr, "CLIENT_PLAYER_MOVE")) {
                int indexes[1];
                int lens[1];
                get_param_index_and_len(&indexes, &lens, AcceptedStr, strlen(AcceptedStr));
                strncpy_s(your_guess, lens[0], *(AcceptedStr + indexes[0]), strlen(your_guess));
            }
            game_session(Ind, message_to_file, oppsite_user_name, TRUE, offest_end_file, &offest_end_file, user_title, user_opposite_title, oppsite_ind);

        }



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


BOOL Create_Thread_data(SOCKET* socket, int num_of_thread, ThreadData** ptr_to_thread_data) {
    (*ptr_to_thread_data) = (ThreadData*)malloc(sizeof(ThreadData));
    if (*ptr_to_thread_data == NULL) {
        printf("ERROR: allocation failed!\n");
        return FALSE;
    }
    (*ptr_to_thread_data)->thread_number = num_of_thread;
    (*ptr_to_thread_data)->p_socket = socket;
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

    file_mutex = CreateMutex(
        NULL,	/* default security attributes */
        FALSE,	/* initially not owned */
        NULL);	/* unnamed mutex */
    if (NULL == file_mutex)
    {
        printf("Error when creating mutex: %d\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}

BOOL file_handle(char* user_name, int Ind) {
    // check if need to open file - protected by mutex;
    DWORD wait_code;
    BOOL ret_val;
    char file_path[16] = "GameSession.txt";
    BOOL bErrorFlag = FALSE;
    DWORD lpNumberOfBytesWritten;
    LPDWORD* end_of_file_offset;
    
   /* Create the mutex that will be used to synchronize access to queue */
    wait_code = WaitForSingleObject(file_mutex, INFINITE);
    if (WAIT_OBJECT_0 != wait_code)
    {
        printf("-ERROR: %d - WaitForSingleObject failed !\n", GetLastError());
        return FALSE;
    }
    //critical section- check if queue is not empty and pop the mission 
    if (file == NULL) {
        file = CreateFileA((LPCSTR)file_path,// file name 
            GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    else {
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
        strcpy_s(user_name, 7, "user0:");
        strcpy_s(oppsite_user_name, 7, "user1:");
        return 1;
    }
    if (Ind == 1) {
        strcpy_s(user_name, 7, "user1:");
        strcpy_s(oppsite_user_name, 7, "user0:");
        
        return 0;
    }
}


BOOL game_session(int Ind , char* message_to_file, char* message_from_file, BOOL users_name_flag, int offset_first, int* offset_end ,char* user_title, char* user_opposite_title , int oppsite_ind) {

    DWORD wait_code;
    BOOL bErrorFlag = FALSE;
    DWORD lpNumberOfBytesWritten;
    BOOL ret_val;
    DWORD dwWaitResultOtherClient;
    char string_from_file[50];
    char* user_opposite_pointer;
    
    wait_code = WaitForSingleObject(file_mutex, INFINITE);
    if (WAIT_OBJECT_0 != wait_code)
    {
        printf("-ERROR: %d - WaitForSingleObject failed !\n", GetLastError());
        return FALSE;
    }

    //critical 
    SetFilePointer(file, 0, NULL, FILE_END); 
    WriteFile(file, message_to_file, strlen(message_to_file), &lpNumberOfBytesWritten, NULL);
    SetEvent(event_thread[Ind]);

    ret_val = ReleaseMutex(file_mutex);
    if (FALSE == ret_val)
    {
        printf("-ERROR: %d - release semaphore failed !\n", GetLastError());
        return FALSE;
    }
    
    //Wait for the second thread to write
    dwWaitResultOtherClient = WaitForSingleObject(
    event_thread[oppsite_ind], // event handle
    INFINITE);    // indefinite wait
    offset_end = GetFileSize(file, NULL);
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
    ret_val = read_from_pointer_in_file(offset_first, string_from_file);
    if (ret_val) {
        user_opposite_pointer = strstr(string_from_file, user_opposite_title);
        int indexes[1];
        int lens[1];
        get_param_index_and_len(&indexes, &lens, user_opposite_pointer, 5);
        strncpy_s(message_from_file, lens[0], user_opposite_pointer, strlen(user_opposite_pointer));
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

    
    dw_pointer = SetFilePointer(file, offset, NULL, FILE_BEGIN);//set poniter to offset
    end_of_file_offset = GetFileSize(file,NULL);
    nBytesToRead = end_of_file_offset - offset;
    bResult = ReadFile(file, buffer, nBytesToRead, &dwBytesRead, NULL);
    if (!bResult) {
        return FALSE;
    }
    printf("READ from tasks file success!");
    return TRUE;
}

