#include "flags.h"
#include<iostream>
#include<fstream>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#define CLIENT_BUF_SIZE 5
#define ARGC -1
#define FILE_DOESNT_EXISTS -2
#define SOCKET_ERROR -3
#define CONNECTION_ERROR -4
#define IS_NOT_EXECUTABLE -5
#define EPOLL_ERROR -6
#define SERVER_DISCONNECT -7
using namespace std;
char bufer[CLIENT_BUF_SIZE];
void clear(char *pointer, unsigned int size);
void acceptHandler();
void pingHandler();
void sendProgHandler();
void resultHandler();
void positionHandler();
int file_check(char *file_name) {
	if(access(file_name, F_OK) != 0) {
		cout << "Cannot find a file: " << file_name << endl;	
		return FILE_DOESNT_EXISTS;
	}
	if(access(file_name, X_OK) != 0) {
		cout << "The file: " << file_name << " is not an executable file" << endl;
		return IS_NOT_EXECUTABLE;
	}
	return true;
}
bool arg_check(int argc, char *argv[]) {
	if(argc != 2) {
		cout << "./client file_you_want_to_send" << endl;			
		return false;
	}
	return true;
}
int main(int argc, char *argv[]) {
	// checking
	int errors = 0;
	if(arg_check(argc, argv) != true) return ARGC;
	if((errors=file_check(argv[1])) != true) return errors;
	int epoll_handler;
	if((epoll_handler= epoll_create(1)) < 0) {
		cout << "epoll error" << endl;
		return EPOLL_ERROR;	
	}
	fstream file;
	struct sockaddr_in sck_addr;
	int server;
	//////////////////////////////////////////////// Connecting
	cout << "Connecting to the server" << endl;
	sck_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &sck_addr.sin_addr);
	sck_addr.sin_port = htons(5534);
	if((server=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		cout << "cant create a socket" << endl;
		return SOCKET_ERROR;	
	}
	if(connect(server, (struct sockaddr*)&sck_addr, sizeof(sck_addr)) < 0) {
		cout << "Cant connect to the server" << endl;
		return CONNECTION_ERROR;
	}
	//////////////////////////////////////////////////////////////////
	epoll_event event;
	epoll_event event_wait;
	event.data.fd = server;
	event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
	if(epoll_ctl(epoll_handler, EPOLL_CTL_ADD, server, &event) < 0) {
		close(server);
		perror("epoll_ctl error\n");
		return 	EPOLL_ERROR;
	} 
	cout << "Connected to the server" << endl;
	int answer = 0;
	int *position = new int;
	char *prog_bufor = new char[100];
	char *without_a_flag = bufer+1;
	bool ok=true;
	while(ok) {
		int result = epoll_wait(epoll_handler, &event_wait, 3, -1);
		if(result >= 0) {
			if(event_wait.events & EPOLLERR) {
				cout << "Something has happened with the server! " << endl;
				close(server);
				close(epoll_handler);
				return EPOLL_ERROR;	
			}
			if(event_wait.events & EPOLLHUP) {
				cout << "Server is down" << endl;
				close(server);
				close(epoll_handler);
				return 	SERVER_DISCONNECT;		
			}
			answer = read(server, bufer, CLIENT_BUF_SIZE);
			switch(bufer[0]) {
				case ACCEPTED:
					cout << "The server said hi" << endl;
					break;
				case SEND_PROG:
					file.open(argv[1], ios::in);		
					if(!file.is_open()) {
						cout << "The file: " << argv[1] << " doesnt exists" << endl;
						close(server);
						close(epoll_handler);
						return FILE_DOESNT_EXISTS;
					}
					else { 
						cout << "Sending a program" << endl;
						do {
							file.read(prog_bufor, 100);
							write(server, prog_bufor, file.gcount());
						} while(file.gcount() > 0);
					}
					break;		
				case RESULT:
					cout << without_a_flag;
					clear(prog_bufor, 100);
					while(read(server, prog_bufor, 100) > 0) {
						cout << prog_bufor;					
					}
					ok=false;
					break;
				case POSITION:
					memcpy(position, bufer+1, sizeof(int));
					cout << "your position: " << *position << endl;
					break;
				case PROG_RCV:
					cout << "Program sent" << endl;
					break;
				default:
					cout << "Unknown flag" << endl;
					break;
			}
		}
		else { ok=false; }
	}
	cout << "The end" << endl;
	delete []prog_bufor;
	delete position;
	close(server);
	return 0;
}
void clear(char *pointer, unsigned int size) {
	for(unsigned int i=0; i<size; i++) pointer[i]=0;
}
