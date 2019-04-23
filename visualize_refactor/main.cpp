
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

#include "states.hpp"

#include <imgui.h>
#include <imgui-SFML.h>

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

static PollSet set;
static Socket socket;

enum ServerState{DISCONNECTED, CONNECTING, CONNECTED};
enum GameState{NOT_IN_LOBBY, IN_LOBBY, SPECTATE_GAME, PLAY_GAME};
static ServerState server_state = DISCONNECTED;
static GameState game_state = NOT_IN_LOBBY;

uint64_t lobby_id = 0;

std::thread server_thread;
std::mutex mutex;

bool lobby_list_updated = false;
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

bool server_thread_running = false;

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









































static void quit(sf::RenderWindow& window){
	//set.disconnect(socket);
	tcp_socket::shutdown(socket);
	if(server_thread.joinable()){
		server_thread.join();
	}
	window.close();
}










static void send_command(int s, const std::string& command){
	BinaryData data;
	encode::u32(data, command.size());
	encode::u8(data, 4);
	append(data, command);
	tcp_socket::send_all(s, data.data(), data.size());
}

void input_loop(PollSet& set){
	while(server_thread_running){
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
					lobby_list_updated = true;
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
					game_state = IN_LOBBY;
					break;
				}
		}
	}
}


void connect_to_server(Socket& socket, PollSet& set, const char* ip, const char* port){
	server_thread_running = true;
	for(int i = 0; server_thread_running && i < 5; i++){
		if(socket = client_bind(ip, port); socket != tcp_socket::INVALID){
			tcp_socket::set_nonblocking(socket);

			server_state = CONNECTED;

			set.add(socket, receive_data);
			set.on_disconnect(socket, [&](int){
					server_state = DISCONNECTED;
					game_state = NOT_IN_LOBBY;
					server_thread_running = false;
					tcp_socket::close(socket);
					});

			break;
		}else{
		}
		
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	if(server_state == CONNECTED){
		send_command(socket, "ls");
		input_loop(set);
	}
}














// main menu screen



int main(){
	//server_thread = std::thread(reconnect, std::ref(socket), std::ref(set));
	sf::ContextSettings settings;
	settings.antialiasingLevel = 8;
	sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "0x", sf::Style::Close, settings);
	ImGui::SFML::Init(window);

	const std::string font_path = "../fonts/Roboto-Regular.ttf";
	if(!font.loadFromFile(font_path)){
		printf("Unable to load font %s (file not found)!\n", font_path.c_str());
		exit(1);
	}

	stats_text.setFont(font);
	stats_text.setCharacterSize(15);
	stats_text.setFillColor(sf::Color::Black);
	stats_text.setPosition({800-300, 50});

	sf::Clock delta_clock;
	while (window.isOpen()){
		sf::Event event;
		while(window.pollEvent(event)){
			ImGui::SFML::ProcessEvent(event);
			if(event.type == sf::Event::Closed)
				quit(window);
			
			state::event(&event);
		}

		ImGui::SFML::Update(window, delta_clock.restart());

		ImGui::Begin("Server");

		if(server_state == CONNECTED){
			ImGui::TextColored({0,255,0,255}, "status: Connected");
		}else if(server_state == CONNECTING){
			ImGui::TextColored({125,125,0,255}, "status: Connecting");
		}else if(server_state == DISCONNECTED){
			ImGui::TextColored({255,0,0,255}, "status: Disconnected");
		}

		static char ip_input[16] = "127.0.0.1";
		static char port_input[16] = "1111";
		ImGui::InputText("Ip", ip_input, 16);
		ImGui::InputText("Port", port_input, 16);
		ImGui::Columns(2, NULL, false);
		if(server_state == DISCONNECTED && ImGui::Button("Connect")){
			server_state = CONNECTING;
			if(server_thread.joinable()){
				server_thread.join();
			}
			server_thread = std::thread(connect_to_server, std::ref(socket), std::ref(set), ip_input, port_input);
		}else if(server_state != DISCONNECTED && ImGui::Button("Disconnect")){
			// close server thread
			// this code is repeated a bunch
			server_state = DISCONNECTED;
			game_state = NOT_IN_LOBBY;
			server_thread_running = false;
			tcp_socket::shutdown(socket);
			if(server_thread.joinable()){
				server_thread.join();
			}
		}
		ImGui::NextColumn();
		if(ImGui::Button("Quit")){
			quit(window);
		}
		ImGui::Columns(1);

		ImGui::Separator();

		if(server_state == CONNECTED){
			if(ImGui::CollapsingHeader("Lobbies", ImGuiTreeNodeFlags_DefaultOpen)){
				static int lobbies_selected = 0;
				static const char* combo_items[2] = {"observer", "player"};
				static int combo_selected = 0;

				ImGui::Columns(3, NULL, false);
				if(ImGui::Button("Refresh")){
					send_command(socket, "ls");
				}
				ImGui::NextColumn();
				static bool create_open = false;
				if(ImGui::Button("Create")){
					create_open = !create_open;
				}
				ImGui::NextColumn();
				if(ImGui::Button("Join")){
					game_state = NOT_IN_LOBBY;
					send_command(socket, "coj " + std::string(combo_items[combo_selected]) + " " + sessions[lobbies_selected]);
					map.clear();
				}
				ImGui::Columns(1);

				ImGui::Combo("Mode", &combo_selected, combo_items, 2);

				if(create_open){
					static char input_name[16] = "";
					static int input_max_players = 2;
					static int input_map_radius = 3;
					static bool input_restart_on_win = false;

					ImGui::Begin("Create new lobby");
					ImGui::InputText("Name", input_name, 16);
					if(ImGui::InputInt("Max players", &input_max_players) && input_max_players < 1){
						input_max_players = 1;
					}
					if(ImGui::InputInt("Map radius", &input_map_radius) && input_map_radius < 1){
						input_map_radius = 1;
					}
					ImGui::Checkbox("Restart on win", &input_restart_on_win);
					ImGui::Separator();
					ImGui::Columns(2, NULL, false);
					if(ImGui::Button("Cancel")){
						create_open = false;
					}
					ImGui::NextColumn();
					if(ImGui::Button("Create")){
						send_command(socket, "coj o " + std::string(input_name) + " " + std::to_string(input_max_players) + " " + std::to_string(input_map_radius) + " " + ((input_restart_on_win) ? "true" : "false"));
						create_open = false;
					}
					ImGui::Columns(1);
					ImGui::End();
				}
				
				if(!sessions.empty()){
					ImGui::ListBox("Lobbies", &lobbies_selected, [](void* data, int idx, const char** out_text)->bool{
								auto v = reinterpret_cast<std::vector<std::string>*>(data);
								*out_text = (*v)[idx].c_str();
								return true;
							}, (void*)&sessions, sessions.size());
				}
			}
		}

		if(ImGui::CollapsingHeader("Console")){
			ImGui::BeginChild("console");
			ImGui::TextUnformatted("asdfasdfasfd");
			ImGui::EndChild();
			char console_input[255] = "";
			ImGui::InputText("Send Command", console_input, 255, ImGuiInputTextFlags_EnterReturnsTrue);
		}

		if(game_state == IN_LOBBY){
			ImGui::Begin("Lobby");
			ImGui::Columns(3, NULL, false);

			ImGui::TextUnformatted("Color");
			ImGui::NextColumn();
			ImGui::TextUnformatted("Id");
			ImGui::NextColumn();
			ImGui::TextUnformatted("Score");
			ImGui::NextColumn();
			ImGui::Separator();

			ImVec4 color;
			for(auto& [id, score] : player_scores){
				auto [r,g,b] = hsl_to_rgb({id*360.0f/max_players, 0.5f, 0.5f});
				color = {r, g, b, 1.0f};
				ImGui::ColorButton("", color);
				//ImGuiColorEditFlags_NoSmallPreview|ImGuiColorEditFlags_NoTooltip|ImGuiColorEditFlags_NoDragDrop);
				ImGui::NextColumn();
				ImGui::Text("%i", id);
				ImGui::NextColumn();
				ImGui::Text("%i", score);
				ImGui::NextColumn();
			}
			ImGui::Columns(1);
			ImGui::Separator();

			ImGui::Button("Pause");
			ImGui::Button("Step");
			ImGui::Button("Change game state");

			ImGui::End();
		}


		ImGui::End();

		window.clear(background_color);

		if(!map.empty()){
			// Using a capturing lambda here is slow (due to virtual function call?),
			// especially since it's being called in a tight loop
			for(auto& cell : map){
				auto& [q,r,resources,player_id] = cell;

				sf::Color color = dark_color;
				sf::Color	text = background_color;
				if(player_id > 0){
					float l = 0.2f + fmin((float)resources, 0.6f*300.0f)/300.0f;
					if(l > 0.5f){
						text = dark_color;
					}

					auto [r,g,b] = hsl_to_rgb({player_id*360.0f/max_players, 0.5f, l});
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

				auto [r,g,b] = hsl_to_rgb({id*360.0f/max_players, 0.5f, 0.5f});
				auto color = sf::Color(255*r, 255*g, 255*b);
				rect.setFillColor(color);
				rect.setPosition(pos + sf::Vector2f{-15,20 + 10.0f/2}); 
				window.draw(stats_text);
				window.draw(rect);
			}
		}
		
		ImGui::SFML::Render(window);

		window.display();
	}

	ImGui::SFML::Shutdown();

	window.close();

	return 0;
}
