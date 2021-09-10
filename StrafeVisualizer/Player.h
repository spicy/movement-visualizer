#include <Eigen/Dense>

enum PositionType
{
	GROUND,
	AIR
};

class Player
{
public:
	Eigen::Vector3d		forward;
	Eigen::Vector3d		right;
	Eigen::Vector3d		up;

	Eigen::Vector3d		viewAngles;		// Command view angles (local space)
	Eigen::Vector3d		velocity;
	Eigen::Vector3d		wishVel;

	float				surfaceFriction;
	PositionType		positionType;

	int					buttons;		// Attack buttons.
	int					oldButtons;		// From host_client->oldbuttons;
	float				forwardMove;
	float				sideMove;
	float				upMove;

	float				maxSpeed;

	// Output only
	Eigen::Vector3d		outWishVel;		// This is where you tried 

	Player()
	{
		forward.setZero();
		right.setZero();
		up.setZero();

		velocity.setZero();
		wishVel.setZero();
		outWishVel.setZero();

		viewAngles.setZero();
		buttons = 0;
		oldButtons = 0;
		forwardMove = 0;
		sideMove = 0;
		upMove = 0;

		maxSpeed = 3500;

		surfaceFriction = 1;
		positionType = PositionType::AIR;
	}
};
