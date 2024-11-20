/**
 * OpenGL/SDL Hello World example for Guichan.
 */

// Include all necessary headers.
#include <iostream>
#include <guichan.hpp>
#include <guichan/sdl.hpp>
#include <guichan/opengl.hpp>
// A class used to load images for OpenGL using SDL
#include <guichan/opengl/openglsdlimageloader.hpp>
#include "SDL.h"

#if defined (_WIN32)
#include <windows.h>
// Sometimes windows.h defines DELETE which causes a compilation
// error in a Guichan header.
#if defined (DELETE)
#undef DELETE 
#endif
#endif

#if defined (__amigaos4__)
#include <mgl/gl.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

/*
 * Common stuff we need
 */
bool running = true;

/*
 * SDL Stuff we need
 */
SDL_Surface* screen;
SDL_Event event;

/*
 * Guichan OpenGL/SDL stuff we need
 */
gcn::SDLInput* input;                    // Input driver
gcn::OpenGLGraphics* graphics;           // Graphics driver
gcn::OpenGLSDLImageLoader* imageLoader;  // For loading images

/*
 * Guichan stuff we need
 */
gcn::Gui* gui;            // A Gui object - binds it all together
gcn::Container* top;      // A top container
gcn::ImageFont* font;     // A font
gcn::Label* label;        // And a label for the Hello World text

/**
 * Initializes the Hello World
 */
void init()
{
    /*
     * Here we initialize SDL as we would do with any SDL application.
     */
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    screen = SDL_SetVideoMode(640, 480, 32, SDL_HWSURFACE | SDL_OPENGL | SDL_HWACCEL);

    // Setup OpenGL
    glViewport(0, 0, 640, 480);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // We want unicode
    SDL_EnableUNICODE(1);
    // We want to enable key repeat
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

    /*
     * Now it's time for Guichan OpenGL/SDL stuff
     */
    imageLoader = new gcn::OpenGLSDLImageLoader();

    // The ImageLoader in use is static and must be set to be
    // able to load images
    gcn::Image::setImageLoader(imageLoader);
    graphics = new gcn::OpenGLGraphics();
    // We need to tell OpenGL graphics how big the screen is.
    graphics->setTargetPlane(640, 480);
    input = new gcn::SDLInput();

    /*
     * Last but not least it's time to initialize and create the gui
     * with Guichan stuff.
     */
    top = new gcn::Container();
    // Set the dimension of the top container to match the screen.
    top->setDimension(gcn::Rectangle(0, 0, 640, 480));
    gui = new gcn::Gui();
    // Set gui to use the OpenGLGraphics object.
    gui->setGraphics(graphics);
    // Set gui to use the SDLInput object
    gui->setInput(input);
    // Set the top container
    gui->setTop(top);
    // Load the image font.
    font = new gcn::ImageFont("fixedfont.bmp", " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    // The global font is static and must be set.
    gcn::Widget::setGlobalFont(font);

    // Create a label with test hello world
    label = new gcn::Label("Hello World");
    // Set the labels position
    label->setPosition(280, 220);
    // Add the label to the top container
    top->add(label);
}

/**
 * Halts the application
 */
void halt()
{
    /*
     * Destroy Guichan stuff
     */
    delete label;
    delete font;
    delete top;
    delete gui;

    /*
     * Destroy Guichan OpenGL/SDL stuff
     */
    delete input;
    delete graphics;
    delete imageLoader;

    /*
     * Destroy SDL stuff
     */
    SDL_Quit();
}

/**
 * Checks input. On escape halt the application.
 */
void checkInput()
{
    /*
     * Poll SDL events
     */
    while(SDL_PollEvent(&event))
    {
        if (event.type == SDL_KEYDOWN)
        {
            if (event.key.keysym.sym == SDLK_ESCAPE)
            {
                running = false;
            }
            if (event.key.keysym.sym == SDLK_f)
            {
                if (event.key.keysym.mod & KMOD_CTRL)
                {
                    // Works with X11 only
                    SDL_WM_ToggleFullScreen(screen);
                }
            }
        }
        else if(event.type == SDL_QUIT)
        {
            running = false;
        }

        /*
         * Now that we are done polling and using SDL events we pass
         * the leftovers to the SDLInput object to later be handled by
         * the Gui. (This example doesn't require us to do this 'cause a
         * label doesn't use input. But will do it anyway to show how to
         * set up an SDL application with Gui-chan.)
         */
        input->pushInput(event);
    }
}

/**
 * Runs the application
 */
void run()
{
    while(running)
    {
        // Poll input
        checkInput();
        // Let the gui perform it's logic (like handle input)
        gui->logic();
        // Draw the gui
        gui->draw();
        // Update the screen
        SDL_GL_SwapBuffers();
    }
}

int main(int argc, char **argv)
{
    try
    {
        init();
        run();
        halt();
    }
    /*
     * Catch all Gui-chan exceptions
     */
    catch (gcn::Exception e)
    {
        std::cerr << e.getMessage() << std::endl;
        return 1;
    }
    /*
     * Catch all Std exceptions
     */
    catch (std::exception e)
    {
        std::cerr << "Std exception: " << e.what() << std::endl;
        return 1;
    }
    /*
     * Catch all Unknown exceptions
     */
    catch (...)
    {
        std::cerr << "Unknown exception" << std::endl;
        return 1;
    }

    return 0;
}
