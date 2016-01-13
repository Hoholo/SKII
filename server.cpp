#include "server.h"
#include <ctime>
#include <signal.h>
int main() {
	signal(SIGPIPE, SIG_IGN);
	srand(time(0));
	try {
		JobScheduler server(5534, 50);
		server.start();
	}
	catch(const ErrorClass &blad) {
		cout << blad.getErrorInt() << endl;
		cout << blad.getErrorString() << endl;
	}
}
