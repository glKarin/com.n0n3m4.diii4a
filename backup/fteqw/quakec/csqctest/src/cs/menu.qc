float inmenu;

enum {
	ME_KEYDOWN,
	ME_KEYUP,
	ME_MOUSEMOVE,
	ME_DRAW = 200
};

nonstatic void(void(float, float, float, float) fnc) Menu_Activate =
{
	inmenu = TRUE;
	MenuEventFunc = fnc;
};

noref vector mousepos; //z is not set
float (float event, float parama, float paramb) Menu_InputEvent =
{
	if (!inmenu)
		return false;	//let the engine do what it wants.

	if (event == INPUT_MOUSEMOVE)
	{
		mousepos_x += parama;
		if (mousepos_x < 0)
			mousepos_x = 0;
		if (mousepos_x >= 640)
			mousepos_x = 640;
		mousepos_y += paramb;
		if (mousepos_y < 0)
			mousepos_y = 0;
		if (mousepos_y >= 480)
			mousepos_y = 480;
	}

	MenuEventFunc(event, parama, mousepos_x, mousepos_y);

	return true;
};

nonstatic void() Menu_Think =
{
	if (!inmenu)
		return;

	MenuEventFunc(ME_MOUSEMOVE, 0, mousepos_x, mousepos_y);

	MenuEventFunc(ME_DRAW, 0, mousepos_x, mousepos_y);
};

//We don't have a main menu for the csqc yet, so just pop it instead, so submenus can close properly.
nonstatic void() Menu_Main =
{
	inmenu = FALSE;
};
