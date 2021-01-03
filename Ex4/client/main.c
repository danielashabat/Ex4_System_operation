#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
//#include <Windows.h>
#include "SocketShared.h"
#include "SocketSendRecvTools.h"

int main() {
	char str[16] = "hello:anat;gil\n";
	int indexes[2];
	int lens[2];

	get_param_index_and_len_2_param(&indexes, &lens, &str);
	
}


