/*
 *
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 * Contributor of the original skeleton: Jiri Hnidek <jiri.hnidek@tul.cz>.
 * https://github.com/jirihnidek/daemon/blob/master/src/daemon.c
 *
 */

#include <sys/syslog.h>
#define  _POSIX_C_SOURCE  200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>

#include <signal.h>

#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include <glib.h>
#include <glib/gprintf.h>


static int running = 0;
static char *conf_filename = NULL;
static char *app_name = NULL;
static char *datafile = NULL;
static char *server_path = NULL;

/**
 * \brief Read configuration from config file
 */
int read_conf_file(int reload)
{
	int ret = 0;
	g_autoptr(GError) error = NULL;
	g_autoptr(GKeyFile) key_file = g_key_file_new ();
	
	if (!g_key_file_load_from_file (key_file, conf_filename, G_KEY_FILE_NONE, &error)) {
	  if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
	    syslog(LOG_ERR, "Error loading key file: %s", error->message);
	    
	    return -1;
	  }
	}
	
	g_autofree gchar *val1 = g_key_file_get_string (key_file, "Files", "DataFile", &error);
	if (val1 == NULL &&
	    !g_error_matches (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND)) {
		syslog(LOG_ERR, "Error finding key in key file: %s", error->message);
			
		ret = -1;
	}
	else if (val1 == NULL) {
		syslog(LOG_ERR, "The DataFile should be specified in .ini");
		
		ret = -2;
	}

	g_autofree gchar *val2 = g_key_file_get_string (key_file, "Files", "SocketFile", &error);
	if (val2 == NULL &&
	    !g_error_matches (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND)) {
	  syslog(LOG_CRIT, "Error to get socket filename from %s", conf_filename);
	  free(conf_filename); conf_filename = NULL;
		
	  return EXIT_FAILURE;
	}
	else if (val2 == NULL) {
		syslog(LOG_CRIT, "The SocketFile field is empty in .ini file"); 
		free(conf_filename); conf_filename = NULL;
		
		return EXIT_FAILURE;
	}
	
	if (!ret) {
		if (reload == 1) {
			syslog(LOG_INFO, "Reloaded configuration file %s of %s",
			       conf_filename,
			       app_name);
		} else {
			syslog(LOG_INFO, "Configuration of %s read from file %s",
			       app_name,
			       conf_filename);
		}

		if (datafile) { free(datafile); datafile = NULL; }
		datafile = g_strdup(val1);

		server_path = g_strdup(val2);
	}

	return ret;
}

/**
 * \brief This function tries to test config file
 */
int test_conf_file(char *conf_fname)
{
	g_autoptr(GError) error = NULL;
	g_autoptr(GKeyFile) key_file = g_key_file_new ();

	if (!g_key_file_load_from_file (key_file, conf_fname, G_KEY_FILE_NONE, &error)) {
		if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
			fprintf(stderr, "Error loading key file: %s\n", error->message);
		return EXIT_FAILURE;
	}

	g_autofree gchar *val = g_key_file_get_string (key_file, "Files", "DataFile", &error);
	if (val == NULL &&
	    !g_error_matches (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND)) {
		fprintf(stderr, "Error finding key in key file: %s\n", error->message);
		return EXIT_FAILURE;
	}
	else if (val == NULL) {
		fprintf(stderr, "The DataFile should be specified in .ini\n");
		
		return EXIT_FAILURE;
	}

	fprintf(stdout, "The datafile is %s\n", val);

	return EXIT_SUCCESS;
}

/**
 * \brief This function will daemonize this app
 */
static void daemonize()
{
	pid_t pid = 0;
	int fd;

	/* Fork off the parent process */
	pid = fork();

	/* An error occurred */
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}

	/* Success: Let the parent terminate */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* On success: The child process becomes session leader */
	if (setsid() < 0) {
		exit(EXIT_FAILURE);
	}

	/* Ignore signal sent from child to parent process */
	signal(SIGCHLD, SIG_IGN);

	/* Fork off for the second time*/
	pid = fork();

	/* An error occurred */
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}

	/* Success: Let the parent terminate */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* Set new file permissions */
	umask(0);

	/* Change the working directory to the root directory */
	/* or another appropriated directory */
	chdir("/");
	
	/* Close all open file descriptors */
	for (fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--) {
		close(fd);
	}

	/* Reopen stdin (fd = 0), stdout (fd = 1), stderr (fd = 2) */
	stdin = fopen("/dev/null", "r");
	stdout = fopen("/dev/null", "w+");
	stderr = fopen("/dev/null", "w+");

	/* Try to write PID of daemon to lockfile */
	/*
	const char *pid_file_name = "/home/dasalam/job/otus/c/otus-c-2023-01-denis-salamatin/hw-19/daemon.pid";
	if (pid_file_name != NULL)
	{
		char str[256];
		int pid_fd = open(pid_file_name, O_RDWR|O_CREAT, 0640);
		if (pid_fd < 0) {
		  syslog(LOG_CRIT, "Error in open file to write PID, fd = %d ", fd);
		  
		  exit(EXIT_FAILURE);
		}

		sprintf(str, "%d\n", getpid());

		write(pid_fd, str, strlen(str));
		close(pid_fd);
	}
	*/
}

/**
 * \brief Print help for this application
 */
void print_help(void)
{
	printf("\n Usage: %s [OPTIONS]\n\n", app_name);
	printf("  Options:\n");
	printf("   -h --help                 Print this help\n");
	printf("   -c --conf_file filename   Read configuration from the file\n");
	printf("   -t --test_conf filename   Test configuration file\n");
	printf("   -d --daemon               Daemonize this application\n");
	printf("\n");
}

/* Main function */
int main(int argc, char *argv[])
{	
	static struct option long_options[] = {
		{"conf_file", required_argument, 0, 'c'},
		{"test_conf", required_argument, 0, 't'},
		{"help", no_argument, 0, 'h'},
		{"daemon", no_argument, 0, 'd'},
		{NULL, 0, 0, 0}
	};
	int value, option_index = 0;
	int start_daemonized = 0;

	app_name = argv[0];

	/* Try to process all command line arguments */
	while ((value = getopt_long(argc, argv, "c:t:dh", long_options, &option_index)) != -1) {
		switch (value) {
			case 'c':
				conf_filename = g_strdup(optarg);
				break;
				break;
			case 't':
				return test_conf_file(optarg);
			case 'd':
				start_daemonized = 1;
				break;
			case 'h':
				print_help();
				return EXIT_SUCCESS;
			case '?':
				print_help();
				return EXIT_FAILURE;
			default:
				break;
		}
	}

	if (!conf_filename) {
		fprintf(stderr, "Wrong usage!\n");
		print_help();

		return EXIT_FAILURE;
	}

	/* Open system log and write message to it */
	openlog(argv[0], LOG_PID | LOG_CONS, LOG_DAEMON);
	syslog(LOG_INFO, "Started %s", app_name);
	
	// create and initialize AF_UNIX socket
	// example from https://www.ibm.com/docs/en/i/7.2?topic=uauaf-example-server-application-that-uses-af-unix-address-family

	/* Read configuration from config file */
	read_conf_file(0);

	/* When daemonizing is requested at command line. */
	if (start_daemonized == 1) {
		/* It is also possible to use glibc function deamon()
		 * at this point, but it is useful to customize your daemon. */
		daemonize();
	}
	
	int sd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sd < 0) {
	  perror("socket() failed");
	  syslog(LOG_CRIT, "socket() failed | %s", strerror(errno));
	  free(conf_filename); conf_filename = NULL;
	  free(server_path); server_path = NULL;
		
	  return EXIT_FAILURE;
	}

	struct sockaddr_un serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sun_family = AF_UNIX;
	strcpy(serveraddr.sun_path, server_path);
	unlink(serveraddr.sun_path);
	
	int rc = bind(sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if (rc < 0) {
		perror("bind() failed");
		syslog(LOG_CRIT, "bind() failed | %s", strerror(errno));
		free(conf_filename); conf_filename = NULL;
		free(server_path); server_path = NULL;
		
		return EXIT_FAILURE;
	}

	rc = listen(sd, 10);
	if (rc < 0)  {
		close(sd);
		unlink(server_path);
		free(conf_filename); conf_filename = NULL;
		free(server_path); server_path = NULL;
		
		perror("listen() failed");
		syslog(LOG_CRIT, "listen() failed | %s", strerror(errno));
		
		return EXIT_FAILURE;
	}

	/* This global variable can be changed in function handling signal */
	running = 1;

	int sd2 = -1;
	/* Never ending loop of server */
	while (running == 1) {
		/* TODO: dome something useful here */
		// see https://gist.github.com/tscho/397539/05eab96d26dd73bf3cb0a47fbe717a9402582edf
		printf("Listening...\n");
		syslog(LOG_INFO, "Listening...");
		
		sd2 = accept(sd, NULL, NULL);
		if (sd2 < 0) {
			syslog(LOG_CRIT, "accept() failed : %s ", strerror(errno));
			
			break;
		}

		struct stat st;
		char buf[65536];
		int ret = stat(datafile, &st);
		if (!ret) {
			ssize_t datafile_size = st.st_size;
			snprintf(buf, 65536, "%lu", datafile_size);
		}
		else {
		  syslog(LOG_ERR, "Error in stat : %s | ret = %d | datafile = %s", strerror(errno), ret, datafile);
		  snprintf(buf, 3, "-1");
		}
		
		rc = send(sd2, buf, sizeof(buf), 0);
		if (rc < 0) {
			syslog(LOG_ERR, "send() failed : %s ", strerror(errno));
		}
		close(sd2); sd2 = -1;
	}

	/* Write system log and close it. */
	syslog(LOG_INFO, "Stopped %s", app_name);
	closelog();

	if (sd != -1) {
		close(sd);
	}
		
	if (sd2 != -1) {
		close(sd2);
	}

	unlink(server_path);

	free(conf_filename); conf_filename = NULL;
	free(datafile); datafile = NULL;
	free(server_path); server_path = NULL;
	
	return EXIT_SUCCESS;
}
