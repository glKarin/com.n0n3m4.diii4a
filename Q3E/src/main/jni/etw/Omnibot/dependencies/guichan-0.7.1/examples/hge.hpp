/*
 * Code that sets up an HGE application with Guichan using the 
 * Guichan HGE back end.
 */

#include <guichan.hpp>
#include <guichan/hge.hpp>
#include <string>

namespace hge
{
    HGE *hge = NULL;
    bool running = false;

    // All back ends contain objects to make Guichan work on a
    // specific target - in this case HGE - and they are a Graphics
    // object to make Guichan able to draw itself using HGE, an
    // input objec to make Guichan able to get user input using HGE
    // and an ImageLoader object to make Guichan able to load images
    // using SDL.
    gcn::HGEGraphics* graphics;
    gcn::HGEInput* input;
    gcn::HGEImageLoader* imageLoader;

    /**
     * The frame function for the HGE application.
     */
    bool frameFunc()
    {
        if (hge->Input_GetKeyState(HGEK_ESCAPE) || running) 
        {
            return true;
        }

        // Now we let the Gui object perform its logic.
        globals::gui->logic();
        // Next we begin a scene.
        hge->Gfx_BeginScene();
        hge->Gfx_Clear(0);
        // Now we let the Gui object draw itself.
        globals::gui->draw();
        // Finally we end the scene causing the screen to be updated.
        hge->Gfx_EndScene();

        return false;
    }

    /**
     * Initialises the HGE application. This function creates the global
     * Gui object that can be populated by various examples.
     */
    void init()
    {
         // We simply initialise HGE as we would do with any HGE application.
        hge = hgeCreate(HGE_VERSION);
      
        hge->System_SetState(HGE_FRAMEFUNC, frameFunc);
        hge->System_SetState(HGE_SCREENWIDTH, 640);
        hge->System_SetState(HGE_SCREENHEIGHT, 480);
        hge->System_SetState(HGE_WINDOWED, true);
        hge->System_SetState(HGE_HIDEMOUSE, false);
        hge->System_SetState(HGE_USESOUND, false);
        hge->System_SetState(HGE_SHOWSPLASH, false);
        hge->System_SetState(HGE_LOGFILE, "hgelog.txt");

        if (!hge->System_Initiate())
        {
             throw GCN_EXCEPTION("Unable to initialse HGE: " + std::string(hge->System_GetErrorMessage()));
        }

        // Now it's time to initialise the Guichan HGE back end.

        imageLoader = new gcn::HGEImageLoader();
        // The ImageLoader Guichan should use needs to be passed to the Image object
        // using a static function.
        gcn::Image::setImageLoader(imageLoader);
        graphics = new gcn::HGEGraphics();
        input = new gcn::HGEInput();

        // Now we create the Gui object to be used with this HGE application.
        globals::gui = new gcn::Gui();
        // The Gui object needs a Graphics to be able to draw itself and an Input
        // object to be able to check for user input. In this case we provide the
        // Gui object with HGE implementations of these objects hence making Guichan
        // able to utilise HGE.
        globals::gui->setGraphics(graphics);
        globals::gui->setInput(input);
    }

    /**
     * Halts the HGE application.
     */
    void halt()
    {
        delete globals::gui;

        delete imageLoader;
        delete input;
        delete graphics;

        hge->System_Shutdown();
        hge->Release();
    }

    /**
     * Runs the HGE application.
     */
    void run()
    {
        hge->System_Start();
    }
}
