/**************************************************************************/
/* This sample program provides code for a client application that uses     */
/* AF_UNIX address family                                                 */
/**************************************************************************/
/**************************************************************************/
/* Header files needed for this sample program                            */
/**************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>


static const char *server_path = "/tmp/otus-server";

int main(int argc, char *argv[])
{
   int    sd = -1, rc, bytesReceived;
   char   buf[65536];
   struct sockaddr_un serveraddr;

   sd = socket(AF_UNIX, SOCK_STREAM, 0);
   if (sd < 0) {
	   perror("socket() failed");
	   return -1;
   }
   
   memset(&serveraddr, 0, sizeof(serveraddr));
   serveraddr.sun_family = AF_UNIX;
   strcpy(serveraddr.sun_path, server_path);

   rc = connect(sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
   if (rc < 0) {
	   close(sd);
	   perror("connect() failed");
	   
	   return -1;
   }

   bytesReceived = 0;
   while (bytesReceived < sizeof(buf)) {
	   rc = recv(sd, &buf[bytesReceived],
		     sizeof(buf) - bytesReceived, 0);
	   if (rc < 0) {
		   perror("recv() failed");
		   break;
	   }
	   else if (rc == 0) {
		   printf("The server closed the connection\n");
		   break;
	   }

	   bytesReceived += rc;
   }

   printf("The size of the interesting file is %s bytes\n", buf);
   
   /***********************************************************************/
   /* Close down any open socket descriptors                              */
   /***********************************************************************/
   if (sd != -1) {
	   close(sd);
   }

   return 0;
}
