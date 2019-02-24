#include "socket.hpp"
#include "server_connection.hpp"
#include "poll_set.hpp"

void recieve_data(int s){
	Socket socket(s, "0.0.0.0");

	std::string total_data;
	std::string incoming_data;
	while(true){
		incoming_data = socket.recv();
		if(incoming_data.empty()) break;

		total_data += incoming_data;
	}

	printf("Received data: %s\n", total_data.c_str());
	socket.send_all(total_data);
}

void on_disconnect(int s){
	Socket socket(s, "0.0.0.0");

	printf("Connection to %i : %s closed!\n", socket.handle, socket.ip_address.c_str());
	socket.close();
}

int main(){
	PollSet set;

	auto server = server_bind("1111");
	server.set_nonblocking();
	
	server_listen(server, 10);

	set.add(server.handle, [&](int){
				printf("Accepting new connection!\n");

				auto new_socket = server_accept(server);
				new_socket.set_nonblocking();

				printf("New connection %i : %s\n", new_socket.handle, new_socket.ip_address.c_str());

				set.add(new_socket.handle, recieve_data);
				set.on_disconnect(new_socket.handle, on_disconnect);
			});

	while(true){
		set.poll();
	}

}
