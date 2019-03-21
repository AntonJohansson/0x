#include <iostream>
#include <string>
#include <math.h>
#include <algorithm>
#include <utility>
#include <SFML/Graphics.hpp>
#include <array>
#include <assert.h>
#include <thread>
#include <future>
#include <chrono>
#include <vector>
#include <mutex>

#include "hex.hpp"
#include "tcp/socket.hpp"
#include "tcp/socket_connection.hpp"
#include "io/poll_set.hpp"
#include "network/binary_encoding.hpp"

constexpr int32_t SCREEN_WIDTH  = 800;
constexpr int32_t SCREEN_HEIGHT = 600;

std::thread server_thread;

std::vector<std::string> sessions;
sf::Text sessions_text;
sf::Text stats_text;

bool connected_to_server = false;

HexagonalMap temp_map;
HexagonalMap map;

int current_turn = 0;
std::vector<int> player_scores;

std::array<sf::Vertex,8> vertices;
void draw_hexagon(sf::RenderWindow& window, float size, float x, float y, sf::Color color){
	vertices[0] = sf::Vertex({x,y}, color);

	for(int i = 0; i < 7; i++){
		vertices[i+1] = sf::Vertex({x+size*cos(axial::pi_over_six+2*axial::pi_over_six*i), y+size*sin(axial::pi_over_six+2*axial::pi_over_six*i)}, color);
	}

	window.draw(&vertices[0], 8, sf::TriangleFan);
}






///////////// COLORS
struct RGB{float r, g, b;};
struct HSL{float h, s, l;};

RGB hsl_to_rgb(HSL hsl){
	static auto f = [](HSL hsl, int n) -> float{
		auto k = fmod((n + hsl.h/30), 12);
		auto a = hsl.s*fmin(hsl.l, 1 - hsl.l);
		return hsl.l - a*fmax(fmin(fmin(k-3, 9-k),1),-1);
	};

	return {f(hsl, 0), f(hsl, 8), f(hsl, 4)};
}











































void input_loop(PollSet& set){
	while(connected_to_server){
		set.poll();
	}
}

void receive_data(int s){
	Socket socket(s, "0.0.0.0");
	printf("received data!\n");

	// Receive all data
	std::string total_data;
	std::string incoming_data;
	while(true){
		incoming_data = socket.recv();
		if(incoming_data.empty()) break;

		total_data += incoming_data;
	}


	BinaryData data(total_data.begin(), total_data.end());
	printf("received data of size %lu\n", data.size());

	// handle packet
	while(!data.empty()){
		uint32_t total_data_size = data.size();
		uint32_t packet_size;
		decode::integer(data, packet_size);
		printf("handling packet of size: %u\n", packet_size);

		assert(packet_size <= data.size());

		uint8_t packet_type = 0;
		decode::integer(data, packet_type);

		if(packet_type == 1){ // LIST
			std::string session_name, text_string;
			sessions.clear();
			while(data.size() > total_data_size - packet_size){
				session_name = decode::string(data);
				sessions.push_back(session_name);
				text_string += std::to_string(sessions.size()) + "\t" + session_name + "\n";
			}
			sessions_text.setString(text_string);
		}else if(packet_type == 2){ // MAP
			int radius, players, current_turn;
			decode::multiple_integers(data, radius, players, current_turn);
			printf("radius: %i, players: %i, current_turn: %i\n", radius, players);

			std::string text = "Current turn: " + std::to_string(current_turn) + "\n";

			player_scores.clear();
			for(int i = 0; i < players; i++){
				int player, score;
				decode::multiple_integers(data, player, score);
				text += std::to_string(player) + "\t" + std::to_string(score) + "\n";
			}

			stats_text.setString(text);

			temp_map.storage = nullptr;
			temp_map.players = players;
			temp_map.generate_storage(radius);

			while(data.size() > total_data_size - packet_size){
				int q;
				int r;
				int player_id;
				int resources;
				decode::multiple_integers(data, q, r, player_id, resources);
				printf("%i, %i, %i, %i\n", q, r, player_id, resources);
				auto& h = temp_map.at(q, r);
				h.q = q;
				h.r = r;
				h.player_id = player_id;
				h.resources = resources;
			}

			delete[] map.storage;
			map = temp_map;
		}
	}
}

void reconnect(Socket& socket, PollSet& set){
	while(!connected_to_server){ 
		if(socket = client_bind("127.0.0.1", "1111"); socket.handle != Socket::INVALID){
			socket.set_nonblocking();
			//socket.send_all("coj observer hej 1");
			connected_to_server = true;
			printf("Connected to server!\n");

			set.add(socket.handle, receive_data);

			// handle server disconnect
			set.on_disconnect(socket.handle, [&](int){
					connected_to_server = false;
					printf("Server disconnected!\n");
					});

			input_loop(set);
		}else{
			printf("Reconnecting!\n");
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}













































bool toggle_hex_positions = true;

int main(){
	PollSet set;
	//std::thread input_thread;
	Socket socket;
	server_thread = std::thread(reconnect, std::ref(socket), std::ref(set));

	sf::ContextSettings settings;
	settings.antialiasingLevel = 8;
	sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "0x", sf::Style::Close, settings);


	const std::string font_path = "../fonts/Roboto-Regular.ttf";
	sf::Font font;
	if(!font.loadFromFile(font_path)){
		printf("Unable to load font %s (file not found)!\n", font_path.c_str());
		return 1;
	}

	sf::Text text;
	text.setFont(font);
	text.setCharacterSize(12);
	text.setFillColor(sf::Color(244,240,219));

	sessions_text.setFont(font);
	sessions_text.setCharacterSize(30);
	sessions_text.setFillColor(sf::Color::Black);
	sessions_text.setPosition({50, 50});

	stats_text.setFont(font);
	stats_text.setCharacterSize(20);
	stats_text.setFillColor(sf::Color::Black);
	stats_text.setPosition({800-300, 50});

	while (window.isOpen()){
		sf::Event event;
		while(window.pollEvent(event)){
			if(event.type == sf::Event::Closed)
				window.close();
			if(event.type == sf::Event::KeyPressed){
				switch(event.key.code){
					case sf::Keyboard::Q:
						window.close();
						break;
					case sf::Keyboard::A:
						toggle_hex_positions = !toggle_hex_positions;
						break;
					case sf::Keyboard::R:
						socket.send_all("ls");
						break;
					case sf::Keyboard::Num1: if(sessions.size() > 0){socket.send_all("coj o " + sessions[0] + " 1");} break;
					case sf::Keyboard::Num2: if(sessions.size() > 1){socket.send_all("coj o " + sessions[1] + " 1");} break;
					case sf::Keyboard::Num3: if(sessions.size() > 2){socket.send_all("coj o " + sessions[2] + " 1");} break;
					case sf::Keyboard::Num4: if(sessions.size() > 3){socket.send_all("coj o " + sessions[3] + " 1");} break;
					case sf::Keyboard::Num5: if(sessions.size() > 4){socket.send_all("coj o " + sessions[4] + " 1");} break;
				}
			}
		}

		window.clear(sf::Color(244,240,219));

		if(map.has_storage()){
			// Using a capturing lambda here is slow (due to virtual function call?),
			// especially since it's being called in a tight loop
			map.for_each([&](int q, int r, HexCell& cell){
					auto [x, y] = axial::to_screen(map, q, r);
					x *= 1.05f;
					y *= 1.05f;
					x += SCREEN_WIDTH/2.0f;
					y += SCREEN_HEIGHT/2.0f;
					y = SCREEN_HEIGHT - y;

					if(cell.player_id >= 0){
						auto [r,g,b] = hsl_to_rgb({cell.player_id*360.0f/map.players, 0.5f, 0.2f + fmin((float)cell.resources, 0.6f*300.0f)/300.0f});
						auto color = sf::Color(255*r, 255*g, 255*b);
						//auto color = sf::Color(
						//	0.5f*(cell.player_id+1)*255/(map.players),
						//	0.5f*(cell.player_id+2)*255/(map.players),
						//	0.5f*(cell.player_id+3)*255/(map.players));
					draw_hexagon(window, map.hex_size, x, y, color);
					} else{
					draw_hexagon(window, map.hex_size, x, y, sf::Color(47,47,47));
					}

					// Debug draw text
					if(toggle_hex_positions){
						text.setString(std::to_string(q) + ", " + std::to_string(r));
						text.setPosition(x, y);

						auto text_rect = text.getLocalBounds();
						text.setOrigin(text_rect.left + text_rect.width/2.0f,
								text_rect.top  + text_rect.height/2.0f);
						window.draw(text);
					}else{
						text.setString(std::to_string(cell.resources));
						text.setPosition(x, y);

						auto text_rect = text.getLocalBounds();
						text.setOrigin(text_rect.left + text_rect.width/2.0f,
								text_rect.top  + text_rect.height/2.0f);
						window.draw(text);
					}
			});
		}

		window.draw(sessions_text);
		window.draw(stats_text);

		window.display();
	}




	window.close();

	return 0;
}
