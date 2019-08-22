#pragma once

namespace server{

extern void start();
extern void close();
extern bool is_running();

extern void poll_fds();

extern void receive_client_data(int socket);
extern void accept_client_connections(int socket);
extern void on_client_disconnect(int socket);

}

