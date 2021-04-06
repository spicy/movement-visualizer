#include <Eigen/Dense>

class CMoveData
{
public:
	bool			m_bGameCodeMovedPlayer : 1;

	Eigen::Vector3d m_vecViewAngles;	// Command view angles (local space)
	Eigen::Vector3d m_vecAbsViewAngles;	// Command view angles (world space)
	int				m_nButtons;			// Attack buttons.
	int				m_nOldButtons;		// From host_client->oldbuttons;
	float			m_flForwardMove;
	float			m_flSideMove;
	float			m_flUpMove;

	float			m_flMaxSpeed;
	float			m_flClientMaxSpeed;

	// Variables from the player edict (sv_player) or entvars on the client.
	// These are copied in here before calling and copied out after calling.
	Eigen::Vector3d m_vecVelocity;		// edict::velocity		// Current movement direction.
	Eigen::Vector3d	m_vecAngles;		// edict::angles
	Eigen::Vector3d m_vecOldAngles;

	// Output only
	float			m_outStepHeight;	// how much you climbed this move
	Eigen::Vector3d	m_outWishVel;		// This is where you tried 
	Eigen::Vector3d	m_outJumpVel;		// This is your jump velocity

	void  SetAbsOrigin(const Eigen::Vector3d& vec);
	const Eigen::Vector3d& GetAbsOrigin() const;

private:
	Eigen::Vector3d m_vecAbsOrigin;		// edict::origin
};

void CMoveData::SetAbsOrigin(const Eigen::Vector3d& vec)
{
	m_vecAbsOrigin = vec;
}

inline const Eigen::Vector3d& CMoveData::GetAbsOrigin() const
{
	return m_vecAbsOrigin;
}

class BasePlayer
{
public:
	Eigen::Vector3d		m_vecForward;
	Eigen::Vector3d		m_vecRight;
	Eigen::Vector3d		m_vecUp;

	Eigen::Vector3d		m_vecVelocity;
	Eigen::Vector3d		m_vecWishVel;

	float				m_surfaceFriction;
};
