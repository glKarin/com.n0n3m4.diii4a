/*
 * Code that sets up an Allegro application with Guichan using the 
 * Guichan Allegro back end.
 */

#include <guichan.hpp>
#include <guichan/allegro.hpp>

namespace allegro
{
    BITMAP* screenBuffer;

    // All back ends contain objects to make Guichan work on a
    // specific target - in this case Allegro - and they are a Graphics
    // object to make Guichan able to draw itself using Allegro, an
    // input objec to make Gopenglsdluichan able to get user input using Allegro
    // and an ImageLoader object to make Guichan able to load images
    // using Allegro.
    gcn::AllegroGraphics* graphics;
    gcn::AllegroInput* input;
    gcn::AllegroImageLoader* imageLoader;

    /**
     * Initialises the Allegro application. This function creates the global
     * Gui object that can be populated by various examples.
     */
    void init()
    {
        // We simply initialise Allegro as we would do with any Allegro application.
        allegro_init();

        int bpp = desktop_color_depth();
        if (bpp == 0)
        {
            bpp = 16;
        }

        set_color_depth(bpp);

        if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0))
        {
            if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0))
            {
                throw GCN_EXCEPTION("Unable to set graphics mode");
            }
        }

        screenBuffer = create_bitmap(SCREEN_W, SCREEN_H);

        if (screenBuffer == NULL)
        {
            throw GCN_EXCEPTION("Unable to create a screen buffer");
        }

        install_keyboard();
        install_mouse();
        install_timer();

        // Now it's time to initialise the Guichan Allegro back end.

        imageLoader = new gcn::AllegroImageLoader();
        // The ImageLoader Guichan should use needs to be passed to the Image object
        // using a static function.
        gcn::Image::setImageLoader(imageLoader);
        graphics = new gcn::AllegroGraphics();
        // Set the target for the graphics object to be the doublebuffer
        // for the screen. Drawing to the screen directly is not a good
        // idea, as it will produce flicker, unless you use page flipping.
        graphics->setTarget(screenBuffer);
        input = new gcn::AllegroInput();

        // Now we create the Gui object to be used with this Allegro application.
        globals::gui = new gcn::Gui();
        // The Gui object needs a Graphics to be able to draw itself and an Input
        // object to be able to check for user input. In this case we provide the
        // Gui object with Allegro implementations of these objects hence making Guichan
        // able to utilise Allegro.
        globals::gui->setGraphics(graphics);
        globals::gui->setInput(input);
    }

    /**
     * Halts the Allegro application.
     */
    void halt()
    {
        delete globals::gui;

        delete imageLoader;
        delete input;
        delete graphics;

        destroy_bitmap(screenBuffer);
    }

    /**
     * Runs the Allegro application.
     */
    void run()
    {
        while(!key[KEY_ESC])
        {
            // Now we let the Gui object perform its logic.
            globals::gui->logic();
            // Now we let the Gui object draw itself.
            globals::gui->draw();

            // We draw the mouse pointer manually, as Allegro's mouse
            // drawing code is so wierd.
            draw_sprite(screenBuffer, mouse_sprite, mouse_x, mouse_y);

            // Finally we update the screen.
            blit(screenBuffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
        }
    }
}
