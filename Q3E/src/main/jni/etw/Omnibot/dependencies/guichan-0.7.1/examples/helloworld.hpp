/**
 * Code to populate a global Gui object with a simple
 * Hello World example.
 */

namespace helloworld
{
    gcn::Container* top;
    gcn::ImageFont* font;
    gcn::Label* label;

    /**
     * Initialises the Hello World example by populating the global Gui
     * object.
     */
    void init()
    {
        // We first create a container to be used as the top widget.
        // The top widget in Guichan can be any kind of widget, but
        // in order to make the Gui contain more than one widget we
        // make the top widget a container.
        top = new gcn::Container();
        // We set the dimension of the top container to match the screen.
        top->setDimension(gcn::Rectangle(0, 0, 640, 480));
        // Finally we pass the top widget to the Gui object.
        globals::gui->setTop(top);

        // Now we load the font used in this example.
        font = new gcn::ImageFont("fixedfont.bmp", " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
        // Widgets may have a global font so we don't need to pass the
        // font object to every created widget. The global font is static.
        gcn::Widget::setGlobalFont(font);

        // Now we create a label with the text "Hello World".
        label = new gcn::Label("Hello World");
        // We give the label a position.
        label->setPosition(280, 220);
        // And finally we add the label to the top container.
        top->add(label);
    }
    
    /**
     * Halts the Hello World example.
     */
    void halt()
    {
        delete label;
        delete font;
        delete top;
    }
}
