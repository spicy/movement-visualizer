#pragma once
#include <Eigen/Dense>
#include "convars.h"
#include "CMoveData.h"

enum PositionType
{
	GROUND,
	AIR
};

class StrafeMath
{
protected:
	// Input/Output for this movement
	static BasePlayer*	player;
	static CMoveData*	mv;
	static PositionType	positionType;

public:
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

	void StartGravity();
	void FinishGravity();

	void CheckVelocity();
	void CaptureMovementKeys();



	//Other
	void AngleVectors(Eigen::Vector3d& angles, Eigen::Vector3d& forward, Eigen::Vector3d& right, Eigen::Vector3d& up);
	double VecMagnitude(Eigen::Vector3d& vec);
	double DotProduct(Eigen::Vector3d& a, Eigen::Vector3d& b);
};
