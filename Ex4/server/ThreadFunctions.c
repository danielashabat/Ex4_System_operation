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

#define USER_LEN 21
#define USER_NAME_MSG 30
#define SEND_STR_SIZE 100
#define USER_TITLE 7
#define GUESS 5//need to include '\0'

#define CHECK_CONNECTION(RESULT) if (RESULT != TRNS_SUCCEEDED) {\
    SetEvent(opponent_failed);\
    printf("set event 'opponent failed'!\n");\
    connected_to_client=0;\
    ret_val = disconnect_client();\
    IS_FALSE(ret_val, "disconnect_client failed\n");\
    break;}

#define IS_FALSE(RESULT,MSG) if (RESULT != TRUE)\
{\
    printf("function:[%s line:%d] %s\n", __func__,__LINE__,MSG);\
    printf("set event opponent failed!\n");\
    SetEvent(opponent_failed);\
    connected_to_client=0; break;\
}

HANDLE event_thread[NUM_OF_WORKER_THREADS];
HANDLE event_thread_end_reading[NUM_OF_WORKER_THREADS];
HANDLE event_file = NULL;
HANDLE close_file_event = NULL;
HANDLE opponent_failed = NULL;
HANDLE file = NULL;
HANDLE connected_clients_mutex = NULL;

int connected_clients;
HANDLE file_mutex;




 DWORD ServiceThread(LPVOID lpParam) {

    BOOL Done = FALSE;
    BOOL retval = FALSE;
    SOCKET* t_socket = NULL;
    HANDLE file_mutex = NULL;
    int Ind;
    char user_number[GUESS] = { 0 };
    char opponent_number[GUESS] = { 0 };
    char opponent_guess[GUESS] = { 0 };
    char your_guess[GUESS] = { 0 };
    char session_result[USER_NAME_MSG] = { 0 };
    char user_title[USER_TITLE] = { 0 };
    char user_opposite_title[USER_TITLE] = { 0 };
    char* AcceptedStr = NULL;
    char user_name[USER_LEN] = { 0 };
    char oppsite_user_name[USER_NAME_MSG] = { 0 };
    char cows[GUESS] = { 0 };
    char bulls[GUESS] = { 0 };
    int state = CLIENT_REQUEST;

    int message_type = CLIENT_REQUEST;
    int message_type_to_send = CLIENT_REQUEST;

    char* send_params[MAX_PARAMS] = { NULL };
    char* recieve_params[MAX_PARAMS] = { NULL };
    int connected_to_client = 1;
    BOOL ret_val = TRUE;
    BOOL ret_val_connection = TRUE;
    char oppsite_user_name_with_title[30] = { 0 };
    int round_result = 0;
    int timeout = DEFUALT_TIMEOUT;
    int oponnent_alive = 0;
    //get thread params
    ThreadData* p_params;
    p_params = (ThreadData*)lpParam;
    t_socket = p_params->p_socket;
    Ind = p_params->thread_number;
    //file_mutex = p_params->file_mutex;

    int oppsite_ind = other_thread_ind(Ind, user_title, user_opposite_title);
    if (INVALID_SOCKET == *t_socket) {
        printf("ERRROR:socket is invalid!\n");
        return 1;
    }

    while (connected_to_client) {
        ret_val_connection = RecieveMsg(*t_socket, &message_type, recieve_params, timeout);
        timeout = TEN_MIN;
        CHECK_CONNECTION(ret_val_connection);
        switch (message_type)
        {
        case CLIENT_REQUEST://get user name from client
            if (recieve_params[0] != NULL) {
                char* p = recieve_params[0];
                strcpy_s(user_name, USER_LEN, p);
            }
            else {
                ret_val = FALSE;
                break;
            }
            ret_val = get_server_responed(&message_type_to_send);
            IS_FALSE(ret_val, "get_server_responed failed\n");
            send_params[0] = "server is full of clients";//in case sending SERVER_DENIED
            ret_val_connection = SendMsg(*t_socket, message_type_to_send, send_params);
            CHECK_CONNECTION(ret_val_connection);
            if (message_type_to_send == SERVER_DENIED) {
                connected_to_client = 0;
                break;
            }

            ret_val_connection = SendMsg(*t_socket, SERVER_MAIN_MENU, NULL);
            CHECK_CONNECTION(ret_val_connection);
            break;

        case CLIENT_VERSUS:
            //search for another client
            ret_val = wait_for_another_client(Ind, oppsite_ind, &oponnent_alive);
            IS_FALSE(ret_val, "wait_for_another_client failed\n");
            if (oponnent_alive) {
                ret_val = clien_versus(user_title, user_name, user_opposite_title, oppsite_ind, Ind, oppsite_user_name, file_mutex, &oponnent_alive);
                if (!oponnent_alive) { ret_val = reset_game(*t_socket); IS_FALSE(ret_val, "reset_game failed\n") break; }
                IS_FALSE(ret_val, " clien_versus failed\n");
                send_params[0] = oppsite_user_name;
                ret_val_connection = SendMsg(*t_socket, SERVER_INVITE, send_params);
                CHECK_CONNECTION(ret_val_connection);

                ret_val_connection = SendMsg(*t_socket, SERVER_SETUP_REQUEST, NULL);
                CHECK_CONNECTION(ret_val_connection);
            }
            else {
                ret_val_connection = SendMsg(*t_socket, SERVER_NO_OPPONENTS, send_params);
                CHECK_CONNECTION(ret_val_connection);
            }
            break;

        case CLIENT_SETUP:
            if (recieve_params[0] != NULL) {
                char* p = recieve_params[0];
                strcpy_s(user_number, GUESS, p);
            }
            ret_val = get_opponent_number(Ind, user_title, user_opposite_title, oppsite_ind, file_mutex, opponent_number, user_number, &oponnent_alive);
            if (!oponnent_alive) { ret_val = reset_game(*t_socket); IS_FALSE(ret_val, "reset_game failed\n") break; }
            IS_FALSE(ret_val, "get_opponent_number failed\n");

            ret_val_connection = SendMsg(*t_socket, SERVER_PLAYER_MOVE_REQUEST, NULL);
            CHECK_CONNECTION(ret_val_connection);
            printf("the opponent number is: %s\n", opponent_number);
            break;

        case CLIENT_PLAYER_MOVE:
            if (recieve_params[0] != NULL) {
                char* p = recieve_params[0];
                strcpy_s(your_guess, GUESS, p);

            }
            //calculate moves
            ret_val = client_move(Ind, your_guess, session_result, FALSE, user_title, user_opposite_title, oppsite_ind, user_number, file_mutex, bulls, cows, opponent_guess, &oponnent_alive);
            if (!oponnent_alive) { ret_val = reset_game(*t_socket); IS_FALSE(ret_val, "reset_game failed\n") break; }
            IS_FALSE(ret_val, "client_move failed\n");
            send_params[0] = bulls; send_params[1] = cows; send_params[2] = oppsite_user_name; send_params[3] = opponent_guess;
            ret_val_connection = SendMsg(*t_socket, SERVER_GAME_RESULTS, send_params);
            CHECK_CONNECTION(ret_val_connection);

            ret_val = get_round_result(Ind, user_title, user_opposite_title, oppsite_ind, file_mutex, bulls, &round_result, &message_type_to_send, &oponnent_alive);
            if (!oponnent_alive) { ret_val = reset_game(*t_socket); IS_FALSE(ret_val, "reset_game failed\n") break; }
            IS_FALSE(ret_val, "get_round_result failed\n");

            if (round_result == USER_WIN)
                send_params[0] = user_name;
            else
                send_params[0] = oppsite_user_name;
            send_params[1] = opponent_number;//opponent 4 digits
            ret_val_connection = SendMsg(*t_socket, message_type_to_send, send_params);
            CHECK_CONNECTION(ret_val_connection);

            if (round_result != NO_WINNER) {
                ret_val_connection = SendMsg(*t_socket, SERVER_MAIN_MENU, NULL);
                CHECK_CONNECTION(ret_val_connection);
            }
            break;


        case CLIENT_DISCONNECT:
            ret_val = disconnect_client();
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
    if (!ret_val) {
        return 0;
    }
    if (ret_val_connection == TRNS_FAILED) {
        return 0;
    }
    return 1;


}

BOOL get_server_responed(int* server_denied_or_approved) {
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


BOOL create_mutexs_and_events() {
    
    

    //Initalize all event to NULL
    for (int Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++) {
        event_thread[Ind] = NULL;
        event_thread_end_reading[Ind] = NULL;
    }

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
    event_file = NULL;
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
    close_file_event = NULL;
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
    opponent_failed = NULL;
    opponent_failed = CreateEvent(
        NULL,               // default security attributes
        TRUE,               // manual-reset event
        FALSE,              // initial state is nonsignaled
        TEXT("Opponent_failed")  // object name
    );

    if (opponent_failed == NULL)
    {
        printf("CreateEvent failed (%d)\n", GetLastError());
        return FALSE;
    }

    //initilizse connected clients
    connected_clients = 0;
    connected_clients_mutex = NULL;
    connected_clients_mutex = CreateMutex(
        NULL,	/* default security attributes */
        FALSE,	/* initially not owned */
        NULL);	/* unnamed mutex */

    if (NULL == connected_clients_mutex)
    {
        printf("Error when creating mutex: %d\n", GetLastError());
        return FALSE;
    }

    file_mutex = NULL;
    file_mutex = CreateSemaphore(
        NULL,           // default security attributes
        1,  // initial count
        1,  // maximum count
        NULL);          // unnamed semaphore

    if (NULL == file_mutex)
    {
        printf("Error when creating mutex: %d\n", GetLastError());
    }
    return TRUE;
}

void close_event_and_mutex() {
    if (event_file != NULL) {
        CloseHandle(event_file);
    }

    if (file_mutex != NULL) {
        CloseHandle(file_mutex);
    }
    if (file != NULL) {
        CloseHandle(file);
    }
    for (int i = 0; i < 1; i++) {
        if (event_thread[i] != NULL) CloseHandle(event_thread[i]);
        if (event_thread_end_reading[i] != NULL) CloseHandle(event_thread_end_reading[i]);
    }
    if (connected_clients_mutex != NULL) {
        CloseHandle(connected_clients_mutex);
    }
    if (close_file_event != NULL) {
        CloseHandle(close_file_event);
    }
    if (opponent_failed != NULL) {
        CloseHandle(opponent_failed);
    }
}

/*the first thread get in this function need to open a new file, the other thread release event_file (mutex) */
BOOL file_handle() {
    // check if need to open file - protected by mutex;
    DWORD wait_code;
    BOOL ret_val;
    char file_path[MAX_PATH] = { 0 };//daniela changed the path to relative path
    BOOL bErrorFlag = FALSE;
    //printf("start file handle\n");
    sprintf_s(file_path, MAX_PATH, "%s", "GameSession.txt");
    /* Create the mutex that will be used to synchronize access to queue */
    printf(" waiting for file mutex: %p \n", file_mutex);
    wait_code = WaitForSingleObject(file_mutex, INFINITE);
    //printf("enter file mutex \n");
    if (WAIT_OBJECT_0 != wait_code)
    {
        printf("-ERROR: %d - WaitForSingleObject failed !\n", GetLastError());
        return FALSE;
    }
    printf("enter file mutex \n");
    //critical section- check if queue is not empty and pop the mission 
    if (file == NULL) {
        ret_val = ResetEvent(close_file_event);
        if (!ret_val) return FALSE;
        file = CreateFileA((LPCSTR)file_path,// file name 
            GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        printf("create new file succees!\n");
        if (file == INVALID_HANDLE_VALUE)
        {
            printf("ERROR:create file failed!\n");
            return FALSE;

        }
    }
    else {
        ret_val = SetEvent(event_file);
        if (!ret_val) return FALSE;
        printf("seconed player entered to file handle!!\n");

    }
    //end of critical section 
    //*Release queue mutex

    ret_val = ReleaseSemaphore(file_mutex, 1, NULL);
    if (FALSE == ret_val)
    {
        printf("-ERROR: %d - release semaphore failed !\n", GetLastError());
        return FALSE;
    }
    else {
        printf("ret_val: %d release file mutex succeed! \n", ret_val);
    }
    return TRUE;


}


int other_thread_ind(int Ind, char* user_title, char* oppsite_title) {

    if (Ind == 0) {
        strcpy_s(user_title, USER_TITLE, "user0:");
        strcpy_s(oppsite_title, USER_TITLE, "user1:");
        return 1;
    }
    else if (Ind == 1) {
        strcpy_s(user_title, 7, "user1:");
        strcpy_s(oppsite_title, USER_TITLE, "user0:");

        return 0;
    }
    return 0;
}



BOOL game_session(int Ind, char* message_to_file, char* message_from_file, BOOL users_name_flag, char* user_title, char* user_opposite_title, int oppsite_ind, int* opponent_alive) {
    DWORD wait_code;
    BOOL bErrorFlag = FALSE;
    DWORD lpNumberOfBytesWritten;
    BOOL ret_val;
    int opponent_alive_from_function = 1;
    DWORD retval;
    HANDLE wait_for_multiple[2] = { NULL };
    char string_from_file[50] = { 0 };
    //printf("waiting for file mutex (game session) \n");
    wait_code = WaitForSingleObject(file_mutex, INFINITE);
    if (WAIT_OBJECT_0 != wait_code)
    {
        printf("-ERROR: %d - WaitForSingleObject failed !\n", GetLastError());
        return FALSE;
    }
   // printf("enter file mutex \n");
    //critical 
    retval = SetFilePointer(file, 0, NULL, FILE_END); //set pointer to end of file
    if (retval == INVALID_SET_FILE_POINTER) return FALSE;
    ret_val = WriteFile(file, message_to_file, strlen(message_to_file), &lpNumberOfBytesWritten, NULL);
    if (!ret_val) return FALSE;
    ret_val = SetEvent(event_thread[Ind]);
    if (!ret_val) return FALSE;

    ret_val = ReleaseSemaphore(file_mutex, 1, NULL);
    if (FALSE == ret_val)
    {
        printf("-ERROR: %d - release semaphore failed !\n", GetLastError());
        return FALSE;
    }

    ////Wait for the second thread to write
    ret_val = wait_for_client_answer(&opponent_alive_from_function, 2, oppsite_ind);
    *opponent_alive = opponent_alive_from_function;
    if (!ret_val) return FALSE;



    //in your turn read the other user play 
   // printf("waiting for file mutex \n");
    wait_code = WaitForSingleObject(file_mutex, INFINITE);
    if (WAIT_OBJECT_0 != wait_code)
    {
        printf("-ERROR: %d - WaitForSingleObject failed !\n", GetLastError());
        return FALSE;
    }
   // printf("enter file mutex \n");
    retval = SetFilePointer(file, 0, NULL, FILE_BEGIN);
    if (retval == INVALID_SET_FILE_POINTER) return FALSE;
    ret_val = read_from_pointer_in_file(0, string_from_file);
    if (!ret_val) return FALSE;
    //printf("string_from_file %s \n", string_from_file);
    if (ret_val) {
        char param[20] = { 0 };

        ret_val = get_param_from_file(string_from_file, param, USER_TITLE - 1, user_opposite_title);
        if (!ret_val) return FALSE;
        //printf("get_param_succseed\n");
        sprintf_s(message_from_file, USER_NAME_MSG, "%s", param);
        //printf("oppsinte name %s\n", message_from_file);
        SetEvent(event_thread_end_reading[Ind]);
    }

    ret_val = ReleaseSemaphore(file_mutex, 1, NULL);

    if (FALSE == ret_val)
    {
        printf("-ERROR: %d - release mutex failed !\n", GetLastError());
        return FALSE;
    }

    ret_val = wait_for_client_answer(&opponent_alive_from_function, 3, oppsite_ind);
    *opponent_alive = opponent_alive_from_function;
    if (!ret_val) return FALSE;

    ret_val = close_file_handle(file_mutex);
    if (!ret_val) return FALSE;
    ret_val = wait_for_client_answer(&opponent_alive_from_function, 4, oppsite_ind);
    *opponent_alive = opponent_alive_from_function;
    if (!ret_val) return FALSE;
    return TRUE;
}

/*if oponnent_alive=1, the oponnent still in the game. otherwise there is no opponent or he quit*/
BOOL wait_for_another_client(int Ind, int oppsite_ind, int* oponnent_alive) {
    DWORD dwWaitResultOtherClient;
    BOOL ret_val = FALSE;
    BOOL retval;
    ret_val = SetEvent(event_thread[Ind]);
    if (ret_val == FALSE) {
        return FALSE;
    }
    //check if file mutex is free, if not release it
    if (Ind == 0) {
        dwWaitResultOtherClient = WaitForSingleObject(
            file_mutex, // event handle
            0);
        if (WAIT_OBJECT_0 != dwWaitResultOtherClient) {
            ret_val = ReleaseSemaphore(file_mutex, 1, NULL);
            if (ret_val)
                printf("user:%d released file_mutex semaphore\n", Ind);
        }
        else {
            ret_val = ReleaseSemaphore(file_mutex, 1, NULL);
            if (!ret_val)
               printf("ERRORL released file_mutex semaphore failed\n", Ind);
        }
    }

    //check if there was opponent quit in last round 
    dwWaitResultOtherClient = WaitForSingleObject(
        opponent_failed, // event handle
        0);
    if (WAIT_OBJECT_0 == dwWaitResultOtherClient) {
        retval = ResetEvent(event_thread_end_reading[0]);
        if (!retval) return FALSE;
        retval = ResetEvent(event_thread_end_reading[1]);
        if (!retval) return FALSE;
        retval = ResetEvent(close_file_event);
        if (!retval) return FALSE;
        retval = ResetEvent(event_file);
        if (!retval) return FALSE;
        retval = ResetEvent(opponent_failed);
        if (!retval) return FALSE;
        file = NULL;

    }


    dwWaitResultOtherClient = WaitForSingleObject(
        event_thread[oppsite_ind], // event handle
        15000);//15 sec
    if (WAIT_OBJECT_0 != dwWaitResultOtherClient)
    {
        //printf("there is no other client to play :( %d\n", GetLastError());
        ret_val = ResetEvent(event_thread[Ind]);
        if (!ret_val) return FALSE;
        *oponnent_alive = 0;
        return TRUE;
    }    // indefinite wait



    ret_val = ResetEvent(event_thread[oppsite_ind]);
    if (!ret_val) return FALSE;



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
    return TRUE;
}




BOOL read_from_pointer_in_file(int offset, char* buffer) {


    DWORD nBytesToRead;
    DWORD dwBytesRead;
    BOOL bResult = FALSE;

    DWORD end_of_file_offset;
    char* buffer_to_read;

    //dw_pointer = SetFilePointer(file, offset, NULL, FILE_BEGIN);//set poniter to offset
    end_of_file_offset = GetFileSize(file, NULL);
    if (end_of_file_offset == INVALID_FILE_SIZE) {
        printf("get size error (%d)\n", GetLastError());
        return FALSE;
    }
    //printf("file size %10d\n", end_of_file_offset);
    nBytesToRead = end_of_file_offset - offset;
    buffer_to_read = (char*)malloc((nBytesToRead + 1) * sizeof(char));
    if (buffer_to_read == NULL) {
        return FALSE;
    }
    bResult = ReadFile(file, buffer_to_read, nBytesToRead, &dwBytesRead, NULL);
    if (!bResult) {
        free(buffer_to_read);
        //printf("READ from tasks file NOT success!");
        return FALSE;
    }
    buffer_to_read[dwBytesRead] = '\0';
    sprintf_s(buffer, 50, "%s", buffer_to_read);
    free(buffer_to_read);
    //printf("READ from tasks file success!");
    return TRUE;
}



BOOL clien_versus(char* user_title, char* user_name, char* user_opposite_title, int oppsite_ind, int Ind, char* oppsite_user_name, HANDLE file_mutex, int* opponent_alive) {

    char message_to_file[USER_NAME_MSG] = { 0 };
    int offest_end_file = 0;
    BOOL retval;
    int opponent_alive_from_function = 1;

    //printf("start file handle\n");
    retval = file_handle();

    if (!retval) return FALSE;
    //printf("waiting for other player\n");
    retval = wait_for_client_answer(&opponent_alive_from_function, 1, oppsite_ind);
    *opponent_alive = opponent_alive_from_function;
    if (!retval) return FALSE;
    //printf("start game session\n");
    strcpy_s(message_to_file, USER_NAME_MSG, user_title);
    strcat_s(message_to_file, USER_NAME_MSG, user_name);
    strcat_s(message_to_file, USER_NAME_MSG, "\n\r");
    retval = game_session(Ind, message_to_file, oppsite_user_name, TRUE, user_title, user_opposite_title, oppsite_ind, &opponent_alive_from_function);
    *opponent_alive = opponent_alive_from_function;
    if (!retval) return FALSE;
    retval = ResetEvent(event_thread[Ind]);
    if (!retval) return FALSE;
    retval = ResetEvent(event_thread_end_reading[Ind]);
    if (!retval) return FALSE;
    //printf("oppsinte name %s%s \n", oppsite_user_name, user_title);
    return TRUE;
}

BOOL client_move(int Ind, char* your_guess, char* message_from_file, BOOL users_name_flag,
    char* user_title, char* user_opposite_title, int oppsite_ind, char* your_number
    , HANDLE file_mutex, char* bulls, char* cows, char* opponent_guess, int* opponent_alive) {
    int cows_calculate;
    int bulls_calculate;
    BOOL retval;
    int opponent_alive_from_function = 1;

    retval = file_handle();
    if (!retval) return FALSE;
    retval = wait_for_client_answer(&opponent_alive_from_function, 1, oppsite_ind);
    *opponent_alive = opponent_alive_from_function;
    if (!retval) return FALSE;

    char guess_message_to_file[USER_NAME_MSG] = { 0 };
    char cows_and_bulls[USER_NAME_MSG] = { 0 };
    char cows_and_bulls_message_to_file[USER_NAME_MSG] = { 0 };
    sprintf_s(guess_message_to_file, USER_NAME_MSG, "%s%s\n\r", user_title, your_guess);
    retval = game_session(Ind, guess_message_to_file, message_from_file, FALSE, user_title, user_opposite_title, oppsite_ind, &opponent_alive_from_function);
    *opponent_alive = opponent_alive_from_function;
    if (!retval) return FALSE;
    sprintf_s(opponent_guess, GUESS, "%s", message_from_file);
    retval = ResetEvent(event_thread[Ind]);
    if (!retval) return FALSE;
    retval = ResetEvent(event_thread_end_reading[Ind]);
    if (!retval) return FALSE;
    if (!opponent_alive_from_function) return TRUE;

    get_bulls_and_cows(your_number, message_from_file, &cows_calculate, &bulls_calculate);
    sprintf_s(cows_and_bulls, USER_NAME_MSG, "%d,%d", bulls_calculate, cows_calculate);
    sprintf_s(cows_and_bulls_message_to_file, USER_NAME_MSG, "%s%s\n\r", user_title, cows_and_bulls);

    retval = file_handle();
    if (!retval) return FALSE;
    //printf("waiting for other playar\n");
    retval = wait_for_client_answer(&opponent_alive_from_function, 1, oppsite_ind);
    *opponent_alive = opponent_alive_from_function;
    if (!retval) return FALSE;
    retval = game_session(Ind, cows_and_bulls_message_to_file, message_from_file, FALSE, user_title, user_opposite_title, oppsite_ind, &opponent_alive_from_function);
    *opponent_alive = opponent_alive_from_function;
    if (!retval) return FALSE;
    sprintf_s(bulls, 5, "%c", message_from_file[0]);
    sprintf_s(cows, 5, "%c", message_from_file[2]);

    retval = ResetEvent(event_thread[Ind]);
    if (!retval) return FALSE;
    retval = ResetEvent(event_thread_end_reading[Ind]);
    if (!retval) return FALSE;

    return TRUE;
}

//result values: no winner= 0, user won=1, opponent won=2, tie =3
BOOL get_round_result(int Ind, char* user_title, char* user_opposite_title, int oppsite_ind,
    HANDLE file_mutex, char* bulls, int* round_result, int* message_type_to_send, int* opponent_alive) {

    DWORD retval;
    int opponent_alive_from_function = 1;
    char message_to_file[USER_NAME_MSG] = { 0 };
    char message_from_file[USER_NAME_MSG] = { 0 };
    char opponent_bulls[USER_NAME_MSG] = { 0 };

    retval = file_handle();
    if (!retval) return FALSE;
    retval = wait_for_client_answer(&opponent_alive_from_function, 1, oppsite_ind);
    *opponent_alive = opponent_alive_from_function;
    if (!retval) return FALSE;

    *round_result = NO_WINNER;
    *message_type_to_send = SERVER_PLAYER_MOVE_REQUEST;
    //check if the user won in this round (winning=4 bulls)
    if (strcmp("4", bulls) == 0) {//if 0, the strings are indetical
        *round_result = USER_WIN;
        *message_type_to_send = SERVER_WIN;
    }
    sprintf_s(message_to_file, USER_NAME_MSG, "%s%s\n\r", user_title, bulls);


    retval = game_session(Ind, message_to_file, message_from_file, FALSE, user_title, user_opposite_title, oppsite_ind, &opponent_alive_from_function);
    *opponent_alive = opponent_alive_from_function;
    if (!retval) return FALSE;
    sprintf_s(opponent_bulls, USER_NAME_MSG, "%c", message_from_file[0]);//get opponent result
    retval = ResetEvent(event_thread[Ind]);
    if (!retval) return FALSE;
    retval = ResetEvent(event_thread_end_reading[Ind]);
    if (!retval) return FALSE;

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
    char* user_opposite_pointer = NULL;
    char check_param[100] = { 0 };
    user_opposite_pointer = strstr(string_from_file, user_opposite_title);
    if (user_opposite_pointer == NULL) {
        return FALSE;
    }
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


    return TRUE;
}


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
    //printf("number of bulls:%d; number of cows:%d\n", *bulls, *cows);
    return;

}


BOOL close_file_handle(HANDLE file_mutex) {
    // check if need to open file - protected by mutex;
    DWORD wait_code;
    BOOL ret_val;
    char file_path[74] = { 0 };



    printf("waiting for file mutex (close file handle)\n");
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
        ret_val = ResetEvent(event_file);
        if (!ret_val) return FALSE;
        file = NULL;
        //printf("file is null");
    }
    else {
        ret_val = SetEvent(close_file_event);
        if (!ret_val) return FALSE;

    }
    //end of critical section 
    //*Release queue mutex

    ret_val = ReleaseSemaphore(file_mutex, 1, NULL);
    if (FALSE == ret_val)
    {
        printf("-ERROR: %d - release semaphore failed !\n", GetLastError());
        return FALSE;
    }
    else
        printf("release file mutex (close handle)\n");
    return TRUE;


}

BOOL get_opponent_number(int Ind, char* user_title, char* user_opposite_title, int oppsite_ind,
    HANDLE file_mutex, char* opponent_number, char* your_number, int* opponent_alive) {

    DWORD retval;
    int opponent_alive_from_function = 1;
    retval = file_handle();
    if (!retval) return FALSE;

    //printf("waiting for other player\n");
    retval = wait_for_client_answer(&opponent_alive_from_function, 1, oppsite_ind);
    *opponent_alive = opponent_alive_from_function;
    if (!retval) return FALSE;
    char message_from_file[USER_NAME_MSG] = { 0 };
    char real_number_message_to_file[USER_NAME_MSG] = { 0 };

    sprintf_s(real_number_message_to_file, USER_NAME_MSG, "%s%s\n\r", user_title, your_number);
    retval = game_session(Ind, real_number_message_to_file, message_from_file, FALSE, user_title, user_opposite_title, oppsite_ind, &opponent_alive_from_function);
    *opponent_alive = opponent_alive_from_function;
    if (!retval) return FALSE;
    sprintf_s(opponent_number, GUESS, "%s", message_from_file);
    retval = ResetEvent(event_thread[Ind]);
    if (!retval) return FALSE;
    retval = ResetEvent(event_thread_end_reading[Ind]);
    if (!retval) return FALSE;
    return TRUE;
}


BOOL wait_for_client_answer(int* opponent_alive, int choose_event, int opponnent_ind) {
    HANDLE wait_for_multiple_handles[2] = { NULL };
    wait_for_multiple_handles[1] = opponent_failed;
    DWORD dwWaitResultFile;


    switch (choose_event)
    {
    case 2:
        wait_for_multiple_handles[0] = event_thread[opponnent_ind];
        break;
    case 3:
        wait_for_multiple_handles[0] = event_thread_end_reading[opponnent_ind];
        break;
    case 1:
        wait_for_multiple_handles[0] = event_file;
        break;
    case 4:
        wait_for_multiple_handles[0] = close_file_event;
        break;
    default:
        break;
    }
    //printf("check if opponeent alive \n");
    dwWaitResultFile = WaitForMultipleObjects(2, wait_for_multiple_handles, FALSE, INFINITE);
    if ((dwWaitResultFile - WAIT_OBJECT_0) != 0 && (dwWaitResultFile - WAIT_OBJECT_0) != 1) {
        printf("-ERROR: %d - WaitForMultipleObject failed !\n", GetLastError());
        return FALSE;
    }
    else if (dwWaitResultFile - WAIT_OBJECT_0 == 1) {
        *opponent_alive = 0;
    }
    else {
        *opponent_alive = 1;

    }
    //printf("check if opponeent alive answer:%d\n", *opponent_alive);
    return TRUE;
}