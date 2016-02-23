#include "flags.h"
#include <iostream>
#include <fstream>
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
#include <signal.h>
#include <vector>
#define CLIENT_BUF_SIZE 5
#define ARGC -1
#define FILE_DOESNT_EXISTS -2
#define SOCKET_ERROR -3
#define CONNECTION_ERROR -4
#define IS_NOT_EXECUTABLE -5
#define EPOLL_ERROR -6
#define SERVER_DISCONNECT -7
#define FILE_CRASH -8
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
	if(argc < 2) {
		cout << "./client file_you_want_to_send arg1 arg2 ..." << endl;			
		return false;
	}
	return true;
}
int main(int argc, char *argv[]) {
	signal(SIGPIPE, SIG_IGN);
	vector<string> args;
	// checking
	int errors = 0;
	if(arg_check(argc, argv) != true) return ARGC;
	if(argc == 2) {
		cout << "No arguments" << endl;
		string back;
		back += (char)ARG_END;	
		args.push_back(back);
	}
	else {
		for(int i=3; i<argc; i++) {
			args.push_back(argv[i]);
			if((i+1) == argc) {
				args[i-3] += (char)ARG_END;		
			}	
			else args[i-3]+=(char)ARG_SEPARATOR;	
		}
	}
	//string back;
	//back+=(char)ARG_END;
	//args.push_back(back);
	if((errors=file_check(argv[2])) != true) return errors;
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
	inet_aton(argv[1], &sck_addr.sin_addr); // adres argv[1]
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
	//int answer = 0;
	int *position = new int;
	char *prog_bufor = new char[100];
	char *without_a_flag = bufer+1;
	bool ok=true;
	int answer;
	string result_s;
	unsigned int how_many = 0;
	unsigned int pos = 0;
	while(ok) {
		int result = epoll_wait(epoll_handler, &event_wait, 3, -1);
		if(result >= 0) {
			if(event_wait.events & EPOLLERR) {
				cout << "Something has happened with the server! " << endl;
				delete position;
				delete [] prog_bufor;
				close(server);
				close(epoll_handler);
				return EPOLL_ERROR;	
			}
			if(event_wait.events & EPOLLHUP) {
				cout << "Server is down" << endl;
				delete position;
				delete [] prog_bufor;
				close(server);
				close(epoll_handler);
				return 	SERVER_DISCONNECT;		
			}
			answer = read(server, bufer, CLIENT_BUF_SIZE);
			if(answer == 0) {
				cout << "Something has happened with the server! " << endl;
				delete position;
				delete [] prog_bufor;
				close(server);
				close(epoll_handler);
				return EPOLL_ERROR;			
			}
			switch(bufer[0]) {
				case ACCEPTED:
					cout << "The server said hi" << endl;
					break;
                                case POSITION:
                                        memcpy(position, bufer+1, sizeof(int));
                                        cout << "your position: " << *position << endl;
                                        break;
				case SEND_PROG:
					file.open(argv[2], ios::in);		
					if(!file.is_open()) {
						cout << "The file: " << argv[2] << " doesnt exists" << endl;
						close(server);
						close(epoll_handler);
						return FILE_DOESNT_EXISTS;
					}
					else { 
						file.seekg(0, ios_base::end);
						int size = file.tellg();
						file.seekg(0, ios_base::beg);
						cout << "Sending a program" << endl;
						do {
							//sleep(1);
							file.read(prog_bufor, 100);
							if(write(server, prog_bufor, file.gcount()) == EPIPE) { 
								cout << "The server is offline" << endl;
								ok=false;							
							}
							size-=file.gcount();
						} while(file.gcount() > 0);
						if(size==0) {
							cout << "The whole file has been sent" << endl;		
						}
						else {
							cout << "The whole file hasnt been sent" << endl;
							cout << "missed: " << size << " bytes" << endl;
							close(server);
							close(epoll_handler);
							return FILE_CRASH;
						}
					}
					break;
				case SEND_ARGS:
					cout << "Sending args" << endl;
					for(unsigned int i=0; i<args.size(); i++) {
#ifdef DEBUG
						cout << args[i].c_str() << " ";
#endif
						write(server, args[i].c_str(), args[i].size());
					}			
					cout << "\nArgs sent" << endl;
				case RESULT:
					cout << without_a_flag;
					clear(prog_bufor, 100);
					//result_s.clear();
					cout << "Wynik:" << endl;
					while((how_many = read(server, prog_bufor, 100)) > 0) {
						result_s.clear();
						result_s.append(prog_bufor, how_many);
						//cout << prog_bufor;
						if((pos = result_s.find(FILE_SEP)) != string::npos) {
							cout << result_s.substr(0, pos-1); 
							break;						
						}
						else cout << result_s;
						//clear(prog_bufor, 100);					
					}
					cout << endl;
					cout << "Bledy:" << endl;
					cout << result_s.substr(pos+1, result_s.size()-pos+1);
					////////// ???
					clear(prog_bufor, 100);
					while(read(server, prog_bufor, 100) > 0) {
						result_s.clear();
						result_s.append(prog_bufor, how_many);
						cout << result_s;
					}
					ok=false;
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
