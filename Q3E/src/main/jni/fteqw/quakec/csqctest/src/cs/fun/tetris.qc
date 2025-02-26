/*
Quake tetris, origionally created by FrikaC

Ported to csqc by Spike, primarily for testing in a fun way.
*/

float tetris_on, tet_time, tet_autodown, tet_old_keys, tet_keys;
vector piece_pos;
float piece_type, next_piece, tet_score, tet_lines;

var float tet_high_score = 0;

float TET_LINES = 20;
float TET_WIDTH = 10;
//character values
float TET_BORDER = 132;
float TET_BLOCKS = 132; // +1 = first color, +2, +3;
float TET_SPACE = 32; // blankness



float TETKEY_UP = 1;
float TETKEY_DOWN = 2;
float TETKEY_LEFT = 4;
float TETKEY_RIGHT = 8;
float TETKEY_ROTLEFT = 16;
float TETKEY_ROTRIGHT = 32;
float KEYS_IMMEDIATE = 60;//32 + 16 + 8 + 4;

float PIECES = 7;

float line[20];


string printline;

/*
*********************************

Library Functions

*********************************
*/
void (string snd) tetsnd =
{
	localcmd("play ");
	localcmd(snd);
	localcmd("\n");

};

float () Tetris_Level =
{
	return floor(tet_lines / 20) + 1;
};

#define SetLine(l,v) line[l] = v
#define GetLine(l) line[l]

float(float x, float dat) GetXBlock =
{
	if (x == 0)
		return dat & 3;
	else if (x == 1)
		return (dat & 12) / 4;
	else if (x == 2)
		return (dat & 48) / 16;
	else if (x == 3)
		return (dat & 192) / 64;
	else if (x == 4)
		return (dat & 768) / 256;
	else if (x == 5)
		return (dat & 3072) / 1024;
	else if (x == 6)
		return (dat & 12288) / 4096;
	else if (x == 7)
		return (dat & 49152) / 16384;
	else if (x == 8)
		return (dat & 196608) / 65536;
	else if (x == 9)
		return (dat & 786432) / 262144;
	else
		return 0;
};

float(float x, float dat, float new) SetXBlock =
{
	if (x == 0)
		return (dat - (dat & 3)) | new;
	else if (x == 1)
		return (dat - (dat & 12)) | (new*4);
	else if (x == 2)
		return (dat - (dat & 48)) | (new*16);
	else if (x == 3)
		return (dat - (dat & 192)) | (new*64);
	else if (x == 4)
		return (dat - (dat & 768)) | (new*256);
	else if (x == 5)
		return (dat - (dat & 3072)) | (new*1024);
	else if (x == 6)
		return (dat - (dat & 12288)) | (new*4096);
	else if (x == 7)
		return (dat - (dat & 49152)) | (new*16384);
	else if (x == 8)
		return (dat - (dat & 196608)) | (new*65536);
	else if (x == 9)
		return (dat - (dat & 786432)) | (new*262144);
	else
		return dat;
};


float(float x, float y) GetSquare =
{
	return GetXBlock(x,  GetLine(y));
};

void (float x, float y, float val) SetSquare =
{
	float dat;
	
	dat = GetLine(y);
	dat  = SetXBlock(x, dat, val & 3);
	SetLine(y, dat);
};



vector(float pc) PieceShape =
{
// third value is width?

/*
1 =
## 
## 
*/
	if (pc == 1)
		return '5 5 2'; // 1 * 4 + 1 * 16
/*
2 =

####
*/
	else if (pc == 2)
		return '85 0 4';

/*
3 =

###
#
*/
	else if (pc == 3)
		return '21 1 3';
/*
4 =

#
###
*/
	else if (pc == 4)
		return '1 21 3';
/*
5 =
##
 ##
*/
	else if (pc == 5)
		return '5 20 3';

/*
6 =
 ##
##
*/
	else if (pc == 6)
		return '20 5 3';

/*
7 =
 #
###
*/
	else if (pc == 7)
		return '4 21 3';


	else
		return '0 0 0';

}

// do x 1..4 and y 1..4 in case of rotation
float(float x, float y, float rot, float pc) PieceMetric =
{
	float t;
	vector piece_dat;
	float wid;
	piece_dat = PieceShape(pc);
	wid = piece_dat_z;
	
	// return bits of a piece
	if (rot == 1) // 90 degrees
	{
		t = y;
		y = x;
		x = wid - t;
	}
//	else if (rot == 2)//180
//	{
//		x = wid-1 - x;
//		y = 3 - y;
//	}
//	else if (rot == 3) // 270
//	{
//		t = y;
//		y = wid-1 - x;
//		x = t;
//	}
print(ftos(x));
print(", ");
print(ftos(y));
print("\n");
	if (x < 0 || y < 0 || x >= 4 || y >= 4)
		return 0;
	if (y == 0)
		return GetXBlock(x, piece_dat_x); // first row
	else if (y == 1)
		return GetXBlock(x, piece_dat_y); // second row
	else
		return 0; // illegal parms
};
/*
*********************************

Draw

*********************************
*/


/* some prydon gate functions to make life easier.... 

somewhat modified because we don't need all the fanciness Prydon Gate is capable of

*/

void(float c1, float c2, float c3, float c4, float c5, float c6) p6 =
{
	printline = strcat(printline, chr2str(c1, c2, c3, c4, c5, c6));
};

float(float num, float dig) pnum =
{
	local string str;
	str = ftos(num);
	printline = strcat(printline, str);
	num = strlen(str);
	while(num < dig)
	{
		printline = strcat(printline, " ");
		num++;
	}
	return 6;
};

void (float ln) DrawLine =
{
	float x, d;
	printline = strcat(printline, chr2str(TET_BORDER));
	
	for (x = 0; x < TET_WIDTH; x = x + 1)
	{
		d = GetSquare(x, ln);
		if (d)
			printline = strcat(printline, chr2str(TET_BLOCKS + d));
		else
			printline = strcat(printline, chr2str(TET_SPACE));
	}
	printline = strcat(printline, chr2str(TET_BORDER));
}

void (float pc, float ln) DrawPiece =
{
	float x, d, piece_ln, color;
	vector piece_dat;
	color = pc & 3;
	if (color == 0) // 4
		color = 1;
	printline = strcat(printline, chr2str(TET_SPACE));
	
	piece_dat = PieceShape(pc);
	if (ln == 1)
		piece_ln = piece_dat_x;
	else
		piece_ln = piece_dat_y;
	for (x = 0; x < 4; x = x + 1)
	{
		d = GetXBlock(x, piece_ln) * color;
		if (d)
			printline = strcat(printline, chr2str(TET_BLOCKS + d));
		else
			printline = strcat(printline, chr2str(TET_SPACE));
	}
	printline = strcat(printline, chr2str(TET_SPACE));
}
void() Draw_Tetris =
{
	float i;

	// decoration
	for (i = 0; i < (TET_WIDTH + 2); i = i + 1)
		printline = strcat(printline, chr2str(TET_BORDER));
	p6(' ', ' ', ' ', ' ', ' ', ' ');
	printline = strcat(printline, "\n");
	for (i = 0; i < TET_LINES; i = i + 1)
	{
		if (tetris_on == 2 && i == 11)
		{
			p6(TET_BORDER, 'G', 'A', 'M', 'E', ' ');
			p6('O', 'V', 'E', 'R', ' ', TET_BORDER);
		}
		else
			DrawLine(i);
		if (i == 1)
			p6(' ', 'N', 'E', 'X', 'T', ' ');
		else if (i == 3)
			DrawPiece(next_piece, 1);
		else if (i == 4)
			DrawPiece(next_piece, 2);
		else if (i == 6)
			p6(' ', 'L', 'I', 'N', 'E', 'S');
		else if (i == 7)
			pnum(tet_lines, 6);
		else if (i == 9)
			p6(' ', 'S', 'C', 'O', 'R', 'E');
		else if (i == 10)
			pnum(tet_score, 6);
		else if (i == 12)
			p6(' ', 'H', 'I', 'G', 'H', ' ');
		else if (i == 13)
			p6(' ', 'S', 'C', 'O', 'R', 'E');
		else if (i == 14)
			pnum(tet_high_score, 6);
		else if (i == 16)
			p6(' ', 'L', 'E', 'V', 'E', 'L');
		else if (i == 17)
			pnum(Tetris_Level(), 6);
		else
			p6(' ', ' ', ' ', ' ', ' ', ' ');
		printline = strcat(printline, "\n");
	}
	// decoration

	for (i = 0; i < (TET_WIDTH + 2); i = i + 1)
		printline = strcat(printline, chr2str(TET_BORDER));
	p6(' ', ' ', ' ', ' ', ' ', ' ');


	printline = strcat(printline, "\n");

	cprint(printline);
	printline = "";
}
/*
*********************************

Game Functions

*********************************
*/

// reset the game
void() ResetTetris =
{
	float i;
	
	for (i=0; i<TET_LINES; i = i + 1)
		SetLine(i, 0);
	piece_pos = '0 0 0';
	piece_type = 0;
	next_piece = tet_lines = tet_score = 0;
	
};

/*
*********************************

Game Mechanics

*********************************
*/
float() RandomPiece =
{
	return floor(random() * PIECES) + 1;
};

void(float n) TetAddScore =
{
	tet_score = tet_score + n * Tetris_Level();
	if (tet_score > tet_high_score)
		tet_high_score = tet_score; 
};
float CheckMetrics(float piece, float orgx, float orgy, float rot) = 
{
	// check to see if the piece, if moved to the locations will overlap
	
	float x, y;

	orgx = orgx;
	orgy = orgy;
	
	for (y = 0; y < 4; y = y + 1)	
	{
		for (x = 0; x < 4; x = x + 1)
		{
			if (PieceMetric(x, y, rot, piece))
			{
				if (GetSquare(x + orgx, y + orgy))
					return FALSE; // uhoh, gonna hit something.
				if (x+orgx<0 || x+orgx >= TET_WIDTH || y+orgy<0 || y+orgy>= TET_LINES)
					return FALSE; // ouside the level
			}
		}
	}
	return TRUE;
}

void ClearPiece(float piece, float orgx, float orgy, float rot) =
{
	
	float x, y;
	orgx = orgx;
	orgy = orgy;
	
	for (y = 0; y < 4; y = y + 1)	
	{
		for (x = 0; x < 4; x = x + 1)
		{
			if (PieceMetric(x, y, rot, piece))
			{
				SetSquare(x + orgx, y + orgy, 0);
			}
		}
	}
}
void CementPiece(float piece, float orgx, float orgy, float rot) =
{
	float color;
	float x, y;

	orgx = orgx;
	orgy = orgy;

	color = piece & 3;
	if (color == 0) // 4
		color = 1;
		
	for (y = 0; y < 4; y = y + 1)	
	{
		for (x = 0; x < 4; x = x + 1)
		{
			if (PieceMetric(x, y, rot, piece))
			{
				SetSquare(x + orgx, y + orgy, color);
			}
		}
	}
}

float LINE_LOW = 349525;
float LINE_HIGH = 699050; // above number times 2

void() CompletedLines =
{
	float y, cleared, ln;

	cleared = 0;
	y = TET_LINES-1;
	while(y >= 0)
	{
		ln = GetLine(y);
		if (((ln & LINE_LOW) | ((ln & LINE_HIGH)/2)) == LINE_LOW)
			cleared = cleared + 1;
		else
			y = y - 1;
		ln = GetLine(y - cleared);
		SetLine(y, ln);
	}
	if (cleared == 4)
		tetsnd("tetris");
	else if (cleared)
		tetsnd("tetline");
	else
		tetsnd("tetland");
	tet_lines = tet_lines + cleared;
	TetAddScore(cleared * cleared * 10);
};

void(float keys) HandleGame =
{
	vector check_pos, piece_data;
	float brand_new, nudge, i;
	brand_new = 0;
	nudge = 0;
	if ((tet_time > time) && !(keys & KEYS_IMMEDIATE))
		return;
	tet_time = time + 0.1;
	// first off, we need to see if we need a new piece

	if (tetris_on==2)
		return;

	if (piece_type == 0)
	{
		piece_pos = '5 1 0'; // that's about middle top, we count from 1 ARGH
		if (next_piece)
			piece_type = next_piece;
		else
			piece_type = RandomPiece();
		next_piece =  RandomPiece();
		keys = 0; // no movement first frame
		tet_autodown = time + 1;
		brand_new = 1;
	}
	else
		ClearPiece(piece_type, piece_pos_x, piece_pos_y, piece_pos_z);
	
	// next we need to check the piece metrics against what's on the level
	// based on the key order

	check_pos = piece_pos;

	if (keys & TETKEY_RIGHT)
	{
		check_pos_x = check_pos_x + 1;
		tetsnd("tetmove");
	}
	else if (keys & TETKEY_LEFT)
	{
		check_pos_x = check_pos_x - 1;
		tetsnd("tetmove");
	}
	else if (keys & TETKEY_ROTRIGHT)
	{
		check_pos_z = check_pos_z + 1;
		// nudge stuff
		piece_data = PieceShape(piece_type);
		nudge = piece_data_z - 2;
		tetsnd("tetrot");
	}
	else if (keys & TETKEY_ROTLEFT)
	{
		check_pos_z = check_pos_z - 1;
		// nudge stuff
		piece_data = PieceShape(piece_type);
		nudge = piece_data_z - 2;
		tetsnd("tetrot");
	}
	// bounds check
	if (check_pos_z > 3)
		check_pos_z = 0;
	else if (check_pos_z < 0)
		check_pos_z = 3;
	
	// reality check
	// more nudge stuff
	if (CheckMetrics(piece_type, check_pos_x, check_pos_y, check_pos_z))
		piece_pos = check_pos;
	else if (brand_new)
	{
		tetris_on = 2;
		return;
	}
	else if (nudge)
	{
		for (i = nudge; i >= (0-nudge); i = i - 1)
		{
			if (CheckMetrics(piece_type, check_pos_x + i, check_pos_y, check_pos_z))
			{
				piece_pos = check_pos + '1 0 0' * i;
				break;
			}
		}
	}
	check_pos = piece_pos;
	if (keys & TETKEY_DOWN)
		check_pos_y = check_pos_y + 1;
	else if (tet_autodown < time)
	{
		check_pos_y = check_pos_y + 1;
		tet_autodown = time + 1 / Tetris_Level();
	}
	if (CheckMetrics(piece_type, check_pos_x, check_pos_y, check_pos_z))
		piece_pos = check_pos;
	else
	{
		CementPiece(piece_type, piece_pos_x, piece_pos_y, piece_pos_z);
		TetAddScore(1);
		CompletedLines();
		piece_type = 0;
		return;
	}
	CementPiece(piece_type, piece_pos_x, piece_pos_y, piece_pos_z);
};

/*
*********************************

Important Linking Into Quake stuff

*********************************
*/

void(float event, float button, float mousex, float mousey) TetrisMenuEvent =
{
	local float key = 0;

	if (event == ME_DRAW)
	{			
		HandleGame(tet_keys);
		Draw_Tetris();
		return;
	}

	if (event == ME_MOUSEMOVE)	//0 is a mouse move
		return;

	if (button == K_ESCAPE && event == ME_KEYDOWN)
	{
		cprint("");
		Menu_Main();
		return;
	}
	switch(button)
	{
	case K_LEFTARROW:	key = TETKEY_LEFT;		break;
	case K_RIGHTARROW:	key = TETKEY_RIGHT; 	break;
	case K_UPARROW:		key = TETKEY_ROTRIGHT;	break;
	case K_SPACE:		key = TETKEY_DOWN;		break;
	case K_DOWNARROW:	key = TETKEY_DOWN;		break;
	default:
		return;	//we didn't recognise it.
	}

	if (event == ME_KEYDOWN)
		tet_keys = tet_keys | key;
	if (event == ME_KEYUP)
		tet_keys -= tet_keys&key;

	HandleGame(tet_keys);

	tet_keys -= tet_keys & KEYS_IMMEDIATE;
	tet_old_keys = tet_keys;
};

nonstatic void() Menu_Tetris =
{
	tetris_on = 1;
	ResetTetris();
	Menu_Activate(TetrisMenuEvent);
	MenuEventFunc = TetrisMenuEvent;
};
