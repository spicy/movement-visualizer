#define NOGDI
#include <iostream>
#include <random>
#include <sstream>
#include <cassert>
#include <fstream>

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

BasePlayer*     StrafeMath::player;
CMoveData*      StrafeMath::mv;
PositionType	StrafeMath::positionType;


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

static inline double IntersectT(const Eigen::Vector2d& p1, const Eigen::Vector2d& v1, const Eigen::Vector2d& p2, const Eigen::Vector2d& v2)
{
    Eigen::Matrix2d x;
    x << v1, v2;
    return (x.inverse() * (p2 - p1))(0);
}

static inline Eigen::Vector2d Intersect(const Eigen::Vector2d& p1, const Eigen::Vector2d& v1, const Eigen::Vector2d& p2, const Eigen::Vector2d& v2) 
{
    return p1 + IntersectT(p1, v1, p2, v2)*v1;
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

bool Animations::ShowVectors(sf::RenderTarget& window)
{
    enum Mode 
    {
        MODE_POINTS,
        MODE_TEXT1,
        MODE_ARROWS,
        MODE_TEXT2,
        MODE_EYEANGLES,
        MODE_TEXT3,
        MODE_KEYS,
        MODE_WISHDIR,
        MODE_WISHVEL_EYEANGLES,
        MODE_END
    };

    static Mode mode = MODE_POINTS;
    static double tolerance = 0;
    static double snappy_tolerance = 0;

    sf::Vector2u screenDimensions = window.getSize();

    RUN_ONCE
    {
      unfilteredPts[0] = Eigen::Vector2d(1, 0);
      unfilteredPts[1] = Eigen::Vector2d(1, 0);
    }

    //Get points
    const Eigen::Vector2d& ptVelocity = moveablePts[0];
    const Eigen::Vector2d& ptYaw = moveablePts[1];

    double bounce = (!animate_out) ? DrawUtil::SmoothBounce(tolerance, 0.4, 50.0) : 1.0;

    Animations::vecVelocity[0] = Animations::moveablePts[0][0] * DrawUtil::scale;
    Animations::vecVelocity[1] = -Animations::moveablePts[0][1] * DrawUtil::scale; //flip y axis sign

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
            DrawUtil::DrawRect(window, (screenDimensions.x / 2) - 250, 0, sf::Vector2f(500, 150), sf::Color(61, 61, 60, 240 * tolerance));
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
            DrawUtil::DrawRect(window, (screenDimensions.x / 2) - 420, 0, sf::Vector2f(840, 150), sf::Color(61, 61, 60, 240));
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
            DrawUtil::DrawRect(window, 100 + ptVelocity[0], 100 + ptVelocity[1], sf::Vector2f(500, 300), sf::Color(61, 61, 60, 230 * tolerance));
            DrawUtil::DrawTextSF(window, 130 + ptVelocity[0], 130 + ptVelocity[1], font, velxy, 60, sf::Color(255, 255, 255, 255 * tolerance + 0.5));
        }
        else if (mode >= MODE_EYEANGLES)
        {
            RUN_ONCE
            {
                tolerance = 0;
                bounce = 0;
            }
            DrawUtil::DrawLine(window, DrawUtil::center, ptYaw, sf::Color(191, 55, 10, 255 * tolerance), false, 20.0 * bounce);
            DrawUtil::DrawPoint(window, ptYaw, sf::Color(191, 55, 10), 20.0 * bounce);
            DrawUtil::DrawPoint(window, DrawUtil::center, sf::Color(255, 255, 255), 20.0 * bounce);

            if (mode == MODE_EYEANGLES)
            {
                sf::String text = "This vector will represent the direction we are looking towards.\n Its length does not matter since we will only use its direction";
                DrawUtil::DrawRect(window, (screenDimensions.x / 2) - 500, 0, sf::Vector2f(1000, 150), sf::Color(61, 61, 60, 240));
                DrawUtil::DrawTextSF(window, (screenDimensions.x / 2) - 500 + 20, 20, font, text, 40, sf::Color(255, 255, 255, 255 * tolerance));

                const int boxX = 70;
                const int boxY = 200;

                sf::String text2 = "Lets construct the angle we are looking at";
                sf::String text3 = "arctangent(              ) (to deg)";
                sf::String xval = std::to_string(vecYaw[0]);
                sf::String yval = std::to_string(vecYaw[1]);
                sf::String theta = std::to_string(atan2f(vecYaw[1], vecYaw[0]) * (180 / M_PI)) + "°";
                DrawUtil::DrawRect(window, boxX + ptYaw[0], boxY + ptYaw[1], sf::Vector2f(700, 330), sf::Color(61, 61, 60, 230 * tolerance));
                DrawUtil::DrawTextSF(window, boxX + 30 + ptYaw[0], boxY + 30 + ptYaw[1], font, text2, 40, sf::Color(255, 255, 255, 255 * tolerance));
                DrawUtil::DrawTextSF(window, boxX + 30 + ptYaw[0], boxY + 100 + ptYaw[1], font, text3, 60, sf::Color(255, 255, 255, 255 * tolerance));
                DrawUtil::DrawTextSF(window, boxX + 300 + ptYaw[0], boxY + 95 + ptYaw[1], font, yval, 40, sf::Color(255, 255, 255, 255 * tolerance));
                DrawUtil::DrawTextSF(window, boxX + 300 + ptYaw[0], boxY + 130 + ptYaw[1], font, xval, 40, sf::Color(255, 255, 255, 255 * tolerance));
                DrawUtil::DrawTextSF(window, boxX + 140 + ptYaw[0], boxY + 200 + ptYaw[1], font, theta, 80, sf::Color(255, 255, 255, 255 * tolerance));
            }
        }
    }
    UpdateTolerance(tolerance, 0.02, 0.04);
    return UpdateMode(mode);
}

