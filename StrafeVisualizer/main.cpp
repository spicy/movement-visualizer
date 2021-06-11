#define NOGDI
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <iostream>
#include <random>
#include <sstream>
#include <cassert>
#include <fstream>

#include "Animations.h"
#include "DrawUtil.h"
#include "Settings.h"
#include "MouseSharedDefs.h"
#include "convars.h"

int cur_anim = -1;
const double pt_smoothing = 0.7;

typedef bool (*sfFuncPtr)(sf::RenderTarget& window);
static sfFuncPtr ANIM_ARRAY[] =
{ 
  Animations::PerfAngleDemo,
  Animations::WishVelDemonstration
};

static const int NUM_ANIMS = sizeof(ANIM_ARRAY) / sizeof(ANIM_ARRAY[0]);

static void ActivatePoint(const sf::RenderTarget& window) 
{
  const Eigen::Vector2d p = DrawUtil::FromPix(window, mouse.pos * settings.video.render_scale);
  for (int i = 0; i < 2; ++i) 
  {
    const double d = (Animations::moveablePts[i] - p).norm();
    if (d * DrawUtil::scale < 10.0 * settings.video.render_scale)
    {
      mouse.select = i;
      return;
    }
  }
  mouse.select = -1;
}

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
// windows main
INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) 
{
#else
int main(int argc, char *argv[]) {
#endif
    //Get the screen size
    sf::VideoMode screenSize = sf::VideoMode::getDesktopMode();
    screenSize = sf::VideoMode(settings.video.start_w, settings.video.start_h, screenSize.bitsPerPixel);
    DrawUtil::render_scale = float(settings.video.render_scale) * 0.5f;
    
    //GL settings
    sf::ContextSettings cSettings;
    cSettings.depthBits = 24;
    cSettings.stencilBits = 8;
    cSettings.antialiasingLevel = 16;
    cSettings.majorVersion = 2;
    cSettings.minorVersion = 0;

    //Create the window
    sf::RenderWindow window;
    sf::Uint32 window_style = (settings.video.start_fullscreen ? sf::Style::Fullscreen : sf::Style::Resize | sf::Style::Close);
    window.create(screenSize, "Strafe Visualizer by spicy", window_style, cSettings);
    window.setFramerateLimit(60);
    window.requestFocus();
    sf::View view = window.getDefaultView();

    //Create the render texture 4 times larger than the window
    sf::RenderTexture renderTexture;
    renderTexture.create(window.getSize().x * settings.video.render_scale, window.getSize().y * settings.video.render_scale, cSettings);
    renderTexture.setSmooth(true);
    renderTexture.setActive(true);

    //Setup OpenGL things
    glHint(GL_POINT_SMOOTH, GL_NICEST);
    glHint(GL_LINE_SMOOTH, GL_NICEST);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_SMOOTH);

    //Create geometry
    DrawUtil::center = Eigen::Vector2d::Zero();
    DrawUtil::scale = 75 * settings.video.render_scale; //130 before
    Animations::unfilteredPts[0] = Eigen::Vector2d(1, 0);
    Animations::unfilteredPts[1] = Eigen::Vector2d(1, 0);

    for (int i = 0; i < 2; ++i)
    {
        Animations::moveablePts[i] = Animations::unfilteredPts[i];
    }

    //Main Loop
    while (window.isOpen()) 
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
                break;
            }
            else if (event.type == sf::Event::KeyPressed)
            {
                sf::Keyboard::Key keycode = event.key.code;
                if (keycode == sf::Keyboard::Escape)
                {
                    window.close();
                    break;
                }
                else if (keycode == sf::Keyboard::Space)
                {
                    if (cur_anim > 0)
                    {
                        Animations::animate_out = true;
                    }
                    else
                    {
                        cur_anim += 1;
                    }
                }

                // Create local vars to handle counter strafing (W+S/A+D)
                double fwdmv = 0, sidemv = 0;

                switch (keycode)
                {
                case sf::Keyboard::W:
                    fwdmv += sv_walkspeed;
                    Animations::fsmove[0] = fwdmv;
                    break;
                case sf::Keyboard::S:
                    fwdmv -= sv_walkspeed;
                    Animations::fsmove[0] = fwdmv;
                    break;
                case sf::Keyboard::A:
                    sidemv -= sv_walkspeed;
                    Animations::fsmove[1] = sidemv;
                    break;
                case sf::Keyboard::D:
                    sidemv += sv_walkspeed;
                    Animations::fsmove[1] = sidemv;
                    break;
                }
            }
            else if (event.type == sf::Event::KeyReleased)
            {
                sf::Keyboard::Key keycode = event.key.code;

                switch (keycode)
                {
                case sf::Keyboard::W:
                    Animations::fsmove[0] = 0;
                    break;
                case sf::Keyboard::S:
                    Animations::fsmove[0] = 0;
                    break;
                case sf::Keyboard::A:
                    Animations::fsmove[1] = 0;
                    break;
                case sf::Keyboard::D:
                    Animations::fsmove[1] = 0;
                    break;
                }
            }
            else if (event.type == sf::Event::Resized)
            {
                const sf::FloatRect visibleArea(0, 0, (float)event.size.width, (float)event.size.height);
                window.setView(sf::View(visibleArea));
                renderTexture.create(window.getSize().x * settings.video.render_scale, window.getSize().y * settings.video.render_scale, cSettings);
                renderTexture.setSmooth(true);
                renderTexture.setActive(true);
            }
            else if (event.type == sf::Event::MouseMoved)
            {
                 mouse.pos = sf::Vector2i(event.mouseMove.x, event.mouseMove.y);
            }
            else if (event.type == sf::Event::MouseButtonPressed)
            {
                mouse.pos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
                mouse.pressed = true;
                ActivatePoint(renderTexture);
            }
            else if (event.type == sf::Event::MouseButtonReleased)
            {
                mouse.pos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
                mouse.pressed = false;
                mouse.select = -1;
            }
        }

        //Move active point
        if (mouse.select >= 0)
        {
            Animations::unfilteredPts[mouse.select] = DrawUtil::FromPix(renderTexture, mouse.pos * settings.video.render_scale);
        }

        //Filter points
        for (int i = 0; i < 2; ++i) 
        {
            Animations::moveablePts[i] *= pt_smoothing;
            Animations::moveablePts[i] += Animations::unfilteredPts[i] * (1.0 - pt_smoothing);
        }

        //Draw the background
        renderTexture.setActive(true);
        Animations::Background(renderTexture, cur_anim >= 0);

        //Draw the foreground
        if (cur_anim > 0 && cur_anim <= NUM_ANIMS && ANIM_ARRAY[cur_anim - 1](renderTexture)) 
        {
           Animations::animate_out = false;
           cur_anim += 1;
        }

        //Finish drawing to the texture
        renderTexture.display();

        //Draw texture to window
        window.setActive(true);
        const sf::Texture& texture = renderTexture.getTexture();
        sf::Sprite sprite(texture);
        sprite.setScale(1.0f / float(settings.video.render_scale), 1.0f / float(settings.video.render_scale));
        window.draw(sprite);

        //Flip the screen buffer
        window.display();
        Animations::frame += 1;
    }

  return 0;
}
