/**
 * This is an example that demonstrates how to use actions
 * in Guichan. The example uses the HGE back end.
 */

#include <guichan.hpp>
#include <iostream>

// Here we store a global Gui object.  We make it global
// so it's easily accessable. Of course, global variables
// should normally be avioded when it comes to OOP, but
// this examples it not an example that shows how to make a 
// good and clean C++ application but merely an example
// that shows how to use Guichan.
namespace globals
{
    gcn::Gui* gui;
}

// Include code to set up an HGE application with Guichan.
// The hge.hpp file is responsible for creating and deleting
// the global Gui object.
#include "hge.hpp"
// Include code to set up a Guichan GUI that demonstrates how
// to use actions in Guichan. The code populates the global Gui object.
#include "action.hpp"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    HGE* hge = hgeCreate(HGE_VERSION);
    try
    {
        hge::init();
        action::init();
        hge::run();
        action::halt();
        hge::halt();
    }
    // Catch all Guichan exceptions.
    catch (gcn::Exception e)
    {
        MessageBox(NULL,  
                   e.getMessage().c_str(),
                   "Guichan exception",
                   MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        return 1;
    }
    // Catch all Std exceptions.
    catch (std::exception e)
    {
        MessageBox(NULL, 
                   e.what(),
                   "Std exception",
                   MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        return 1;
    }
    // Catch all unknown exceptions.
    catch (...)
    {
       MessageBox(NULL, 
                  hge->System_GetErrorMessage(), 
                  "Unknown exception", 
                  MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        return 1;
    }

    return 0;
}
