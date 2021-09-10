#include "StrafeMath.h"

#ifndef PI 
#define PI 3.1415926535 
#endif

Player* StrafeMath::player;

inline void VectorMultiply(const Eigen::Vector3d& a, float b, Eigen::Vector3d& c)
{
	c[0] = a[0] * b;
	c[1] = a[1] * b;
	c[2] = a[2] * b;
}

inline void VectorScale(const Eigen::Vector3d& in, float scale, Eigen::Vector3d& result)
{
	VectorMultiply(in, scale, result);
}


void StrafeMath::ManuallyUpdateStuff()
{
	Eigen::Vector3d angles = Eigen::Vector3d(0, atan2f(player->viewAngles[1], player->viewAngles[0]) * (180 / PI), 0);

	// Sets forward, right
	AngleVectors(angles, player->forward, player->right, player->up);

	// Zero out z components of movement vectors
	player->forward[2] = 0;
	player->right[2] = 0;
	player->forward.normalize();
	player->right.normalize();

	// Set wishvel
	for (int i = 0; i < 2; i++)
	{
		player->wishVel[i] = player->forward[i] * player->forwardMove + player->right[i] * player->sideMove;
	}
	player->wishVel[2] = 0; // Zero out z part of velocity
}

void StrafeMath::ProcessMovement()
{
	PlayerMove();
	FinishMove();
}

void StrafeMath::PlayerMove()
{
    switch (player->positionType)
    {
    case GROUND:
        WalkMove();
        break;
    case AIR:
        AirMove();
        break;
    }
}

void StrafeMath::FinishMove()
{
	player->oldButtons = player->buttons;
}


//General movement wrapping function
void StrafeMath::FullWalkMove()
{
	// Fricion is handled before we add in any base velocity.
	switch (player->positionType)
	{
	case GROUND:
		player->velocity[2] = 0;
		Friction();
		CheckVelocity();
		WalkMove();
		break;
	case AIR:
		CheckVelocity();
		AirMove();
		break;
	}

	// Make sure velocity is valid
	CheckVelocity();

	// If we are on ground, no downward velocity.
	if (player->positionType == GROUND)
	{
		player->velocity[2] = 0;
	}
	//CheckLanding();
}


//Ground movement
void StrafeMath::WalkMove()
{
	Eigen::Vector3d wishdir;
	float wishspeed;
	float spd;

	// Determine movement angles
	Eigen::Vector3d angles = Eigen::Vector3d(0, atan2f(player->viewAngles[1], player->viewAngles[0]) * (180 / PI), 0);
	AngleVectors(angles, player->forward, player->right, player->up);

	// Copy movement amounts

	// Zero out z components of eye vectors
	player->forward[2] = 0;
	player->right[2] = 0;
	player->forward.normalize();
	player->right.normalize();

	// Determine x and y parts of velocity
	for (int i = 0; i < 2; i++)
	{
		player->wishVel[i] = player->forward[i] * player->forwardMove + player->right[i] * player->sideMove;
	}

	// Zero out z part of velocity
	player->wishVel[2] = 0;

	// Isolate direction component of velocity
	wishdir = player->wishVel;
	wishdir.normalize();

	// Isolate magnitude component of velocity
	wishspeed = VecMagnitude(player->wishVel);
	// Clamp to server defined max speed
	if ((wishspeed != 0.0f) && (wishspeed > player->maxSpeed))
	{
		VectorScale(player->wishVel, player->maxSpeed / wishspeed, player->wishVel);
		wishspeed = player->maxSpeed;
	}

	// Set pmove velocity
	player->velocity[2] = 0;
	Accelerate(wishdir, wishspeed, sv_accelerate);
	player->velocity[2] = 0;

	spd = VecMagnitude(player->velocity);

	// first try just moving to the destination	
	//dest[0] = player->vecAbsOrigin[0] + player->velocity[0] * intervalPerTick;
	//dest[1] = player->vecAbsOrigin[1] + player->velocity[1] * intervalPerTick;
	//dest[2] = player->vecAbsOrigin[2];

	// first try moving directly to the next spot
	//TracePlayerBBox(player->GetAbsOrigin(), dest, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm);

	// If we made it all the way, then copy trace end as new player position.
	player->outWishVel += wishdir * wishspeed;
}

void StrafeMath::Accelerate(Eigen::Vector3d& wishdir, float wishspeed, float accel)
{
	float addspeed, accelspeed, currentspeed;

	// See if we are changing direction a bit

	currentspeed = DotProduct(player->velocity, wishdir);

	// Reduce wishspeed by the amount of veer.
	addspeed = wishspeed - currentspeed;

	// If not going to add any speed, done.
	if (addspeed <= 0)
		return;

	// Determine amount of accleration.
	accelspeed = accel * intervalPerTick * wishspeed * player->surfaceFriction;

	// Cap at addspeed
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	// Adjust velocity.
	for (int i = 0; i < 3; i++)
	{
		player->velocity[i] += accelspeed * wishdir[i];
	}
}

void StrafeMath::Friction()
{
	float speed, newspeed, control;
	float friction;
	float drop;

	// speed is magnitude of player velocity
	speed = VecMagnitude(player->velocity);
	// If too slow, return
	if (speed < 0.1f)
	{
		return;
	}

	drop = 0;
	// apply ground friction
	if (player->positionType == GROUND)  // On an entity that is the ground
	{
		friction = sv_friction * player->surfaceFriction;

		// Bleed off some speed, but if we have less than the bleed
		// threshold, bleed the threshold amount.
		control = (speed < sv_stopSpeed) ? sv_stopSpeed : speed;

		// Add the amount to the drop amount.
		drop += control * friction * intervalPerTick;
	}

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
		newspeed = 0;

	if (newspeed != speed)
	{
		// Determine proportion of old speed we are using.
		newspeed /= speed;

		// Adjust velocity according to proportion
		for (int i = 0; i < 3; i++)
		{
			player->velocity[i] * newspeed;
		}
	}

	player->outWishVel -= (1.f - newspeed) * player->velocity;
}


//Air movement
void StrafeMath::AirMove()
{
	Eigen::Vector3d wishdir;
	float fmove, smove;
	float wishspeed;

	Eigen::Vector3d angles = Eigen::Vector3d(0, atan2f(player->viewAngles[1], player->viewAngles[0]) * (180 / PI), 0);
	AngleVectors(angles, player->forward, player->right, player->up);

	// Zero out z components of movement vectors
	player->forward[2] = 0;
	player->right[2] = 0;
	player->forward.normalize();
	player->right.normalize();

	for (int i = 0; i < 2; i++)       // Determine x and y parts of velocity
		player->wishVel[i] = player->forward[i] * player->forwardMove + player->right[i] * player->sideMove;

	player->wishVel[2] = 0;             // Zero out z part of velocity

	wishdir = player->wishVel;
	wishdir.normalize();

	wishspeed = VecMagnitude(player->wishVel);

	if (wishspeed != 0 && (wishspeed > player->maxSpeed))
	{
		VectorScale(player->wishVel, player->maxSpeed / wishspeed, player->wishVel);
		wishspeed = player->maxSpeed;
	}

	AirAccelerate(wishdir, wishspeed, sv_airAccelerate);
	//TryPlayerMove();
}

void StrafeMath::AirAccelerate(Eigen::Vector3d& wishdir, float wishspeed, float accel)
{
	float addspeed, accelspeed, currentspeed;
	float wishspd;

	wishspd = wishspeed;

	// Cap speed to 30
	if (wishspd > airSpeedCap)
		wishspd = airSpeedCap;

	// Determine veer amount
	currentspeed = DotProduct(player->velocity, wishdir);

	// See how much to add
	addspeed = wishspd - currentspeed;

	// If not adding any, done.
	if (addspeed <= 0)
		return;

	// Determine acceleration speed after acceleration
	accelspeed = accel * wishspeed * intervalPerTick * player->surfaceFriction;

	// Cap it
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	// Adjust pmove vel.
	for (int i = 0; i < 3; i++)
	{
		player->velocity[i] += accelspeed * wishdir[i];
		player->outWishVel[i] += accelspeed * wishdir[i];
	}
}


void StrafeMath::CheckVelocity()
{
	for (int i = 0; i < 3; i++)
	{
		// Check if it's INF
		if (player->velocity[i] == NAN)
		{
			player->velocity[i] = 0;
		}

		// Cap its value
		if (player->velocity[i] > sv_maxVelocity)
		{
			player->velocity[i] = sv_maxVelocity;
		}
		else if (player->velocity[i] < -sv_maxVelocity)
		{
			player->velocity[i] = -sv_maxVelocity;
		}
	}
}


//Other
void AngleVectors(Eigen::Vector3d& angles, Eigen::Vector3d& forward, Eigen::Vector3d& right, Eigen::Vector3d& up)
{
    float angle;
    static float sinpitch, sinyaw, sinroll, cospitch, cosyaw, cosroll;

    angle = angles[0] * (PI / 180);
    sinpitch = sin(angle);
    cospitch = cos(angle);
    angle = angles[1] * (PI / 180);
    sinyaw = sin(angle);
    cosyaw = cos(angle);
    angle = angles[2] * (PI / 180);
    sinroll = sin(angle);
    cosroll = cos(angle);

    forward[0] = cospitch * cosyaw;
    forward[1] = cospitch * sinyaw;
    forward[2] = -sinpitch;
    right[0] = (-1 * sinroll * sinpitch * cosyaw + -1 * cosroll * -sinyaw);
    right[1] = (-1 * sinroll * sinpitch * sinyaw + -1 * cosroll * cosyaw);
    right[2] = -1 * sinroll * cospitch;
    up[0] = (cosroll * sinpitch * cosyaw + -sinroll * -sinyaw);
    up[1] = (cosroll * sinpitch * sinyaw + -sinroll * cosyaw);
    up[2] = cosroll * cospitch;
}

double VecMagnitude(Eigen::Vector3d& vec)
{
	return std::sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
}

double DotProduct(Eigen::Vector3d& a, Eigen::Vector3d& b)
{
	return (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
}

