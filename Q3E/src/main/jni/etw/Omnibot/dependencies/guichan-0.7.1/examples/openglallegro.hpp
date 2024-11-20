/*
 * Code that sets up an OpenGL application with Guichan using the 
 * Guichan OpenGL back end and the Allegro back end (as OpenGL cannot
 * load images nor check for user input an additional back end needs
 * to be used).
 */

// The openglallegroimageloader.hpp header file needs to be included
// in order to get the image loader that uses OpenGL and Allegro.
#include <guichan/opengl/openglallegroimageloader.hpp>
#include <guichan.hpp>
#include <guichan/opengl.hpp>
#include <guichan/allegro.hpp>

namespace openglallegro
{
    // All back ends contain objects to make Guichan work on a
    // specific target. They are a Graphics object to make Guichan 
    // able to draw itself using OpenGL, an input objec to make 
    // Guichan able to get user input using Allegro and an ImageLoader 
    // object to make Guichan able to load images using OpenGL and Allegro.
    gcn::OpenGLGraphics* graphics;
    gcn::AllegroInput* input;
    gcn::OpenGLAllegroImageLoader* imageLoader;

    /**
     * Initialises the OpenGL and Allegro application. This function creates the global
     * Gui object that can be populated by various examples.
     */
    void init()
    {
        // We simply initialise OpenGL and Allegro as we would do with any OpenGL
        // and Allegro application.
        allegro_init();
      
        install_allegro_gl();

        allegro_gl_clear_settings();
	    allegro_gl_set (AGL_COLOR_DEPTH, 32);
	    allegro_gl_set (AGL_Z_DEPTH, 24);
	    allegro_gl_set (AGL_FULLSCREEN, TRUE);
	    allegro_gl_set (AGL_DOUBLEBUFFER, 1);
	    allegro_gl_set (AGL_SUGGEST, AGL_COLOR_DEPTH | AGL_Z_DEPTH | AGL_DOUBLEBUFFER);

        set_gfx_mode(GFX_OPENGL_WINDOWED, 640, 480, 0, 0);

        install_keyboard();
        install_mouse();
        install_timer();

        glViewport(0, 0, 640, 480);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        // Now it's time to initialise the Guichan OpenGL back end
        // and the Guichan Allegro back end.

        imageLoader = new gcn::OpenGLAllegroImageLoader();
        // The ImageLoader Guichan should use needs to be passed to the Image object
        // using a static function.
        gcn::Image::setImageLoader(imageLoader);
        graphics = new gcn::OpenGLGraphics();
        // We need to tell the OpenGL Graphics object how big the screen is.
        graphics->setTargetPlane(640, 480);
        input = new gcn::AllegroInput();

        // Now we create the Gui object to be used with this OpenGL
        // and Allegro application.
        globals::gui = new gcn::Gui();
        // The Gui object needs a Graphics to be able to draw itself and an Input
        // object to be able to check for user input. In this case we provide the
        // Gui object with OpenGL and Allegro implementations of these objects hence 
        // making Guichan able to utilise OpenGL and Allegro.
        globals::gui->setGraphics(graphics);
        globals::gui->setInput(input);
    }

    /**
     * Halts the OpenGL and Allegro application.
     */
    void halt()
    {
        delete globals::gui;

        delete imageLoader;
        delete input;
        delete graphics;
    }

    /**
     * Runs the OpenGL and Allegro application.
     */
    void run()
    {
        while(!key[KEY_ESC])
        {
            // Now we let the Gui object perform its logic.
            globals::gui->logic();
            // Now we let the Gui object draw itself.
            globals::gui->draw();
            
            // Now we draw the mouse cursor.
            allegro_gl_set_allegro_mode();
            show_mouse(screen);
            algl_draw_mouse();
            allegro_gl_unset_allegro_mode();
        
            // Finally we update the screen.
            allegro_gl_flip();
        }
    }
}
