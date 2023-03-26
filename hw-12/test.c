#include "otuslog.h"
#include <string.h>
#include <unistd.h>
#include <pthread.h>

//check two threads

void *foo(void *data)
{
	int x = 12;
	
	if (!strcmp((char *)data, "info")) {
		LOG_INFO("x = %d", x);
	}
	else {
		LOG_ERROR("error is here");
	}

	return NULL;
}


int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Error!!! This program should be called as './test FILENAME_TO_LOG'\n");

		return 0;
	}

	print_help();

	init_log(argv[1], 128);

	pthread_t th1, th2;
	pthread_create(&th1, NULL, foo, "info");
	pthread_create(&th2, NULL, foo, "error");
	
	sleep(2);
	
	final_log();
  
	return 0;
}
