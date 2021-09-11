#include <iostream>
#include <random>
#include <sstream>

#include <fstream>
#include <assert.h>

#include "Animations.h"
#include "Settings.h"
#include "DrawUtil.h"
#include "StrafeMath.h"
#include "MouseSharedDefs.h"

#define RUN_ONCE static int once = 0; if (!once && !once++)
#ifndef PI 
#define PI 3.1415926535 
#endif

using namespace StrafeMath;

Eigen::Vector2d Animations::moveablePts[2];        // moveablePts is the filtered/smoothed unfilteredPts
Eigen::Vector2d Animations::unfilteredPts[2];      // unfilteredPts is some point on the screen (not pixels)

sf::Color textColor(0, 0, 0);
sf::Color wishVelColor(122, 235, 52);
sf::Color velocityColor(14, 60, 158);
sf::Color nextVelColor(245, 197, 103);
sf::Color viewanglesColor(191, 55, 10);
const int fontSize = 25;

int Animations::frame = 0;
bool Animations::animate_out = false;

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
        MODE_KEYS,
        MODE_VIEWANGLES,
        MODE_EYEANGLES,
        MODE_WISHVEL_EYEANGLES,
        MODE_SET_VELOCITY,
        MODE_PROCESS_MOVEMENT_TEXT,
        MODE_PROCESS_MOVEMENT,
        MODE_END
    };

    static Mode mode = MODE_KEYS;
    sf::Vector2u screenDimensions = window.getSize();

    static double tolerance = 0;
    double bounce = (!animate_out) ? DrawUtil::SmoothBounce(tolerance, 0.4, 50.0) : 1.0;

    // Get points
    const Eigen::Vector2d& ptVelocity = moveablePts[0];
    const Eigen::Vector2d& ptYaw = moveablePts[1];

    sf::Font font;
    if (font.loadFromFile("fonts/Dosis-Regular.ttf"))
    {
        if (mode == MODE_KEYS)
        {
            RUN_ONCE
            {
                tolerance = 0;
            }

            sf::String text = "We can create two variables that handle the value\nof our forward and sideways movement speed.";
            sf::String text2 = "This is so we can isolate our intended direction\n(ie. LEFT, FORWARD-RIGHT) to its components\nfor math later.\n\n";
            sf::String text3 = "Test out some keypresses with WASD!\nForwardmove: "
                + std::to_string(int(player->forwardMove)) + "\nSidemove: " + std::to_string(int(player->sideMove));

            DrawUtil::DrawRect(window, 0, 0, sf::Vector2f(520, 360), sf::Color(255, 255, 255, 255 * tolerance));
            DrawUtil::DrawTextSF(window, 20, 20, font, text, fontSize, textColor);
            DrawUtil::DrawTextSF(window, 20, 120, font, text2, fontSize, textColor);
            DrawUtil::DrawTextSF(window, 20, 240, font, text3, fontSize, textColor);
        }
        else if (mode == MODE_VIEWANGLES)
        {
            RUN_ONCE
            {
                tolerance = 0;
                bounce = 0;
            }

            DrawUtil::DrawLine(window, DrawUtil::center, ptYaw, sf::Color(191, 55, 10, 255 * tolerance), false, 20.0 * bounce);
            DrawUtil::DrawPoint(window, ptYaw, viewanglesColor, 20.0 * bounce);
            DrawUtil::DrawPoint(window, DrawUtil::center, sf::Color(255, 255, 255), 20.0 * bounce);

            sf::String text = "Choose some direction by interacting with the points. This represents\n the direction our player is looking towards. Its length does not\n matter since we will only use its direction";
            DrawUtil::DrawRect(window, (screenDimensions.x / 2) - 350, 0, sf::Vector2f(700, 140), sf::Color(255, 255, 255, 255 * tolerance));
            DrawUtil::DrawTextSF(window, (screenDimensions.x / 2) - 350 + 20, 20, font, text, fontSize, textColor);

            sf::String text2 = "Lets construct the angle we are looking at";
            sf::String text3 = "arctangent(                          ) (to deg)";
            sf::String xval = "x:" + std::to_string(player->viewAngles[0]);
            sf::String yval = "y:" + std::to_string(player->viewAngles[1]);
            sf::String theta = std::to_string(atan2f(player->viewAngles[1], 
                               player->viewAngles[0]) * (180 / PI)) + "°";

			const int xOffset = 300;
			const int yOffset = 400;
            DrawUtil::DrawRect(window, xOffset + ptYaw[0], yOffset + ptYaw[1], sf::Vector2f(440, 180), sf::Color(255, 255, 255, 255 * tolerance));
            DrawUtil::DrawTextSF(window, xOffset + 20 + ptYaw[0], yOffset + 20 + ptYaw[1], font, text2, fontSize, textColor);
            DrawUtil::DrawTextSF(window, xOffset + 20 + ptYaw[0], yOffset + 70 + ptYaw[1], font, text3, fontSize, textColor);
            DrawUtil::DrawTextSF(window, xOffset + 140 + ptYaw[0], yOffset + 60 + ptYaw[1], font, yval, fontSize, textColor);
            DrawUtil::DrawTextSF(window, xOffset + 140 + ptYaw[0], yOffset + 80 + ptYaw[1], font, xval, fontSize, textColor);
            DrawUtil::DrawTextSF(window, xOffset + 130 + ptYaw[0], yOffset + 110 + ptYaw[1], font, theta, fontSize + 10, textColor);
        }
        else if (mode == MODE_EYEANGLES)
        {
            RUN_ONCE
            {
                tolerance = 0;
            }

            DrawUtil::DrawLine(window, DrawUtil::center, ptYaw, viewanglesColor, false, 20.0);
            DrawUtil::DrawPoint(window, ptYaw, viewanglesColor, 20.0);
            DrawUtil::DrawPoint(window, DrawUtil::center, sf::Color::White, 20.0);

            Eigen::Vector3d angles, forward, right, up;

            angles = Eigen::Vector3d(0, atan2f(player->viewAngles[1], player->viewAngles[0]) * (180 / PI), 0);
            AngleVectors(angles, forward, right, up);
            
            sf::String text = "Once we have isolated our intended movement direction\nWe can do the same and isolate the direction we are looking\nby using two vectors associated with the players viewangles-\ncalled the players view vectors.";
            sf::String text2 = "For this discussion, we will only be focused on the\nhorizontal plane. So, in 2 dimensions the view vectors are:\n\nUnit Forward Vector = <cos(yaw), sin(yaw)>\nUnit Sideways Vector = <sin(yaw), -cos(yaw)>";

            sf::String forwardTxt = "forward = (" + std::to_string(forward[0]) + ", " + std::to_string(forward[1]) + ")";
            sf::String rightTxt = "right = (" + std::to_string(right[0]) + ", " + std::to_string(right[1]) + ")";

            DrawUtil::DrawRect(window, 0, 0, sf::Vector2f(620, 440), sf::Color(255, 255, 255, 255 * tolerance));
            DrawUtil::DrawTextSF(window, 20, 20, font, text, fontSize, textColor);
            DrawUtil::DrawTextSF(window, 20, 160, font, text2, fontSize, textColor);
            DrawUtil::DrawTextSF(window, 20, 340, font, forwardTxt, fontSize, textColor);
            DrawUtil::DrawTextSF(window, 20, 370, font, rightTxt, fontSize, textColor);

        }
        else if (mode == MODE_WISHVEL_EYEANGLES)
        {
            RUN_ONCE
            {
                tolerance = 0;
            }

            ManuallyUpdateStuff();

            double wishspeed = VecMagnitude(player->wishVel);
            Eigen::Vector2d ptWishVel = Eigen::Vector2d(player->wishVel[0] / wishspeed, -player->wishVel[1] / wishspeed);

            // Draw Viewangles
            DrawUtil::DrawLine(window, DrawUtil::center, ptYaw, viewanglesColor, false, 20.0);
            DrawUtil::DrawPoint(window, ptYaw, viewanglesColor, 20.0);
            DrawUtil::DrawPoint(window, DrawUtil::center, sf::Color::White, 20.0);

            // Draw WishVel
            DrawUtil::DrawLine(window, DrawUtil::center, ptWishVel, wishVelColor, false, 20.0);
            DrawUtil::DrawPoint(window, ptWishVel, wishVelColor, 20.0);
            DrawUtil::DrawPoint(window, DrawUtil::center, sf::Color::White, 20.0);

            sf::String text = "When pressing A, the player is essentially saying\n'I want to move left relative to where im looking'\n\nSince we have the intended direction based on keypresses,\nand the view vectors representing where we are looking,\nWe can now multiply these by their components and get the\nresulting vector, which we call the 'Wish velocity' or\nthe 'Acceleration vector'\n\nTry out some keypresses to see the Wish velocity!";
            // Draw the top rect
            DrawUtil::DrawRect(window, 0, 0, sf::Vector2f(620, 360), sf::Color(255, 255, 255, 255 * tolerance));
            DrawUtil::DrawTextSF(window, 20, 20, font, text, fontSize, textColor);

            sf::String text2 = "wishvel.x = forward.x * forwardmove + right.x * sidemove\nwishvel.y = forward.y * forwardmove + right.y * sidemove";
            sf::String wishvelXText = "wishvel.x = " + std::to_string(player->forward[0]) + " * " + std::to_string(int(player->forwardMove))
                + " + " + std::to_string(player->right[0]) + " * " + std::to_string(int(player->sideMove)) + " = " + std::to_string(int(player->wishVel[0]));

            sf::String wishvelYText = "wishvel.y = " + std::to_string(player->forward[1]) + " * " + std::to_string(int(player->forwardMove))
                + " + " + std::to_string(player->right[1]) + " * " + std::to_string(int(player->sideMove)) + " = " + std::to_string(int(player->wishVel[1]));

            // Draw the bottom rect
            DrawUtil::DrawRect(window, 0, 400, sf::Vector2f(620, 185), sf::Color(255, 255, 255, 255 * tolerance));
            DrawUtil::DrawTextSF(window, 20, 420, font, text2, fontSize, textColor);
            DrawUtil::DrawTextSF(window, 60, 500, font, wishvelXText, fontSize, textColor);
            DrawUtil::DrawTextSF(window, 60, 530, font, wishvelYText, fontSize, textColor);
        }
        else if (mode == MODE_SET_VELOCITY)
        {
            RUN_ONCE
            {
                tolerance = 0;
            }

            // Draw velocity
            DrawUtil::DrawLine(window, DrawUtil::center, ptVelocity, velocityColor, false, 20.0 * tolerance);
            DrawUtil::DrawPoint(window, DrawUtil::center, sf::Color(255, 255, 255), 20.0);
            DrawUtil::DrawPoint(window, ptVelocity, velocityColor, 20.0);

            // Set the players velocity corresponding to the point scaled up
            player->velocity[0] = moveablePts[0][0] * DrawUtil::scale;
            player->velocity[1] = -moveablePts[0][1] * DrawUtil::scale;

            sf::String text = "The blue dot will represent our players velocity vector\nVectors have both direction and length.";
            DrawUtil::DrawRect(window, (screenDimensions.x / 2) - 275, 0, sf::Vector2f(550, 110), sf::Color(255, 255, 255, 255 * tolerance));
            DrawUtil::DrawTextSF(window, (screenDimensions.x / 2) - 275 + 20, 20, font, text, fontSize, textColor);

            double speed = std::sqrt(player->velocity[0] * player->velocity[0]
                + player->velocity[1] * player->velocity[1]);

            sf::String velxy = "velx: " + std::to_string(player->velocity[0])
                + "\nvely: " + std::to_string(player->velocity[1])
                + "\nspeed: " + std::to_string(speed);

            DrawUtil::DrawRect(window, 100 + ptVelocity[0], 100 + ptVelocity[1],
                sf::Vector2f(220, 140), sf::Color(255, 255, 255, 255 * tolerance));
            DrawUtil::DrawTextSF(window, 120 + ptVelocity[0], 120 + ptVelocity[1], font,
                velxy, fontSize, textColor);
        }
        else if (mode >= MODE_PROCESS_MOVEMENT_TEXT)
        {
            RUN_ONCE
            {
                tolerance = 0;
            }

            // Draw Viewangles
            DrawUtil::DrawLine(window, DrawUtil::center, ptYaw, viewanglesColor, false, 20.0);
            DrawUtil::DrawPoint(window, ptYaw, viewanglesColor, 20.0);
            DrawUtil::DrawPoint(window, DrawUtil::center, sf::Color::White, 20.0);

            if (mode == MODE_PROCESS_MOVEMENT_TEXT)
            {
                sf::String text = "We can now watch how the wish velocity interacts with\nour players velocity.\n\nTry various viewangles and keypresses!";
                // Draw the top rect
                DrawUtil::DrawRect(window, 0, 0, sf::Vector2f(570, 180), sf::Color(255, 255, 255, 255 * tolerance));
                DrawUtil::DrawTextSF(window, 20, 20, font, text, fontSize, textColor);
            }

            ProcessMovement();

            // Draw Vel arrow
            Eigen::Vector2d ptVelocity = Eigen::Vector2d(player->velocity[0] / DrawUtil::scale, -player->velocity[1] / DrawUtil::scale);
            DrawUtil::DrawLine(window, DrawUtil::center, ptVelocity, velocityColor, false, 20.0);
            DrawUtil::DrawPoint(window, ptVelocity, velocityColor, 20.0);

            // Draw Vel Text
            sf::String text2 = std::to_string((int)VecMagnitude(player->velocity));
            Eigen::Vector2d ptVel(player->velocity[0] + 20, -player->velocity[1] - 20);
            DrawUtil::DrawTextSF(window, ptVel / DrawUtil::scale, font, text2, fontSize, velocityColor);

            // Draw WishVel arrow
            double wishspeed = VecMagnitude(player->wishVel);
            Eigen::Vector2d ptWishVel = Eigen::Vector2d(player->wishVel[0] / wishspeed, -player->wishVel[1] / wishspeed);
            DrawUtil::DrawLine(window, DrawUtil::center, ptWishVel, wishVelColor, false, 20.0);
            DrawUtil::DrawPoint(window, ptWishVel, wishVelColor, 20.0);

            // Draw Center point
            DrawUtil::DrawPoint(window, DrawUtil::center, sf::Color::White, 20.0);
        }
    }
    UpdateTolerance(tolerance, 0.03, 0.08);
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
        MODE_CODE_REVIEW,
        MODE_VELOCITY,
        MODE_WISHVEL,
        MODE_NEXTVEL,
        MODE_TEXT2,
        MODE_COMBINATIONS,
        MODE_ALL_PERFANGLES,
        MODE_END
    };

    static Mode mode = MODE_VELOCITY;

    static double tolerance = 0;
    double bounce = (!animate_out) ? DrawUtil::SmoothBounce(tolerance, 0.45, 45.0) : 1.0;

    // Get points
    const Eigen::Vector2d& ptVelocity = moveablePts[0];
    const Eigen::Vector2d& ptYaw = moveablePts[1];

    sf::Vector2u screenDimensions = window.getSize();

    sf::Font font;
    if (font.loadFromFile("fonts/Dosis-Regular.ttf"))
    {
        if (mode == MODE_CODE_REVIEW)
        {

        }
        else if (mode >= MODE_VELOCITY)
        {
            static double anim = 0;
            static double animDelta = 0;

            Eigen::Vector2d left(-7, 0);
            Eigen::Vector2d ptTriangleSide(-7 + anim, 0);
            Eigen::Vector2d ptTriangleTip(-7 + anim, -1);

            if (mode == MODE_VELOCITY)
            {
                RUN_ONCE
                {
                    tolerance = 0;
                    bounce = 0;
                }
                DrawUtil::DrawLine(window, left, ptTriangleSide, velocityColor, false, 10.0);
                DrawUtil::DrawLine(window, left, ptTriangleTip, sf::Color::White, false, 10.0);
                DrawUtil::DrawLine(window, ptTriangleSide, ptTriangleTip, sf::Color::White, false, 10.0);

                DrawUtil::DrawPoint(window, ptTriangleSide, velocityColor, 20 * bounce);

                sf::String text = "Velocity";
                Eigen::Vector2d point(left[0], left[1] + 0.25);
                DrawUtil::DrawTextSF(window, point, font, text, fontSize, sf::Color(65, 111, 196, 255 * tolerance));
            }
            else if (mode == MODE_WISHVEL)
            {
                RUN_ONCE
                {
                    tolerance = 0;
                    bounce = 0;
                }
                DrawUtil::DrawLine(window, left, ptTriangleSide, velocityColor, false, 10.0);
                DrawUtil::DrawLine(window, left, ptTriangleTip, sf::Color::White, false, 10.0);
                DrawUtil::DrawLine(window, ptTriangleSide, ptTriangleTip, wishVelColor, false, 10.0);

                DrawUtil::DrawPoint(window, ptTriangleSide, velocityColor, 20);
                DrawUtil::DrawPoint(window, ptTriangleTip, wishVelColor, 20 * bounce);

                sf::String text = "Velocity";
                sf::String text2 = "Wishvel";
                Eigen::Vector2d point(left[0], left[1] + 0.25);
                Eigen::Vector2d point2(left[0] + 6, left[1] + 0.25);
                DrawUtil::DrawTextSF(window, point, font, text, fontSize, sf::Color(65, 111, 196));
                DrawUtil::DrawTextSF(window, point2, font, text2, fontSize, sf::Color(122, 235, 52, 255 * tolerance));
            }
            else if (mode == MODE_NEXTVEL)
            {
                RUN_ONCE
                {
                    tolerance = 0;
                    bounce = 0;
                }
                DrawUtil::DrawLine(window, left, ptTriangleSide, velocityColor, false, 10.0);
                DrawUtil::DrawLine(window, left, ptTriangleTip, nextVelColor, false, 10.0);
                DrawUtil::DrawLine(window, ptTriangleSide, ptTriangleTip, wishVelColor, false, 10.0);

                DrawUtil::DrawPoint(window, ptTriangleSide, velocityColor, 20);
                DrawUtil::DrawPoint(window, ptTriangleTip, wishVelColor, 20);

                sf::String text  = "Velocity";
                sf::String text2 = "Wishvel";
                sf::String text3 = "Next Velocity";
                Eigen::Vector2d point(left[0], left[1] + 0.25);
                Eigen::Vector2d point2(left[0] + 6, left[1] + 0.25);
                Eigen::Vector2d point3(left[0] + 12, left[1] + 0.25);
                DrawUtil::DrawTextSF(window, point, font, text, fontSize, velocityColor);
                DrawUtil::DrawTextSF(window, point2, font, text2, fontSize, wishVelColor);
                DrawUtil::DrawTextSF(window, point3, font, text3, fontSize, sf::Color(245, 197, 103, 255 * tolerance));

                sf::String text4 = "These vectors show why higher velocities require 'slower' mouse movements.\nAs our velocity increases, the angle between our next ticks velocity and our\ncurrent velocity decreases.\n\nSince we have already proved in our engine code review that maximizing speed\ngain is done by making the wishvel perpendicular to the current velocity, we\nknow that in order to make our wishvel perpendicular we can make our view\nangle our current velocity direction offsetted some specific keypress angle.\n\nIn the normal style, the perfect angle to be looking would be our current ticks\nvelocity.";
                // of all of these keypress combinations, we can notice a pattern. Each possible key direction has a difference of 45 degrees. This means that whatever our current velocity is, there are 8 possible viewangles that allow for perfect speedgain depending on the keypresses.
                
                DrawUtil::DrawRect(window, 50, 50, sf::Vector2f(800, 400), sf::Color(255, 255, 255, 255 * tolerance));
                DrawUtil::DrawTextSF(window, 70, 70, font, text4, fontSize, textColor);
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

    UpdateTolerance(tolerance, 0.03, 0.08);
    return UpdateMode(mode);
}