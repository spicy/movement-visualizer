#pragma once
#include <Eigen/Dense>
#include "convars.h"

class StrafeMath
{
private:
	static Eigen::Vector2d velocity;
	static Eigen::Vector2d wishvel;
	static Eigen::Vector2d nextVelocity;

	// Eye angles
	static Eigen::Vector2d eyesForward;
	static Eigen::Vector2d eyesSide;

	// Keypress data
	static float forwardmove, sidemove;

	// Playerstate data

public:
	void PlayerMove();

	//Ground movement
	void WalkMove();
	void Accelerate();
	void Friction();

	//Air movement
	void AirMove();
	void AirAccelerate(Eigen::Vector2d& wishdir, float wishspeed, float accel);

	//Other
	void AngleVectors(const double yaw, Eigen::Vector2d& forward, Eigen::Vector2d& right);
};
