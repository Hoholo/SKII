#ifndef _SERVER_H_
#define _SERVER_H_
#include<list>
#include<fstream>
#include<string>
#include<iostream>
#include<thread>
#include<cstdio>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <mutex>
#include "flags.h"
#include "errorclass.h"
#include "my_pos.h"
using namespace std;
class JobScheduler {
	private:
		bool set;
		mutex clients_list;
		mutex ping_stuff;
		int serv_socket;
		sockaddr_in sck_addr;
		list<int> clients; // chronic semaforami
		void guard_queue();
		void queue_send(unsigned int where_to_start=0);
		void download_and_execute();
		void clientHandler(int sock_id, int pos_start);
		void errorHandler(const ErrorClass &error);
		void posInfo();
		bool read_and_save(int sock_id);
		void send_result(int sock_id);
		int epoll_handler;
		fstream program;
		fstream result;
		fstream errors;
		epoll_event *ping_poll;
	public:
		JobScheduler(int port, int max_clients);
		~JobScheduler() {
			delete [] ping_poll;
		};
		void start();
};
void JobScheduler::guard_queue() {
	while(true) {	
		int result = epoll_wait(epoll_handler, ping_poll, 3, -1);
		for(int i=0; i<result; i++) {
			cout << "Ktos uciekl" << endl;
			epoll_ctl(epoll_handler, EPOLL_CTL_DEL, ping_poll[i].data.fd, NULL);
			// TODO 
		}
	}
}
void JobScheduler::queue_send(unsigned int where_to_start) {
	char flag = POSITION;
	char *to_send = new char[sizeof(int)+1];
	my_pos *pos = new my_pos(1);
	to_send[0] = flag;
	char *pointer = to_send+1;
	//int start=0;
	memcpy(pointer, pos->my_pos_char, sizeof(int));
	////////////////////////////////
	clients_list.lock();
	for(list<int>::iterator iter=clients.begin(); iter != clients.end(); iter++) {
		write(*iter, to_send, sizeof(int)+sizeof(char));
		pos->my_pos_int++;
		memcpy(pointer, pos->my_pos_char, sizeof(int));
	}
	clients_list.unlock();
	//////////////////////////////
	delete pos;
	delete [] to_send;
}
bool JobScheduler::read_and_save(int sock_id) {
	program.open("to_execute.out", ios::out | ios::trunc);
	char *blocks_of_data = new char[100];
	int how_many = 0;
	while(true) {
		sleep(1);
		how_many = read(sock_id, blocks_of_data, 100);
		if(how_many == 0) break;
		program.write(blocks_of_data, how_many);
		if(how_many < 100) break;
		if(blocks_of_data[99] == EOF) break;
	}
	delete [] blocks_of_data;
	program.close();
	if(how_many==0) return false;
	else return true; // 
}
void JobScheduler::send_result(int sock_id) {
	errors.open("error.txt", ios::in);
	result.open("result.txt", ios::in);
	if(!errors.is_open() || !result.is_open()) {
		cout << "Cant open result files!\n";
		// TODO poinformowac klienta o problemie 
	}
	char *blocks_of_data = new char[100];
	do {
		result.read(blocks_of_data, 100);
		if(write(sock_id, blocks_of_data, result.gcount()) == EPIPE) break; // nikt nie slucha
	} while(result.gcount() > 0);
	do {
		errors.read(blocks_of_data, 100);
		if(write(sock_id, blocks_of_data, errors.gcount()) == EPIPE) break;
	} while(errors.gcount() > 0);
	delete [] blocks_of_data;
	result.close();
	errors.close();
}
void JobScheduler::download_and_execute() {
	unsigned char flag_to_send;
	char *blocks_of_data = new char[100];
	while(true) {
		while(clients.empty());
		int first = clients.front();
		flag_to_send = SEND_PROG;
		write(first, &flag_to_send, 1);
		cout << "Waiting for a file" << endl;
		if(read_and_save(first) == false) {
			cout << "Lost connection!!" << endl;
			close(first);
			clients_list.lock();
			clients.pop_front();
			clients_list.unlock();		
			queue_send();
		}
		else {
			cout << "Program received" << endl;
			flag_to_send = PROG_RCV;
			write(first, &flag_to_send, 1);
			system("./to_execute.out > result.txt 2> error.txt");
			flag_to_send = RESULT;
			write(first, &flag_to_send, 1);
			cout << "Wysylanie odpowiedzi" << endl;
			send_result(first);// wysylanie rezultatu
			cout << "Zakonczono" << endl;
			close(first);
			clients_list.lock();
			clients.pop_front();
			clients_list.unlock();
			queue_send();
		}
	}
	delete [] blocks_of_data;
}
void JobScheduler::clientHandler(int sock_id, int pos_start) {
	write(sock_id,(const void*)ACCEPTED, 1); // polaczono
	//my_pos *pos = new my_pos(pos_start);
	my_pos pozycja(pos_start);
	unsigned char *to_send = new unsigned char[sizeof(int)+1];
	to_send[0] = POSITION;
	unsigned char *help = to_send+1;
	memcpy(help, pozycja.my_pos_char, sizeof(int));
	write(sock_id, to_send, sizeof(int)+1);
	delete []to_send;
	help=NULL;
}
JobScheduler::JobScheduler(int port, int max_clients) {
	set = false;
	sck_addr.sin_family = AF_INET;
	sck_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	sck_addr.sin_port = htons(port);
	if((serv_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		throw ErrorClass(serv_socket, "Blad funkcji socker()");
	}
	int bind_er;
	if((bind_er = bind(serv_socket, (struct sockaddr*)&sck_addr,sizeof(struct sockaddr))) < 0) {
		throw ErrorClass(bind_er, "Blad funkcji bind()");
	}
	int listen_er;
	if((listen_er = listen(serv_socket, max_clients)) < 0) {
		throw ErrorClass(listen_er, "Blad funkcji listen()");
	}	
	int epoll_handler;
	if((epoll_handler = epoll_create(max_clients)) < 0) {
		cout << "epoll error" << endl;
		throw ErrorClass(epoll_handler, "Blad funkcji epoll_create()");
	}
	ping_poll = new epoll_event[max_clients];
	set = true;
}
void JobScheduler::start() {
	if(!set) return;
	thread(&JobScheduler::download_and_execute, this).detach();
	//thread(&JobScheduler::guard_queue, this).detach();
	while(true) {
		try {
			int nowy;
			if((nowy = accept(serv_socket, NULL, 0)) < 0) {
				throw ErrorClass(nowy, "Problem z nowym kliente");
			}
			else {
				clients_list.lock();
				clients.push_back(nowy);
				clients_list.unlock();
				/*epoll_event event;
				event.data.fd = nowy;
				event.events = EPOLLERR | EPOLLHUP;
				epoll_ctl(epoll_handler, EPOLL_CTL_ADD, nowy, &event); */
				thread(&JobScheduler::clientHandler, this, nowy, clients.size()).detach();
			}
		}
		catch(ErrorClass &error) {
			errorHandler(error);
		}
	}
}
void JobScheduler::errorHandler(const ErrorClass &error) {}
#endif
