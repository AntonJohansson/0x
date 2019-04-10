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

#include <game/hex.hpp>
#include <network/socket.hpp>
#include <network/socket_connection.hpp>
#include <io/poll_set.hpp>
#include <serialize/binary_encoding.hpp>
#include <crc/crc32.hpp>




constexpr float pi_over_six = M_PI/6.0f;
constexpr float hex_size = 10.0f;
// 0.866025403784438646763723170752936183471402626905190314
constexpr float hex_width = 2*0.866025403784438646763723170752936183471402626905190314f*hex_size;

constexpr int32_t SCREEN_WIDTH  = 800;
constexpr int32_t SCREEN_HEIGHT = 600;



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




static hex::AxialVec hex_round(int q, int r){
	auto cube = hex::axial_to_cube({q, r});

	auto rx = std::round(cube.x);
	auto ry = std::round(cube.y);
	auto rz = std::round(cube.z);

	auto x_diff = std::abs(rx - cube.x);
	auto y_diff = std::abs(ry - cube.y);
	auto z_diff = std::abs(rz - cube.z);

	if(x_diff > y_diff && x_diff > z_diff){
		rx = -ry - rz;
	}else if(y_diff > z_diff){
		ry = -rx - rz;
	}else{
		rz = -rx - ry;
	}

	return hex::cube_to_axial({x_diff, y_diff, z_diff});
}

static hex::AxialVec from_screen(int x, int y){
	y = SCREEN_HEIGHT - y;
	y -= SCREEN_HEIGHT/2.0f;
	x -= SCREEN_WIDTH/2.0f;
	x /= 1.05f;
	y /= 1.05f;

	int q = (sqrt(3)/3.0*x - 	1.0/3*y)/hex_size;
	int r = (									2.0/3*y)/hex_size;
	return hex_round(q, r);
}




static std::pair<float, float> to_screen(int q, int r){
	//const float y1 = cos(pi_over_six)*cos(pi_over_six*4);
	//const float y2 = cos(pi_over_six)*sin(pi_over_six*4);
	// cos(pi/6)  = 0.866025403784438646763723170752936183471402626905190314
	// cos(4pi/6) = -0.5
	// sin(4pi/6) = 0.866025403784438646763723170752936183471402626905190314
	constexpr float x1 = 1.0f;
	constexpr float x2 = 0.0f;
	constexpr float y1 = 0.8660254f*(-0.5f);
	constexpr float y2 = 0.8660254f*0.8660254;
	return {r*2*hex_size*y1 + q*hex_width*x1, r*2*hex_size*y2};
}



























uint64_t lobby_id = 0;

std::thread server_thread;
bool my_turn = false;

std::vector<std::string> sessions;
sf::Text sessions_text;
sf::Text stats_text;

sf::Text text;
bool doing_transfer = false;
bool m1_selected = false;
int transfer_amount = 0;
sf::Vector2i m0, m1;
bool toggle_hex_positions = true;

struct Cell;

struct Cell{
	int q = 0, r = 0;
	int resources = 0;
	int player_id = -1;

	struct Neighbour{int q = 0, r = 0, resources = 0, player_id = -1;};
	std::array<Neighbour, 6> neighbours;
};
std::vector<Cell> map;

bool connected_to_server = false;

std::array<sf::Vertex,8> vertices;
void draw_hexagon(sf::RenderWindow& window, float size, float x, float y, sf::Color color){
	vertices[0] = sf::Vertex({x,y}, color);

	for(int i = 0; i < 7; i++){
		vertices[i+1] = sf::Vertex({x+size*cos(pi_over_six+2*pi_over_six*i), y+size*sin(pi_over_six+2*pi_over_six*i)}, color);
	}

	window.draw(&vertices[0], 8, sf::TriangleFan);
}

void draw_cell(sf::RenderWindow& window, int q, int r, int  resources, int player_id, sf::Color color, sf::Color text_color){
	text.setFillColor(text_color);

	auto [x, y] = to_screen(q,r);
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
		if(doing_transfer && q == m1.x && r == m1.y){
			if(player_id == 2){
				text.setString(std::to_string(transfer_amount - resources));
			}else{
				text.setString(std::to_string(resources + transfer_amount));
			}
		}else if(doing_transfer && q == m0.x && r == m0.y){
			text.setString(std::to_string(resources - transfer_amount));
		}else{
			text.setString(std::to_string(resources));
		}
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
	BinaryData data;

	constexpr size_t size = 100;
	uint8_t buffer[size];
	size_t bytes;
	// Receive all data
	std::string total_data;
	std::string incoming_data;

	while(bytes = tcp_socket::recv(s, buffer, size)){
		append(data, buffer, bytes);
	}

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
		}else if(packet_type == 3){ // MAP
			printf("received player map\n");
			my_turn = true;
			sessions_text.setString(std::to_string(my_turn));
			map.clear();

			while(data.size() > data_left_size){
				int32_t q;
				int32_t r;
				int32_t player_id;
				uint32_t resources;

				decode::multiple_integers(data, q, r, player_id, resources);
				map.push_back(Cell{q,r,resources,player_id, {}});
				
				for(auto& n : map.back().neighbours){
					decode::multiple_integers(data, q, r, player_id, resources);
					// TODO: this is really bad
					n = {q, r, resources, player_id};
					//if(std::find_if(map.begin(),map.end(),[&n](auto& c){return c.q == n.q && c.r == n.r;}) == map.end()){
					//	map.push_back(Cell{});
					//}
				}
			}
		}else if(packet_type == 7){
			decode::integer(data, lobby_id);
		}
	}
}

void reconnect(Socket& socket, PollSet& set){
	while(!connected_to_server){ 
		if(socket = client_bind("127.0.0.1", "1111"); socket != tcp_socket::INVALID){
			tcp_socket::set_nonblocking(socket);
tcp:
			//socket.send_all("coj observer hej 1");
			connected_to_server = true;
			printf("Connected to server!\n");

			set.add(socket, receive_data);

			// handle server disconnect
			set.on_disconnect(socket, [&](int){
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
						tcp_socket::send(socket, "ls");
						break;
					case sf::Keyboard::Z:
						transfer_amount++;
						break;
					case sf::Keyboard::X:
						transfer_amount--;
						break;
					case sf::Keyboard::Num1: if(sessions.size() > 0){tcp_socket::send_all(socket, "coj p " + sessions[1] + " 1");} break;
					case sf::Keyboard::Num2: if(sessions.size() > 1){tcp_socket::send_all(socket, "coj p " + sessions[2] + " 1");} break;
					case sf::Keyboard::Num3: if(sessions.size() > 2){tcp_socket::send_all(socket, "coj p " + sessions[3] + " 1");} break;
					case sf::Keyboard::Num4: if(sessions.size() > 3){tcp_socket::send_all(socket, "coj p " + sessions[4] + " 1");} break;
					case sf::Keyboard::Num5: if(sessions.size() > 4){tcp_socket::send_all(socket, "coj p " + sessions[5] + " 1");} break;
				}
			}else if(event.type == sf::Event::MouseButtonPressed){
				if(doing_transfer){
					BinaryData data;
					encode::u8(data, 3);
					encode::u64(data, lobby_id);
					encode::u32(data, transfer_amount);
					encode::u32(data, m0.x);
					encode::u32(data, m0.y);
					encode::u32(data, m1.x);
					encode::u32(data, m1.y);
					auto str = std::string(data.begin(), data.end());
					tcp_socket::send_all(socket, str);
					my_turn = false;
					sessions_text.setString(std::to_string(my_turn));
					doing_transfer = false;
				}else{
					doing_transfer = true;
				}
			}
		}

		if(doing_transfer){
			m1 = sf::Mouse::getPosition(window);
			auto [x, y] = from_screen(m1.x, m1.y);
			m1.x = x; m1.y = y;
		}else{
			m0 = sf::Mouse::getPosition(window);
			auto [x, y] = from_screen(m0.x, m0.y);
			m0.x = x; m0.y = y;
		}


		window.clear(sf::Color(244,240,219));

		if(!map.empty()){
			// Using a capturing lambda here is slow (due to virtual function call?),
			// especially since it's being called in a tight loop
			for(auto& cell : map){
				auto& [q,r,resources,player_id, neighbours] = cell;
				static sf::Color dark_color{47,47,47};
				static sf::Color background_color{244,240,219};

				sf::Color color = dark_color;
				sf::Color	text = background_color;
				if(player_id > 0){
					int players = 2;
					float l = 0.2f + fmin((float)resources, 0.6f*300.0f)/300.0f;
					if(l > 0.5f){
						text = dark_color;
					}

					auto [r,g,b] = hsl_to_rgb({player_id*360.0f/players, 0.5f, l});
					color = sf::Color(255*r, 255*g, 255*b);
				}

				if(q == m0.x && r == m0.y){
					color = sf::Color(123,47,47);
				}
				if(doing_transfer && q == m1.x && r == m1.y){
					color = sf::Color(47,123,47);
				}


				draw_cell(window, q, r, resources, player_id, color, text);

				//printf("-----\n");
				for(auto& n : neighbours){
					color = dark_color;
					text = background_color;

					if(n.player_id > 0){
						int players = 2;
						float l = 0.2f + fmin((float)n.resources, 0.6f*300.0f)/300.0f;
						if(l > 0.5f){
							text = dark_color;
						}

						auto [r,g,b] = hsl_to_rgb({n.player_id*360.0f/players, 0.5f, l});
						color = sf::Color(255*r, 255*g, 255*b);
					}
					if(n.q == m0.x && n.r == m0.y){
						color = sf::Color(123,47,47);
					}
					if(doing_transfer && n.q == m1.x && n.r == m1.y){
						color = sf::Color(47,123,47);
					}

					if((!doing_transfer && q == m0.x && r == m0.y) || (doing_transfer && q == m1.x && r == m1.y)){
						color = sf::Color(47,47,123);
						//printf("%i,%i\n", n.q , n.r);
					}

					draw_cell(window, n.q, n.r, n.resources, n.player_id, color, text);
				}
			}
		}

		window.draw(sessions_text);

		window.display();
	}




	window.close();

	return 0;
}
