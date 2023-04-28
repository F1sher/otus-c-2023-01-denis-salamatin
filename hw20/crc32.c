#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>


// crc32 algorithm from https://rosettacode.org/wiki/CRC-32#C
uint32_t rc_crc32(uint32_t crc, const char *buf, size_t len)
{
	static uint32_t table[256];
	static int have_table = 0;
	uint32_t rem;
	uint8_t octet;
	int i, j;
	const char *p, *q;

	/* This check is not thread safe; there is no mutex. */
	if (have_table == 0) {
		/* Calculate CRC table. */
		for (i = 0; i < 256; i++) {
			rem = i;  /* remainder from polynomial division */
			for (j = 0; j < 8; j++) {
				if (rem & 1) {
					rem >>= 1;
					rem ^= 0xedb88320;
				} else
					rem >>= 1;
			}
			table[i] = rem;
		}
		have_table = 1;
	}

	crc = ~crc;
	q = buf + len;
	for (p = buf; p < q; p++) {
		octet = *p;  /* Cast to unsigned octet. */
		crc = (crc >> 8) ^ table[(crc & 0xff) ^ octet];
	}
	
	return ~crc;
}


void print_help()
{
	printf("Call as:\n");
	printf("./crc32 FILENAME\n");
	
	return ;
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Wrong program call!\n");
		print_help();
		
		return -1;
	}

	char *fname = argv[1];

	int fd = open (fname, O_RDONLY);
	assert (fd != -1);

	struct stat finfo;
	assert (fstat (fd, &finfo) != -1);
	
	char *mmap_addr = mmap (NULL, finfo.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	assert (mmap_addr != MAP_FAILED);

	uint32_t crc_val = rc_crc32(0, mmap_addr, finfo.st_size); //calc_crc32(mmap_addr, finfo.st_size);

	printf("crc32 value of '%s' is 0x%x\n", fname, crc_val);
	
	munmap (mmap_addr, finfo.st_size);
	close (fd);
	
	return 0;
}
