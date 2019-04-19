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
#include <game/packet.hpp>

#include "ui.hpp"
#include "states.hpp"

constexpr float pi_over_six = M_PI/6.0f;
constexpr float hex_size = 10.0f;
// 0.866025403784438646763723170752936183471402626905190314
constexpr float hex_width = 2*0.866025403784438646763723170752936183471402626905190314f*hex_size;

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

sf::Font font;
static sf::Color dark_color{47,47,47};
static sf::Color background_color{244,240,219};

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
uint32_t max_players = 0;
std::vector<Cell> map;
std::vector<Cell> tmp_map;
std::vector<PlayerScores> player_scores;

bool connected_to_server = false;

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

	//// Debug draw text
	//if(toggle_hex_positions){
	//	text.setString(std::to_string(q) + ", " + std::to_string(r) + "\n  " + std::to_string(player_id));
	//	text.setPosition(x, y);

	//	auto text_rect = text.getLocalBounds();
	//	text.setOrigin(text_rect.left + text_rect.width/2.0f,
	//			text_rect.top  + text_rect.height/2.0f);
	//	window.draw(text);
	//}else{
	//	text.setString(std::to_string(resources));
	//	text.setPosition(x, y);

	//	auto text_rect = text.getLocalBounds();
	//	text.setOrigin(text_rect.left + text_rect.width/2.0f,
	//			text_rect.top  + text_rect.height/2.0f);
	//	window.draw(text);
	//}

}






































static void send_command(int s, const std::string& command){
	BinaryData data;
	encode::u32(data, command.size());
	encode::u8(data, 4);
	append(data, command);
	tcp_socket::send_all(s, data.data(), data.size());
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

		auto header = decode_packet_header(data, true);
		if(!header.valid)
			continue;
		
		//// DECODE PACKET ID
		//decode::integer(data, packet_size);

		//assert(packet_size <= data.size());

		// DECODE PACKET TYPE
		//uint8_t packet_type = 0;
		//decode::integer(data, packet_type);

		//// DECODE CRC
		//uint32_t crc = 0;
		//decode::integer(data, crc);

		//printf("total data size received: %u\nheader:\n\tpacket_size: %u\n\tpacket_id: %u\n", total_data_size, packet_size, packet_type);

		//uint32_t payload_crc = buffer_crc32(data.data(), packet_size);
		//if(crc != payload_crc){
		//	printf("CRC mismatch (%x != %0x)!\n", crc, payload_crc);
		//}
		uint32_t data_left_size = data.size() - header.payload_size;
		switch(header.packet_type){
			case PacketType::SESSION_LIST:{
					std::string session_name, text_string;
					sessions.clear();
					while(data.size() > data_left_size){
						session_name = decode::string(data);
						sessions.push_back(session_name);
						text_string += std::to_string(sessions.size()) + "\t" + session_name + "\n";
					}
					sessions_text.setString(text_string);
					break;
				}
			case PacketType::ERROR_MESSAGE:{
					std::string message = decode::string(data);
					printf("Error: %s\n", message.c_str());
					break;
				}
			case PacketType::OBSERVER_MAP:{
					printf("received observer map\n");
					tmp_map.clear();

					decode::multiple_integers(data, map_radius, player_count, max_players, current_turn);

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
						tmp_map.push_back(Cell{q,r,resources,player_id});
					}

					map = tmp_map;

					break;
				}
			case PacketType::CONNECTED_TO_LOBBY:{
					decode::integer(data, lobby_id);
					break;
				}
		}

		//if(packet_type == 1){ // LIST
		//	std::string session_name, text_string;
		//	sessions.clear();
		//	while(data.size() > data_left_size){
		//		session_name = decode::string(data);
		//		sessions.push_back(session_name);
		//		text_string += std::to_string(sessions.size()) + "\t" + session_name + "\n";
		//	}
		//	sessions_text.setString(text_string);
		//}else if(packet_type == 6){
		//std::string message = decode::string(data);
		//printf("Error: %s\n", message.c_str());
		//}else if(packet_type == 2){ // OBSERVER MAP
		//std::lock_guard<std::mutex> lock(mutex);

		//printf("received observer map\n");
		//tmp_map.clear();

		//decode::multiple_integers(data, map_radius, player_count, max_players, current_turn);

		//player_scores.clear();
		//for(uint32_t i = 0; i < player_count; i++){
		//	uint32_t player_id, resources;
		//	decode::multiple_integers(data, player_id, resources);
		//	player_scores.push_back({player_id, resources});
		//}

		//std::sort(player_scores.begin(), player_scores.end(), [](const auto& a, const auto& b){
		//		return a.resources > b.resources;
		//		});

		//while(data.size() > data_left_size){
		//		int32_t q;
		//		int32_t r;
		//		int32_t player_id;
		//		uint32_t resources;

		//		decode::multiple_integers(data, q, r, player_id, resources);
		//		tmp_map.push_back(Cell{q,r,resources,player_id});
		//	}

		//	map = tmp_map;
		//}else if(packet_type == 7){
		//	decode::integer(data, lobby_id);
		//}
	}
}

void connect_to_server(Socket& socket, PollSet& set){
	for(int i = 0; i < 5; i++){
		if(socket = client_bind("127.0.0.1", "1111"); socket != tcp_socket::INVALID){
			tcp_socket::set_nonblocking(socket);

			connected_to_server = true;
			printf("Connected to server!\n");

			set.add(socket, receive_data);

			// handle server disconnect
			set.on_disconnect(socket, [&](int){
					connected_to_server = false;
					printf("Server disconnected!\n");
					state::change("connect");
					});

			break;
		}else{
			printf("Reconnecting!\n");
		}
		
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	if(connected_to_server){
		state::change("main menu");
		input_loop(set);
	}else{
		state::change("connect");
	}
}













static PollSet set;
static Socket socket;










// STATES

namespace game{
static void enter(){
}

static void leave(){
}

static void update(void* data){
}
}

namespace lobby{
static void enter(){
}

static void leave(){
}

static void update(void* data){
}
}

namespace main_menu{
static ButtonMenu menu;

static void enter(){
	auto [r,g,b] = hsl_to_rgb({0.5f, 0.25f, 0.65f});
	menu.background_hover = {255*r,255*g,255*b};
	menu.background_passive = dark_color;
	menu.text_hover = dark_color;
	menu.text_passive = background_color;
	menu.font = font;
	add_button(menu, "aaaaa", [](){puts("1");});
	add_button(menu, "bbbbb", [](){puts("2");});
	add_button(menu, "ccccc", [](){puts("3");});
	center_menu(menu, 0, SCREEN_WIDTH, 0, SCREEN_HEIGHT);
}

static void event(void* data){
	sf::Event event = *reinterpret_cast<sf::Event*>(data);
	if(event.type == sf::Event::KeyPressed){
		switch(event.key.code){
			case sf::Keyboard::Down:
				move_down(menu);
				break;
			case sf::Keyboard::Up:
				move_up(menu);
				break;
			case sf::Keyboard::Enter:
				push_active_button(menu);
				break;
		}
	}
}

static void update(void* data){
	sf::RenderWindow& window = *reinterpret_cast<sf::RenderWindow*>(data);
	draw_button_menu(window, menu);
}

static void leave(){
}
}

namespace connect{
static std::string ip = "127.0.0.1";
static std::string port = "1111";

sf::Text ip_text, port_text, ip_entered_text, port_entered_text;
sf::Text* selected_text_field;
sf::RectangleShape ip_background, port_background;
sf::Text connecting_text;

sf::String entered_text;

static sf::Color selected_color{
	static_cast<uint8_t>(0.75*244),
	static_cast<uint8_t>(0.75*240),
	static_cast<uint8_t>(0.75*219)
};

enum State{IP,PORT,CONNECTING};
State state;

static std::chrono::high_resolution_clock::time_point start, end;

static void enter(){
	constexpr int font_size = 25;
	ip_text.setFont(font);
	ip_text.setString("ip:");
	ip_text.setCharacterSize(font_size);
	ip_text.setFillColor(dark_color);
	port_text.setFont(font);
	port_text.setString("port:");
	port_text.setCharacterSize(font_size);
	port_text.setFillColor(dark_color);
	ip_entered_text.setFont(font);
	ip_entered_text.setString(ip);
	ip_entered_text.setCharacterSize(0.75f*font_size);
	ip_entered_text.setFillColor(background_color);
	port_entered_text.setFont(font);
	port_entered_text.setString(port);
	port_entered_text.setCharacterSize(0.75f*font_size);
	port_entered_text.setFillColor(background_color);
	connecting_text.setFont(font);
	connecting_text.setString("Connecting    (1/5)");
	connecting_text.setCharacterSize(font_size);
	connecting_text.setFillColor(dark_color);

	auto ip_text_bounds = ip_text.getLocalBounds();
	ip_text.setOrigin(ip_text_bounds.left + ip_text_bounds.width, ip_text_bounds.top + ip_text_bounds.height/2);
	auto port_text_bounds = port_text.getLocalBounds();
	port_text.setOrigin(port_text_bounds.left + port_text_bounds.width, port_text_bounds.top + port_text_bounds.height/2);

	auto connecting_text_bounds = connecting_text.getLocalBounds();
	connecting_text.setOrigin(connecting_text_bounds.left + connecting_text_bounds.width/2, connecting_text_bounds.top + connecting_text_bounds.height/2);
	connecting_text.setPosition({0.5*SCREEN_WIDTH, 0.5*SCREEN_HEIGHT});


	constexpr int spacing = 5;
	ip_text.setPosition({0.5f*SCREEN_WIDTH - spacing, 0.25*SCREEN_HEIGHT});
	port_text.setPosition({0.5f*SCREEN_WIDTH - spacing, 0.25*SCREEN_HEIGHT + 0.05f*SCREEN_HEIGHT});

	ip_background.setFillColor(dark_color);
	ip_background.setSize({100, font_size});
	auto ip_background_bounds = ip_background.getLocalBounds();
	ip_background.setOrigin(ip_background_bounds.left, ip_background_bounds.top + ip_background_bounds.height/2);
	ip_background.setPosition({0.5f*SCREEN_WIDTH + spacing, 0.25*SCREEN_HEIGHT});


	port_background.setFillColor(dark_color);
	port_background.setSize({100, font_size});
	auto port_background_bounds = port_background.getLocalBounds();
	port_background.setOrigin(port_background_bounds.left, port_background_bounds.top + port_background_bounds.height/2);
	port_background.setPosition({0.5f*SCREEN_WIDTH + spacing, 0.25*SCREEN_HEIGHT + 0.05f*SCREEN_HEIGHT});

	auto ip_entered_text_bounds = ip_entered_text.getLocalBounds();
	ip_entered_text.setOrigin(ip_entered_text_bounds.left, ip_entered_text_bounds.top + ip_entered_text_bounds.height/2);
	ip_entered_text.setPosition({0.5f*SCREEN_WIDTH + 2*spacing, 0.25*SCREEN_HEIGHT});

	auto port_entered_text_bounds = port_entered_text.getLocalBounds();
	port_entered_text.setOrigin(port_entered_text_bounds.left, port_entered_text_bounds.top + port_entered_text_bounds.height/2);
	port_entered_text.setPosition({0.5f*SCREEN_WIDTH + 2*spacing, 0.25*SCREEN_HEIGHT + 0.05f*SCREEN_HEIGHT});

	selected_text_field = &ip_entered_text;
	selected_text_field->setFillColor(selected_color);
	state = IP;
}

static void event(void* data){
	sf::Event event = *reinterpret_cast<sf::Event*>(data);
	if(event.type == sf::Event::TextEntered && state != CONNECTING){
		if(event.text.unicode == 8){
			// backspace
			//current_entered_text.pop_back();
			if(entered_text.getSize() > 0){
				entered_text.erase(entered_text.getSize() - 1);
				selected_text_field->setString(entered_text);
			}
		}else if(event.text.unicode == 13){
			selected_text_field->setFillColor(background_color);
			entered_text.clear();
			if(state == IP){
				state = PORT;
				selected_text_field = &port_entered_text;
			}else if(state == PORT){
				state = CONNECTING;
				start = std::chrono::high_resolution_clock::now();

				// if we did not manage to connect and needs to try again
				// the server thread will need to be joined
				if(server_thread.joinable()){
					server_thread.join();
				}
				server_thread = std::thread(connect_to_server, std::ref(socket), std::ref(set));
			}
			selected_text_field->setFillColor(selected_color);
		}else if(event.text.unicode < 128){
			selected_text_field->setFillColor(background_color);
			entered_text.insert(entered_text.getSize(),event.text.unicode);
			selected_text_field->setString(entered_text);
		}
	}
}

static void update(void* data){
	sf::RenderWindow& window = *reinterpret_cast<sf::RenderWindow*>(data);

	if(state == CONNECTING){
		end = std::chrono::high_resolution_clock::now();

		static int dots = 0;
		static int max_dots = 3;
		static std::string dots_string = "    ";
		if(end - start >= std::chrono::seconds(1)){
			dots_string = "";
			for(int i = 0; i <= dots; i++){
				dots_string += ".";
			}
			for(int i = 0; i <= max_dots-dots; i++){
				dots_string += " ";
			}
			dots = (dots+1)%max_dots;
			start = end;
		}
		connecting_text.setString("Connecting" + dots_string + " (1/5)");
		window.draw(connecting_text);
	}

	window.draw(ip_text);
	window.draw(port_text);
	window.draw(ip_background);
	window.draw(port_background);
	window.draw(ip_entered_text);
	window.draw(port_entered_text);
}

static void leave(){
	//if(!connect_to_server){
	//	server_thread.join();
	//}
}
}



// main menu screen

int main(){
	state::add("connect", {
			connect::enter, 
			connect::event, 
			connect::update, 
			connect::leave
			});

	state::add("main menu", {
			main_menu::enter, 
			main_menu::event, 
			main_menu::update, 
			main_menu::leave
			});
	state::add("lobby", {
			lobby::enter,
			nullptr,
			lobby::update,
			lobby::leave
			});
	state::add("game", {
			game::enter,
			nullptr,
			game::update,
			game::leave
			});

	//server_thread = std::thread(reconnect, std::ref(socket), std::ref(set));

	sf::ContextSettings settings;
	settings.antialiasingLevel = 8;
	sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "0x", sf::Style::Close, settings);

	const std::string font_path = "../fonts/Roboto-Regular.ttf";
	if(!font.loadFromFile(font_path)){
		printf("Unable to load font %s (file not found)!\n", font_path.c_str());
		exit(1);
	}

	state::change("connect");


	//text.setFont(font);
	//text.setCharacterSize(12);
	//text.setFillColor(sf::Color(244,240,219));

	//sessions_text.setFont(font);
	//sessions_text.setCharacterSize(30);
	//sessions_text.setFillColor(sf::Color::Black);
	//sessions_text.setPosition({50, 50});

	//stats_text.setFont(font);
	//stats_text.setCharacterSize(15);
	//stats_text.setFillColor(sf::Color::Black);
	//stats_text.setPosition({800-300, 50});

	while (window.isOpen()){
		//std::lock_guard<std::mutex> lock(mutex);
		sf::Event event;
		while(window.pollEvent(event)){
			if(event.type == sf::Event::Closed)
				window.close();
			
			state::event(&event);
			//if(event.type == sf::Event::KeyPressed){
			//	switch(event.key.code){
			//		case sf::Keyboard::Down:
			//			move_down(menu);
			//			break;
			//		case sf::Keyboard::Up:
			//			move_up(menu);
			//			break;
			//		case sf::Keyboard::Enter:
			//			push_active_button(menu);
			//			break;
			//		case sf::Keyboard::Q:
			//			window.close();
			//			break;
			//		case sf::Keyboard::A:
			//			toggle_hex_positions = !toggle_hex_positions;
			//			break;
			//		case sf::Keyboard::R:
			//			send_command(socket, "ls");
			//			break;
			//		case sf::Keyboard::Num1: if(sessions.size() > 0){send_command(socket, "coj o " + sessions[0] + " 1");} break;
			//		case sf::Keyboard::Num2: if(sessions.size() > 1){send_command(socket, "coj o " + sessions[1] + " 1");} break;
			//		case sf::Keyboard::Num3: if(sessions.size() > 2){send_command(socket, "coj o " + sessions[2] + " 1");} break;
			//		case sf::Keyboard::Num4: if(sessions.size() > 3){send_command(socket, "coj o " + sessions[3] + " 1");} break;
			//		case sf::Keyboard::Num5: if(sessions.size() > 4){send_command(socket, "coj o " + sessions[4] + " 1");} break;
			//	}
			//}
		}

		window.clear(background_color);

		//if(!map.empty()){
		//	// Using a capturing lambda here is slow (due to virtual function call?),
		//	// especially since it's being called in a tight loop
		//	for(auto& cell : map){
		//		auto& [q,r,resources,player_id] = cell;

		//		sf::Color color = dark_color;
		//		sf::Color	text = background_color;
		//		if(player_id > 0){
		//			float l = 0.2f + fmin((float)resources, 0.6f*300.0f)/300.0f;
		//			if(l > 0.5f){
		//				text = dark_color;
		//			}

		//			auto [r,g,b] = hsl_to_rgb({player_id*360.0f/max_players, 0.5f, l});
		//			color = sf::Color(255*r, 255*g, 255*b);
		//		}

		//		draw_cell(window, q, r, resources, player_id, color, text);
		//	}
		//}

		//if(!player_scores.empty()){
		//	sf::RectangleShape rect({10,10});

		//	stats_text.setPosition({800-100, 50});
		//	stats_text.setString("Scores");
		//	window.draw(stats_text);

		//	auto pos = stats_text.getPosition();
		//	stats_text.setPosition(pos + sf::Vector2f{15,0});
		//	for(auto& [id, score] : player_scores){
		//		auto pos = stats_text.getPosition();
		//		stats_text.setPosition(pos + sf::Vector2f{0,20});
		//		stats_text.setString(std::to_string(id) + ": " + std::to_string(score));

		//		auto [r,g,b] = hsl_to_rgb({id*360.0f/max_players, 0.5f, 0.5f});
		//		auto color = sf::Color(255*r, 255*g, 255*b);
		//		rect.setFillColor(color);
		//		rect.setPosition(pos + sf::Vector2f{-15,20 + 10.0f/2}); 
		//		window.draw(stats_text);
		//		window.draw(rect);
		//	}
		//}

		//draw_button_menu(window, menu);
		//window.draw(sessions_text);
		state::update(&window);

		window.display();
	}

	window.close();

	return 0;
}
