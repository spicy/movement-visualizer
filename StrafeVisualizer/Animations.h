#pragma once
#include <SFML/Graphics.hpp>
#include <Eigen/Dense>

class Animations 
{
public:
	static Eigen::Vector2d unfiltered_points[2];
	static Eigen::Vector2d moveable_points[2];
	static int frame;
	static bool animate_out;

	static bool Background(sf::RenderTarget& window, bool staret_anim);
	static bool ShowVectors(sf::RenderTarget& window);
};
