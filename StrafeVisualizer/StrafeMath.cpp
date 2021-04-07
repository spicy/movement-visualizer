#include "StrafeMath.h"

#ifndef M_PI 
#define M_PI 3.1415926535 
#endif

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


void StrafeMath::ProcessMovement()
{
	PlayerMove();
	FinishMove();
}

void StrafeMath::PlayerMove()
{
	//capture keypresses
	CaptureMovementKeys();
    //start some movement
    switch (positionType)
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
	mv->m_nOldButtons = mv->m_nButtons;
}


//General movement wrapping function
void StrafeMath::FullWalkMove()
{
	StartGravity();

	// Fricion is handled before we add in any base velocity.
	switch (positionType)
	{
	case GROUND:
		player->m_vecVelocity[2] = 0;
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

	FinishGravity();

	// If we are on ground, no downward velocity.
	if (positionType == GROUND)
	{
		player->m_vecVelocity[2] = 0;
	}
	//CheckLanding();
}


//Ground movement
void StrafeMath::WalkMove()
{
	Eigen::Vector3d wishvel, wishdir;
	float wishspeed;
	float spd;

	Eigen::Vector3d dest;

	// Determine movement angles
	AngleVectors(mv->m_vecViewAngles, player->m_vecForward, player->m_vecRight, player->m_vecUp);

	// Copy movement amounts

	// Zero out z components of eye vectors
	player->m_vecForward[2] = 0;
	player->m_vecRight[2] = 0;
	player->m_vecForward.normalize();
	player->m_vecRight.normalize();

	// Determine x and y parts of velocity
	for (int i = 0; i < 2; i++)
	{
		wishvel[i] = player->m_vecForward[i] * mv->m_flForwardMove + player->m_vecRight[i] * mv->m_flSideMove;
	}

	// Zero out z part of velocity
	wishvel[2] = 0; 

	// Isolate direction component of velocity
	wishdir = wishvel;
	wishdir.normalize();

	// Isolate magnitude component of velocity
	wishspeed = VecMagnitude(wishvel);
	// Clamp to server defined max speed
	if ((wishspeed != 0.0f) && (wishspeed > mv->m_flMaxSpeed))
	{


		VectorScale(wishvel, mv->m_flMaxSpeed / wishspeed, wishvel);
		wishspeed = mv->m_flMaxSpeed;
	}

	// Set pmove velocity
	mv->m_vecVelocity[2] = 0;
	Accelerate(wishdir, wishspeed, sv_accelerate);
	mv->m_vecVelocity[2] = 0;

	spd = VecMagnitude(player->m_vecVelocity);

	// first try just moving to the destination	
	dest[0] = mv->m_vecAbsOrigin[0] + mv->m_vecVelocity[0] * intervalPerTick;
	dest[1] = mv->m_vecAbsOrigin[1] + mv->m_vecVelocity[1] * intervalPerTick;
	dest[2] = mv->m_vecAbsOrigin[2];

	// first try moving directly to the next spot
	//TracePlayerBBox(mv->GetAbsOrigin(), dest, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm);

	// If we made it all the way, then copy trace end as new player position.
	mv->m_outWishVel += wishdir * wishspeed;
}

void StrafeMath::Accelerate(Eigen::Vector3d& wishdir, float wishspeed, float accel)
{
	int i;
	float addspeed, accelspeed, currentspeed;

	// See if we are changing direction a bit

	currentspeed = DotProduct(mv->m_vecVelocity, wishdir);

	// Reduce wishspeed by the amount of veer.
	addspeed = wishspeed - currentspeed;

	// If not going to add any speed, done.
	if (addspeed <= 0)
		return;

	// Determine amount of accleration.
	accelspeed = accel * intervalPerTick * wishspeed * player->m_surfaceFriction;

	// Cap at addspeed
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	// Adjust velocity.
	for (i = 0; i < 3; i++)
	{
		mv->m_vecVelocity[i] += accelspeed * wishdir[i];
	}

}

void StrafeMath::Friction()
{
	float speed, newspeed, control;
	float friction;
	float drop;

	// speed is magnitude of player velocity
	speed = VecMagnitude(player->m_vecVelocity);
	// If too slow, return
	if (speed < 0.1f)
	{
		return;
	}

	drop = 0;
	// apply ground friction
	if (positionType == GROUND)  // On an entity that is the ground
	{
		friction = sv_friction * player->m_surfaceFriction;

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
			player->m_vecVelocity[i] * newspeed;
		}
	}

	mv->m_outWishVel -= (1.f - newspeed) * player->m_vecVelocity;
}


//Air movement
void StrafeMath::AirMove()
{
	int			i;
	Eigen::Vector3d wishvel, wishdir;
	float		fmove, smove;
	float		wishspeed;
	Eigen::Vector3d forward, right, up;

	AngleVectors(mv->m_vecViewAngles, player->m_vecForward, player->m_vecRight, player->m_vecUp);

	// Copy movement amounts
	fmove = mv->m_flForwardMove;
	smove = mv->m_flSideMove;

	// Zero out z components of movement vectors
	forward[2] = 0;
	right[2] = 0;
	forward.normalize();
	right.normalize();

	for (i = 0; i < 2; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i] * fmove + right[i] * smove;

	wishvel[2] = 0;             // Zero out z part of velocity

	wishdir = wishvel;
	wishdir.normalize();

	wishspeed = VecMagnitude(wishvel);

	if (wishspeed != 0 && (wishspeed > mv->m_flMaxSpeed))
	{
		VectorScale(wishvel, mv->m_flMaxSpeed / wishspeed, wishvel);
		wishspeed = mv->m_flMaxSpeed;
	}

	AirAccelerate(wishdir, wishspeed, sv_airAccelerate);
	//TryPlayerMove();
}

void StrafeMath::AirAccelerate(Eigen::Vector3d& wishdir, float wishspeed, float accel)
{
	int i;
	float addspeed, accelspeed, currentspeed;
	float wishspd;

	wishspd = wishspeed;

	// Cap speed to 30
	if (wishspd > airSpeedCap)
		wishspd = airSpeedCap;

	// Determine veer amount
	currentspeed = DotProduct(mv->m_vecVelocity, wishdir);

	// See how much to add
	addspeed = wishspd - currentspeed;

	// If not adding any, done.
	if (addspeed <= 0)
		return;

	// Determine acceleration speed after acceleration
	accelspeed = accel * wishspeed * intervalPerTick * player->m_surfaceFriction;

	// Cap it
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	// Adjust pmove vel.
	for (i = 0; i < 3; i++)
	{
		mv->m_vecVelocity[i] += accelspeed * wishdir[i];
		mv->m_outWishVel[i] += accelspeed * wishdir[i];
	}
}


void StrafeMath::CheckVelocity()
{
	for (int i = 0; i < 3; i++)
	{
		// Check if it's INF
		if (player->m_vecVelocity[i] == NAN)
		{
			player->m_vecVelocity[i] = 0;
		}

		// Cap its value
		if (player->m_vecVelocity[i] > sv_maxVelocity)
		{
			player->m_vecVelocity[i] = sv_maxVelocity;
		}
		else if (player->m_vecVelocity[i] < -sv_maxVelocity)
		{
			player->m_vecVelocity[i] = -sv_maxVelocity;
		}
	}
}

void StrafeMath::CaptureMovementKeys()
{
		//if pressing W
	//player.forwardmove += sv_forwardspeed;
		//if pressing S
	//player.forwardmove -= sv_forwardspeed;
		//if pressing A
	//player.sidemove += sv_sidespeed;
		//if pressing D
	//player.sidemove -= sv_sidespeed;
}


//Gravity
void StrafeMath::StartGravity()
{
	float ent_gravity;
	//mv->m_fl
	if (player->m_flGravity)
		ent_gravity = player->m_flGravity;
	else
		ent_gravity = 1.0;

	// Add gravity so they'll be in the correct position during movement
	// yes, this 0.5 looks wrong, but it's not.  
	mv->m_vecVelocity[2] -= (ent_gravity * sv_gravity * 0.5 * intervalPerTick);

	CheckVelocity();
}

void StrafeMath::FinishGravity()
{
	float ent_gravity;

	if (player->m_flGravity)
		ent_gravity = player->m_flGravity;
	else
		ent_gravity = 1.0;

	// Get the correct velocity for the end of the dt 
	mv->m_vecVelocity[2] -= (ent_gravity * sv_gravity * intervalPerTick * 0.5);

	CheckVelocity();
}


//Other
void StrafeMath::AngleVectors(Eigen::Vector3d& angles, Eigen::Vector3d& forward, Eigen::Vector3d& right, Eigen::Vector3d& up)
{
    float angle;
    static float sinpitch, sinyaw, sinroll, cospitch, cosyaw, cosroll;

    angle = angles[0] * (M_PI / 180);
    sinpitch = sin(angle);
    cospitch = cos(angle);
    angle = angles[1] * (M_PI / 180);
    sinyaw = sin(angle);
    cosyaw = cos(angle);
    angle = angles[2] * (M_PI / 180);
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

double StrafeMath::VecMagnitude(Eigen::Vector3d& vec)
{
	return std::sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
}

double StrafeMath::DotProduct(Eigen::Vector3d& a, Eigen::Vector3d& b)
{
	return (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
}

