#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <ctype.h>
#include "chatsvr.h"
#include "parse.h"
#include "util.h"

int lookup(int argc, char **argv);
void chatter(int size, int socketFd, fd_set fdlist);
void marvinCalc(char *recv_msg, int socketFd);
void parseCheck(struct expr *e, int socketFd, char *otherName);
int main(int argc, char **argv)
{
    int socketFd; //Socket
    struct sockaddr_in r;
    char *handle = "Marvin\n";
    char id[MAXMESSAGE + 1];
    fd_set fdlist;
    
	if (!(lookup(argc,argv))) {
		if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			perror("socket");
			exit (1);
		}
		memset(&r, 0, sizeof r);
		r.sin_family = AF_INET; 
		r.sin_addr.s_addr = INADDR_ANY;
		if (argc == 2) 
			r.sin_port = htons(1234);
		else
			r.sin_port = htons(atoi(argv[2]));
		
		if (connect(socketFd, (struct sockaddr *)&r, sizeof(r)) < 0) { //Connect
			close(socketFd);
			perror("Connect");
			exit (1);
		}
		read(socketFd, id, MAXMESSAGE);
		
		if (strstr(id, CHATSVR_ID_STRING)) {
			send(socketFd,handle,strlen(handle),0); //Send Marvin handle
			while(1){
				FD_ZERO(&fdlist);
				FD_SET(socketFd, &fdlist);
				FD_SET(0, &fdlist);
				
				switch (select(FD_SETSIZE , &fdlist, NULL, NULL, NULL)) {
					case -1:
						perror("select");
						close(socketFd);
						exit(1);
					default:
						chatter(FD_SETSIZE, socketFd, fdlist);
				}
			}
			
			close(socketFd);
		}
	}
    return(0);
}
int lookup(int argc, char **argv) //Check if localhost, port exist
{
    struct hostent *hp;

    if (argc != 2 && argc != 3) {
		fprintf(stderr, "usage: ./marvin host [port]\n");
		return(1);
    }

    if ((hp = gethostbyname(argv[1])) == NULL) {
		fprintf(stderr, "%s: no such host\n", argv[1]);
		return(1);
    }
    if (hp->h_addr_list[0] == NULL || hp->h_addrtype != AF_INET) {
		fprintf(stderr, "%s: not an internet protocol host name\n", argv[1]);
		return(1);
    }

    return (0);
}

void chatter(int size, int socketFd, fd_set fdlist) //send and recieve msgs
{
	int result, fdCheck = 0;
	char recv_msg[MAXMESSAGE + 1], send_msg[MAXMESSAGE + 1];
	
	while (fdCheck < size){
		
		if (FD_ISSET(fdCheck, &fdlist)) {
			
			if (fdCheck == socketFd) { //Recieve msg
				//If server shutdown
				if ((result = read(socketFd, recv_msg, MAXMESSAGE)) == 0) {
					printf("Server shut down\n");
					close(socketFd);
					exit(1);
				}
				recv_msg[result] = '\0';  /* Terminate string with null */
				printf("%s", recv_msg );
				marvinCalc(recv_msg, socketFd);
				fflush(stdout);
				
			}
			else if(fdCheck == 0){ //Sending msg
				
				if (fgets(send_msg, MAXMESSAGE+1, stdin) == NULL) {
					close(socketFd);
					exit(0);
				}
				write(socketFd, send_msg, strlen(send_msg)); //Write to other
			}
		}
		fflush(stdout);
		fdCheck++;
	}
}
void marvinCalc(char *recv_msg, int socketFd) //Checks if auto calc needed
{
	char *name = strstr(recv_msg,":");	//Name of the usr
	int index = name - recv_msg;
	int i, total = index+2;
	char check[MAXMESSAGE] = "";
	char marv[MAXMESSAGE+1]; //Calculation
	
	for(i = index+2; i < strlen(recv_msg) ;i++) //Makes sure no number before
	{
		if (recv_msg[i] != 32 && recv_msg[i] != 9) { 
			check[strlen(check)] = recv_msg[i];
			recv_msg[i] = tolower(recv_msg[i]);
		} 
		total += 1;
		
		if (strcasecmp(check, "HeyMarvin,") == 0) 
			break;	
	}
	if (strcasecmp(check,"HeyMarvin,") == 0 && strstr(recv_msg, "hey marvin,")){
		
		strncpy(marv, recv_msg+total, strlen(recv_msg)); //get the numbers
		struct expr *e = parse(marv); //pass numbers in parse
		char otherName[MAXHANDLE + 1]; //Get other person name
		strncpy(otherName, recv_msg, index+1);
		otherName[index] = '\0';
		parseCheck(e, socketFd, otherName); 
	}
}
void parseCheck(struct expr *e, int socketFd, char *otherName) //Check if valid parse args
{
		
	if (e) {
		char answer[MAXMESSAGE+1];
		sprintf(answer,"Hey %s, %d\n",otherName,evalexpr(e));
		if (write(socketFd, answer, strlen(answer)) == -1){
			close(socketFd);
			exit(1);
		}
		freeexpr(e);
	}
	else {
		char error[MAXMESSAGE + 1]; //if no numbers are given after hey marvin
		sprintf(error,"Hey %s, I don't like that.\n",otherName);
		if (write(socketFd, error, strlen(error)) == -1){
			close(socketFd);
			exit(1);
		}
		printf("%s\n", errorstatus); //if any errors in args
	}
}
