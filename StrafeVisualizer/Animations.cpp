#define NOGDI
#include "Animations.h"
#include "DrawUtil.h"
#include "StrafeMath.h"
#include <iostream>
#include <random>
#include <sstream>
#include <cassert>
#include <fstream>
#define RUN_ONCE static int once = 0; if (!once && !once++)

Eigen::Vector2d Animations::moveable_points[2];
Eigen::Vector2d Animations::unfiltered_points[2];
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
        MODE_ARROWS,
        MODE_TEXT1,
        MODE_END
    };

    static Mode mode = MODE_POINTS;
    static double tolerance = 0;
    static double snappy_tolerance = 0;

    RUN_ONCE
    {
      unfiltered_points[0] = Eigen::Vector2d(-1, 0);
      unfiltered_points[1] = Eigen::Vector2d(0, 1);
    }

    //Get points
    const Eigen::Vector2d& ptVelocity = moveable_points[0];
    const Eigen::Vector2d& ptYaw = moveable_points[1];

    //Draw the Point
    double bounce = (!animate_out) ? DrawUtil::SmoothBounce(tolerance, 0.4, 50.0) : 1.0;

    if (mode == MODE_POINTS)
    {
        DrawUtil::DrawPoint(window, DrawUtil::center, sf::Color(255, 255, 255), 20.0 * bounce);
        DrawUtil::DrawPoint(window, ptVelocity, sf::Color(14, 60, 158), 20.0 * bounce);
        DrawUtil::DrawPoint(window, ptYaw, sf::Color(138, 23, 15), 20.0 * bounce);
    }
    else if (mode >= MODE_ARROWS)
    {
        RUN_ONCE
        {
            tolerance = 0;
        }

        DrawUtil::DrawLine(window, DrawUtil::center, ptVelocity, sf::Color(14, 60, 158), false, 20.0 * tolerance);
        DrawUtil::DrawLine(window, DrawUtil::center, ptYaw, sf::Color(138, 23, 15), false, 20.0 * tolerance);
        DrawUtil::DrawPoint(window, DrawUtil::center, sf::Color(255, 255, 255), 20.0);
        DrawUtil::DrawPoint(window, ptVelocity, sf::Color(14, 60, 158), 20.0);
        DrawUtil::DrawPoint(window, ptYaw, sf::Color(138, 23, 15), 20.0);

        sf::Font font;
        if (font.loadFromFile("fonts/Dosis-Regular.ttf"))
        {
            sf::String velxy = "velx: " + std::to_string(ptVelocity[0]) + "\nvely: " + std::to_string(ptVelocity[1]);
            DrawUtil::DrawTextSF(window, ptVelocity[0] + 15, ptVelocity[1] + 15, font, velxy, 100, sf::Color(255, 255, 255, 255 * tolerance + 0.5));
        }
    }
    if (mode == MODE_TEXT1)
    {
        sf::Vector2u screenDimensions = window.getSize();
        const int quarterRightOfScreen = screenDimensions.x * 0.75;
        //Draw a text border box on the right hand side
        DrawUtil::DrawRect(window, quarterRightOfScreen, 0, (sf::Vector2f)screenDimensions, sf::Color(61, 61, 60, 220 * tolerance));

        //Draw the text over it
        sf::Font font;
        if (font.loadFromFile("fonts/Dosis-Regular.ttf"))
        {
            sf::String velxy = "yeet";
            DrawUtil::DrawTextSF(window, quarterRightOfScreen * 1.1, 300, font, velxy, 100, sf::Color::White);
        }
    }

    UpdateTolerance(tolerance, 0.02, 0.04);
    return UpdateMode(mode);
}