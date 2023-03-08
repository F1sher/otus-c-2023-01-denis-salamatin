// The hash table realization is taken from
// https://en.wikipedia.org/wiki/Open_addressing

#ifdef __STDC_ALLOC_LIB__
#define __STDC_WANT_LIB_EXT2__ 1
#else
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


size_t table_size = 1024; // table size, could be increased in rebuild()
int *table_v = NULL; // values in hash table
char **table_k = NULL; // keys in hash table
bool *occ = NULL; // occupation of the field in hash table

// calc hash of string s
unsigned long hash(char *s)
{
#ifdef DEBUG
	return strlen(s);
#endif
	const int p = 31;
	unsigned long hash = 0, p_pow = 1;
	
	for (size_t i = 0; i < strlen(s); i++) {
		hash += (s[i] - 'a' + 1) * p_pow;
		p_pow *= p;	
	}

	return hash;
}

// create empty hash table with size table_size
void create_table(void)
{
	table_v = (int *)calloc(sizeof(int), table_size);
	if (!table_v) {
		exit(EXIT_FAILURE);
	}

	table_k = calloc(sizeof(char *), table_size);
	
	if (!table_k) {
		exit(EXIT_FAILURE);
	}

	occ = calloc(sizeof(bool), table_size);
	if (!occ) {
		exit(EXIT_FAILURE);
	}
	
	return ;
}

void onexit_free(void)
{
	printf("Exit...\n");

	if (table_k) {
		for (size_t i = 0; i < table_size; i++) {
			if (occ[i]) {
				free(table_k[i]); table_k[i] = NULL;
			}
		}
		free(table_k); table_k = NULL;
	}
	if (table_v) { free(table_v); table_v = NULL; }
	if (occ) { free(occ); occ = NULL; }
}


// search key with empty cell or the element in table
size_t find_key(char *k)
{
	size_t i = hash(k) % table_size;

	while (occ[i] && !((strlen(k) == strlen(table_k[i])) && !strcmp(k, table_k[i]))) {
		i = (i + 1) % table_size;
	}

	return i;
}

// if element with key k exist in hash table?
// if exist v takes the appropriate value
bool lookup(char *k, int *v)
{
	size_t i = find_key(k);

	if (occ[i]) {
		*v = table_v[i];
		return true;
	}
	else {
		return false;
	}
}

// add element with key k and value v to hash table
// if the element exist increment v
void add(char *k, int v)
{
	size_t i = find_key(k);
	
	if (occ[i]) {
		table_v[i] = table_v[i] + 1;
	}
	else {
		occ[i] = true;
		table_k[i] = strndup(k, 1024);
		table_v[i] = v;
	}
#ifdef DEBUG
	printf("key = %s, i = %lu, occ = %d, v = %d\n", k, i, occ[i], table_v[i]);
#endif
	
	return ;
}

// remove the element with key from the hash table
// performs appropriate shift of the elements
bool rmv(char *k)
{
	size_t i = find_key(k);
	if (!occ[i]) {
		return false;
	}

	occ[i] = false;
	free(table_k[i]); table_k[i] = NULL;
	size_t j = i;

	while (true) {
		j = (j + 1) % table_size;
		if (!occ[j]) {
			break;
		}
		size_t k = hash(table_k[j]) % table_size;

		if (i <= j) {
			if ((i < k) && (k <= j)) {
				continue;
			}
		}
		else {
			if ((i < k) || (k <= j)) {
				continue;
			}
		}

		table_k[i] = strndup(table_k[j], 1024);
		table_v[i] = table_v[j];
		occ[i] = true;
		
		occ[j] = false;
		free(table_k[j]); table_k[j] = NULL;
		i = j;
	}
		
	return true;
}

// increase the size of hash table by x2
void rebuild(void)
{
	table_size *= 2;
	char **oldtable_k = table_k;
	int *oldtable_v = table_v;
	bool *oldocc = occ;

	create_table();

	for (size_t i = 0; i < table_size / 2; i++) {
		if (oldocc[i]) {
			add(oldtable_k[i], oldtable_v[i]);			
		}
	}

	for (size_t i = 0; i < table_size / 2; i++) {
		if (oldocc[i]) {
			free(oldtable_k[i]); oldtable_k[i] = NULL;
		}
	}

	free(oldtable_k); oldtable_k = NULL;
	free(oldtable_v); oldtable_v = NULL;
	free(oldocc); oldocc = NULL;
}

void print_all_table(void)
{
	printf("Hash table:\n");
	
	for (size_t i = 0; i < table_size; i++) {
		if (occ[i]) {
			printf("[%s] = %d\t", table_k[i], table_v[i]);
		}
		else {
			printf("[NULL]\t");
		}
	}
	printf("\n");

	return ;
}


int main(int argc, char *argv[])
{
	if (atexit(onexit_free)) {
		return EXIT_FAILURE;
	}

#ifdef DEBUG
	create_table();
	add("1", -1);
	add("2", -2);
	print_all_table();
	rebuild();
	print_all_table();
	rmv("2");
	print_all_table();
#endif
	if (argc != 2) {
		printf("You should run this program as:\n");
		printf("open-hash-table INPUT_FILENAME\n");
		
		return EXIT_FAILURE;
	}

	create_table();

	struct stat statbuf_in;
	if (stat(argv[1], &statbuf_in)) {
		perror("Error in stat input file");

		return EXIT_FAILURE;
	}
	
	FILE *fp_in = fopen(argv[1], "r");
	if (!fp_in) {
		//print_help();
		perror("Error in open input file");
		
		return EXIT_FAILURE;
	}

	// read input file in buf
	const size_t max_buf_size = statbuf_in.st_size;
	char *buf = calloc(sizeof(char), max_buf_size);
	if (!buf) {
		fprintf(stderr, "Error in malloc buf for words in file\n");
		fclose(fp_in);

		return EXIT_FAILURE;
	}

	fread(buf, sizeof(char), max_buf_size, fp_in);

	// replace \n with whitespace for strtok
	for (size_t i = 0; i < strlen(buf); i++) {
		if (buf[i] == '\n') {
			buf[i] = ' ';
		}
	}

	// parse buf and fill hashtable with words
	char delim[] = " \t";
	char *tmpstr = strtok(buf, delim);
	
	while (tmpstr) {
		add(tmpstr, 1);
		tmpstr = strtok(NULL, " ");
	}

	// print words and their counts
	for (size_t i = 0; i < table_size; i++) {
		if (occ[i]) {
			printf("'%s' count is %d\n", table_k[i], table_v[i]);
		}
	}
	
	fclose(fp_in);
	free(buf); buf = NULL;
	
	return EXIT_SUCCESS;


}
