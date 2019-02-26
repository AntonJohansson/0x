#include <iostream>
#include <string>
#include <math.h>
#include <algorithm>
#include <utility>
#include <SFML/Graphics.hpp>
#include <array>
#include <assert.h>

#include "hex.hpp"

constexpr int32_t SCREEN_WIDTH  = 800;
constexpr int32_t SCREEN_HEIGHT = 600;

std::array<sf::Vertex,8> vertices;
void draw_hexagon(sf::RenderWindow& window, float size, float x, float y, sf::Color color){
	vertices[0] = sf::Vertex({x,y}, color);

	for(int i = 0; i < 7; i++){
		vertices[i+1] = sf::Vertex({x+size*cos(axial::pi_over_six+2*axial::pi_over_six*i), y+size*sin(axial::pi_over_six+2*axial::pi_over_six*i)}, color);
	}

	window.draw(&vertices[0], 8, sf::TriangleFan);
}




























































void transfer_resource(HexagonalMap& map, int q1, int r1, int q2, int r2, int amount){
	auto have = map.at(q1, r1).resources;
	assert(have >= amount);
	map.at(q1, r1).resources -= amount;
	map.at(q2, r2).resources += amount;
}

bool toggle_hex_positions = true;
bool selecting_transfer = false;
sf::Vector2i mouse_pos;
int mouse_q;
int mouse_r;
int transfer_amount = 10;
int transfer_from_q;
int transfer_from_r;

int main(){
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

	HexagonalMap map(3, 20, 5);
	//printf("size: %f\nwidth: %f\nheight: %f\n", map.hex_size, map.hex_width, map.hex_height);

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
				}
			}else if(event.type == sf::Event::MouseButtonPressed){
				if(!selecting_transfer){
					transfer_from_q = mouse_q;
					transfer_from_r = mouse_r;

					if(map.contains(transfer_from_q, transfer_from_r)){
						map.at(transfer_from_q, transfer_from_r).selected = true;
						selecting_transfer = true;
					}
				}else if(map.contains(mouse_q, mouse_r)){
					transfer_resource(map, transfer_from_q, transfer_from_r, mouse_q, mouse_r, transfer_amount);
					map.at(transfer_from_q, transfer_from_r).selected = false;
					selecting_transfer = false;
				}
			}

		}

		mouse_pos = sf::Mouse::getPosition(window);
		std::tie(mouse_q, mouse_r) = axial::closest_hex(map, mouse_pos.x, mouse_pos.y);

		window.clear(sf::Color(244,240,219));

		// Using a capturing lambda here is slow (due to virtual function call?),
		// especially since it's being called in a tight loop
		map.for_each([&](int q, int r, HexCell& cell){
					auto [x, y] = axial::to_screen(map, q, r);
					x *= 1.05f;
					y *= 1.05f;
					x += SCREEN_WIDTH/2.0f;
					y += SCREEN_HEIGHT/2.0f;
					y = SCREEN_HEIGHT - y;

					if(mouse_q == q && mouse_r == r){
						draw_hexagon(window, map.hex_size, x, y, sf::Color(0,0,100));
					}else if(cell.selected){
						draw_hexagon(window, map.hex_size, x, y, sf::Color(100,0,0));
					}else if(cell.player_id > 0){
						auto color = sf::Color(
							0.5f*(cell.player_id+0)*255/map.players,
							0.5f*(cell.player_id+1)*255/map.players,
							0.5f*(cell.player_id+2)*255/map.players);
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

		auto n = hex_neighbours(map, mouse_q, mouse_r);
		for(auto hex : n){
			//if(hex.player_id == 0)continue;

			auto [x, y] = axial::to_screen(map, hex.q, hex.r);
			x *= 1.05f;
			y *= 1.05f;
			x += SCREEN_WIDTH/2.0f;
			y += SCREEN_HEIGHT/2.0f;
			y = SCREEN_HEIGHT - y;

			draw_hexagon(window, map.hex_size, x, y, sf::Color(0,100,0));
		}

		window.display();
	}

	


	window.close();

	return 0;
}
