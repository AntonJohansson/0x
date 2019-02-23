#include <iostream>
#include <string>
#include <math.h>
#include <utility>
#include <SFML/Graphics.hpp>

float size = 20;
float w = sqrt(3) * size;
float h = 2 * size;

constexpr float pi_over_six = M_PI/6.0f;

std::array<sf::Vertex,8> vertices;
void draw_hexagon(sf::RenderWindow& window, float x, float y){
	vertices[0] = sf::Vertex({x,y});

	for(int i = 0; i < 7; i++){
		vertices[i+1] = sf::Vertex({x+size*cos(pi_over_six+2*pi_over_six*i), y+size*sin(pi_over_six+2*pi_over_six*i)});
	}

	window.draw(&vertices[0], 8, sf::TriangleFan);
}

std::pair<float, float> axial_to_screen(int x, int y){
	return {y*2*size*cos(pi_over_six*4)+x*w, y*2*size*sin(pi_over_six*4)};
}

int main(){
	sf::RenderWindow window(sf::VideoMode(800, 600), "0x");

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

		window.clear(sf::Color::Black);

		for(int x = -2; x <= 2; x++){
			for(int y = -2; y <= 2; y++){
				auto [screen_x, screen_y] = axial_to_screen(x, y);
				screen_x += 400.0f;
				screen_y += 300.0f;

				draw_hexagon(window, screen_x, screen_y);
			}
		}

		window.display();
	}

	return 0;
}
