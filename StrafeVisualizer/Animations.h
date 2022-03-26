#pragma once
#include <SFML/Graphics.hpp>
#include <Eigen/Dense>

class Animations 
{
public:
	static Eigen::Vector2d unfilteredPts[2];
	static Eigen::Vector2d moveablePts[2];

	static int frame;
	static bool animate_out;

	static bool Background(sf::RenderTarget& window, bool staret_anim);
	static bool WishVelDemonstration(sf::RenderTarget& window);
	static bool PerfAngleDemo(sf::RenderTarget& window);
	static bool End(sf::RenderTarget& window);
	static bool PerfVelDemo(sf::RenderTarget& window);
};