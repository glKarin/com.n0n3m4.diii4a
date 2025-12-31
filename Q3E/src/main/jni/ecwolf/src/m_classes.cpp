#include "c_cvars.h"
#include "m_classes.h"
#include "wl_def.h"
#include "wl_menu.h"
#include "wl_play.h"
#include "id_sd.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
#include "textures/textures.h"
#include "g_mapinfo.h"
#include "v_palette.h"
#include "colormatcher.h"

bool 			Menu::close = false;
FTexture		*Menu::cursor = NULL;
unsigned int	Menu::lastIndexDrawn = 0;

EColorRange MenuItem::getTextColor() const
{
	static const GameInfo::EFontColors colors[2][4] =
	{
		{GameInfo::MENU_DISABLED, GameInfo::MENU_LABEL, GameInfo::MENU_HIGHLIGHT, GameInfo::MENU_INVALID},
		{GameInfo::MENU_DISABLED, GameInfo::MENU_SELECTION, GameInfo::MENU_HIGHLIGHTSELECTION, GameInfo::MENU_INVALIDSELECTION}
	};
	return gameinfo.FontColors[colors[isSelected()][getActive()]];
}

MenuItem::MenuItem(const char string[80], MENU_LISTENER_PROTOTYPE(activateListener)) :
	activateListener(activateListener), enabled(true), highlight(false),
	picture(NULL), pictureX(-1), pictureY(-1), visible(true),
	activateSound("menu/activate")
{
	setText(string);
}

void MenuItem::activate()
{
	if(activateListener != NULL)
		activateListener(menu->getCurrentPosition());
}

void MenuItem::draw()
{
	if(picture)
		VWB_DrawGraphic(picture, pictureX == -1 ? menu->getX() + 32 : pictureX, pictureY == -1 ? PrintY : pictureY, MENU_CENTER);

	US_Print(BigFont, getString(), getTextColor());
	PrintX = menu->getX() + menu->getIndent();
}

bool MenuItem::isSelected() const
{
	return !menu->isAnimating() && menu->getIndex(menu->getCurrentPosition()) == this;
}

void MenuItem::setPicture(const char* picture, int x, int y)
{
	this->picture = TexMan(picture);
	pictureX = x;
	pictureY = y;
}

void MenuItem::setText(const char string[80])
{
	height = 13;
	strcpy(this->string, string);
	for(unsigned int i = 0;i < 80;i++)
	{
		if(string[i] == '\n')
			height += 13;
		else if(string[i] == '\0')
			break;
	}
}

LabelMenuItem::LabelMenuItem(const char string[36]) : MenuItem(string)
{
	setEnabled(false);
}

void LabelMenuItem::draw()
{
	int oldWindowX = WindowX;
	int oldWindowY = WindowY;
	WindowX = menu->getX();
	WindowW = menu->getWidth();
	US_CPrint(BigFont, string, gameinfo.FontColors[GameInfo::MENU_TITLE]);
	WindowX = oldWindowX;
	WindowY = oldWindowY;
}

BooleanMenuItem::BooleanMenuItem(const char string[36], bool &value, MENU_LISTENER_PROTOTYPE(activateListener)) : MenuItem(string, activateListener), value(value)
{
}

void BooleanMenuItem::activate()
{
	value ^= 1;
	MenuItem::activate();
}

void BooleanMenuItem::draw()
{
	static FTexture *selected = TexMan("M_SELCT"), *deselected = TexMan("M_NSELCT");
	if (value)
		VWB_DrawGraphic (selected, PrintX - 24, PrintY + 3, MENU_CENTER);
	else
		VWB_DrawGraphic (deselected, PrintX - 24, PrintY + 3, MENU_CENTER);
	MenuItem::draw();
}

MenuSwitcherMenuItem::MenuSwitcherMenuItem(const char string[36], Menu &menu, MENU_LISTENER_PROTOTYPE(activateListener)) : MenuItem(string, activateListener), menu(menu)
{
}

void MenuSwitcherMenuItem::activate()
{
	// If there is an activateListener then use it to determine if the menu should switch
	if(activateListener == NULL || activateListener(MenuItem::menu->getCurrentPosition()))
	{
		MenuFadeOut();
		menu.show();
		if(!Menu::areMenusClosed())
		{
			MenuItem::menu->draw();
			MenuFadeIn();
		}
	}
}

SliderMenuItem::SliderMenuItem(int &value, int width, int max, const char begString[36], const char endString[36], MENU_LISTENER_PROTOTYPE(activateListener)) : MenuItem(endString, activateListener), value(value), width(width), max(max)
{
	strcpy(this->begString, begString);
}

void SliderMenuItem::draw()
{
	US_Print(BigFont, begString, getTextColor());
	PrintX += 8;

	unsigned int bx = PrintX, by = PrintY+1, bw = width, bh = 10;
	MenuToRealCoords(bx, by, bw, bh, MENU_CENTER);

	DrawWindow(PrintX, PrintY+1, width, 10, MENUWIN_BACKGROUND, MENUWIN_BOTBORDER, MENUWIN_TOPBORDER);

	//calc position
	int x = int(ceil((double(width-20)/double(max))*double(value)));
	x -= x+20 >= width ? 1 : 0;

	bx = PrintX + x;
	by = PrintY + 1;
	bw = 20;
	bh = 10;
	MenuToRealCoords(bx, by, bw, bh, MENU_CENTER);

	DrawWindow(PrintX + x, PrintY + 1, 20, 10, MENUWINHGLT_BACKGROUND, MENUWINHGLT_BOTBORDER, MENUWINHGLT_TOPBORDER);

	PrintX += width+8;
	MenuItem::draw();
}

void SliderMenuItem::left()
{
	value -= value > 0 ? 1 : 0;
	if(activateListener != NULL)
		activateListener(menu->getCurrentPosition());
	SD_PlaySound("menu/move1");
}

void SliderMenuItem::right()
{
	value += value < max ? 1 : 0;
	if(activateListener != NULL)
		activateListener(menu->getCurrentPosition());
	SD_PlaySound("menu/move1");
}

MultipleChoiceMenuItem::MultipleChoiceMenuItem(MENU_LISTENER_PROTOTYPE(changeListener), const char** options, unsigned int numOptions, int curOption) : MenuItem("", changeListener),
	curOption(curOption), numOptions(numOptions)
{
	// Copy all of the options
	this->options = new char *[numOptions];
	for(unsigned int i = 0;i < numOptions;i++)
	{
		if(options[i] == NULL)
			this->options[i] = NULL;
		else
		{
			this->options[i] = new char[strlen(options[i])+1];
			strcpy(this->options[i], options[i]);
		}
	}

	// clamp current option
	if(curOption < 0)
		curOption = 0;
	else if((unsigned)curOption >= numOptions)
		curOption = numOptions-1;

	while(options[curOption] == NULL)
	{
		curOption--; // Easier to go backwards
		if(curOption < 0)
			curOption = numOptions-1;
	}

	if(numOptions > 0)
		setText(options[curOption]);
}

MultipleChoiceMenuItem::~MultipleChoiceMenuItem()
{
	for(unsigned int i = 0;i < numOptions;i++)
		delete[] options[i];
	delete[] options;
}

void MultipleChoiceMenuItem::activate()
{
	right();
}

void MultipleChoiceMenuItem::draw()
{
	DrawWindow(PrintX, PrintY, menu->getWidth()-menu->getIndent()-menu->getX(), BigFont->GetHeight(), BKGDCOLOR, BKGDCOLOR, BKGDCOLOR);
	MenuItem::draw();
}

void MultipleChoiceMenuItem::left()
{
	do
	{
		curOption--;
		if(curOption < 0)
			curOption = numOptions-1;
	}
	while(options[curOption] == NULL);
	setText(options[curOption]);
	if(activateListener != NULL)
		activateListener(curOption);
	SD_PlaySound("menu/move1");
}

void MultipleChoiceMenuItem::right()
{
	do
	{
		curOption++;
		if((unsigned)curOption >= numOptions)
			curOption = 0;
	}
	while(options[curOption] == NULL);
	setText(options[curOption]);
	if(activateListener != NULL)
		activateListener(curOption);
	SD_PlaySound("menu/move1");
}

TextInputMenuItem::TextInputMenuItem(const FString &text, unsigned int max, MENU_LISTENER_PROTOTYPE(preeditListener), MENU_LISTENER_PROTOTYPE(posteditListener), bool clearFirst) : MenuItem("", posteditListener), clearFirst(clearFirst), max(max), preeditListener(preeditListener)
{
	setValue(text);
}

void TextInputMenuItem::activate()
{
	if(preeditListener == NULL || preeditListener(menu->getCurrentPosition()))
	{
		PrintY = menu->getHeight(menu->getCurrentPosition()) + menu->getY() + 1 + (5-SmallFont->GetHeight()/2);
		char* buffer = new char[max+1];
		bool accept = US_LineInput(SmallFont,
			menu->getX() + menu->getIndent() + 2,
			PrintY, buffer,
			clearFirst ? "" : getValue(), true, max, menu->getWidth() - menu->getIndent() - 16,
			BKGDCOLOR, getTextColor()
		);

		if(accept)
			setValue(buffer);
		delete[] buffer;
		if(accept)
			MenuItem::activate();
		else
		{
			SD_PlaySound("menu/escape");
			PrintY = menu->getHeight(menu->getCurrentPosition()) + menu->getY();
			draw();
		}
	}
}

void TextInputMenuItem::draw()
{
	int color = ColorMatcher.Pick(V_LogColorFromColorRange(getTextColor()));

	DrawWindow(menu->getX() + menu->getIndent(), PrintY, menu->getWidth() - menu->getIndent() - 12, 11, BKGDCOLOR, color, color);
	PrintX = menu->getX() + menu->getIndent() + 2;
	PrintY += 1 + (5-SmallFont->GetHeight()/2);
	US_Print(SmallFont, getValue(), getTextColor());
}

int ControlMenuItem::column = 0;
static const char* const KeyNames[512] =
{
	"?","?","?","?","?","?","?","?",                                //   0
	"BkSp","Tab","?","?","?","Ret","?","?",                      //   8
	"?","?","?","Paus","?","?","?","?",                            //  16
	"?","?","?","Esc","?","?","?","?",                              //  24
	"Spce","!","\"","#","$","?","&","'",                           //  32
	"(",")","*","+",",","-",".","/",                                //  40
	"0","1","2","3","4","5","6","7",                                //  48
	"8","9",":",";","<","=",">","?",                                //  56
	"@","A","B","C","D","E","F","G",                                //  64
	"H","I","J","K","L","M","N","O",                                //  72
	"P","Q","R","S","T","U","V","W",                                //  80
	"X","Y","Z","[","\\","]","^","_",                               //  88
	"`","a","b","c","d","e","f","g",                                //  96
	"h","i","j","k","l","m","n","o",                                // 104
	"p","q","r","s","t","u","v","w",                                // 112
	"x","y","z","{","|","}","~","Del",                              // 120
	"?","?","?","?","?","?","?","?",                                // 128
	"?","?","?","?","?","?","?","?",                                // 136
	"?","?","?","?","?","?","?","?",                                // 144
	"?","?","?","?","?","?","?","?",                                // 152
	"?","?","?","?","?","?","?","?",                                // 160
	"?","?","?","?","?","?","?","?",                                // 168
	"?","?","?","?","?","?","?","?",                                // 176
	"?","?","?","?","?","?","?","?",                                // 184
	"?","?","?","?","?","?","?","?",                                // 192
	"?","?","?","?","?","?","?","?",                                // 200
	"?","?","?","?","?","?","?","?",                                // 208
	"?","?","?","?","?","?","?","?",                                // 216
	"?","?","?","?","?","?","?","?",                                // 224
	"?","?","?","?","?","?","?","?",                                // 232
	"?","?","?","?","?","?","?","?",                                // 240
	"?","?","?","?","?","?","?","?",                                // 248
	"KP0","KP1","KP2","KP3","KP4","KP5","KP6","KP7",                // 256
	"KP8","KP9","Perd","Divd","Mult","Mins","Plus","Entr",          // 264
	"Equl","Up","Down","Rght","Left","Ins","Home","End",            // 272
	"PgUp","PgDn","F1","F2","F3","F4","F5","F6",                    // 280
	"F7","F8","F9","F10","F11","F12","F13","F14",                   // 288
	"F15","?","?","?","NmLk","CpLk","ScLk","RShf",                  // 296
	"Shft","RCtl","Ctrl","RAlt","Alt","RMet","Meta","Supr",         // 304
	"RSpr","Mode","Comp","Help","PrtS","Brk","Pwr","Euro",          // 312
	"Undo","?"                                                      // 320
};

static const char* const MWheelNames[4] = {
	"WhLt", "WhRt", "WhDn", "WhUp"
};

ControlMenuItem::ControlMenuItem(ControlScheme &button) : MenuItem(button.name), button(button)
{
}

void ControlMenuItem::activate()
{
	if(mouseenabled)
	{
		// Check for mouse up
		ControlInfo ci;
		do { ReadAnyControl(&ci); } while(ci.button0 || ci.button1);
	}

	DrawWindow(160 + (52*column), PrintY + 1, 50 - 2, 11, MENUWIN_BACKGROUND, MENUWIN_BOTBORDER, MENUWINHGLT_TOPBORDER);
	PrintX = 162 + (52*column);
	US_Print(BigFont, "???");
	VW_UpdateScreen();

	IN_ClearKeysDown();
	ControlInfo ci;
	bool exit = false;
	int btn = 0;
	while(!exit)
	{
		SDL_Delay(5);

		switch(column)
		{
			default:
			case 0:
				if(LastScan != 0)
				{
					ControlScheme::setKeyboard(controlScheme, button.button, LastScan);
					ShootSnd();
					IN_ClearKeysDown();
					exit = true;
				}
				break;
			case 1:
			{
				if(!mouseenabled)
				{
					exit = true;
					break;
				}

				if((btn = IN_MouseButtons()))
				{
					for(int i = 0;btn != 0 && i < 32;i++)
					{
						if(btn & (1<<i))
						{
							ControlScheme::setMouse(controlScheme, button.button, i);
							exit = true;
						}
					}
				}
				else
				{
					if(MouseWheel[di_west])
						btn = ControlScheme::MWheel_Left;
					if(MouseWheel[di_east])
						btn = ControlScheme::MWheel_Right;
					if(MouseWheel[di_north])
						btn = ControlScheme::MWheel_Up;
					if(MouseWheel[di_south])
						btn = ControlScheme::MWheel_Down;

					if(btn)
					{
						ControlScheme::setMouse(controlScheme, button.button, btn);
						exit = true;
					}
				}

				break;
			}
			case 2:
				if(!IN_JoyPresent())
				{
					exit = true;
					break;
				}

				btn = IN_JoyButtons();
				if(btn != 0)
				{
					for(int i = 0;btn != 0 && i < 32;i++)
					{
						if(btn & (1<<i))
						{
							ControlScheme::setJoystick(controlScheme, button.button, i);
							exit = true;
						}
					}
				}
				else
				{
					btn = IN_JoyAxes();
					for(int i = 0;btn != 0 && i < 32;i++)
					{
						if(btn & (1<<i))
						{
							ControlScheme::setJoystick(controlScheme, button.button, i+32);
							exit = true;
						}
					}
				}
				break;
		}

		ReadAnyControl(&ci);
		if(LastScan == sc_Escape)
			break;
	}

	PrintX = menu->getX() + menu->getIndent();

	MenuItem::activate();

	// Setting one vale could affect another
	menu->draw();

	if(mouseenabled)
	{
		// Check for mouse up
		ControlInfo ci;
		do { ReadAnyControl(&ci); } while(ci.button0 || ci.button1);
	}
}

void ControlMenuItem::draw()
{
	extern int SDL2Backconvert(int sc);

	DrawWindow(159, PrintY, ((52)*3) - 1, 13, BKGDCOLOR, BKGDCOLOR, BKGDCOLOR);
	if(isSelected())
		DrawWindow(160 + (52*column), PrintY + 1, 50 - 2, 11, MENUWIN_BACKGROUND, MENUWIN_BOTBORDER, MENUWIN_TOPBORDER);

	US_Print(BigFont, getString(), getTextColor());

	const int key = SDL2Backconvert(button.keyboard);

	if(button.keyboard >= 0 && button.keyboard < 512 && KeyNames[key])
	{
		PrintX = 162;
		US_Print(BigFont, KeyNames[key], getTextColor());
	}
	if(button.mouse != -1)
	{
		PrintX = 214;
		FString btn;
		if(button.mouse >= ControlScheme::MWheel_Left && button.mouse <= ControlScheme::MWheel_Up)
			btn = MWheelNames[button.mouse - ControlScheme::MWheel_Left];
		else
			btn.Format("MS%d", button.mouse);
		US_Print(BigFont, btn, getTextColor());
	}
	if(button.joystick != -1)
	{
		PrintX = 266;
		FString btn;
		if(button.joystick < 32)
			btn.Format("JY%d", button.joystick);
		else
			btn.Format("A%d%c", (button.joystick-32)/2, (button.joystick&1) ? 'D' : 'U');
		US_Print(BigFont, btn, getTextColor());
	}

	PrintX = menu->getX() + menu->getIndent();
}

void ControlMenuItem::left()
{
	if(column != 0)
		column--;
}

void ControlMenuItem::right()
{
	if(column != 2)
		column++;
}

void Menu::drawGunHalfStep(int x, int y)
{
	if(MenuStyle != MENUSTYLE_Blake)
	{
		VWB_DrawGraphic (cursor, x, y-2, MENU_CENTER);
		VW_UpdateScreen ();
		SD_PlaySound ("menu/move1");
		SDL_Delay (TICS2MS(8));
	}
}

void Menu::eraseGun(int x, int y)
{
	if(MenuStyle != MENUSTYLE_Blake)
	{
		int gx = x, gy = y, gw = cursor->GetScaledWidth(), gh = cursor->GetScaledHeight();
		MenuToRealCoords(gx, gy, gw, gh, MENU_CENTER);
		VWB_Clear(BKGDCOLOR, gx, gy, gx+gw, gy+gh);
	}
}

Menu::Menu(int x, int y, int w, int indent, MENU_LISTENER_PROTOTYPE(entryListener)) :
	entryListener(entryListener), animating(false), controlHeaders(false),
	curPos(0), headPicture(NULL), headTextInStripes(false),
	headPictureIsAlternate(false), height(0), indent(indent), x(x), y(y), w(w),
	itemOffset(0)
{
	for(unsigned int i = 0;i < 36;i++)
		headText[i] = '\0';
}
Menu::~Menu()
{
	clear();
}

void Menu::addItem(MenuItem *item)
{
	item->setMenu(this);
	items.Push(item);
	if(item->isVisible() && !item->isEnabled() && (signed)countItems()-1 == curPos)
		curPos++;
	height += item->getHeight();
}

void Menu::clear()
{
	for(unsigned int i = 0;i < items.Size();i++)
		delete items[i];
	items.Delete(0, items.Size());
}

void Menu::closeMenus(bool close)
{
	if(close)
	{
		MenuFadeOut();
		VL_FadeClear();
	}

	Menu::close = close;
}

unsigned int Menu::countItems() const
{
	unsigned int num = 0;
	for(unsigned int i = 0;i < items.Size();i++)
	{
		if(items[i]->isVisible())
			num++;
	}
	return num;
}

int Menu::getHeight(int position) const
{
	// Make sure we have the position we think we have.
	if(position != -1)
	{
		for(unsigned int i = 0;i < items.Size() && i < (unsigned)position;i++)
		{
			if(!items[i]->isVisible())
				position++;
		}
	}

	unsigned int num = 0;
	unsigned int ignore = itemOffset;
	for(unsigned int i = 0;i < items.Size();i++)
	{
		if((unsigned)position == i)
			break;

		if(items[i]->isVisible())
		{
			if(getY() + num + items[i]->getHeight() + 6 >= 200)
				break;

			if(ignore)
				--ignore;
			else
				num += items[i]->getHeight();
		}
	}
	if(position >= 0)
		return num;
	return num + 6;
}

MenuItem *Menu::getIndex(int index) const
{
	unsigned int idx = 0;
	for(idx = 0;idx < items.Size() && index >= 0;idx++)
	{
		if(items[idx]->isVisible())
			index--;
	}
	idx--;
	return idx >= items.Size() ? items[items.Size()-1] : items[idx];
}

void Menu::drawMenu() const
{
	if(cursor == NULL && MenuStyle != MENUSTYLE_Blake)
		cursor = TexMan("M_CURS1");

	lastIndexDrawn = 0;

	WindowX = PrintX = getX() + getIndent();
	WindowY = PrintY = getY();
	WindowW = 320;
	WindowH = 200;

	PrintY = getY();
	int y = getY();
	int selectedY = getY(); // See later

	unsigned int count = countItems();
	for (unsigned int i = itemOffset; i < count; i++)
	{
		if(i == (unsigned)curPos)
			selectedY = y;
		else
		{
			PrintY = y;
			if(PrintY + getIndex(i)->getHeight() + 6 >= 200)
				break;
			getIndex(i)->draw();
			lastIndexDrawn = i;
		}
		y += getIndex(i)->getHeight();
	}

	if(MenuStyle == MENUSTYLE_Blake)
	{
		double curx = getX() + getIndent() - 1;
		double curw = getWidth() - getIndent() + 1;
		double cury = selectedY;
		double curh = getIndex(curPos)->getHeight();
		MenuToRealCoords(curx, cury, curw, curh, MENU_CENTER);
		VWB_Clear(MENUWIN_BACKGROUND, curx, cury, curx+curw, cury+curh);
	}

	// In order to draw the skill menu correctly we need to draw the selected option now
	if(curPos < (signed)count && curPos >= (signed)itemOffset)
	{
		PrintY = selectedY;
		getIndex(curPos)->draw();
		if(curPos > (signed)lastIndexDrawn)
			lastIndexDrawn = curPos;
	}
}

void Menu::draw() const
{
	static FTexture * const mcontrol = TexMan("M_MCONTL");
	ClearMScreen();
	if(headPicture && !headPictureIsAlternate)
	{
		DrawStripes(10);
		VWB_DrawGraphic(headPicture, 160-headPicture->GetScaledWidth()/2, 0, MENU_TOP);
	}
	VWB_DrawGraphic(mcontrol, 160-mcontrol->GetScaledWidth()/2, 200-mcontrol->GetScaledHeight(), MENU_BOTTOM);

	WindowX = 0;
	WindowW = 320;
    PrintY = getY() - 22;
	if(controlHeaders)
	{
		PrintX = getX() + getIndent();
		US_Print(BigFont, "Control", gameinfo.FontColors[GameInfo::MENU_TITLE]);
		PrintX = 168;
		US_Print(BigFont, "Key", gameinfo.FontColors[GameInfo::MENU_TITLE]);
		PrintX = 220;
		US_Print(BigFont, "Mse", gameinfo.FontColors[GameInfo::MENU_TITLE]);
		PrintX = 272;
		US_Print(BigFont, "Joy", gameinfo.FontColors[GameInfo::MENU_TITLE]);
	}
	else
	{
		if(headTextInStripes)
		{
			DrawStripes(10);
			PrintY = 15;
		}

		if(headPictureIsAlternate && headPicture)
			VWB_DrawGraphic(headPicture, 160-headPicture->GetScaledWidth()/2, PrintY, MENU_CENTER);
		else
			US_CPrint(BigFont, headText, gameinfo.FontColors[GameInfo::MENU_TITLE]);
	}

	if(MenuStyle != MENUSTYLE_Blake)
		DrawWindow(getX() - 8, getY() - 3, getWidth(), getHeight(), BKGDCOLOR);
	drawMenu();

	if(cursor && !isAnimating() && countItems() > 0)
		VWB_DrawGraphic (cursor, x - 4, y + getHeight(curPos) - 2, MENU_CENTER);
	VW_UpdateScreen ();
}

int Menu::handle()
{
	char key;
	static int redrawitem = 1, lastitem = -1;
	int x, y, basey, exit, shape;
	int32_t lastBlinkTime;
	ControlInfo ci;

	if(close)
		return -1;

	x = getX() - 4;
	basey = getY();
	y = basey + getHeight(curPos);

	if (redrawitem)
	{
		PrintX = getX() + getIndent();
		PrintY = getY() + getHeight(curPos);
		getIndex(curPos)->draw();
	}
	VW_UpdateScreen ();

	shape = 0;
	exit = 0;
	lastBlinkTime = GetTimeCount ();
	IN_ClearKeysDown ();

	do
	{
		//
		// CHANGE GUN SHAPE
		//
		if (getIndent() != 0 && lastBlinkTime != GetTimeCount())
		{
			lastBlinkTime = GetTimeCount();
			TexMan.UpdateAnimations(lastBlinkTime*14);

			if(MenuStyle != MENUSTYLE_Blake)
				cursor = TexMan("M_CURS1");
			draw();
		}
		else SDL_Delay(5);

		CheckPause ();

		//
		// SEE IF ANY KEYS ARE PRESSED FOR INITIAL CHAR FINDING
		//
		key = LastASCII;
		if (key)
		{
			int ok = 0;

			if (key >= 'a')
				key -= 'a' - 'A';

			for (unsigned int i = curPos + 1; i < countItems(); i++)
				if (getIndex(i)->isEnabled() && getIndex(i)->getString()[0] == key)
				{
					curPos = i;
					ok = 1;
					SD_PlaySound("menu/move1");
					IN_ClearKeysDown ();
					break;
				}

			//
			// DIDN'T FIND A MATCH FIRST TIME THRU. CHECK AGAIN.
			//
			if (!ok)
			{
				for (int i = 0; i < curPos; i++)
				{
					if (getIndex(i)->isEnabled() && getIndex(i)->getString()[0] == key)
					{
						curPos = i;
						SD_PlaySound("menu/move1");
						IN_ClearKeysDown ();
						break;
					}
				}
			}
		}

		if(LastScan == SDLx_SCANCODE(DELETE))
		{
			handleDelete();
			LastScan = 0;

			// Leave menu if we delete everything.
			if(countItems() == 0)
			{
				lastitem = curPos; // Prevent redrawing
				exit = 2;
			}
		}

		//
		// GET INPUT
		//
		ReadAnyControl (&ci);
		switch (ci.dir)
		{
			default:
				break;

				////////////////////////////////////////////////
				//
				// MOVE UP
				//
			case dir_North:
			{
				if(countItems() <= 1)
					break;

				//
				// MOVE TO NEXT AVAILABLE SPOT
				//
				int oldPos = curPos;
				do
				{
					if (curPos == 0)
					{
						curPos = countItems() - 1;
						itemOffset = curPos - lastIndexDrawn;
					}
					else if (itemOffset > 0 && (unsigned)curPos == itemOffset+1)
					{
						--itemOffset;
						--curPos;
					}
					else
						--curPos;
				}
				while (!getIndex(curPos)->isEnabled());

				if(oldPos - curPos == 1)
				{
					animating = true;
					draw();
					drawGunHalfStep(x, getY() + getHeight(oldPos) - 6);
					animating = false;
				}
				draw();
				SD_PlaySound("menu/move2");
				//
				// WAIT FOR BUTTON-UP OR DELAY NEXT MOVE
				//
				TicDelay (20);
				break;
			}

				////////////////////////////////////////////////
				//
				// MOVE DOWN
				//
			case dir_South:
			{
				if(countItems() <= 1)
					break;

				int oldPos = curPos;
				do
				{
					unsigned int lastPos = countItems() - 1;
					if ((unsigned)curPos == lastPos)
					{
						curPos = 0;
						itemOffset = 0;
					}
					else if (lastIndexDrawn != lastPos && (unsigned)curPos >= lastIndexDrawn-1)
					{
						++itemOffset;
						++curPos;
					}
					else
						++curPos;
				}
				while (!getIndex(curPos)->isEnabled());

				if(oldPos - curPos == -1)
				{
					animating = true;
					draw();
					drawGunHalfStep(x, getY() + getHeight(oldPos) + 6);
					animating = false;
				}
				draw();
				SD_PlaySound("menu/move2");
				//
				// WAIT FOR BUTTON-UP OR DELAY NEXT MOVE
				//
				TicDelay (20);
				break;
			}
			case dir_West:
				getIndex(curPos)->left();
				PrintX = getX() + getIndent();
				PrintY = getY() + getHeight(curPos);
				getIndex(curPos)->draw();
				VW_UpdateScreen();
				TicDelay(20);
				break;
			case dir_East:
				getIndex(curPos)->right();
				PrintX = getX() + getIndent();
				PrintY = getY() + getHeight(curPos);
				getIndex(curPos)->draw();
				VW_UpdateScreen();
				TicDelay(20);
				break;
		}

		if (ci.button0 || Keyboard[sc_Space] || Keyboard[sc_Enter])
			exit = 1;

		if ((ci.button1 && !Keyboard[sc_Alt]) || Keyboard[sc_Escape])
			exit = 2;

	}
	while (!exit);


	IN_ClearKeysDown ();

	//
	// ERASE EVERYTHING
	//
	if (lastitem != curPos)
	{
		PrintX = getX() + getIndent();
		PrintY = getY() + getHeight(curPos);
		getIndex(curPos)->draw();
		redrawitem = 1;
	}
	else
		redrawitem = 0;

	VW_UpdateScreen ();

	lastitem = curPos;
	switch (exit)
	{
		case 1:
			if(getIndex(curPos)->playActivateSound())
				SD_PlaySound (getIndex(curPos)->getActivateSound());
			getIndex(curPos)->activate();
			PrintX = getX() + getIndent();
			PrintY = getY() + getHeight(curPos);
			if(!Menu::areMenusClosed())
			{
				getIndex(curPos)->draw();
				VW_UpdateScreen();
			}

			// Check for mouse up
			do { ReadAnyControl(&ci); } while(ci.button0);
			return curPos;

		case 2:
			SD_PlaySound("menu/escape");

			// Check for mouse up
			do { ReadAnyControl(&ci); } while(ci.button1);
			return -1;
	}

	return 0;                   // JUST TO SHUT UP THE ERROR MESSAGES!
}

void Menu::setCurrentPosition(int position)
{
	unsigned int count;

	if(position <= 0) // At start
	{
		curPos = 0;
		itemOffset = 0;
	}
	else if((unsigned) position >= (count = countItems())-1) // At end
	{
		curPos = count-1;
		itemOffset = curPos;
		unsigned int accumulatedHeight = getY() + getIndex(itemOffset)->getHeight() + 6;
		while(accumulatedHeight < 200)
		{
			if(itemOffset == 0)
				break;

			accumulatedHeight += getIndex(--itemOffset)->getHeight();
		}
		if(accumulatedHeight >= 200)
			++itemOffset;
	}
	else // Somewhere in the middle
	{
		curPos = position;
		itemOffset = curPos;
		unsigned int accumulatedHeight = getY() + getIndex(itemOffset)->getHeight() + 6;
		unsigned int lastIndex = curPos;
		while(accumulatedHeight < 200)
		{
			if(lastIndex < items.Size()-1)
			{
				accumulatedHeight += getIndex(++lastIndex)->getHeight();
				if(accumulatedHeight >= 200)
					break;
			}

			if(itemOffset > 0)
				accumulatedHeight += getIndex(--itemOffset)->getHeight();
			else
				break;
		}
		if(accumulatedHeight >= 200)
			++itemOffset;
	}
}

void Menu::setHeadPicture(const char* picture, bool isAlt)
{
	FTextureID picID = TexMan.CheckForTexture(picture, FTexture::TEX_Any);
	if(picID.isValid())
	{
		headPicture = TexMan(picID);
		headPictureIsAlternate = isAlt;
	}
}

void Menu::setHeadText(const char text[36], bool drawInStripes)
{
	strcpy(headText, text);
	headTextInStripes = drawInStripes;
}

void Menu::show()
{
	if(Menu::areMenusClosed())
		return;

	if(entryListener != NULL)
		entryListener(0);

	if(countItems() == 0) // Do nothing.
		return;
	validateCurPos();

	draw();
	MenuFadeIn();
	WaitKeyUp();

	int item = 0;
	while((item = handle()) != -1) {}

	if(!Menu::areMenusClosed())
		MenuFadeOut ();
}

void Menu::validateCurPos()
{
	if(curPos >= (signed)countItems())
		curPos = countItems()-1;

	// If current item is disable try to move off it
	const int oldCurPos = curPos;
	while(!getIndex(curPos)->isEnabled() && curPos > 0)
		--curPos;

	// Reached top? Try searching downwards
	if(curPos == 0 && !getIndex(0)->isEnabled())
	{
		curPos = oldCurPos+1;
		while(!getIndex(curPos)->isEnabled() && curPos < (signed)countItems())
			++curPos;
	}
}
