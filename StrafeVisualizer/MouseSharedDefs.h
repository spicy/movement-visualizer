#include <SFML/Graphics.hpp>

struct
{
	sf::Vector2i pos;
	bool pressed = false;
	int select = -1;
}mouse;
