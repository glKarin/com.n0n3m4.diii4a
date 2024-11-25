/**
 * Code to populate a global Gui object with all the widgets
 * of Guichan.
 */

namespace widgets
{
    gcn::ImageFont* font;
    gcn::Container* top;
    gcn::Label* label;
    gcn::Icon* icon;
    gcn::Button* button;
    gcn::TextField* textField;
    gcn::TextBox* textBox;
    gcn::ScrollArea* textBoxScrollArea;
    gcn::ListBox* listBox;
    gcn::DropDown* dropDown;
    gcn::CheckBox* checkBox1;
    gcn::CheckBox* checkBox2;
    gcn::RadioButton* radioButton1;
    gcn::RadioButton* radioButton2;
    gcn::RadioButton* radioButton3;
    gcn::Slider* slider;
    gcn::Image *image;
    gcn::Window *window;
    gcn::Image *darkbitsImage;
    gcn::Icon* darkbitsIcon;
    gcn::TabbedArea* tabbedArea;
    gcn::Button* tabOneButton;
    gcn::CheckBox* tabTwoCheckBox;

    /*
     * List boxes and drop downs need an instance of a list model
     * in order to display a list.
     */
    class DemoListModel : public gcn::ListModel
    {
    public:
        int getNumberOfElements()
        {
            return 5;
        }

        std::string getElementAt(int i)
        {
            switch(i)
            {
              case 0:
                  return std::string("zero");
              case 1:
                  return std::string("one");
              case 2:
                  return std::string("two");
              case 3:
                  return std::string("three");
              case 4:
                  return std::string("four");
              default: // Just to keep warnings away
                  return std::string("");
            }
        }
    };

    DemoListModel demoListModel;

    /**
     * Initialises the widgets example by populating the global Gui
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

        // Now we create the widgets

        label = new gcn::Label("Label");

        image = gcn::Image::load("gui-chan.bmp");
        icon = new gcn::Icon(image);

        button = new gcn::Button("Button");

        textField = new gcn::TextField("Text field");

        textBox = new gcn::TextBox("Multiline\nText box");
        textBoxScrollArea = new gcn::ScrollArea(textBox);
        textBoxScrollArea->setWidth(200);
        textBoxScrollArea->setHeight(100);
        textBoxScrollArea->setFrameSize(1);

        listBox = new gcn::ListBox(&demoListModel);
        listBox->setFrameSize(1);
        dropDown = new gcn::DropDown(&demoListModel);
        
        checkBox1 = new gcn::CheckBox("Checkbox 1");
        checkBox2 = new gcn::CheckBox("Checkbox 2");

        radioButton1 = new gcn::RadioButton("RadioButton 1", "radiogroup", true);
        radioButton2 = new gcn::RadioButton("RadioButton 2", "radiogroup");
        radioButton3 = new gcn::RadioButton("RadioButton 3", "radiogroup");

        slider = new gcn::Slider(0, 10);
        slider->setSize(100, 10);

        window = new gcn::Window("I am a window  Drag me");
        window->setBaseColor(gcn::Color(255, 150, 200, 190));

        darkbitsImage = gcn::Image::load("darkbitslogo_by_haiko.bmp");
        darkbitsIcon = new gcn::Icon(darkbitsImage);
        window->add(darkbitsIcon);
        window->resizeToContent();

        tabbedArea = new gcn::TabbedArea();
        tabbedArea->setSize(200, 100);
        tabOneButton = new gcn::Button("A button in tab 1");
        tabbedArea->addTab("Tab 1", tabOneButton);
        tabTwoCheckBox = new gcn::CheckBox("A check box in tab 2");
        tabbedArea->addTab("Tab 2", tabTwoCheckBox);

        // Now it's time to add the widgets to the top container
        // so they will be conected to the GUI.
        
        top->add(label, 10, 10);
        top->add(icon, 10, 30);
        top->add(button, 200, 10);
        top->add(textField, 250, 10);
        top->add(textBoxScrollArea, 200, 50);
        top->add(listBox, 200, 200);
        top->add(dropDown, 500, 10);
        top->add(checkBox1, 500, 130);
        top->add(checkBox2, 500, 150);
        top->add(radioButton1, 500, 200);
        top->add(radioButton2, 500, 220);
        top->add(radioButton3, 500, 240);
        top->add(slider, 500, 300);
        top->add(window, 50, 350);
        top->add(tabbedArea, 400, 350);
    }
    
    /**
     * Halts the widgets example.
     */
    void halt()
    {
        delete font;
        delete top;
        delete label;
        delete icon;
        delete button;
        delete textField;
        delete textBox;
        delete textBoxScrollArea;
        delete listBox;
        delete dropDown;
        delete checkBox1;
        delete checkBox2;
        delete radioButton1;
        delete radioButton2;
        delete radioButton3;
        delete slider;
        delete window;
        delete darkbitsIcon;
        delete darkbitsImage;
        delete tabbedArea;
        delete tabOneButton;
        delete tabTwoCheckBox;
    }
}
