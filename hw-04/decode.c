#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>


unsigned short koi8r[0x100] = {[0xc0] = 0xd18e, [0xc1] = 0xd0b0, [0xc2] = 0xd0b1, [0xc3] = 0xd186, [0xc4] = 0xd0b4, [0xc5] = 0xd0b5, [0xc6] = 0xd1b4, [0xc7] = 0xd0b3, 
			       [0xc8] = 0xd185, [0xc9] = 0xd0b8, [0xca] = 0xd0b9, [0xcb] = 0xd0ba, [0xcc] = 0xd0bb, [0xcd] = 0xd0bc, [0xce] = 0xd0bd, [0xcf] = 0xd0be,
			       [0xd0] = 0xd0bf, [0xd1] = 0xd18f, [0xd2] = 0xd180, [0xd3] = 0xd181, [0xd4] = 0xd182, [0xd5] = 0xd183, [0xd6] = 0xd0b6, [0xd7] = 0xd0b2,
			       [0xd8] = 0xd18c, [0xd9] = 0xd18b, [0xda] = 0xd0b7, [0xdb] = 0xd188, [0xdc] = 0xd18d, [0xdd] = 0xd189, [0xde] = 0xd187, [0xdf] = 0xd18a,
			       [0xe0] = 0xd0ae, [0xe1] = 0xd090, [0xe2] = 0xd091, [0xe3] = 0xd0a6, [0xe4] = 0xd094, [0xe5] = 0xd095, [0xe6] = 0xd0a4, [0xe7] = 0xd093,
			       [0xe8] = 0xd0a5, [0xe9] = 0xd098, [0xea] = 0xd099, [0xeb] = 0xd09a, [0xec] = 0xd09b, [0xed] = 0xd09c, [0xee] = 0xd09d, [0xef] = 0xd09e,
			       [0xf0] = 0xd09f, [0xf1] = 0xd0af, [0xf2] = 0xd0a0, [0xf3] = 0xd0a1, [0xf4] = 0xd0a2, [0xf5] = 0xd0a3, [0xf6] = 0xd096, [0xf7] = 0xd092,
			       [0xf8] = 0xd0ac, [0xf9] = 0xd0ab, [0xfa] = 0xd097, [0xfb] = 0xd0a8, [0xfc] = 0xd0ad, [0xfd] = 0xd0a9, [0xfe] = 0xd0a7, [0xff] = 0xd0aa};
unsigned short cp1251[0x100];
unsigned short iso8859[0x100];


void print_help() {
	printf("You should run this program as:\n");
	printf("decode ENCODING INPUT_FILENAME OUTPUT_FILENAME\n");
	printf("Where ENCODING is one of the supported encoding of INPUT_FILENAME: koi8-r, cp-1251 or iso-8859.\n");
	printf("OUTPUT_FILENAME will be overwritten if exist or created.\n");
	
	return ;
}

int main(int argc, char *argv[])
{
	if (argc != 4) {
		print_help();
		
		return 0;
	}

	if (strncmp(argv[1], "koi8-r", 7) &&
	    strncmp(argv[1], "cp-1251", 8) &&
	    strncmp(argv[1], "iso-8859", 9)) {
		printf("Wrong encoding!\n\n");
		print_help();
		
		return 0;
	}
	
	//fill koi8r -> utf8 table
	for (int i = 0; i < 0x7e; i++) {
		koi8r[i] = i;
	}

	//swap lsb and msb
	for (int i = 0; i < 0x100; i++) {
		koi8r[i] = (koi8r[i] >> 8) + (koi8r[i] << 8);
	}

	//fill cp1251 to utf8 table
	for (int i = 0; i < 0x7f; i++) {
		cp1251[i] = i;
	}

	for (int i = 0xc0; i <= 0xcf; i++) {
		cp1251[i] = 0xd090 + (i - 0xc0);
	}
	for (int i = 0xd0; i <= 0xdf; i++) {
		cp1251[i] = 0xd0a0 + (i - 0xd0);
	}
	for (int i = 0xe0; i <= 0xef; i++) {
		cp1251[i] = 0xd0b0 + (i - 0xe0);
	}
	for (int i = 0xf0; i <= 0xff; i++) {
		cp1251[i] = 0xd180 + (i - 0xf0);
	}

	//swap lsb ans msb
	for (int i = 0; i < 0x100; i++) {
		cp1251[i] = (cp1251[i] >> 8) + (cp1251[i] << 8);
	}

	//fill iso8595
	for (int i = 0; i < 0x7f; i++) {
		iso8859[i] = i;
	}

	for (int i = 0xb0; i < 0xdf; i++) {
		iso8859[i] = 0xd090 + (i - 0xb0);
	}
	for (int i = 0xe0; i < 0xef; i++) {
		iso8859[i] = 0xd180 + (i - 0xe0);
	}
	
	//swap lsb and msb
	for (int i = 0; i < 0x100; i++) {
		iso8859[i] = (iso8859[i] >> 8) + (iso8859[i] << 8);
	}

	//read text from input file, the max filesize is inbuf_size
	int fd_in = open(argv[2], O_RDONLY);
	if (fd_in < 0) {
		print_help();
		perror("Error in open input file");
		
		return 0;
	}
	
	size_t inbuf_size = 1024 * 1024;
	unsigned char *inbuf = (unsigned char *)calloc(inbuf_size, 1);
	if (!inbuf) {
		close(fd_in);
		fprintf(stderr, "Error in memory allocation for input buffer\n");
		
		return -1;
	}
	
	int sz = read(fd_in, inbuf, inbuf_size);
	if (sz < 0) {
		close(fd_in);
		perror("Error in read input file");
		
		return -1;
	}

	unsigned short *outbuf = (unsigned short *)calloc(sz, sizeof(unsigned short));

	if (!outbuf) {
		close(fd_in);
		free(outbuf); outbuf = NULL;
		fprintf(stderr, "Error in memory allocation for output buffer\n");
		
		return -1;
	}

	//convert
	for (int i = 0; i < sz; i++) {
		if (!strncmp(argv[1], "koi8-r", 7)) {
			outbuf[i] = koi8r[inbuf[i]];
		}
		else if (!strncmp(argv[1], "cp-1251", 8)) {
			outbuf[i] = cp1251[inbuf[i]];
		}
 		else if (!strncmp(argv[1], "iso-8859", 9)) {
			outbuf[i] = iso8859[inbuf[i]];
		}
	}

	//write text to the output file
	int fd_out = open(argv[3], O_WRONLY | O_TRUNC | O_CREAT, 0644);
	if (!fd_out) {
		close(fd_in);
		close(fd_out);
		free(inbuf); inbuf = NULL;
		free(outbuf); outbuf = NULL;
		perror("Error in open output file");

		return -1;
	}
	
	sz = write(fd_out, outbuf, sz * sizeof(unsigned short));

	close(fd_in);
	close(fd_out);
	free(inbuf); inbuf = NULL;
	free(outbuf); outbuf = NULL;

	printf("The converted text is written in '%s'\n", argv[3]);
	
	return 0;
}
