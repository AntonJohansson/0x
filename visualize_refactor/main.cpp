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
#include <array>
#include <mutex>
#include <algorithm>

#include "hex.hpp"
#include "tcp/socket.hpp"
#include "tcp/socket_connection.hpp"
#include "io/poll_set.hpp"
#include "network/binary_encoding.hpp"
#include "../0x/crc/crc32.hpp"






///////////// COLORS 
struct RGB{float r, g, b;}; struct HSL{float h, s, l;};

RGB hsl_to_rgb(HSL hsl){
	static auto f = [](HSL hsl, int n) -> float{
		auto k = fmod((n + hsl.h/30), 12);
		auto a = hsl.s*fmin(hsl.l, 1 - hsl.l);
		return hsl.l - a*fmax(fmin(fmin(k-3, 9-k),1),-1);
	};

	return {f(hsl, 0), f(hsl, 8), f(hsl, 4)};
}

































constexpr int32_t SCREEN_WIDTH  = 800;
constexpr int32_t SCREEN_HEIGHT = 600;

uint64_t lobby_id = 0;

std::thread server_thread;
std::mutex mutex;

std::vector<std::string> sessions;
sf::Text sessions_text;
sf::Text stats_text;

sf::Text text;
bool toggle_hex_positions = true;

struct Cell;

struct Cell{
	int32_t q = 0, r = 0;
	uint32_t resources = 0;
	int32_t player_id = -1;
};

struct PlayerScores{
	uint32_t id;
	uint32_t resources;
};

uint32_t current_turn = 0;
uint32_t map_radius = 0;
uint32_t player_count = 0;
std::vector<Cell> map;
std::vector<PlayerScores> player_scores;

bool connected_to_server = false;

std::array<sf::Vertex,8> vertices;
void draw_hexagon(sf::RenderWindow& window, float size, float x, float y, sf::Color color){
	vertices[0] = sf::Vertex({x,y}, color);

	for(int i = 0; i < 7; i++){
		vertices[i+1] = sf::Vertex({x+size*cos(axial::pi_over_six+2*axial::pi_over_six*i), y+size*sin(axial::pi_over_six+2*axial::pi_over_six*i)}, color);
	}

	window.draw(&vertices[0], 8, sf::TriangleFan);
}

void draw_cell(sf::RenderWindow& window, int q, int r, int  resources, int player_id, sf::Color color, sf::Color text_color){
	text.setFillColor(text_color);

	auto [x, y] = axial::to_screen(q,r);
	x *= 1.05f;
	y *= 1.05f;
	x += SCREEN_WIDTH/2.0f;
	y += SCREEN_HEIGHT/2.0f;
	y = SCREEN_HEIGHT - y;

	draw_hexagon(window, hex_size, x, y, color);

	// Debug draw text
	if(toggle_hex_positions){
		text.setString(std::to_string(q) + ", " + std::to_string(r) + "\n  " + std::to_string(player_id));
		text.setPosition(x, y);

		auto text_rect = text.getLocalBounds();
		text.setOrigin(text_rect.left + text_rect.width/2.0f,
				text_rect.top  + text_rect.height/2.0f);
		window.draw(text);
	}else{
		text.setString(std::to_string(resources));
		text.setPosition(x, y);

		auto text_rect = text.getLocalBounds();
		text.setOrigin(text_rect.left + text_rect.width/2.0f,
				text_rect.top  + text_rect.height/2.0f);
		window.draw(text);
	}

}








































void input_loop(PollSet& set){
	while(connected_to_server){
		set.poll();
	}
}

void receive_data(int s){
	Socket socket(s, "0.0.0.0");

	// Receive all data
	std::string total_data;
	std::string incoming_data;
	while(true){
		incoming_data = socket.recv();
		if(incoming_data.empty()) break;

		total_data += incoming_data;
	}


	BinaryData data(total_data.begin(), total_data.end());
	//printf("received data of size %lu\n", data.size());

	// handle packet
	while(!data.empty()){
		uint32_t total_data_size = data.size();
		uint32_t packet_size;
		
		// DECODE PACKET ID
		decode::integer(data, packet_size);

		assert(packet_size <= data.size());

		// DECODE PACKET TYPE
		uint8_t packet_type = 0;
		decode::integer(data, packet_type);

		// DECODE CRC
		uint32_t crc = 0;
		decode::integer(data, crc);

		printf("total data size received: %u\nheader:\n\tpacket_size: %u\n\tpacket_id: %u\n", total_data_size, packet_size, packet_type);

		uint32_t payload_crc = buffer_crc32(data.data(), packet_size);
		if(crc != payload_crc){
			printf("CRC mismatch (%x != %0x)!\n", crc, payload_crc);
		}

		uint32_t data_left_size = data.size() - packet_size;
		if(packet_type == 1){ // LIST
			std::string session_name, text_string;
			sessions.clear();
			while(data.size() > data_left_size){
				session_name = decode::string(data);
				sessions.push_back(session_name);
				text_string += std::to_string(sessions.size()) + "\t" + session_name + "\n";
			}
			sessions_text.setString(text_string);
		}else if(packet_type == 6){
			std::string message = decode::string(data);
			printf("Error: %s\n", message.c_str());
		}else if(packet_type == 2){ // OBSERVER MAP
			//std::lock_guard<std::mutex> lock(mutex);

			printf("received observer map\n");
			map.clear();

			decode::multiple_integers(data, map_radius, player_count, current_turn);

			player_scores.clear();
			for(uint32_t i = 0; i < player_count; i++){
				uint32_t player_id, resources;
				decode::multiple_integers(data, player_id, resources);
				player_scores.push_back({player_id, resources});
			}

			std::sort(player_scores.begin(), player_scores.end(), [](const auto& a, const auto& b){
						return a.resources > b.resources;
					});

			while(data.size() > data_left_size){
				int32_t q;
				int32_t r;
				int32_t player_id;
				uint32_t resources;

				decode::multiple_integers(data, q, r, player_id, resources);
				map.push_back(Cell{q,r,resources,player_id});
			}
		}else if(packet_type == 7){
			decode::integer(data, lobby_id);
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

	text.setFont(font);
	text.setCharacterSize(12);
	text.setFillColor(sf::Color(244,240,219));

	sessions_text.setFont(font);
	sessions_text.setCharacterSize(30);
	sessions_text.setFillColor(sf::Color::Black);
	sessions_text.setPosition({50, 50});

	stats_text.setFont(font);
	stats_text.setCharacterSize(15);
	stats_text.setFillColor(sf::Color::Black);
	stats_text.setPosition({800-300, 50});

	
	while (window.isOpen()){
		//std::lock_guard<std::mutex> lock(mutex);
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

		if(!map.empty()){
			// Using a capturing lambda here is slow (due to virtual function call?),
			// especially since it's being called in a tight loop
			for(auto& cell : map){
				auto& [q,r,resources,player_id] = cell;
				static sf::Color dark_color{47,47,47};
				static sf::Color background_color{244,240,219};

				sf::Color color = dark_color;
				sf::Color	text = background_color;
				if(player_id > 0){
					float l = 0.2f + fmin((float)resources, 0.6f*300.0f)/300.0f;
					if(l > 0.5f){
						text = dark_color;
					}

					auto [r,g,b] = hsl_to_rgb({player_id*360.0f/player_count, 0.5f, l});
					color = sf::Color(255*r, 255*g, 255*b);
				}

				draw_cell(window, q, r, resources, player_id, color, text);
			}
		}

		if(!player_scores.empty()){
			sf::RectangleShape rect({10,10});

			stats_text.setPosition({800-100, 50});
			stats_text.setString("Scores");
			window.draw(stats_text);

			auto pos = stats_text.getPosition();
			stats_text.setPosition(pos + sf::Vector2f{15,0});
			for(auto& [id, score] : player_scores){
				auto pos = stats_text.getPosition();
				stats_text.setPosition(pos + sf::Vector2f{0,20});
				stats_text.setString(std::to_string(id) + ": " + std::to_string(score));

				auto [r,g,b] = hsl_to_rgb({id*360.0f/player_count, 0.5f, 0.5f});
				auto color = sf::Color(255*r, 255*g, 255*b);
				rect.setFillColor(color);
				rect.setPosition(pos + sf::Vector2f{-15,20 + 10.0f/2}); 
				window.draw(stats_text);
				window.draw(rect);
			}
		}


		window.draw(sessions_text);

		window.display();
	}




	window.close();

	return 0;
}
