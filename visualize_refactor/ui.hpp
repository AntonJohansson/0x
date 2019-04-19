#pragma once

#include <vector>
#include <SFML/Graphics.hpp>
#include <string>
#include <assert.h>

using Callback = void(*)(void);

enum ButtonState{
	PASSIVE,
	HOVER,
};

struct Button{
	std::string text;
	Callback callback;
	ButtonState state;
};

struct ButtonMenu{
	int x = 0, 
			y = 0,
			button_padding_x = 20,
			button_padding_y = 10,
			button_spacing = 10,
			button_width = 200,
			button_height = 50;

	sf::Font font;
	sf::Color background_hover = sf::Color::White,
						background_passive = sf::Color::Black, 
						text_hover = sf::Color::Black, 
						text_passive = sf::Color::White;

	std::vector<Button> buttons;
	size_t active_index = 0;
};

inline void hover_button(ButtonMenu& menu, size_t index){
	menu.buttons[index].state = HOVER;
}

inline void passive_button(ButtonMenu& menu, size_t index){
	menu.buttons[index].state = PASSIVE;
}

inline void move_down(ButtonMenu& menu){
	if(menu.active_index < menu.buttons.size()-1){
		passive_button(menu, menu.active_index);
		hover_button(menu, ++menu.active_index);
	}
}

inline void move_up(ButtonMenu& menu){
	if(menu.active_index > 0){
		passive_button(menu, menu.active_index);
		hover_button(menu, --menu.active_index);
	}
}

inline void center_menu(ButtonMenu& menu, int x0, int x1, int y0, int y1){
	auto number_of_buttons = menu.buttons.size();
	assert(number_of_buttons);
	menu.x = (x1 - x0 - menu.button_width)/2;
	menu.y = (y1 - y0 - (menu.button_height)*number_of_buttons - (menu.button_spacing)*(number_of_buttons-1))/2;
}

inline void add_button(ButtonMenu& menu, const std::string& text, Callback callback){
	menu.buttons.push_back({text, callback, PASSIVE});
}

inline void push_active_button(ButtonMenu& menu){
	menu.buttons[menu.active_index].callback();
}

inline void draw_button_menu(sf::RenderWindow& window, ButtonMenu& menu){
	auto number_of_buttons = menu.buttons.size();
	sf::RectangleShape background({menu.button_width, menu.button_height});
	sf::Text text;
	text.setFont(menu.font);
	text.setCharacterSize(menu.button_height - 2*menu.button_padding_y);

	for(int i = 0; i < menu.buttons.size(); i++){
		auto& button = menu.buttons[i];

		switch(button.state){
			case HOVER:
				background.setFillColor(menu.background_hover);
				text.setFillColor(menu.text_hover);
				break;
			case PASSIVE:
				background.setFillColor(menu.background_passive);
				text.setFillColor(menu.text_passive);
				break;
		}

		text.setString(button.text);
		text.setPosition({
				menu.x + menu.button_padding_x,
				menu.y + (menu.button_height+menu.button_spacing)*i + menu.button_height/2
				});
		auto text_rect = text.getLocalBounds();
		text.setOrigin(text_rect.left, text_rect.top + text_rect.height/2.0f);
	
		background.setPosition({
				menu.x, 
				menu.y + (menu.button_height+menu.button_spacing)*i
				});

		window.draw(background);
		window.draw(text);
	}
}
