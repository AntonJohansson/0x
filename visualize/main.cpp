#include <iostream>
#include <string>
#include <math.h>
#include <algorithm>
#include <utility>
#include <SFML/Graphics.hpp>
#include <array>

float size = 20;
float w = sqrt(3) * size;
float h = 2 * size;

constexpr float pi_over_six = M_PI/6.0f;

std::array<sf::Vertex,8> vertices;
void draw_hexagon(sf::RenderWindow& window, float x, float y){
	vertices[0] = sf::Vertex({x,y}, sf::Color(47,47,47));

	for(int i = 0; i < 7; i++){
		vertices[i+1] = sf::Vertex({x+size*cos(pi_over_six+2*pi_over_six*i), y+size*sin(pi_over_six+2*pi_over_six*i)}, sf::Color(100,100,100));
	}

	window.draw(&vertices[0], 8, sf::TriangleFan);
}

std::pair<float, float> axial_to_screen(int x, int y){
	return {y*2*size*cos(pi_over_six)*cos(pi_over_six*4)+x*w, y*2*size*cos(pi_over_six)*sin(pi_over_six*4)};
}

int main(){
	sf::RenderWindow window(sf::VideoMode(800, 600), "0x");

	const std::string font_path = "../fonts/Roboto-Regular.ttf";
	sf::Font font;
	if(!font.loadFromFile(font_path)){
		printf("Unable to load font %s (file not found)!\n", font_path.c_str());
		return 1;
	}

	sf::Text text;
	text.setFont(font);
	text.setCharacterSize(10);
	text.setFillColor(sf::Color(244,240,219));

	while (window.isOpen())
	{
		// check all the window's events that were triggered since the last iteration of the loop
		sf::Event event;
		while (window.pollEvent(event))
		{
			// "close requested" event: we close the window
			if (event.type == sf::Event::Closed)
				window.close();
		}

		window.clear(sf::Color(244,240,219));

		constexpr int gridsize = 3;
		for (int q = -gridsize; q <= gridsize; q++) {
			int r1 = std::max(-gridsize, -q - gridsize);
			int r2 = std::min(gridsize, -q + gridsize);
			for (int r = r1; r <= r2; r++) {
				auto x = q;
				auto y = r;

				auto [screen_x, screen_y] = axial_to_screen(x, y);
				screen_x += 400.0f;
				screen_y += 300.0f;
				screen_y = 600.0f - screen_y;

				draw_hexagon(window, screen_x, screen_y);
				auto text_rect = text.getLocalBounds();
				text.setOrigin(text_rect.left + text_rect.width/2.0f,
											 text_rect.top  + text_rect.height/2.0f);
				text.setPosition(sf::Vector2f(800/2.0f,600/2.0f));
				text.setPosition(screen_x, screen_y);
				text.setString(std::to_string(x) + ", " + std::to_string(y));
				window.draw(text);
			}
		}

		//for(int x = -gridsize; x <= gridsize; x++){
		//	for(int y = -gridsize; y <= gridsize; y++){
		//		auto [screen_x, screen_y] = axial_to_screen(x, y);
		//		screen_x += 400.0f;
		//		screen_y += 300.0f;
		//		screen_y = 600.0f - screen_y;

		//		draw_hexagon(window, screen_x, screen_y);
		//		auto text_rect = text.getLocalBounds();
		//		text.setOrigin(text_rect.left + text_rect.width/2.0f,
		//									 text_rect.top  + text_rect.height/2.0f);
		//		text.setPosition(sf::Vector2f(800/2.0f,600/2.0f));
		//		text.setPosition(screen_x, screen_y);
		//		text.setString(std::to_string(x) + ", " + std::to_string(y));
		//		window.draw(text);
		//	}
		//}

		window.display();
	}

	return 0;
}
