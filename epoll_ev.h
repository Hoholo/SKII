#ifndef _EPOLL_EV_H_
#define _EPOL_EV_H_
class epoll_ev {
	epoll_ev():available(true) {}
	bool availabe;
	epoll_event event;
}
#endif
