#define NOGDI
#include <iostream>
#include <random>
#include <sstream>
//#include <cassert>
#include <fstream>
#include <assert.h>

#include "Animations.h"
#include "Settings.h"
#include "DrawUtil.h"
#include "StrafeMath.h"
#include "MouseSharedDefs.h"

#define RUN_ONCE static int once = 0; if (!once && !once++)
#ifndef M_PI 
#define M_PI 3.1415926535 
#endif

Eigen::Vector2d Animations::moveablePts[2];        // moveablePts is the filtered/smoothed unfilteredPts
Eigen::Vector2d Animations::unfilteredPts[2];      // unfilteredPts is some point on the screen (not pixels)
Eigen::Vector2d Animations::vecVelocity;           // moveablePts[0] multiplied by some scalar
Eigen::Vector2d Animations::vecYaw;                // Unit vector of moveablePts[1];

int Animations::frame = 0;
bool Animations::animate_out = false;
double Animations::fsmove[2];

BasePlayer*      StrafeMath::player;
CMoveData*       StrafeMath::mv;
PositionType	 StrafeMath::positionType;


static bool UpdateTolerance(double& tolerance, double tolerance_in, double tolerance_out)
{
    if (Animations::animate_out) 
    {
        tolerance = std::max(tolerance - tolerance_out, 0.0);
        return tolerance <= 0.0;
    } 
    else 
    {
        tolerance = std::min(tolerance + tolerance_in, 1.0);
    }
    return false;
}

template<typename T>
static bool UpdateMode(T& mode) 
{
    if (Animations::animate_out) 
    {
        mode = T(mode + 1);
        if (mode == T::MODE_END) 
        {
             return true;
        } 
        else 
        {
            Animations::animate_out = false;
        }
    } 
    return false;
}

inline double VecMagnitude(Eigen::Vector2d& vec)
{
    return std::sqrt(vec[0] * vec[0] + vec[1] * vec[1]);
}

bool Animations::Background(sf::RenderTarget& window, bool start_anim)
{
    static double tolerance = 0.0;
    DrawUtil::DrawGrid(window, tolerance);
    if (start_anim) 
    {
        tolerance = std::min(tolerance + 0.04, 1.0);
    }
    return animate_out;
}

bool Animations::WishVelDemonstration(sf::RenderTarget& window)
{
    RUN_ONCE
    {
      unfilteredPts[0] = Eigen::Vector2d(1, 0);
      unfilteredPts[1] = Eigen::Vector2d(1, 0);
    }

    enum Mode 
    {
        MODE_POINTS,
        MODE_TEXT1,
        MODE_ARROWS,
        MODE_TEXT2,
        MODE_VIEWANGLES,
        MODE_KEYS,
        MODE_EYEANGLES,
        MODE_WISHVEL_EYEANGLES,
        MODE_END
    };

    static Mode mode = MODE_POINTS;
    sf::Vector2u screenDimensions = window.getSize();

    static double tolerance = 0;
    double bounce = (!animate_out) ? DrawUtil::SmoothBounce(tolerance, 0.4, 50.0) : 1.0;

    // Get points
    const Eigen::Vector2d& ptVelocity = moveablePts[0];
    const Eigen::Vector2d& ptYaw = moveablePts[1];

    Animations::vecVelocity[0] = Animations::moveablePts[0][0] * DrawUtil::scale;
    Animations::vecVelocity[1] = -Animations::moveablePts[0][1] * DrawUtil::scale; //flip y axis sign

    // Make unit vector
    Animations::vecYaw[0] = Animations::moveablePts[1][0] / VecMagnitude(moveablePts[1]); //make unit vector
    Animations::vecYaw[1] = -Animations::moveablePts[1][1] / VecMagnitude(moveablePts[1]); //make unit vector

    sf::Font font;
    if (font.loadFromFile("fonts/Dosis-Regular.ttf"))
    {
        if (mode == MODE_POINTS)
        {
            DrawUtil::DrawPoint(window, DrawUtil::center, sf::Color(255, 255, 255), 20.0 * bounce);
            DrawUtil::DrawPoint(window, ptVelocity, sf::Color(14, 60, 158), 20.0 * bounce);
        }
        else if (mode == MODE_TEXT1)
        {
            RUN_ONCE
            {
                tolerance = 0;
            }
            DrawUtil::DrawPoint(window, DrawUtil::center, sf::Color(255, 255, 255), 20.0);
            DrawUtil::DrawPoint(window, ptVelocity, sf::Color(14, 60, 158), 20.0);

            sf::String text = "Choose some x,y position by\ninteracting with the points";
            DrawUtil::DrawRect(window, (screenDimensions.x / 2) - 250, 0, sf::Vector2f(500, 150), sf::Color(41, 41, 40, 240 * tolerance));
            DrawUtil::DrawTextSF(window, (screenDimensions.x / 2) - 250 + 40, 20, font, text, 40, sf::Color(255, 255, 255, 255 * tolerance));
        }
        else if (mode == MODE_ARROWS)
        {
            RUN_ONCE
            {
                tolerance = 0;
            }
            DrawUtil::DrawLine(window, DrawUtil::center, ptVelocity, sf::Color(14, 60, 158), false, 20.0 * tolerance);
            DrawUtil::DrawPoint(window, DrawUtil::center, sf::Color(255, 255, 255), 20.0);
            DrawUtil::DrawPoint(window, ptVelocity, sf::Color(14, 60, 158), 20.0);

            sf::String text = "The blue dot will represent our players velocity vector\nVectors have both direction and length.";
            DrawUtil::DrawRect(window, (screenDimensions.x / 2) - 420, 0, sf::Vector2f(840, 150), sf::Color(41, 41, 40, 240));
            DrawUtil::DrawTextSF(window, (screenDimensions.x / 2) - 420 + 20, 20, font, text, 40, sf::Color(255, 255, 255, 255 * tolerance));
        }
        else if (mode == MODE_TEXT2)
        {
            DrawUtil::DrawLine(window, DrawUtil::center, ptVelocity, sf::Color(14, 60, 158), false, 20.0);
            DrawUtil::DrawPoint(window, DrawUtil::center, sf::Color(255, 255, 255), 20.0);
            DrawUtil::DrawPoint(window, ptVelocity, sf::Color(14, 60, 158), 20.0);

            RUN_ONCE
            {
                tolerance = 0;
            }
            double speed = std::sqrt(vecVelocity[0] * vecVelocity[0] + vecVelocity[1] * vecVelocity[1]);
            sf::String velxy = "velx: " + std::to_string(vecVelocity[0]) 
                            + "\nvely: " + std::to_string(vecVelocity[1])
                            + "\nspeed: " + std::to_string(speed);

            DrawUtil::DrawRect(window, 100 + ptVelocity[0], 100 + ptVelocity[1], sf::Vector2f(500, 300), sf::Color(41, 41, 40, 230 * tolerance));
            DrawUtil::DrawTextSF(window, 130 + ptVelocity[0], 130 + ptVelocity[1], font, velxy, 60, sf::Color(255, 255, 255, 255 * tolerance + 0.5));
        }
        else if (mode == MODE_VIEWANGLES)
        {
            RUN_ONCE
            {
                tolerance = 0;
                bounce = 0;
            }
            DrawUtil::DrawLine(window, DrawUtil::center, ptYaw, sf::Color(191, 55, 10, 255 * tolerance), false, 20.0 * bounce);
            DrawUtil::DrawPoint(window, ptYaw, sf::Color(191, 55, 10), 20.0 * bounce);
            DrawUtil::DrawPoint(window, DrawUtil::center, sf::Color(255, 255, 255), 20.0 * bounce);

            sf::String text = "This vector will represent the direction we are looking towards.\n Its length does not matter since we will only use its direction";
            DrawUtil::DrawRect(window, (screenDimensions.x / 2) - 500, 0, sf::Vector2f(1000, 150), sf::Color(41, 41, 40, 240));
            DrawUtil::DrawTextSF(window, (screenDimensions.x / 2) - 500 + 20, 20, font, text, 40, sf::Color(255, 255, 255, 255 * tolerance));

            const int boxX = 70;
            const int boxY = 200;
            sf::String text2 = "Lets construct the angle we are looking at";
            sf::String text3 = "arctangent(                 ) (to deg)";
            sf::String xval = "x:" + std::to_string(vecYaw[0]);
            sf::String yval = "y:" + std::to_string(vecYaw[1]);
            sf::String theta = std::to_string(atan2f(vecYaw[1], vecYaw[0]) * (180 / M_PI)) + "°";

            DrawUtil::DrawRect(window, boxX + ptYaw[0], boxY + ptYaw[1], sf::Vector2f(740, 330), sf::Color(41, 41, 40, 230));

            DrawUtil::DrawTextSF(window, boxX + 30 + ptYaw[0], boxY + 30 + ptYaw[1], font, text2, 40, sf::Color(255, 255, 255));
            DrawUtil::DrawTextSF(window, boxX + 30 + ptYaw[0], boxY + 100 + ptYaw[1], font, text3, 60, sf::Color(255, 255, 255));
            DrawUtil::DrawTextSF(window, boxX + 300 + ptYaw[0], boxY + 95 + ptYaw[1], font, yval, 40, sf::Color(255, 255, 255));
            DrawUtil::DrawTextSF(window, boxX + 300 + ptYaw[0], boxY + 130 + ptYaw[1], font, xval, 40, sf::Color(255, 255, 255));
            DrawUtil::DrawTextSF(window, boxX + 200 + ptYaw[0], boxY + 200 + ptYaw[1], font, theta, 80, sf::Color(255, 255, 255));
        }
        else
        {
            DrawUtil::DrawLine(window, DrawUtil::center, ptYaw, sf::Color(191, 55, 10, 255), false, 20.0);
            DrawUtil::DrawPoint(window, ptYaw, sf::Color(191, 55, 10), 20.0);
            DrawUtil::DrawPoint(window, DrawUtil::center, sf::Color(255, 255, 255), 20.0);

            if (mode == MODE_KEYS)
            {
                RUN_ONCE
                {
                    tolerance = 0;
                }

                sf::String text = "We can create two variables that handle the value\nof our forward and sideways movement speed.";
                sf::String text2 = "This is so we can isolate our intended direction\n(ie. LEFT, FORWARD-RIGHT) to its components\nfor math later.\n\n";
                sf::String text3 = "Test out some keypresses with WASD!\nForwardmove: " + std::to_string(fsmove[0]) + "\nSidemove: " + std::to_string(fsmove[1]);
                DrawUtil::DrawRect(window, 0, 0, sf::Vector2f(800, 550), sf::Color(41, 41, 40, 240 * tolerance));
                DrawUtil::DrawTextSF(window, 20, 20, font, text, 40, sf::Color(255, 255, 255, 255 * tolerance));
                DrawUtil::DrawTextSF(window, 20, 150, font, text2, 40, sf::Color(255, 255, 255, 255 * tolerance));
                DrawUtil::DrawTextSF(window, 20, 350, font, text3, 40, sf::Color(255, 255, 255, 255 * tolerance));
            }
            else if (mode == MODE_EYEANGLES)
            {
                RUN_ONCE
                {
                    tolerance = 0;
                }

                Eigen::Vector3d angles, forward, right, up;
                angles[0] = 0;
                angles[1] = atan2f(vecYaw[1], vecYaw[0]) * (180 / M_PI);
                angles[2] = 0;
                AngleVectors(angles, forward, right, up);
            
                sf::String text = "Once we have isolated our intended movement direction\nWe can do the same and isolate the direction we are looking\nby using two vectors associated with the players viewangles-\ncalled the players view vectors.";
                sf::String text2 = "For this discussion, we will only be focused on the\nhorizontal plane. So, in 2 dimensions the view vectors are:\n\nUnit Forward Vector = <cos(yaw), sin(yaw)>\nUnit Sideways Vector = <sin(yaw), -cos(yaw)>";

                sf::String forwardTxt = "forward = (" + std::to_string(forward[0]) + ", " + std::to_string(forward[1]) + ")";
                sf::String rightTxt = "right = (" + std::to_string(right[0]) + ", " + std::to_string(right[1]) + ")";

                DrawUtil::DrawRect(window, 0, 0, sf::Vector2f(950, 600), sf::Color(41, 41, 40, 240 * tolerance));
                DrawUtil::DrawTextSF(window, 20, 20, font, text, 40, sf::Color(255, 255, 255, 255 * tolerance));
                DrawUtil::DrawTextSF(window, 20, 300, font, text2, 40, sf::Color(255, 255, 255, 255 * tolerance));

                DrawUtil::DrawRect(window, 0, 700, sf::Vector2f(950, 170), sf::Color(41, 41, 40, 240 * tolerance));
                DrawUtil::DrawTextSF(window, 120, 720, font, forwardTxt, 50, sf::Color(255, 255, 255, 255 * tolerance));
                DrawUtil::DrawTextSF(window, 120, 780, font, rightTxt, 50, sf::Color(255, 255, 255, 255 * tolerance));
            }
            else if (mode == MODE_WISHVEL_EYEANGLES)
            {
                RUN_ONCE
                {
                    tolerance = 0;
                }

                Eigen::Vector3d wishvel;
                Eigen::Vector3d forward, right, up;
                Eigen::Vector3d angles = Eigen::Vector3d(0, atan2f(vecYaw[1], vecYaw[0]) * (180 / M_PI), 0);

                // Sets forward, right
                AngleVectors(angles, forward, right, up);

                // Zero out z components of movement vectors
                forward[2] = 0;
                right[2] = 0;
                forward.normalize();
                right.normalize();

                for (int i = 0; i < 2; i++)
                {
                    wishvel[i] = forward[i] * fsmove[0] + right[i] * fsmove[1];
                }
                wishvel[2] = 0; // Zero out z part of velocity

                double wishspeed = VecMagnitude(wishvel);
                Eigen::Vector2d ptWishVel = Eigen::Vector2d(wishvel[0] / wishspeed, -wishvel[1] / wishspeed);

                DrawUtil::DrawLine(window, DrawUtil::center, ptWishVel, sf::Color(122, 235, 52), false, 20.0);
                DrawUtil::DrawPoint(window, ptWishVel, sf::Color(122, 235, 52), 20.0);
                DrawUtil::DrawPoint(window, DrawUtil::center, sf::Color(255, 255, 255), 20.0);

                sf::String text = "When pressing A, the player is essentially saying\n'I want to move left relative to where i'm looking'";
                sf::String text2 = "Since we have the intended direction based on keypresses,\nand the view vectors representing where we are looking,\nWe can now multiply these by their components and get the\nresulting vector, which we call the 'Wish velocity' or\nthe 'Acceleration vector'\n\nTry out some keypresses to see the Wish velocity!";
                sf::String text3 = "wishvel.x = forward.x * forwardmove + right.x * sidemove\nwishvel.y = forward.y * forwardmove + right.y * sidemove";
                sf::String wishvelXText = "wishvel.x = " + std::to_string(forward[0]) + " * " + std::to_string(fsmove[0]) + " + " + std::to_string(right[0]) + " * " + std::to_string(fsmove[1]) + " = " + std::to_string(wishvel[0]);
                sf::String wishvelYText = "wishvel.y = " + std::to_string(forward[1]) + " * " + std::to_string(fsmove[0]) + " + " + std::to_string(right[1]) + " * " + std::to_string(fsmove[1]) + " = " + std::to_string(wishvel[1]);

                DrawUtil::DrawRect(window, 0, 0, sf::Vector2f(950, 550), sf::Color(41, 41, 40, 240 * tolerance));
                DrawUtil::DrawTextSF(window, 20, 20, font, text, 40, sf::Color(255, 255, 255, 255 * tolerance));
                DrawUtil::DrawTextSF(window, 20, 150, font, text2, 40, sf::Color(255, 255, 255, 255 * tolerance));

                DrawUtil::DrawRect(window, 0, 600, sf::Vector2f(950, 300), sf::Color(41, 41, 40, 240 * tolerance));
                DrawUtil::DrawTextSF(window, 20, 620, font, text3, 40, sf::Color(255, 255, 255, 255 * tolerance));
                DrawUtil::DrawTextSF(window, 20, 750, font, wishvelXText, 40, sf::Color(255, 255, 255, 255 * tolerance));
                DrawUtil::DrawTextSF(window, 20, 800, font, wishvelYText, 40, sf::Color(255, 255, 255, 255 * tolerance));
            }
        }
    }
    UpdateTolerance(tolerance, 0.02, 0.04);
    return UpdateMode(mode);
}

bool Animations::PerfAngleDemo(sf::RenderTarget& window)
{
    RUN_ONCE
    {
      unfilteredPts[0] = Eigen::Vector2d(1, 0);
      unfilteredPts[1] = Eigen::Vector2d(1, 0);
    }

    enum Mode
    {
        MODE_TEXT1,
        MODE_TRIANGLE,
        MODE_VELOCITY,
        MODE_WISHVEL,
        MODE_NEXTVEL,
        MODE_TEXT2,
        MODE_END
    };

    static Mode mode = MODE_TEXT1;

    static double tolerance = 0;
    double bounce = (!animate_out) ? DrawUtil::SmoothBounce(tolerance, 0.45, 45.0) : 1.0;

    // Get points
    const Eigen::Vector2d& ptVelocity = moveablePts[0];
    const Eigen::Vector2d& ptYaw = moveablePts[1];

    sf::Vector2u screenDimensions = window.getSize();

    Animations::vecVelocity[0] = Animations::moveablePts[0][0] * DrawUtil::scale;
    Animations::vecVelocity[1] = -Animations::moveablePts[0][1] * DrawUtil::scale; //flip y axis sign

    // Make unit vector
    Animations::vecYaw[0] = Animations::moveablePts[1][0] / VecMagnitude(moveablePts[1]);
    Animations::vecYaw[1] = -Animations::moveablePts[1][1] / VecMagnitude(moveablePts[1]);

    sf::Font font;
    if (font.loadFromFile("fonts/Dosis-Regular.ttf"))
    {
        if (mode >= MODE_TEXT1)
        {
            static double anim = 0;
            static double animDelta = 0;

            Eigen::Vector2d left(-7, 0);
            Eigen::Vector2d ptTriangleSide(left[0] + anim, 0);
            Eigen::Vector2d ptTriangleTip(left[0] + anim, -1);

            if (mode == MODE_TRIANGLE || mode == MODE_TEXT1)
            {
                DrawUtil::DrawLine(window, left, ptTriangleSide, sf::Color::White, false, 10.0 * bounce);
                DrawUtil::DrawLine(window, left, ptTriangleTip, sf::Color::White, false, 10.0 * bounce);
                DrawUtil::DrawLine(window, ptTriangleSide, ptTriangleTip, sf::Color::White, false, 10.0 * bounce);

                if (mode == MODE_TEXT1)
                {
                    sf::String text = "This triangle will show why high velocities require 'slower' mouse movements\n";
                    DrawUtil::DrawRect(window, 200, 200, sf::Vector2f(400, 150), sf::Color(41, 41, 40, 240));
                    DrawUtil::DrawTextSF(window, 180, 220, font, text, 40, sf::Color(255, 255, 255, 255));
                }
            }
            else if (mode == MODE_VELOCITY)
            {
                RUN_ONCE
                {
                    tolerance = 0;
                    bounce = 0;
                }
                DrawUtil::DrawLine(window, left, ptTriangleSide, sf::Color(14, 60, 158), false, 10.0);
                DrawUtil::DrawLine(window, left, ptTriangleTip, sf::Color::White, false, 10.0);
                DrawUtil::DrawLine(window, ptTriangleSide, ptTriangleTip, sf::Color::White, false, 10.0);

                DrawUtil::DrawPoint(window, ptTriangleSide, sf::Color(14, 60, 158), 20 * bounce);

                sf::String text = "Velocity";
                DrawUtil::DrawRect(window, 100, 800, sf::Vector2f(400, 200), sf::Color(41, 41, 40, 240 * tolerance));
                DrawUtil::DrawTextSF(window, 120, 820, font, text, 40, sf::Color(65, 111, 196, 255 * tolerance));
            }
            else if (mode == MODE_WISHVEL)
            {
                RUN_ONCE
                {
                    tolerance = 0;
                    bounce = 0;
                }
                DrawUtil::DrawLine(window, left, ptTriangleSide, sf::Color(14, 60, 158), false, 10.0);
                DrawUtil::DrawLine(window, left, ptTriangleTip, sf::Color::White, false, 10.0);
                DrawUtil::DrawLine(window, ptTriangleSide, ptTriangleTip, sf::Color(122, 235, 52), false, 10.0);

                DrawUtil::DrawPoint(window, ptTriangleSide, sf::Color(14, 60, 158), 20);
                DrawUtil::DrawPoint(window, ptTriangleTip, sf::Color(122, 235, 52), 20 * bounce);

                sf::String text = "Velocity";
                sf::String text2 = "Wishvel";
                DrawUtil::DrawRect(window, 100, 800, sf::Vector2f(400, 200), sf::Color(41, 41, 40, 240));
                DrawUtil::DrawTextSF(window, 120, 820, font, text, 40, sf::Color(65, 111, 196));
                DrawUtil::DrawTextSF(window, 120, 870, font, text2, 40, sf::Color(122, 235, 52, 255 * tolerance));
            }
            else if (mode == MODE_NEXTVEL || mode == MODE_TEXT1)
            {
                RUN_ONCE
                {
                    tolerance = 0;
                    bounce = 0;
                }
                DrawUtil::DrawLine(window, left, ptTriangleSide, sf::Color(14, 60, 158), false, 10.0);
                DrawUtil::DrawLine(window, left, ptTriangleTip, sf::Color(245, 197, 103), false, 10.0);
                DrawUtil::DrawLine(window, ptTriangleSide, ptTriangleTip, sf::Color(122, 235, 52), false, 10.0);

                DrawUtil::DrawPoint(window, ptTriangleSide, sf::Color(14, 60, 158), 20);
                DrawUtil::DrawPoint(window, ptTriangleTip, sf::Color(122, 235, 52), 20);

                sf::String text = "Velocity";
                sf::String text2 = "Wishvel";
                sf::String text3 = "Next Velocity";
                DrawUtil::DrawRect(window, 100, 800, sf::Vector2f(400, 200), sf::Color(41, 41, 40, 240));
                DrawUtil::DrawTextSF(window, 120, 820, font, text, 40, sf::Color(65, 111, 196));
                DrawUtil::DrawTextSF(window, 120, 870, font, text2, 40, sf::Color(122, 235, 52));
                DrawUtil::DrawTextSF(window, 120, 920, font, text3, 40, sf::Color(245, 197, 103, 255 * tolerance));

                if (mode == MODE_TEXT2)
                {
                    sf::String text = "Notice how this triangle";
                    DrawUtil::DrawRect(window, (screenDimensions.x / 2) - 200, 800, sf::Vector2f(400, 150), sf::Color(41, 41, 40, 240 * tolerance));
                    DrawUtil::DrawTextSF(window, (screenDimensions.x / 2) - 180, 820, font, text, 40, sf::Color(255, 255, 255, 255 * tolerance));
                }
            }

            if (anim > fabs(left[0] * 2))
            {
                ptTriangleSide[0] = 0;
                ptTriangleTip[0] = 0;
                anim = 0;
                animDelta = 0;
            }

            anim += animDelta;
            animDelta += 0.001;
        }
    }

    UpdateTolerance(tolerance, 0.02, 0.04);
    return UpdateMode(mode);
}