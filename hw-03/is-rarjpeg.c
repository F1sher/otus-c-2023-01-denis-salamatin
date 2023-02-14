#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "zipfiletypes.h"


// search 4-byte sign in data of datalen size from start index. Return first occuerence or -1 if no sign
int find_signature(char *data, int start, int datalen, const char *sign)
{
	for (int i = start; i < datalen-3; i++) {
		if ((data[i+0] == sign[0]) & (data[i+1] == sign[1]) &
		    (data[i+2] == sign[2]) & (data[i+3] == sign[3]))
			return i;
	}
	
	return -1;
}

int main(int argc, char *argv[])
{
	//check program input params
	if (argc != 2) {
		printf("WRONG INPUT: You should enter rarjpeg filename as second parameter\n");
		return 0;
	}
	
	const char *zipfname = argv[1];
	int file_cntr = 0;

	//Local file header signature
	char lfh_sign[4] = {0x50, 0x4b, 0x03, 0x04};

	int fd_sign = open(zipfname, O_RDONLY);
	if (fd_sign < 0) { perror(""); exit(1); }

	int fd_fname = open(zipfname, O_RDONLY);
	if (fd_fname < 0) { perror(""); close(fd_sign); exit(1); }
	
	int chunk_size = 1024 * 16;
	unsigned char fname_size[2] = {0};
	char *data = (char *)calloc(chunk_size, sizeof(char));
	if (!data) {
		perror("");
		close(fd_sign); close(fd_fname);
		exit(1);
	}
	
	int sz = 0;
	int sign_shift = 0, sign_lpos = -1;
	
	// read file by chunks
	while ((sz = read(fd_sign, data, chunk_size))) {
		int start_sign_search = 0;
		
		// search local file header signature
		while((sign_lpos = find_signature(data, start_sign_search, sz, lfh_sign)) != -1) {
			file_cntr++;
			start_sign_search = sign_lpos + 1;
			
			// get file name len
			lseek(fd_fname, sign_lpos + sign_shift + 26, SEEK_SET);
			read(fd_fname, fname_size, 2);

			// get file name
			char *fname = (char *)calloc(fname_size[0] + 16 * fname_size[1], sizeof(char));
			lseek(fd_fname, sign_lpos + sign_shift + 26 + 4, SEEK_SET);
			read(fd_fname, fname, fname_size[0] + 16 * fname_size[1]);

			printf("%s\n", fname);
			
			free(fname);

		}
		
		sign_shift += sz;
	}
	

	free(data); data = NULL;
	close(fd_sign); close(fd_fname);

	if (!file_cntr) {
		printf("Not rarjpeg\n");
	}
	
	return 0;
}
