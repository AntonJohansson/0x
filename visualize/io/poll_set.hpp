#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <cerrno>
#include <cstring>
#if __has_include(<sys/epoll.h>)
	#define __HAS_EPOLL__ 1
	#include <sys/epoll.h>
#elif __has_include(<sys/event.h>)
	#include <sys/event.h> // for kqueue
#endif

class PollSet {
public:
	using Func = std::function<void(int)>;
	PollSet(){
		//_watched_events.reserve(init_size);
		//_triggered_events.reserve(init_size);

#if __HAS_EPOLL__
		_fd = epoll_create1(0);
		if(_fd == -1){
			printf("Error in epoll_create1(): %s\n", std::strerror(errno));
		}
#else
		_fd = kqueue();
		if(_fd == -1){
			printf("Error in kqueue()(): %s\n", std::strerror(errno));
		}
#endif
	}

	void add(int fd, Func f){
		_size++;

#if __HAS_EPOLL__
		static struct epoll_event ev;
		ev.data.fd = fd;
		ev.events = EPOLLIN | EPOLLRDHUP;
		if(epoll_ctl(_fd, EPOLL_CTL_ADD, fd, &ev)){
			printf("Error in epoll_ctl(): %s\n", std::strerror(errno));
		}
		_callback_map[fd] = f;
#else
		//_changes.emplace_back();
		//EV_SET(&_changes.back(), fd, EVFILT_READ, EV_ADD, 0, 0, 0);
		
		//struct kevent e;
		EV_SET(&_changes[0], fd, EVFILT_READ, EV_ADD, 0, 0, 0);
		//_changes.push_back(e);

		_callback_map[fd] = f;

		//if(_events.capacity() < _size){
		//	static int expand_amount = 10;
		//	console->warn("expanding poll set!");
		//	_events.reserve(_size + expand_amount);
		//}
		//kevent(_fd, &_changes[0], 1, nullptr, 0, nullptr);
		_n_changes++;
		apply_changes();
#endif
	}

	void on_disconnect(int fd, Func f){
		_disconnect_map[fd] = f;
		//printf("size: %i\n", _disconnect_map.size());
	}

	void remove(int fd){
		_size--;
#if __HAS_EPOLL__
		static struct epoll_event ev;
		ev.data.fd = fd;
		_callback_map.erase(fd);
		_disconnect_map.erase(fd);

		if(epoll_ctl(_fd, EPOLL_CTL_DEL, fd, &ev)){
			printf("Error in epoll_ctl(): %s\n", std::strerror(errno));
		}
#else
		//_changes.emplace_back();
		//EV_SET(&_changes.back(), fd, EVFILT_READ, EV_DELETE, 0, 0, 0);
		EV_SET(&_changes[0], fd, EVFILT_READ, EV_DELETE, 0, 0, 0);
		_callback_map.erase(fd);
		_disconnect_map.erase(fd);
		_n_changes++;
		apply_changes();
#endif
	} 

	void apply_changes(){
#ifdef __APPLE__
		kevent(_fd, &_changes[0], _n_changes, nullptr, 0, nullptr);
		_n_changes = 0;
#endif
	}

	void poll(){
		static int nev{0};
#if __HAS_EPOLL__
		nev = epoll_wait(_fd, _events, 10, -1);
		if(nev < 0){
			printf("Error in epoll_wait(): %s\n", std::strerror(errno));
		}else if(nev > 0){
			for(int i = 0; i < nev; i++){
				auto& event = _events[i];
				if (event.events & EPOLLRDHUP){
					auto callback = _disconnect_map[event.data.fd];
					remove(event.data.fd);
					callback(event.data.fd);
				}else if(event.events & EPOLLIN){
					_callback_map[event.data.fd](event.data.fd);
				}
			}
		}
#else
		//console->info("{}, {}", _changes.size(), _events.size());
		nev = ::kevent(_fd,
									 nullptr, 0,
									 &_events[0], 10,
									 //_changes.data(), _changes.size(),
									 //_events.data(), _events.size(),
									 nullptr);
		//_changes.clear(); // changes has been applied by kevent()
		_n_changes = 0;

		if(nev < 0){
			printf("Error in kevent(): %s\n", std::strerror(errno));
		}else if(nev > 0){
			//console->info("event occured!");
			for(int i = 0; i < nev; i++){
				auto& event = _events[i];
				if(event.flags & EV_EOF){
					_disconnect_map[event.ident](event.ident);
					remove(event.ident);
					apply_changes();
				}else if(event.flags & EV_ERROR){
					printf("Error occured in PollSet, not handled!\n");
				}else if(event.flags & EVFILT_READ){
					//console->info("rad");
					//print_event_flags(event);
					_callback_map[event.ident](event.ident);
				}
			}
		}
#endif
	}

	inline int size(){
		return _size;
	}
private:
	std::unordered_map<int, Func> _disconnect_map;
	std::unordered_map<int, Func> _callback_map;
#if __HAS_EPOLL__ 
	struct epoll_event _events[10];
#else
	struct kevent _changes[10];
	struct kevent _events[10];
#endif
	//std::vector<struct kevent> 					_changes;
	//std::vector<struct kevent> 					_events;
	int _n_changes;
	int _size;
	int _fd;
};
