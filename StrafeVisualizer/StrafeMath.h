#pragma once
#include <Eigen/Dense>
#include "convars.h"
#include "Player.h"

namespace StrafeMath
{
	void ManuallyUpdateStuff();
	void ProcessMovement();

	void PlayerMove();
	void FinishMove();

	void FullWalkMove();

	//Ground movement
	void WalkMove();
	void Accelerate(Eigen::Vector3d& wishdir, float wishspeed, float accel);
	void Friction();

	//Air movement
	void AirMove();
	void AirAccelerate(Eigen::Vector3d& wishdir, float wishspeed, float airaccel);

	void CheckVelocity();

	// Input/Output for this movement
	extern Player* player;
}

//Other
void AngleVectors(Eigen::Vector3d& angles, Eigen::Vector3d& forward, Eigen::Vector3d& right, Eigen::Vector3d& up);
double VecMagnitude(Eigen::Vector3d& vec);
double DotProduct(Eigen::Vector3d& a, Eigen::Vector3d& b);