#include "server.hpp"
#include "game.hpp"

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
