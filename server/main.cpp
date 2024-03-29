#include "server.hpp"
#include "game_refactor.hpp"
#include <chrono>
#include <thread>

int main(){
	//set.add(0, [](int){
	//			std::string line;
	//			std::getline(std::cin, line);
	//			if(line == "quit"){
	//				should_run = false;
	//			}
	//		});

	server::start();
	//server::poll_fds();

	while(server::is_running()){
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		//game::poll_sessions();
		game::poll();

	//	// at start of new game send inital board to observers

	//	// each round:
	//	// 	receive transactions
	//	// 	ready to start?
	//	// 	filter transactions
	//	// 	send transactions to observers so that they can be animated
	//	// 	apply transactions to map
	}

	server::close();
}
