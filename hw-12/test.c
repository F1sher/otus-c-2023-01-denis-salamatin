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
	int ret = 0;
	ret = pthread_create(&th1, NULL, foo, "info");
	if (ret) {
		fprintf(stderr, "Error in creation of the first thread, ret = %d. Exit...\n", ret);

		final_log();
		return 0;
	}
	
	ret = pthread_create(&th2, NULL, foo, "error");
	if (ret) {
		fprintf(stderr, "Error in creation of the second thread, ret = %d. Exit...\n", ret);
		
		pthread_join(th1, NULL);
		
		final_log();
		return 0;
	}
	
	sleep(2);
	
	pthread_join(th1, NULL);
	pthread_join(th2, NULL);
	
	final_log();
  
	return 0;
}
