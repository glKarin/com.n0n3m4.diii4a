/*
 * Code that sets up an OpenGL application with Guichan using the 
 * Guichan OpenGL back end and the SDL back end (as OpenGL cannot
 * load images nor check for user input an additional back end needs
 * to be used).
 */

#include <guichan.hpp>
#include <guichan/opengl.hpp>
// The openglsdlimageloader.hpp header file needs to be included
// in order to get the image loader that uses OpenGL and SDL.
#include <guichan/opengl/openglsdlimageloader.hpp>
#include <guichan/sdl.hpp>

namespace openglsdl
{
    bool running = true;
    SDL_Surface* screen;

    // All back ends contain objects to make Guichan work on a
    // specific target. They are a Graphics object to make Guichan 
    // able to draw itself using OpenGL, an input objec to make 
    // Guichan able to get user input using SDL and an ImageLoader 
    // object to make Guichan able to load images using OpenGL and SDL.
    gcn::OpenGLGraphics* graphics;
    gcn::SDLInput* input;
    gcn::OpenGLSDLImageLoader* imageLoader;

    /**
     * Initialises the OpenGL and SDL application. This function creates the global
     * Gui object that can be populated by various examples.
     */
    void init()
    {
        // We simply initialise OpenGL and SDL as we would do with any OpenGL
        // and SDL application.
        SDL_Init(SDL_INIT_VIDEO);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

        screen = SDL_SetVideoMode(640, 480, 32, SDL_HWSURFACE | SDL_OPENGL | SDL_HWACCEL);

        glViewport(0, 0, 640, 480);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        // We want unicode for the SDLInput object to function properly.
        SDL_EnableUNICODE(1);
        // We also want to enable key repeat.
        SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

        // Now it's time to initialise the Guichan OpenGL back end
        // and the Guichan SDL back end.

        imageLoader = new gcn::OpenGLSDLImageLoader();
        // The ImageLoader Guichan should use needs to be passed to the Image object
        // using a static function.
        gcn::Image::setImageLoader(imageLoader);
        graphics = new gcn::OpenGLGraphics();
        // We need to tell the OpenGL Graphics object how big the screen is.
        graphics->setTargetPlane(640, 480);
        input = new gcn::SDLInput();

        // Now we create the Gui object to be used with this OpenGL
        // and SDL application.
        globals::gui = new gcn::Gui();
        // The Gui object needs a Graphics to be able to draw itself and an Input
        // object to be able to check for user input. In this case we provide the
        // Gui object with OpenGL and SDL implementations of these objects hence 
        // making Guichan able to utilise OpenGL and SDL.
        globals::gui->setGraphics(graphics);
        globals::gui->setInput(input);
    }

    /**
     * Halts the OpenGL and SDL application.
     */
    void halt()
    {
        delete globals::gui;

        delete imageLoader;
        delete input;
        delete graphics;

        SDL_Quit();
    }

    /**
     * Runs the OpenGL and SDL application.
     */
    void run()
    {
        // The main loop
        while(running)
        {
            // Check user input
            SDL_Event event;
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
                
                // After we have manually checked user input with SDL for
                // any attempt by the user to halt the application we feed
                // the input to Guichan by pushing the input to the Input
                // object.
                input->pushInput(event);
            }
            // Now we let the Gui object perform its logic.
            globals::gui->logic();
            // Now we let the Gui object draw itself.
            globals::gui->draw();
            // Finally we update the screen.
            SDL_GL_SwapBuffers();
        }
    }
}
