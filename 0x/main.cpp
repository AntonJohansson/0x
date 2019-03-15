#include "server.hpp"
#include "game.hpp"
#include <chrono>
#include <thread>

// TODO:
//	- collect game statistics
//	- restructure/rewrite newtork packet code
//	- handle game rules correctly:
//		- use path finding to check if moves are valid (within a closed region)
//			- implement path finding
//		- time limit for turn

int main(){
	//set.add(0, [](int){
	//			std::string line;
	//			std::getline(std::cin, line);
	//			if(line == "quit"){
	//				should_run = false;
	//			}
	//		});

	server::start();

	while(server::is_running()){
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		game::poll_sessions();

		// at start of new game send inital board to observers

		// each round:
		// 	receive transactions
		// 	ready to start?
		// 	filter transactions
		// 	send transactions to observers so that they can be animated
		// 	apply transactions to map
	}

	server::close();
}
