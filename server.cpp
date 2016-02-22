#include "server.h"
#include <ctime>
#include <signal.h>
JobScheduler *x = NULL;
int main() {
	signal(SIGPIPE, SIG_IGN);
	srand(time(0));
	try {
		fstream plik("error.txt", ios::out);
		if(!plik.is_open()) throw ErrorClass(-1, "Nie udalo sie utworzyc error.txt");
		plik.close();
		plik.open("result.txt", ios::out);
		if(!plik.is_open()) throw ErrorClass(-1, "Nie udalo sie utworzyc result.txt");
		plik.close();
		plik.open("to_execute.out", ios::out);
		if(!plik.is_open()) throw ErrorClass(-1, "Nie udalo sie utworzyc to_execute.out");
		plik.close();
		JobScheduler *server = new JobScheduler(5534, 50);
		x = server;
		server->start();
	}
	catch(const ErrorClass &blad) {
		cout << blad.getErrorInt() << endl;
		cout << blad.getErrorString() << endl;
	}
}
