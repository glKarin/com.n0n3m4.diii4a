// Copyright (C) 1999-2000 Id Software, Inc.
//
/**********************************************************************
	UI_QMENU.C

	Quake's menu framework system.
**********************************************************************/




#include "ui_local.h"

sfxHandle_t menu_in_sound;
sfxHandle_t menu_move_sound;
sfxHandle_t menu_out_sound;
sfxHandle_t menu_buzz_sound;
sfxHandle_t menu_null_sound;
sfxHandle_t weaponChangeSound;

static qhandle_t	sliderBar;
static qhandle_t	sliderButton_0;
static qhandle_t	sliderButton_1;

// NEW AND IMPLOVED colors
vec4_t menu_text_color		= {1.0f, 1.0f, 1.0f, 1.0f};
vec4_t menu_dim_color   	= {0.0f, 0.0f, 0.0f, 0.75f};
vec4_t color_black	    	= {0.00f, 0.00f, 0.00f, 1.00f};
vec4_t color_white	    	= {1.00f, 1.00f, 1.00f, 1.00f};
vec4_t color_yellow	    	= {1.00f, 1.00f, 0.00f, 1.00f};
vec4_t color_blue	    	= {0.00f, 0.00f, 1.00f, 1.00f};
vec4_t color_grey	    	= {0.30f, 0.45f, 0.58f, 1.00f};
vec4_t color_red			= {1.00f, 0.00f, 0.00f, 1.00f};
vec4_t color_dim	    	= {0.00f, 0.00f, 0.00f, 0.30f};
vec4_t color_dim80	    	= {0.00f, 0.00f, 0.00f, 0.80f};
vec4_t color_green	    	= {0.00f, 0.99f, 0.00f, 1.00f};
vec4_t color_emerald    	= {0.50f, 0.85f, 0.00f, 1.00f};
vec4_t color_bluo    		= {0.53f, 0.62f, 0.82f, 1.00f};
vec4_t color_lightyellow 	= {1.00f, 0.90f, 0.45f, 1.00f};
vec4_t color_highlight		= {0.53f, 0.62f, 0.82f, 1.00f};

// current color scheme
vec4_t pulse_color          = {1.00f, 1.00f, 1.00f, 1.00f};
vec4_t text_color_disabled  = {0.10f, 0.10f, 0.20f, 1.00f};
vec4_t text_color_normal    = {1.00f, 1.00f, 1.00f, 1.00f};
vec4_t text_color_status    = {1.00f, 1.00f, 1.00f, 1.00f};
vec4_t color_select_bluo    = {0.53f, 0.62f, 0.82f, 0.25f};

// action widget
static void	Action_Init( menuaction_s *a );
static void	Action_Draw( menuaction_s *a );

// radio button widget
static void	RadioButton_Draw( menuradiobutton_s *rb );
static sfxHandle_t RadioButton_Key( menuradiobutton_s *rb, int key );

// slider widget
static void Slider_Init( menuslider_s *s );
static sfxHandle_t Slider_Key( menuslider_s *s, int key );
static void	Slider_Draw( menuslider_s *s );

// spin control widget
static void	SpinControl_Draw( menulist_s *s );
static sfxHandle_t SpinControl_Key( menulist_s *l, int key );

// text widget
static void Text_Init( menutext_s *b );
static void Text_Draw( menutext_s *b );

// scrolllist widget
sfxHandle_t ScrollList_Key( menulist_s *l, int key );

// proportional text widget
static void PText_Draw( menutext_s *b );

// proportional banner text widget
static void BText_Init( menutext_s *b );
static void BText_Draw( menutext_s *b );

/*
===============
UI_FindItem

===============
*/
gitem_t	*UI_FindItem( const char *pickupName ) {
	gitem_t	*it;
	
	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( !Q_stricmp( it->pickup_name, pickupName ) )
			return it;
	}

	return NULL;
}

gitem_t	*UI_FindItemClassname( const char *classname ) {
	gitem_t	*it;
	
	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( !Q_stricmp( it->classname, classname ) )
			return it;
	}

	return NULL;
}

/*
=================
Text_Init
=================
*/
static void Text_Init( menutext_s *t )
{
	t->generic.flags |= QMF_INACTIVE;
}

/*
=================
Text_Draw
=================
*/
static void Text_Draw( menutext_s *t )
{
	int		x;
	int		y;
	char	buff[512];	
	float*	color;

	x = t->generic.x;
	y = t->generic.y;

	buff[0] = '\0';

	// possible label
	if (t->generic.name)
		strcpy(buff,t->generic.name);

	// possible value
	if (t->string)
		strcat(buff,t->string);
		
	if (t->generic.flags & QMF_GRAYED)
		color = text_color_disabled;
	else
		color = t->color;

	UI_DrawString( x, y, buff, t->style, color );
}

/*
=================
BText_Init
=================
*/
static void BText_Init( menutext_s *t )
{
	t->generic.flags |= QMF_INACTIVE;
}

/*
=================
BText_Draw
=================
*/
static void BText_Draw( menutext_s *t )
{
	int		x;
	int		y;
	float*	color;

	x = t->generic.x;
	y = t->generic.y;

	if (t->generic.flags & QMF_GRAYED)
		color = text_color_disabled;
	else
		color = t->color;

	UI_DrawString( x, y, t->string, t->style, color );
}

/*
=================
PText_Init
=================
*/
void PText_Init( menutext_s *t )
{
	int	x;
	int	y;
	int	w;
	int	h;
	float	sizeScale;

	sizeScale = UI_ProportionalSizeScale( t->style, t->customsize );

	x = t->generic.x;
	y = t->generic.y;
	w = UI_ProportionalStringWidth( t->string ) * (sizeScale*0.75);
	h =	PROP_HEIGHT * sizeScale;

	if( t->generic.flags & QMF_RIGHT_JUSTIFY ) {
		x -= w;
	}
	else if( t->generic.flags & QMF_CENTER_JUSTIFY ) {
		x -= w / 2;
	}

	t->generic.left   = x - PROP_GAP_WIDTH;
	t->generic.right  = x + w + PROP_GAP_WIDTH;
	if(t->generic.heightmod){
	t->generic.top    = y - (t->generic.heightmod*h);
	t->generic.bottom = y + (t->generic.heightmod*h);
	} else {
	t->generic.top    = y;
	t->generic.bottom = y + h;	
	}
}

/*
=================
PText_Draw
=================
*/
static void PText_Draw( menutext_s *t )
{
	int		x;
	int		xofs;
	int		y;
	float *	color;
	int		style;

	x = t->generic.x;
	xofs = t->generic.xoffset;
	y = t->generic.y;

	if (t->generic.flags & QMF_GRAYED)
		color = text_color_disabled;
	else
		color = t->color;
	
	if( t->generic.flags & QMF_HIGHLIGHT_IF_FOCUS ) {
	if( Menu_ItemAtCursor( t->generic.parent ) == t ) {
		t->color = color_highlight;
	}
	else {
		t->color = color_white;
	}
	}

	if( t->generic.flags & QMF_HIGHLIGHT ) {
		t->color = color_grey;
	}

	style = t->style;
	if( t->generic.flags & QMF_PULSEIFFOCUS ) {
		t->color = color_white;
		if( Menu_ItemAtCursor( t->generic.parent ) == t ) {
			style |= UI_PULSE;
		}
		else {
			style |= UI_INVERSE;
		}
	}

	UI_DrawStringCustom( x+xofs, y, t->string, style, color, t->customsize, 512 );
}

/*
=================
Bitmap_Init
=================
*/
void Bitmap_Init( menubitmap_s *b )
{
	int	x;
	int	y;
	int	w;
	int	h;

	x = b->generic.x;
	y = b->generic.y;
	w = b->width;
	h =	b->height;
	if( w < 0 ) {
		w = -w;
	}
	if( h < 0 ) {
		h = -h;
	}

	if (b->generic.flags & QMF_RIGHT_JUSTIFY)
	{
		x = x - w;
	}
	else if (b->generic.flags & QMF_CENTER_JUSTIFY)
	{
		x = x - w/2;
	}

	b->generic.left   = x;
	b->generic.right  = x + w;
	b->generic.top    = y;
	b->generic.bottom = y + h;

	b->shader      = 0;
	b->focusshader = 0;
}

/*
=================
Bitmap_Draw
=================
*/
void Bitmap_Draw( menubitmap_s *b )
{
	float	x;
	float	y;
	float	w;
	float	h;
	vec4_t	tempcolor;
	float*	color;

	x = b->generic.x;
	y = b->generic.y;
	w = b->width;
	h =	b->height;

	if (b->generic.flags & QMF_RIGHT_JUSTIFY)
	{
		x = x - w;
	}
	else if (b->generic.flags & QMF_CENTER_JUSTIFY)
	{
		x = x - w/2;
	}

	// used to refresh shader
	if (b->generic.name && !b->shader)
	{
		b->shader = trap_R_RegisterShaderNoMip( b->generic.name );
		if (!b->shader && b->errorpic)
			b->shader = trap_R_RegisterShaderNoMip( b->errorpic );
	}

	if (b->focuspic && !b->focusshader)
		b->focusshader = trap_R_RegisterShaderNoMip( b->focuspic );

	if (b->generic.flags & QMF_GRAYED)
	{
		if (b->shader)
		{
			trap_R_SetColor( colorMdGrey );
			UI_DrawHandlePic( x, y, w, h, b->shader );
			trap_R_SetColor( NULL );
		}
	}
	else
	{
		if (b->shader)
			UI_DrawHandlePic( x, y, w, h, b->shader );

		if ((b->generic.flags & QMF_PULSE) || (b->generic.flags & QMF_PULSEIFFOCUS) && (Menu_ItemAtCursor( b->generic.parent ) == b))
		{	
			if (b->focuscolor)			
			{
				tempcolor[0] = b->focuscolor[0];
				tempcolor[1] = b->focuscolor[1];
				tempcolor[2] = b->focuscolor[2];
				color        = tempcolor;	
			}
			else
				color = pulse_color;
			color[3] = 0.5+0.5*sin(uis.realtime/PULSE_DIVISOR);

			trap_R_SetColor( color );
			UI_DrawHandlePic( x, y, w, h, b->focusshader );
			trap_R_SetColor( NULL );
		}
		else if ((b->generic.flags & QMF_HIGHLIGHT) || ((b->generic.flags & QMF_HIGHLIGHT_IF_FOCUS) && (Menu_ItemAtCursor( b->generic.parent ) == b)))
		{	
			if (b->focuscolor)
			{
				trap_R_SetColor( b->focuscolor );
				UI_DrawHandlePic( x, y, w, h, b->focusshader );
				trap_R_SetColor( NULL );
			}
			else
				UI_DrawHandlePic( x, y, w, h, b->focusshader );
		}
	}
}

/*
=================
UIObject_Init
=================
*/
void UIObject_Init( menuobject_s *b )
{
	int	x;
	int	y;
	int	w;
	int	h;
	int	l;
	int	len;

if(b->type >= 1 && b->type <= 3 || b->type == 6){
	x = b->generic.x;
	y = b->generic.y;
	w = b->width;
	h =	b->height;
	if( w < 0 ) {
		w = -w;
	}
	if( h < 0 ) {
		h = -h;
	}

	b->generic.left   = x;
	b->generic.right  = x + w;
	b->generic.top    = y;
	b->generic.bottom = y + h;
}
if(b->type == 4){

	MField_Clear( &b->field );

	if (b->generic.flags & QMF_SMALLFONT)
	{
		w = SMALLCHAR_WIDTH;
		h = SMALLCHAR_HEIGHT;
	}
	else
	{
		w = BIGCHAR_WIDTH;
		h = BIGCHAR_HEIGHT;
	}	

	if (b->generic.name) {
		l = (strlenru( b->generic.name )+1) * w;		
	}
	else {
		l = 0;
	}

	b->generic.left   = b->generic.x - l;
	b->generic.top    = b->generic.y;
	b->generic.right  = b->generic.x + (w*b->fontsize) + b->field.widthInChars*(w*b->fontsize);
	b->generic.bottom = b->generic.y + (h*b->fontsize);
}
if(b->type == 5){
	
	b->oldvalue = 0;
	b->curvalue = 0;
	b->top      = 0;

	if( !b->columns ) {
		b->columns = 1;
		b->seperation = 0;
	}
	if(b->styles <= 1){
	w = ( (b->width + b->seperation) * b->columns - b->seperation) * (SMALLCHAR_WIDTH*b->fontsize);
	}
	if(b->styles == 2){
	w = ( (b->width + b->seperation) * b->columns - b->seperation) * (SMALLCHAR_WIDTH);
	}

	b->generic.left   =	b->generic.x;
	b->generic.top    = b->generic.y;	
	b->generic.right  =	b->generic.x + w;
	if(b->styles <= 1){
	b->generic.bottom =	b->generic.y + b->height * (SMALLCHAR_HEIGHT*b->fontsize);
	}
	if(b->styles == 2){
	b->generic.bottom =	b->generic.y + b->height * (SMALLCHAR_WIDTH*b->width);
	}

	if( b->generic.flags & QMF_CENTER_JUSTIFY ) {
		b->generic.left -= w / 2;
		b->generic.right -= w / 2;
	}
}

if(b->type == 7){
	// calculate bounds
	if (b->generic.text)
		len = strlenru(b->generic.text);
	else
		len = 0;
	

	b->generic.left   = b->generic.x - (len+1)*(SMALLCHAR_WIDTH*b->fontsize);
	b->generic.right  = b->generic.x + 6*(SMALLCHAR_WIDTH*b->fontsize);
	b->generic.top    = b->generic.y;
	b->generic.bottom = b->generic.y + (SMALLCHAR_HEIGHT*b->fontsize);
}

if(b->type == 8){
	// calculate bounds
	if (b->generic.text)
		len = strlenru(b->generic.text);
	else
		len = 0;

	b->generic.left   = b->generic.x - (len+1)*(SMALLCHAR_WIDTH*b->fontsize); 
	b->generic.right  = b->generic.x + (SLIDER_RANGE+2+1)*(SMALLCHAR_WIDTH*b->fontsize);
	b->generic.top    = b->generic.y;
	b->generic.bottom = b->generic.y + (SMALLCHAR_HEIGHT*b->width);
}

}

/*
=================
UIObject_Draw
=================
*/
void UIObject_Draw( menuobject_s *b ){
	float		x;
	float		y;
	float		w;
	float		h;
	int			style;
	qboolean 	focus;
	float		*color;
	int			u;
	int			i;
	int			base;
	int			column;
	qboolean	hasfocus;
	int			val;
	int			button;
	gitem_t		*it;
	const char	*info;
	char		pic[MAX_QPATH];
	float		select_x;
	float		select_y;
	int			select_i;

	
	if(b->type >= 1 && b->type <= 3 || b->type == 6){
		x = b->generic.x;
		y = b->generic.y;
		w = b->width;
		h =	b->height;

		b->shader = trap_R_RegisterShaderNoMip( b->generic.picn );

		if(b->type == 1){
			UI_DrawRoundedRect(x, y, w, h, b->corner, b->color2);
		}
		if(b->type == 2){
			UI_DrawHandlePic( x, y, w, h, b->shader );
		}
		if(b->type == 3){
			UI_DrawRoundedRect(x, y, w, h, b->corner, b->color2);
			UI_DrawHandlePic( x, y, w, h, b->shader );
		}
		if(b->type == 6){
			UI_DrawHandleModel( x, y, w, h, b->generic.picn, b->corner );
		}
		UI_DrawStringCustom( x, y, b->string, b->style, b->color, b->fontsize, 512 );
	}
	if(b->type == 4){
		x =	b->generic.x;
		y =	b->generic.y;

		if (b->generic.flags & QMF_SMALLFONT)
		{
			w = SMALLCHAR_WIDTH;
			h = SMALLCHAR_HEIGHT;
			style = UI_SMALLFONT;
		}
		else
		{
			w = BIGCHAR_WIDTH;
			h = BIGCHAR_HEIGHT;
			style = UI_BIGFONT;
		}	

		if (Menu_ItemAtCursor( b->generic.parent ) == b) {
			focus = qtrue;
			style |= UI_PULSE;
		}
		else {
			focus = qfalse;
		}

		if (b->generic.flags & QMF_GRAYED) {
			color = text_color_disabled;
		} else if (focus) {
			color = color_highlight;
		} else {
			if(!b->color){
				color = text_color_normal;
			} else {
				color = b->color;
			}
		}

		if ( focus ) {
			UI_DrawCharCustom( x, y, 13, UI_CENTER|UI_BLINK|style, color, b->fontsize);
		}
		if ( b->generic.text ) {
			UI_DrawStringCustom( x - w, y, b->generic.text, style|UI_RIGHT, color, b->fontsize, 512 );
		}
		MField_DrawCustom( &b->field, x + w, y, style, color, b->fontsize );
	}

	if(b->type == 5){
		hasfocus = (b->generic.parent->cursor == b->generic.menuPosition);
	
		x =	b->generic.x;
		
		for (column = 0; column < b->columns; column++) {
		    y = b->generic.y;
		    // Calculate the base index using the top variable
		    for (base = 0; base < b->height; base++) {
		        // Calculate the index based on the column and row, offset by the top variable
		        i = (base * b->columns + column) + b->top;

		        // Check if the calculated index is within the number of items
		        if (i >= b->numitems)
		            break;
				
				color = b->color;
		        style = UI_LEFT | UI_SMALLFONT;

		        if (b->generic.flags & QMF_CENTER_JUSTIFY) {
		            style |= UI_CENTER;
		        }
				if(b->styles <= 0){
					UI_DrawStringCustom(x,y,b->itemnames[i],style,color, b->fontsize, 512 );
				}
				if(b->styles == 1){
					UI_DrawStringCustom(x+SMALLCHAR_HEIGHT*b->fontsize,y,b->itemnames[i],style,color, b->fontsize, 512 );
					b->shader = trap_R_RegisterShaderNoMip( va("%s/%s", b->string, b->itemnames[i]) );
					if(b->shader){
						UI_DrawHandlePic( x, y, SMALLCHAR_HEIGHT*b->fontsize, SMALLCHAR_HEIGHT*b->fontsize, trap_R_RegisterShaderNoMip( va("%s/%s", b->string, b->itemnames[i]) ) );
					}
					b->model = trap_R_RegisterModel( va("%s/%s", b->string, b->itemnames[i]) );
					if(b->model){
						UI_DrawHandleModel( x, y, SMALLCHAR_HEIGHT*b->fontsize, SMALLCHAR_HEIGHT*b->fontsize, va("%s/%s", b->string, b->itemnames[i]), b->corner );
					}
					if(!b->shader && !b->model){
						info = UI_GetBotInfoByName( b->itemnames[i] );
						UI_ServerPlayerIcon( Info_ValueForKey( info, "model" ), pic, MAX_QPATH );
						b->shader = trap_R_RegisterShaderNoMip( pic );
						if(b->shader){
							UI_DrawHandlePic( x, y, SMALLCHAR_HEIGHT*b->fontsize, SMALLCHAR_HEIGHT*b->fontsize, trap_R_RegisterShaderNoMip( pic ));
						}
					}
					it = UI_FindItem(b->itemnames[i]);
					if(it->classname && it->icon && !b->model && !b->shader){
						UI_DrawHandlePic( x, y, SMALLCHAR_HEIGHT*b->fontsize, SMALLCHAR_HEIGHT*b->fontsize, trap_R_RegisterShaderNoMip( it->icon ) );
					}
					if(!it->classname){
						it = UI_FindItemClassname(b->itemnames[i]);
						if(it->classname && !b->model && !b->shader){
							b->model = trap_R_RegisterModel( it->world_model[0] );
							if(b->model){
								UI_DrawHandleModel( x, y, SMALLCHAR_HEIGHT*b->fontsize, SMALLCHAR_HEIGHT*b->fontsize, it->world_model[0], b->corner );
							}
						}
					}
					if(!b->shader && !b->model && !it){
						UI_DrawHandlePicFile( x, y, SMALLCHAR_HEIGHT*b->fontsize, SMALLCHAR_HEIGHT*b->fontsize, va("%s/%s", b->string, b->itemnames[i]) );
					}
				}
					if(b->styles == 2){
						b->shader = trap_R_RegisterShaderNoMip( va("%s/%s", b->string, b->itemnames[i]) );
						if(b->shader){
							UI_DrawHandlePic( x, y, SMALLCHAR_WIDTH*b->width, SMALLCHAR_WIDTH*b->width, trap_R_RegisterShaderNoMip( va("%s/%s", b->string, b->itemnames[i]) ) );
						}
						b->model = trap_R_RegisterModel( va("%s/%s", b->string, b->itemnames[i]) );
						if(b->model){
							UI_DrawHandleModel( x, y, (float)(SMALLCHAR_WIDTH*b->width), (float)(SMALLCHAR_WIDTH*b->width), va("%s/%s", b->string, b->itemnames[i]), b->corner );
						}
						if(!b->shader && !b->model){
							info = UI_GetBotInfoByName( b->itemnames[i] );
							UI_ServerPlayerIcon( Info_ValueForKey( info, "model" ), pic, MAX_QPATH );
							b->shader = trap_R_RegisterShaderNoMip( pic );
							if(b->shader){
								UI_DrawHandlePic( x, y, SMALLCHAR_WIDTH*b->width, SMALLCHAR_WIDTH*b->width, trap_R_RegisterShaderNoMip( pic ));
							}
						}
						it = UI_FindItem(b->itemnames[i]);
						if(it->classname && it->icon && !b->model && !b->shader){
							UI_DrawHandlePic( x, y, SMALLCHAR_WIDTH*b->width, SMALLCHAR_WIDTH*b->width, trap_R_RegisterShaderNoMip( it->icon ) );
						}
						if(!it->classname){
							it = UI_FindItemClassname(b->itemnames[i]);
							if(it->classname && !b->model && !b->shader){
								b->model = trap_R_RegisterModel( it->world_model[0] );
								if(b->model){
									UI_DrawHandleModel( x, y, (float)(SMALLCHAR_WIDTH*b->width), (float)(SMALLCHAR_WIDTH*b->width), it->world_model[0], b->corner );
								}
							}
						}
						if(!b->shader && !b->model && !it){
							UI_DrawHandlePicFile( x, y, (float)(SMALLCHAR_WIDTH*b->width), (float)(SMALLCHAR_WIDTH*b->width), va("%s/%s", b->string, b->itemnames[i]) );
						}
						if(UI_CursorInRect( x, y, SMALLCHAR_WIDTH*b->width, SMALLCHAR_WIDTH*b->width) && hasfocus){
							select_x = x;
							select_y = y;
							select_i = i;
						}
					}
		        	if (i == b->curvalue) {
		        	    u = x;
		        	    if (b->generic.flags & QMF_CENTER_JUSTIFY) {
		        	        if (b->styles <= 1) {
		        	            u -= (b->width * (SMALLCHAR_WIDTH * b->fontsize)) / 2 + 1;
		        	        }
		        	        if (b->styles == 2) {
		        	            u -= (b->width * (SMALLCHAR_WIDTH)) / 2 + 1;
		        	        }
		        	    }
		        	    if (b->styles <= 1) {
		        	        UI_FillRect(u, y, (b->width * SMALLCHAR_WIDTH) * b->fontsize, (SMALLCHAR_HEIGHT) * b->fontsize, color_select_bluo);
		        	    }
		        	    if (b->styles == 2) {
		        	        UI_FillRect(u, y, (b->width * SMALLCHAR_WIDTH), (b->width * SMALLCHAR_WIDTH), color_select_bluo);
							UI_FillRect(u, y+(b->width * SMALLCHAR_WIDTH)-2, (b->width * SMALLCHAR_WIDTH), 2, color_bluo);
		        	    }
		        	}
					if(b->styles <= 1){
						y += (SMALLCHAR_HEIGHT*b->fontsize);
					}
					if(b->styles == 2){
						y += (b->width*SMALLCHAR_WIDTH);
					}
			}
			if(b->styles <= 1){
				x += (b->width + b->seperation) * (SMALLCHAR_WIDTH*b->fontsize);
			}
			if(b->styles == 2){
				x += (b->width + b->seperation) * (SMALLCHAR_WIDTH);
			}
		}
		if(strlen(b->itemnames[select_i]) > 0 && b->styles == 2 && hasfocus && UI_CursorInRect( select_x, select_y, SMALLCHAR_WIDTH*b->width, SMALLCHAR_WIDTH*b->width)){
			float wordsize = (SMALLCHAR_HEIGHT*b->fontsize);
			select_x += wordsize;
			select_y -= wordsize*1.45;
			UI_DrawRoundedRect(select_x-wordsize, select_y-4, (strlen(b->itemnames[select_i])*(SMALLCHAR_HEIGHT*b->fontsize)+wordsize*2), (SMALLCHAR_HEIGHT*b->fontsize)+8, 5, color_dim80);
			UI_DrawStringCustom( select_x, select_y, b->itemnames[select_i], style|UI_DROPSHADOW, color_bluo, b->fontsize, 512 );
		}
	}

	if(b->type == 7){
		x = b->generic.x;
		y = b->generic.y;

		focus = (b->generic.parent->cursor == b->generic.menuPosition);

		if ( b->generic.flags & QMF_GRAYED )
		{
			color = text_color_disabled;
			style = UI_LEFT|UI_SMALLFONT;
		}
		else if ( focus )
		{
			color = color_highlight;
			style = UI_LEFT|UI_PULSE|UI_SMALLFONT;
		}
		else
		{
			if(!b->color){
			color = text_color_normal;
			} else {
			color = b->color;
			}
			style = UI_LEFT|UI_SMALLFONT;
		}

		if ( focus )
		{
			UI_DrawCharCustom( x, y, 13, UI_CENTER|UI_BLINK|UI_SMALLFONT, color, b->fontsize);
		}

		if ( b->generic.text )
			UI_DrawStringCustom( x - SMALLCHAR_WIDTH, y, b->generic.text, UI_RIGHT|UI_SMALLFONT, color, b->fontsize, 512 );

		if ( !b->curvalue )
		{
			UI_DrawHandlePic( x + SMALLCHAR_WIDTH*b->fontsize, y + 2, 12*b->fontsize, 12*b->fontsize, uis.rb_off);
			UI_DrawStringCustom( x + SMALLCHAR_WIDTH*b->fontsize + 12*b->fontsize, y, "off", style, color, b->fontsize, 512 );
		}
		else
		{
			UI_DrawHandlePic( x + SMALLCHAR_WIDTH*b->fontsize, y + 2, 12*b->fontsize, 12*b->fontsize, uis.rb_on );
			UI_DrawStringCustom( x + SMALLCHAR_WIDTH*b->fontsize + 12*b->fontsize, y, "on", style, color, b->fontsize, 512 );
		}
	}

	if(b->type == 8){
		x =	b->generic.x;
		y = b->generic.y;
		val = b->curvalue;
		focus = (b->generic.parent->cursor == b->generic.menuPosition);
	
		if( b->generic.flags & QMF_GRAYED ) {
			color = text_color_disabled;
			style = UI_SMALLFONT;
		}
		else if( focus ) {
			color  = color_highlight;
			style = UI_SMALLFONT | UI_PULSE;
		}
		else {
			color = b->color;
			style = UI_SMALLFONT;
		}
	
		// draw label
		UI_DrawStringCustom( x - (SMALLCHAR_WIDTH*b->fontsize), y, b->generic.text, UI_RIGHT|style, color, b->fontsize, 512 );
		UI_DrawStringCustom( x + (SMALLCHAR_WIDTH*b->fontsize)*11, y, va("%i", val), UI_LEFT|style, color, b->fontsize, 512 );
	
		// draw slider
		UI_SetColor( color );
		UI_DrawHandlePic( x + (SMALLCHAR_WIDTH*b->fontsize), y, 93*b->fontsize, 11*b->fontsize, sliderBar );
		UI_SetColor( NULL );
	
		// clamp thumb
		if( b->maxvalue > b->minvalue )	{
			b->range = ( b->curvalue - b->minvalue ) / ( float ) ( b->maxvalue - b->minvalue );
			if( b->range < 0 ) {
				b->range = 0;
			}
			else if( b->range > 1) {
				b->range = 1;
			}
		}
		else {
			b->range = 0;
		}
	
		// draw thumb
		if( style & UI_PULSE) {
			button = sliderButton_1;
		}
		else {
			button = sliderButton_0;
		}
	
		UI_DrawHandlePic( (int)( x + 2*(SMALLCHAR_WIDTH*b->fontsize) + (SLIDER_RANGE-1)*(SMALLCHAR_WIDTH*b->fontsize)* b->range ) - 2, y - 2*b->fontsize, 9*b->fontsize, 16*b->fontsize, button );
	}

}

/*
==================
UIObject_Key
==================
*/
sfxHandle_t UIObject_Key( menuobject_s* b, int key )
{
	static int clicktime = 0;
	int	x;
	int	y;
	int	w;
	int	i;
	int	j;
	int	c;
	int	cursorx;
	int	cursory;
	int	column;
	int	index;
	int clickdelay;
	int keycode;
	static int lastKeypress = 0;
	sfxHandle_t	sound;
	int			oldvalue;
	if(b->type == 4){

	keycode = key;

	switch ( keycode )
	{
		case K_KP_ENTER:
		case K_ENTER:
		case K_MOUSE1:
			break;
		case K_JOY1:
		case K_JOY2:
		case K_JOY3:
		case K_JOY4:
			// have enter go to next cursor point
			key = K_TAB;
			break;

		case K_TAB:
		case K_KP_DOWNARROW:
		case K_DOWNARROW:
		case K_KP_UPARROW:
		case K_UPARROW:
			break;

		default:
			if ( keycode & K_CHAR_FLAG )
			{
				keycode &= ~K_CHAR_FLAG;

				if ((b->generic.flags & QMF_UPPERCASE) && Q_islower( keycode ))
					keycode -= 'a' - 'A';
				else if ((b->generic.flags & QMF_LOWERCASE) && Q_isupper( keycode ))
					keycode -= 'A' - 'a';
				else if ((b->generic.flags & QMF_NUMBERSONLY) && Q_isalpha( keycode ))
					return (menu_buzz_sound);

				MField_CharEvent( &b->field, keycode);
			}
			else
				MField_KeyDownEvent( &b->field, keycode );
			break;
	}
	lastKeypress = uis.realtime;

	return (0);
	}
	if(b->type == 5){
	switch (key)
	{
		case K_MOUSE1:
			if (b->generic.flags & QMF_HASMOUSEFOCUS)
			{
				// check scroll region
				x = b->generic.x;
				y = b->generic.y;
				if(b->styles <= 1){
				w = ( (b->width + b->seperation) * b->columns - b->seperation) * (SMALLCHAR_WIDTH*b->fontsize);
				}
				if(b->styles == 2){
				w = ( (b->width + b->seperation) * b->columns - b->seperation) * (SMALLCHAR_WIDTH);
				}
				if( b->generic.flags & QMF_CENTER_JUSTIFY ) {
					x -= w / 2;
				}
				
				if(b->styles <= 1){
				if (UI_CursorInRect( x, y, w, b->height*(SMALLCHAR_HEIGHT*b->fontsize) ))
				{
					cursorx = (uis.cursorx - x)/(SMALLCHAR_WIDTH*b->fontsize);
					column = cursorx / (b->width + b->seperation);
					cursory = (uis.cursory - y)/(SMALLCHAR_HEIGHT*b->fontsize);
					index = (cursory * b->columns) + column;
					if (b->top + index < b->numitems)
					{
						b->oldvalue = b->curvalue;
						b->curvalue = b->top + index;

						clickdelay = uis.realtime - clicktime;
						clicktime = uis.realtime;
						if (b->oldvalue != b->curvalue)
						{
							if (b->generic.callback) {
								b->generic.callback( b, QM_GOTFOCUS );
							}
							return (menu_move_sound);
						}
						else {
							// double click
							if ((clickdelay < 350) && !(b->generic.flags & (QMF_GRAYED|QMF_INACTIVE)))
							{
								return (Menu_ActivateItem( b->generic.parent, &b->generic ));
							}
						}
					}
				}
				}
				if(b->styles == 2){
				if (UI_CursorInRect( x, y, w, b->height * (SMALLCHAR_WIDTH*b->width) ))
				{
					cursorx = (uis.cursorx - x)/(SMALLCHAR_WIDTH);
					column = cursorx / (b->width + b->seperation);
					cursory = (uis.cursory - y)/(SMALLCHAR_WIDTH*b->width);
					index = (cursory * b->columns) + column;
					if (b->top + index < b->numitems)
					{
						b->oldvalue = b->curvalue;
						b->curvalue = b->top + index;

						clickdelay = uis.realtime - clicktime;
						clicktime = uis.realtime;
						if (b->oldvalue != b->curvalue)
						{
							if (b->generic.callback) {
								b->generic.callback( b, QM_GOTFOCUS );
							}
							return (menu_move_sound);
						}
						else {
							// double click
							if ((clickdelay < 350) && !(b->generic.flags & (QMF_GRAYED|QMF_INACTIVE)))
							{
								return (Menu_ActivateItem( b->generic.parent, &b->generic ));
							}
						}
					}
				}
				}

				// absorbed, silent sound effect
				return (menu_null_sound);
			}
			break;

		case K_KP_HOME:
		case K_HOME:
			b->oldvalue = b->curvalue;
			b->curvalue = 0;
			b->top      = 0;

			if (b->oldvalue != b->curvalue && b->generic.callback)
			{
				b->generic.callback( b, QM_GOTFOCUS );
				return (menu_move_sound);
			}
			return (menu_buzz_sound);

		case K_KP_END:
		case K_END:
			b->oldvalue = b->curvalue;
			b->curvalue = b->numitems-1;
			if( b->columns > 1 ) {
				c = (b->curvalue / b->height + 1) * b->height;
				b->top = c - (b->columns * b->height);
			}
			else {
				b->top = b->curvalue - (b->height - 1);
			}
			if (b->top < 0)
				b->top = 0;			

			if (b->oldvalue != b->curvalue && b->generic.callback)
			{
				b->generic.callback( b, QM_GOTFOCUS );
				return (menu_move_sound);
			}
			return (menu_buzz_sound);

		case K_PGUP:
		case K_KP_PGUP:
			if( b->columns > 1 ) {
				return menu_null_sound;
			}

			if (b->curvalue > 0)
			{
				b->oldvalue = b->curvalue;
				b->curvalue -= b->height-1;
				if (b->curvalue < 0)
					b->curvalue = 0;
				b->top = b->curvalue;
				if (b->top < 0)
					b->top = 0;

				if (b->generic.callback)
					b->generic.callback( b, QM_GOTFOCUS );

				return (menu_move_sound);
			}
			return (menu_buzz_sound);

		case K_PGDN:
		case K_KP_PGDN:
			if( b->columns > 1 ) {
				return menu_null_sound;
			}

			if (b->curvalue < b->numitems-1)
			{
				b->oldvalue = b->curvalue;
				b->curvalue += b->height-1;
				if (b->curvalue > b->numitems-1)
					b->curvalue = b->numitems-1;
				b->top = b->curvalue - (b->height-1);
				if (b->top < 0)
					b->top = 0;

				if (b->generic.callback)
					b->generic.callback( b, QM_GOTFOCUS );

				return (menu_move_sound);
			}
			return (menu_buzz_sound);

		case K_KP_UPARROW:
		case K_UPARROW:
		case K_MWHEELUP:
			if(b->columns <= 1){
				UIObject_Key(b, K_LEFTARROW);
				return menu_null_sound; //karin: return missing
			}
			if( b->columns == 1 ) {
				return menu_null_sound;
			}

			if( b->curvalue < b->height ) {
				return menu_buzz_sound;
			}

			b->oldvalue = b->curvalue;
			b->curvalue -= b->columns;

			if( b->curvalue < b->top ) {
				b->top -= b->columns;
			}

			if(b->top < 0 || b->curvalue < 0){
				b->curvalue = 0;
				b->top = 0;
			}

			if( b->generic.callback ) {
				b->generic.callback( b, QM_GOTFOCUS );
			}

			return menu_move_sound;

		case K_KP_DOWNARROW:
		case K_DOWNARROW:
		case K_MWHEELDOWN:
			if(b->columns <= 1){
				UIObject_Key(b, K_RIGHTARROW);
				return menu_null_sound; //karin: return missing
			}
			if( b->columns == 1 ) {
				return menu_null_sound;
			}

			if(b->curvalue + b->columns >= b->numitems){
			c = b->numitems - 1;
			} else {
			c = b->curvalue + b->columns;
			}

			if( c >= b->numitems ) {
				return menu_buzz_sound;
			}

			b->oldvalue = b->curvalue;
			b->curvalue = c;

			if( b->curvalue > b->top + b->columns * b->height - 1 ) {
				b->top += b->columns;
			}

			if( b->generic.callback ) {
				b->generic.callback( b, QM_GOTFOCUS );
			}

			return menu_move_sound;

		case K_KP_LEFTARROW:
		case K_LEFTARROW:
			if( b->curvalue == 0 ) {
				return menu_buzz_sound;
			}

			b->oldvalue = b->curvalue;
			b->curvalue--;

			if( b->curvalue < b->top ) {
				if( b->columns == 1 ) {
					b->top--;
				}
				else {
					b->top -= b->columns;
				}
			}

			if(b->top < 0 || b->curvalue < 0){
				b->curvalue = 0;
				b->top = 0;
			}

			if( b->generic.callback ) {
				b->generic.callback( b, QM_GOTFOCUS );
			}

			return (menu_move_sound);

		case K_KP_RIGHTARROW:
		case K_RIGHTARROW:
			if( b->curvalue == b->numitems - 1 ) {
				return menu_buzz_sound;
			}

			b->oldvalue = b->curvalue;
			b->curvalue++;

			if( b->curvalue >= b->top + b->columns * b->height ) {
				if( b->columns == 1 ) {
					b->top++;
				}
				else {
					b->top += b->columns;
				}
			}

			if( b->generic.callback ) {
				b->generic.callback( b, QM_GOTFOCUS );
			}

			return menu_move_sound;
	}

	// cycle look for ascii key inside list items
	if ( !Q_isprint( key ) )
		return (0);

	// force to lower for case insensitive compare
	if ( Q_isupper( key ) )
	{
		key -= 'A' - 'a';
	}

	// iterate list items
	for (i=1; i<=b->numitems; i++)
	{
		j = (b->curvalue + i) % b->numitems;
		c = b->itemnames[j][0];
		if ( Q_isupper( c ) )
		{
			c -= 'A' - 'a';
		}

		if (c == key)
		{
			// set current item, mimic windows listbox scroll behavior
			if (j < b->top)
			{
				// behind top most item, set this as new top
				b->top = j;
			}
			else if (j > b->top+b->height-1)
			{
				// past end of list box, do page down
				b->top = (j+1) - b->height;
			}
			
			if (b->curvalue != j)
			{
				b->oldvalue = b->curvalue;
				b->curvalue = j;
				if (b->generic.callback)
					b->generic.callback( b, QM_GOTFOCUS );
				return ( menu_move_sound );			
			}

			return (menu_buzz_sound);
		}
	}

	return (menu_buzz_sound);
	}
if(b->type == 7){
	switch (key)
	{
		case K_MOUSE1:
			if (!(b->generic.flags & QMF_HASMOUSEFOCUS))
				break;

		case K_JOY1:
		case K_JOY2:
		case K_JOY3:
		case K_JOY4:
		case K_ENTER:
		case K_KP_ENTER:
		case K_KP_LEFTARROW:
		case K_LEFTARROW:
		case K_KP_RIGHTARROW:
		case K_RIGHTARROW:
			b->curvalue = !b->curvalue;
			if ( b->generic.callback )
				b->generic.callback( b, QM_ACTIVATED );

			return (menu_move_sound);
	}
}

if(b->type == 8){
	switch (key)
	{
		case K_MOUSE1:
			x           = uis.cursorx - b->generic.x - 2*(SMALLCHAR_WIDTH*b->fontsize);
			oldvalue    = b->curvalue;
			b->curvalue = (x/(float)(SLIDER_RANGE*(SMALLCHAR_WIDTH*b->fontsize))) * (b->maxvalue-b->minvalue) + b->minvalue;

			if (b->curvalue < b->minvalue)
				b->curvalue = b->minvalue;
			else if (b->curvalue > b->maxvalue)
				b->curvalue = b->maxvalue;
			if (b->curvalue != oldvalue)
				sound = menu_move_sound;
			else
				sound = 0;
			break;

		case K_KP_LEFTARROW:
		case K_LEFTARROW:
			if (b->curvalue > b->minvalue)
			{
				b->curvalue--;
				sound = menu_move_sound;
			}
			else
				sound = menu_buzz_sound;
			break;			

		case K_KP_RIGHTARROW:
		case K_RIGHTARROW:
			if (b->curvalue < b->maxvalue)
			{
				b->curvalue++;
				sound = menu_move_sound;
			}
			else
				sound = menu_buzz_sound;
			break;			

		default:
			// key not handled
			sound = 0;
			break;
	}

	if ( sound && b->generic.callback )
		b->generic.callback( b, QM_ACTIVATED );

	return (sound);
}
	return (menu_buzz_sound);
}

/*
=================
Action_Init
=================
*/
static void Action_Init( menuaction_s *a )
{
	int	len;

	// calculate bounds
	if (a->generic.name)
		len = strlenru(a->generic.name);
	else
		len = 0;

	// left justify text
	a->generic.left   = a->generic.x; 
	a->generic.right  = a->generic.x + len*BIGCHAR_WIDTH;
	a->generic.top    = a->generic.y;
	a->generic.bottom = a->generic.y + BIGCHAR_HEIGHT;
}

/*
=================
Action_Draw
=================
*/
static void Action_Draw( menuaction_s *a )
{
	int		x, y;
	int		style;
	float*	color;

	style = 0;
	color = menu_text_color;
	if ( a->generic.flags & QMF_GRAYED )
	{
		color = text_color_disabled;
	}
	else if (( a->generic.flags & QMF_PULSEIFFOCUS ) && ( a->generic.parent->cursor == a->generic.menuPosition ))
	{
		color = color_highlight;
		style = UI_PULSE;
	}
	else if (( a->generic.flags & QMF_HIGHLIGHT_IF_FOCUS ) && ( a->generic.parent->cursor == a->generic.menuPosition ))
	{
		color = color_highlight;
	}
	else if ( a->generic.flags & QMF_BLINK )
	{
		style = UI_BLINK;
		color = color_highlight;
	}

	x = a->generic.x;
	y = a->generic.y;

	UI_DrawString( x, y, a->generic.name, UI_LEFT|style, color );

	if ( a->generic.parent->cursor == a->generic.menuPosition )
	{
		// draw cursor
		UI_DrawChar( x - BIGCHAR_WIDTH, y, 13, UI_LEFT|UI_BLINK, color);
	}
}

/*
=================
RadioButton_Init
=================
*/
void RadioButton_Init( menuradiobutton_s *rb )
{
	int	len;
	// calculate bounds
	if (rb->generic.name)
		len = strlenru(rb->generic.name);
	else
		len = 0;
	

	rb->generic.left   = rb->generic.x - (len+1)*SMALLCHAR_WIDTH;
	rb->generic.right  = rb->generic.x + 6*SMALLCHAR_WIDTH;
	rb->generic.top    = rb->generic.y;
	rb->generic.bottom = rb->generic.y + SMALLCHAR_HEIGHT;
}

/*
=================
RadioButton_Key
=================
*/
static sfxHandle_t RadioButton_Key( menuradiobutton_s *rb, int key )
{
	switch (key)
	{
		case K_MOUSE1:
			if (!(rb->generic.flags & QMF_HASMOUSEFOCUS))
				break;

		case K_JOY1:
		case K_JOY2:
		case K_JOY3:
		case K_JOY4:
		case K_ENTER:
		case K_KP_ENTER:
		case K_KP_LEFTARROW:
		case K_LEFTARROW:
		case K_KP_RIGHTARROW:
		case K_RIGHTARROW:
			rb->curvalue = !rb->curvalue;
			if ( rb->generic.callback )
				rb->generic.callback( rb, QM_ACTIVATED );

			return (menu_move_sound);
	}

	// key not handled
	return 0;
}

/*
=================
RadioButton_Draw
=================
*/
static void RadioButton_Draw( menuradiobutton_s *rb )
{
	int	x;
	int y;
	float *color;
	int	style;
	qboolean focus;

	x = rb->generic.x;
	y = rb->generic.y;

	focus = (rb->generic.parent->cursor == rb->generic.menuPosition);

	if ( rb->generic.flags & QMF_GRAYED )
	{
		color = text_color_disabled;
		style = UI_LEFT|UI_SMALLFONT;
	}
	else if ( focus )
	{
		color = color_highlight;
		style = UI_LEFT|UI_PULSE|UI_SMALLFONT;
	}
	else
	{
		if(!rb->color){
		color = text_color_normal;
		} else {
		color = rb->color;
		}
		style = UI_LEFT|UI_SMALLFONT;
	}

	if ( focus )
	{
		// draw cursor 
		UI_DrawChar( x, y, 13, UI_CENTER|UI_BLINK|UI_SMALLFONT, color);
	}

	if ( rb->generic.name )
		UI_DrawString( x - SMALLCHAR_WIDTH, y, rb->generic.name, UI_RIGHT|UI_SMALLFONT, color );

	if ( !rb->curvalue )
	{
		UI_DrawHandlePic( x + SMALLCHAR_WIDTH, y + 2, 12, 12, uis.rb_off);
		if(cl_language.integer == 0){
		UI_DrawString( x + SMALLCHAR_WIDTH + 12, y, "off", style, color );
		}
		if(cl_language.integer == 1){
		UI_DrawString( x + SMALLCHAR_WIDTH + 12, y, "откл", style, color );
		}
	}
	else
	{
		UI_DrawHandlePic( x + SMALLCHAR_WIDTH, y + 2, 12, 12, uis.rb_on );
		if(cl_language.integer == 0){
		UI_DrawString( x + SMALLCHAR_WIDTH + 12, y, "on", style, color );
		}
		if(cl_language.integer == 1){
		UI_DrawString( x + SMALLCHAR_WIDTH + 12, y, "вкл", style, color );
		}
	}
}

/*
=================
Slider_Init
=================
*/
static void Slider_Init( menuslider_s *s )
{
	int len;

	// calculate bounds
	if (s->generic.name)
		len = strlenru(s->generic.name);
	else
		len = 0;

	s->generic.left   = s->generic.x - (len+1)*SMALLCHAR_WIDTH; 
	s->generic.right  = s->generic.x + (SLIDER_RANGE+2+1)*SMALLCHAR_WIDTH;
	s->generic.top    = s->generic.y;
	s->generic.bottom = s->generic.y + SMALLCHAR_HEIGHT;
}

/*
=================
Slider_Key
=================
*/
static sfxHandle_t Slider_Key( menuslider_s *s, int key )
{
	sfxHandle_t	sound;
	int			x;
	int			oldvalue;

	switch (key)
	{
		case K_MOUSE1:
			x           = uis.cursorx - s->generic.x - 2*SMALLCHAR_WIDTH;
			oldvalue    = s->curvalue;
			s->curvalue = (x/(float)(SLIDER_RANGE*SMALLCHAR_WIDTH)) * (s->maxvalue-s->minvalue) + s->minvalue;

			if (s->curvalue < s->minvalue)
				s->curvalue = s->minvalue;
			else if (s->curvalue > s->maxvalue)
				s->curvalue = s->maxvalue;
			if (s->curvalue != oldvalue)
				sound = menu_move_sound;
			else
				sound = 0;
			break;

		case K_KP_LEFTARROW:
		case K_LEFTARROW:
			if (s->curvalue > s->minvalue)
			{
				s->curvalue--;
				sound = menu_move_sound;
			}
			else
				sound = menu_buzz_sound;
			break;			

		case K_KP_RIGHTARROW:
		case K_RIGHTARROW:
			if (s->curvalue < s->maxvalue)
			{
				s->curvalue++;
				sound = menu_move_sound;
			}
			else
				sound = menu_buzz_sound;
			break;			

		default:
			// key not handled
			sound = 0;
			break;
	}

	if ( sound && s->generic.callback )
		s->generic.callback( s, QM_ACTIVATED );

	return (sound);
}

/*
=================
Slider_Draw
=================
*/
static void Slider_Draw( menuslider_s *s ) {
	int			x;
	int			y;
	int			val;
	int			style;
	float		*color;
	int			button;
	qboolean	focus;
	
	x =	s->generic.x;
	y = s->generic.y;
	val = s->curvalue;
	focus = (s->generic.parent->cursor == s->generic.menuPosition);

	if( s->generic.flags & QMF_GRAYED ) {
		color = text_color_disabled;
		style = UI_SMALLFONT;
	}
	else if( focus ) {
		color  = color_highlight;
		style = UI_SMALLFONT | UI_PULSE;
	}
	else {
		color = text_color_normal;
		style = UI_SMALLFONT;
	}

	// draw label
	UI_DrawString( x - SMALLCHAR_WIDTH, y, s->generic.name, UI_RIGHT|style, color );
	UI_DrawString( x + SMALLCHAR_WIDTH*13, y, va("%i", val), UI_LEFT|style, color );

	// draw slider
	UI_SetColor( color );
	UI_DrawHandlePic( x + SMALLCHAR_WIDTH, y, 93, 11, sliderBar );
	UI_SetColor( NULL );

	// clamp thumb
	if( s->maxvalue > s->minvalue )	{
		s->range = ( s->curvalue - s->minvalue ) / ( float ) ( s->maxvalue - s->minvalue );
		if( s->range < 0 ) {
			s->range = 0;
		}
		else if( s->range > 1) {
			s->range = 1;
		}
	}
	else {
		s->range = 0;
	}

	// draw thumb
	if( style & UI_PULSE) {
		button = sliderButton_1;
	}
	else {
		button = sliderButton_0;
	}

	UI_DrawHandlePic( (int)( x + 2*SMALLCHAR_WIDTH + (SLIDER_RANGE-1)*SMALLCHAR_WIDTH* s->range ) - 2, y - 2, 9, 16, button );
}

/*
=================
SpinControl_Init
=================
*/
void SpinControl_Init( menulist_s *s ) {
	int	len;
	int	l;
	const char* str;

	if (s->generic.name)
		len = strlenru(s->generic.name) * SMALLCHAR_WIDTH;
	else
		len = 0;

	s->generic.left	= s->generic.x - SMALLCHAR_WIDTH - len;

	len = s->numitems = 0;
	while ( (str = s->itemnames[s->numitems]) != 0 )
	{
		l = strlenru(str);

		if (l > len)
			len = l;

		s->numitems++;
	}		

	s->generic.top	  =	s->generic.y;
	s->generic.right  =	s->generic.x + (len+1)*SMALLCHAR_WIDTH;
	s->generic.bottom =	s->generic.y + SMALLCHAR_HEIGHT;
}

/*
=================
SpinControl_Key
=================
*/
static sfxHandle_t SpinControl_Key( menulist_s *s, int key )
{
	sfxHandle_t	sound;

	sound = 0;
	switch (key)
	{
		case K_MOUSE1:
			s->curvalue++;
			if (s->curvalue >= s->numitems)
				s->curvalue = 0;
			sound = menu_move_sound;
			break;
		
		case K_KP_LEFTARROW:
		case K_LEFTARROW:
			if (s->curvalue > 0)
			{
				s->curvalue--;
				sound = menu_move_sound;
			}
			else
				sound = menu_buzz_sound;
			break;

		case K_KP_RIGHTARROW:
		case K_RIGHTARROW:
			if (s->curvalue < s->numitems-1)
			{
				s->curvalue++;
				sound = menu_move_sound;
			}
			else
				sound = menu_buzz_sound;
			break;
	}

	if ( sound && s->generic.callback )
		s->generic.callback( s, QM_ACTIVATED );

	return (sound);
}

/*
=================
SpinControl_Draw
=================
*/
static void SpinControl_Draw( menulist_s *s )
{
	float *color;
	int	x,y;
	int	style;
	qboolean focus;

	x = s->generic.x;
	y =	s->generic.y;

	style = UI_SMALLFONT;
	focus = (s->generic.parent->cursor == s->generic.menuPosition);

	if ( s->generic.flags & QMF_GRAYED )
		color = text_color_disabled;
	else if ( focus )
	{
		color = color_highlight;
		style |= UI_PULSE;
	}
	else if ( s->generic.flags & QMF_BLINK )
	{
		color = color_highlight;
		style |= UI_BLINK;
	}
	else
		color = text_color_normal;

	if ( focus )
	{
		// draw cursor
		UI_DrawChar( x, y, 13, UI_CENTER|UI_BLINK|UI_SMALLFONT, color);
	}

	UI_DrawString( x - SMALLCHAR_WIDTH, y, s->generic.name, style|UI_RIGHT, color );
	UI_DrawString( x + SMALLCHAR_WIDTH, y, s->itemnames[s->curvalue], style|UI_LEFT, color );
}

/*
=================
ScrollList_Init
=================
*/
void ScrollList_Init( menulist_s *l )
{
	int		w;

	l->oldvalue = 0;
	l->curvalue = 0;
	l->top      = 0;

	if( !l->columns ) {
		l->columns = 1;
		l->seperation = 0;
	}
	else if( !l->seperation ) {
		l->seperation = 3;
	}

	w = ( (l->width + l->seperation) * l->columns - l->seperation) * SMALLCHAR_WIDTH;

	l->generic.left   =	l->generic.x;
	l->generic.top    = l->generic.y;	
	l->generic.right  =	l->generic.x + w;
	l->generic.bottom =	l->generic.y + l->height * SMALLCHAR_HEIGHT;

	if( l->generic.flags & QMF_CENTER_JUSTIFY ) {
		l->generic.left -= w / 2;
		l->generic.right -= w / 2;
	}
}

/*
=================
ScrollList_Key
=================
*/
sfxHandle_t ScrollList_Key( menulist_s *l, int key )
{
	static int clicktime = 0;

	int	x;
	int	y;
	int	w;
	int	i;
	int	j;
	int	c;
	int	cursorx;
	int	cursory;
	int	column;
	int	index;
	int clickdelay;

	switch (key)
	{
		case K_MOUSE1:
			if (l->generic.flags & QMF_HASMOUSEFOCUS)
			{
				// check scroll region
				x = l->generic.x;
				y = l->generic.y;
				w = ( (l->width + l->seperation) * l->columns - l->seperation) * SMALLCHAR_WIDTH;
				if( l->generic.flags & QMF_CENTER_JUSTIFY ) {
					x -= w / 2;
				}
				
				if (UI_CursorInRect( x, y, w, l->height*SMALLCHAR_HEIGHT ))
				{
					cursorx = (uis.cursorx - x)/SMALLCHAR_WIDTH;
					column = cursorx / (l->width + l->seperation);
					cursory = (uis.cursory - y)/SMALLCHAR_HEIGHT;
					index = column * l->height + cursory;
					if (l->top + index < l->numitems)
					{
						l->oldvalue = l->curvalue;
						l->curvalue = l->top + index;

						clickdelay = uis.realtime - clicktime;
						clicktime = uis.realtime;
						if (l->oldvalue != l->curvalue)
						{
							if (l->generic.callback) {
								l->generic.callback( l, QM_GOTFOCUS );
							}
							return (menu_move_sound);
						}
						else {
							// double click
							if ((clickdelay < 350) && !(l->generic.flags & (QMF_GRAYED|QMF_INACTIVE)))
							{
								return (Menu_ActivateItem( l->generic.parent, &l->generic ));
							}
						}
					}
				}

				// absorbed, silent sound effect
				return (menu_null_sound);
			}
			break;

		case K_KP_HOME:
		case K_HOME:
			l->oldvalue = l->curvalue;
			l->curvalue = 0;
			l->top      = 0;

			if (l->oldvalue != l->curvalue && l->generic.callback)
			{
				l->generic.callback( l, QM_GOTFOCUS );
				return (menu_move_sound);
			}
			return (menu_buzz_sound);

		case K_KP_END:
		case K_END:
			l->oldvalue = l->curvalue;
			l->curvalue = l->numitems-1;
			if( l->columns > 1 ) {
				c = (l->curvalue / l->height + 1) * l->height;
				l->top = c - (l->columns * l->height);
			}
			else {
				l->top = l->curvalue - (l->height - 1);
			}
			if (l->top < 0)
				l->top = 0;			

			if (l->oldvalue != l->curvalue && l->generic.callback)
			{
				l->generic.callback( l, QM_GOTFOCUS );
				return (menu_move_sound);
			}
			return (menu_buzz_sound);

		case K_PGUP:
		case K_KP_PGUP:
			if( l->columns > 1 ) {
				return menu_null_sound;
			}

			if (l->curvalue > 0)
			{
				l->oldvalue = l->curvalue;
				l->curvalue -= l->height-1;
				if (l->curvalue < 0)
					l->curvalue = 0;
				l->top = l->curvalue;
				if (l->top < 0)
					l->top = 0;

				if (l->generic.callback)
					l->generic.callback( l, QM_GOTFOCUS );

				return (menu_move_sound);
			}
			return (menu_buzz_sound);

		case K_PGDN:
		case K_KP_PGDN:
			if( l->columns > 1 ) {
				return menu_null_sound;
			}

			if (l->curvalue < l->numitems-1)
			{
				l->oldvalue = l->curvalue;
				l->curvalue += l->height-1;
				if (l->curvalue > l->numitems-1)
					l->curvalue = l->numitems-1;
				l->top = l->curvalue - (l->height-1);
				if (l->top < 0)
					l->top = 0;

				if (l->generic.callback)
					l->generic.callback( l, QM_GOTFOCUS );

				return (menu_move_sound);
			}
			return (menu_buzz_sound);

		case K_KP_UPARROW:
		case K_UPARROW:
		case K_MWHEELUP:
			if(l->columns <= 1){
				ScrollList_Key(l, K_LEFTARROW);
				return menu_null_sound; //karin: return missing
			}
			if( l->curvalue == 0 ) {
				return menu_buzz_sound;
			}

			l->oldvalue = l->curvalue;
			l->curvalue--;

			if( l->curvalue < l->top ) {
				if( l->columns == 1 ) {
					l->top--;
				}
				else {
					l->top -= l->height;
				}
			}

			if( l->generic.callback ) {
				l->generic.callback( l, QM_GOTFOCUS );
			}

			return (menu_move_sound);

		case K_KP_DOWNARROW:
		case K_DOWNARROW:
		case K_MWHEELDOWN:
			if(l->columns <= 1){
				ScrollList_Key(l, K_RIGHTARROW);
				return menu_null_sound; //karin: return missing
			}
			if( l->curvalue == l->numitems - 1 ) {
				return menu_buzz_sound;
			}

			l->oldvalue = l->curvalue;
			l->curvalue++;

			if( l->curvalue >= l->top + l->columns * l->height ) {
				if( l->columns == 1 ) {
					l->top++;
				}
				else {
					l->top += l->height;
				}
			}

			if( l->generic.callback ) {
				l->generic.callback( l, QM_GOTFOCUS );
			}

			return menu_move_sound;

		case K_KP_LEFTARROW:
		case K_LEFTARROW:
			if( l->curvalue == 0 ) {
				return menu_buzz_sound;
			}

			l->oldvalue = l->curvalue;
			l->curvalue--;

			if( l->curvalue < l->top ) {
				if( l->columns == 1 ) {
					l->top--;
				}
				else {
					l->top -= l->columns;
				}
			}

			if(l->top < 0 || l->curvalue < 0){
				l->curvalue = 0;
				l->top = 0;
			}

			if( l->generic.callback ) {
				l->generic.callback( l, QM_GOTFOCUS );
			}

			return (menu_move_sound);

		case K_KP_RIGHTARROW:
		case K_RIGHTARROW:
			if( l->curvalue == l->numitems - 1 ) {
				return menu_buzz_sound;
			}

			l->oldvalue = l->curvalue;
			l->curvalue++;

			if( l->curvalue >= l->top + l->columns * l->height ) {
				if( l->columns == 1 ) {
					l->top++;
				}
				else {
					l->top += l->columns;
				}
			}

			if( l->generic.callback ) {
				l->generic.callback( l, QM_GOTFOCUS );
			}

			return menu_move_sound;
	}

	// cycle look for ascii key inside list items
	if ( !Q_isprint( key ) )
		return (0);

	// force to lower for case insensitive compare
	if ( Q_isupper( key ) )
	{
		key -= 'A' - 'a';
	}

	// iterate list items
	for (i=1; i<=l->numitems; i++)
	{
		j = (l->curvalue + i) % l->numitems;
		c = l->itemnames[j][0];
		if ( Q_isupper( c ) )
		{
			c -= 'A' - 'a';
		}

		if (c == key)
		{
			// set current item, mimic windows listbox scroll behavior
			if (j < l->top)
			{
				// behind top most item, set this as new top
				l->top = j;
			}
			else if (j > l->top+l->height-1)
			{
				// past end of list box, do page down
				l->top = (j+1) - l->height;
			}
			
			if (l->curvalue != j)
			{
				l->oldvalue = l->curvalue;
				l->curvalue = j;
				if (l->generic.callback)
					l->generic.callback( l, QM_GOTFOCUS );
				return ( menu_move_sound );			
			}

			return (menu_buzz_sound);
		}
	}

	return (menu_buzz_sound);
}

/*
=================
ScrollList_Draw
=================
*/
void ScrollList_Draw( menulist_s *l )
{
	int			x;
	int			u;
	int			y;
	int			i;
	int			base;
	int			column;
	float*		color;
	qboolean	hasfocus;
	int			style;
	vec4_t scrollbuttona        = {1.00f, 1.00f, 1.00f, 0.75f};	// transluscent orange

	hasfocus = (l->generic.parent->cursor == l->generic.menuPosition);

	x =	l->generic.x;
	
	for( column = 0; column < l->columns; column++ ) {
		y =	l->generic.y;
		base = l->top + column * l->height;
		for( i = base; i < base + l->height; i++) {
			if (i >= l->numitems)
				break;

			if (i == l->curvalue)
			{
				u = x - 2;
				if( l->generic.flags & QMF_CENTER_JUSTIFY ) {
					u -= (l->width * SMALLCHAR_WIDTH) / 2 + 1;
				}

				UI_FillRect(u,y,l->width*SMALLCHAR_WIDTH,SMALLCHAR_HEIGHT,color_select_bluo);
				color = color_highlight;

				if (hasfocus)
					style = UI_PULSE|UI_LEFT|UI_SMALLFONT;
				else
					style = UI_LEFT|UI_SMALLFONT;
			}
			else
			{
				color = l->color;
				style = UI_LEFT|UI_SMALLFONT;
			}
			if( l->generic.flags & QMF_CENTER_JUSTIFY ) {
				style |= UI_CENTER;
			}
			//if(!l->itemnames2[i]){
			UI_DrawString(x,y,l->itemnames[i],style,color);
			//}
			//if(l->itemnames2[i]){
			//UI_DrawString(x,y,l->itemnames2[i],style,color);
			//}

			y += SMALLCHAR_HEIGHT;
		}
		x += (l->width + l->seperation) * SMALLCHAR_WIDTH;
	}
}

/*
=================
Menu_AddItem
=================
*/
void Menu_AddItem( menuframework_s *menu, void *item )
{
	menucommon_s	*itemptr;

	if (menu->nitems >= MAX_MENUITEMS)
		trap_Error ("Menu_AddItem: excessive items");

	menu->items[menu->nitems] = item;
	((menucommon_s*)menu->items[menu->nitems])->parent        = menu;
	((menucommon_s*)menu->items[menu->nitems])->menuPosition  = menu->nitems;
	((menucommon_s*)menu->items[menu->nitems])->flags        &= ~QMF_HASMOUSEFOCUS;

	// perform any item specific initializations
	itemptr = (menucommon_s*)item;
	if (!(itemptr->flags & QMF_NODEFAULTINIT))
	{
		switch (itemptr->type)
		{
			case MTYPE_ACTION:
				Action_Init((menuaction_s*)item);
				break;

			case MTYPE_FIELD:
				MenuField_Init((menufield_s*)item);
				break;

			case MTYPE_SPINCONTROL:
				SpinControl_Init((menulist_s*)item);
				break;

			case MTYPE_RADIOBUTTON:
				RadioButton_Init((menuradiobutton_s*)item);
				break;

			case MTYPE_SLIDER:
				Slider_Init((menuslider_s*)item);
				break;

			case MTYPE_BITMAP:
				Bitmap_Init((menubitmap_s*)item);
				break;

			case MTYPE_TEXT:
				Text_Init((menutext_s*)item);
				break;

			case MTYPE_SCROLLLIST:
				ScrollList_Init((menulist_s*)item);
				break;

			case MTYPE_PTEXT:
				PText_Init((menutext_s*)item);
				break;

			case MTYPE_BTEXT:
				BText_Init((menutext_s*)item);
				break;
				
			case MTYPE_UIOBJECT:
				UIObject_Init((menuobject_s*)item);
				break;

			default:
				trap_Error( va("Menu_Init: unknown type %d", itemptr->type) );
		}
	}

	menu->nitems++;
}

/*
=================
Menu_CursorMoved
=================
*/
void Menu_CursorMoved( menuframework_s *m )
{
	void (*callback)( void *self, int notification );
	
	if (m->cursor_prev == m->cursor)
		return;

	if (m->cursor_prev >= 0 && m->cursor_prev < m->nitems)
	{
		callback = ((menucommon_s*)(m->items[m->cursor_prev]))->callback;
		if (callback)
			callback(m->items[m->cursor_prev],QM_LOSTFOCUS);
	}
	
	if (m->cursor >= 0 && m->cursor < m->nitems)
	{
		callback = ((menucommon_s*)(m->items[m->cursor]))->callback;
		if (callback)
			callback(m->items[m->cursor],QM_GOTFOCUS);
	}
}

/*
=================
Menu_SetCursor
=================
*/
void Menu_SetCursor( menuframework_s *m, int cursor )
{
	if (((menucommon_s*)(m->items[cursor]))->flags & (QMF_GRAYED|QMF_INACTIVE))
	{
		// cursor can't go there
		return;
	}

	m->cursor_prev = m->cursor;
	m->cursor      = cursor;

	Menu_CursorMoved( m );
}

/*
=================
Menu_SetCursorToItem
=================
*/
void Menu_SetCursorToItem( menuframework_s *m, void* ptr )
{
	int	i;

	for (i=0; i<m->nitems; i++)
	{
		if (m->items[i] == ptr)
		{
			Menu_SetCursor( m, i );
			return;
		}
	}
}

/*
** Menu_AdjustCursor
**
** This function takes the given menu, the direction, and attempts
** to adjust the menu's cursor so that it's at the next available
** slot.
*/
void Menu_AdjustCursor( menuframework_s *m, int dir ) {
	menucommon_s	*item = NULL;
	qboolean		wrapped = qfalse;

wrap:
	while ( m->cursor >= 0 && m->cursor < m->nitems ) {
		item = ( menucommon_s * ) m->items[m->cursor];
		if (( item->flags & (QMF_GRAYED|QMF_MOUSEONLY|QMF_INACTIVE) ) ) {
			m->cursor += dir;
		}
		else {
			break;
		}
	}

	if ( dir == 1 ) {
		if ( m->cursor >= m->nitems ) {
			if ( m->wrapAround ) {
				if ( wrapped ) {
					m->cursor = m->cursor_prev;
					return;
				}
				m->cursor = 0;
				wrapped = qtrue;
				goto wrap;
			}
			m->cursor = m->cursor_prev;
		}
	}
	else {
		if ( m->cursor < 0 ) {
			if ( m->wrapAround ) {
				if ( wrapped ) {
					m->cursor = m->cursor_prev;
					return;
				}
				m->cursor = m->nitems - 1;
				wrapped = qtrue;
				goto wrap;
			}
			m->cursor = m->cursor_prev;
		}
	}
}

/*
=================
Menu_Draw
=================
*/
void Menu_Draw( menuframework_s *menu )
{
	int				i;
	menucommon_s	*itemptr;

	// draw menu
	for (i=0; i<menu->nitems; i++)
	{
		itemptr = (menucommon_s*)menu->items[i];

		if (itemptr->flags & QMF_HIDDEN)
			continue;

		if (itemptr->ownerdraw)
		{
			// total subclassing, owner draws everything
			itemptr->ownerdraw( itemptr );
		}	
		else 
		{
			switch (itemptr->type)
			{	
				case MTYPE_RADIOBUTTON:
					RadioButton_Draw( (menuradiobutton_s*)itemptr );
					break;

				case MTYPE_FIELD:
					MenuField_Draw( (menufield_s*)itemptr );
					break;
		
				case MTYPE_SLIDER:
					Slider_Draw( (menuslider_s*)itemptr );
					break;
 
				case MTYPE_SPINCONTROL:
					SpinControl_Draw( (menulist_s*)itemptr );
					break;
		
				case MTYPE_ACTION:
					Action_Draw( (menuaction_s*)itemptr );
					break;
		
				case MTYPE_BITMAP:
					Bitmap_Draw( (menubitmap_s*)itemptr );
					break;

				case MTYPE_TEXT:
					Text_Draw( (menutext_s*)itemptr );
					break;

				case MTYPE_SCROLLLIST:
					ScrollList_Draw( (menulist_s*)itemptr );
					break;
				
				case MTYPE_PTEXT:
					PText_Draw( (menutext_s*)itemptr );
					break;

				case MTYPE_BTEXT:
					BText_Draw( (menutext_s*)itemptr );
					break;
					
				case MTYPE_UIOBJECT:
					UIObject_Draw( (menuobject_s*)itemptr );
					break;

				default:
					trap_Error( va("Menu_Draw: unknown type %d", itemptr->type) );
			}
		}
#ifndef NDEBUG
		if( uis.debug ) {
			int	x;
			int	y;
			int	w;
			int	h;

			if( !( itemptr->flags & QMF_INACTIVE ) ) {
				x = itemptr->left;
				y = itemptr->top;
				w = itemptr->right - itemptr->left + 1;
				h =	itemptr->bottom - itemptr->top + 1;

				if (itemptr->flags & QMF_HASMOUSEFOCUS) {
					UI_DrawRect(x, y, w, h, colorYellow );
				}
				else {
					UI_DrawRect(x, y, w, h, colorWhite );
				}
			}
		}
#endif
	}

	itemptr = Menu_ItemAtCursor( menu );
	if ( itemptr && itemptr->statusbar)
		itemptr->statusbar( ( void * ) itemptr );
}

/*
=================
Menu_ItemAtCursor
=================
*/
void *Menu_ItemAtCursor( menuframework_s *m )
{
	if ( m->cursor < 0 || m->cursor >= m->nitems )
		return 0;

	return m->items[m->cursor];
}

/*
=================
Menu_ActivateItem
=================
*/
sfxHandle_t Menu_ActivateItem( menuframework_s *s, menucommon_s* item ) {
	if ( item->callback ) {
		item->callback( item, QM_ACTIVATED );
		if( !( item->flags & QMF_SILENT ) ) {
			return menu_move_sound;
		}
	}

	return 0;
}

/*
=================
Menu_DefaultKey
=================
*/
sfxHandle_t Menu_DefaultKey( menuframework_s *m, int key )
{
	sfxHandle_t		sound = 0;
	menucommon_s	*item;
	int				cursor_prev;
	menuobject_s* b;
	
	

	// menu system keys
	switch ( key )
	{
		case K_MOUSE2:
		case K_ESCAPE:
			UI_PopMenu();
			return menu_out_sound;
	}

	if (!m || !m->nitems)
		return 0;

	// route key stimulus to widget
	item = Menu_ItemAtCursor( m );
	b = (menuobject_s*)item;
	if (item && (item->flags & (QMF_HASMOUSEFOCUS)) && !(item->flags & (QMF_GRAYED|QMF_INACTIVE)))
	{
		switch (item->type)
		{
			case MTYPE_SPINCONTROL:
				sound = SpinControl_Key( (menulist_s*)item, key );
				break;

			case MTYPE_RADIOBUTTON:
				sound = RadioButton_Key( (menuradiobutton_s*)item, key );
				break;

			case MTYPE_SLIDER:
				sound = Slider_Key( (menuslider_s*)item, key );
				break;

			case MTYPE_SCROLLLIST:
				sound = ScrollList_Key( (menulist_s*)item, key );
				break;

			case MTYPE_FIELD:
				sound = MenuField_Key( (menufield_s*)item, &key );
				item->callback( item, QM_ACTIVATED );
				break;
				
			case MTYPE_UIOBJECT:
				if(b->type == 4 || b->type == 5 || b->type == 7 || b->type == 8){
				sound = UIObject_Key( (menuobject_s*)item, key );
				if(b->type == 4){
				item->callback( item, QM_ACTIVATED );
				}
				}
				break;

		}

		if (sound) {
			// key was handled
			return sound;		
		}
	}

	// default handling
	switch ( key )
	{
#ifndef NDEBUG
		case K_PGDN:
		case K_KP_PGDN:
		case K_MWHEELDOWN:
		if(uis.menuscroll - 20 >= -uis.activemenu->downlimitscroll){
			uis.menuscroll -= 20;
			uis.cursory += 20*0.666;
		} else {
			uis.menuscroll = -uis.activemenu->downlimitscroll;
		}
			break;
		case K_PGUP:
		case K_KP_PGUP:
		case K_MWHEELUP:
		if(uis.menuscroll + 20 <= -uis.activemenu->uplimitscroll){
			uis.menuscroll += 20;
			uis.cursory -= 20*0.666;	
		} else {
			uis.menuscroll = uis.activemenu->uplimitscroll;
		}
			break;

		case K_F11:
			uis.debug ^= 1;
			break;

		case K_F12:
			trap_Cmd_ExecuteText(EXEC_APPEND, "screenshotJPEG\n");
			break;
#endif
		case K_KP_UPARROW:
		case K_UPARROW:
			cursor_prev    = m->cursor;
			m->cursor_prev = m->cursor;
			m->cursor--;
			Menu_AdjustCursor( m, -1 );
			if ( cursor_prev != m->cursor ) {
				Menu_CursorMoved( m );
				sound = menu_move_sound;
			}
			break;

		case K_TAB:
		case K_KP_DOWNARROW:
		case K_DOWNARROW:
			cursor_prev    = m->cursor;
			m->cursor_prev = m->cursor;
			m->cursor++;
			Menu_AdjustCursor( m, 1 );
			if ( cursor_prev != m->cursor ) {
				Menu_CursorMoved( m );
				sound = menu_move_sound;
			}
			break;

		case K_MOUSE1:
		case K_MOUSE3:
			if (item)
				if ((item->flags & QMF_HASMOUSEFOCUS) && !(item->flags & (QMF_GRAYED|QMF_INACTIVE)))
					return (Menu_ActivateItem( m, item ));
			break;

		case K_JOY1:
		case K_JOY2:
		case K_JOY3:
		case K_JOY4:
		case K_AUX1:
		case K_AUX2:
		case K_AUX3:
		case K_AUX4:
		case K_AUX5:
		case K_AUX6:
		case K_AUX7:
		case K_AUX8:
		case K_AUX9:
		case K_AUX10:
		case K_AUX11:
		case K_AUX12:
		case K_AUX13:
		case K_AUX14:
		case K_AUX15:
		case K_AUX16:
		case K_KP_ENTER:
		case K_ENTER:
			if (item)
				if (!(item->flags & (QMF_MOUSEONLY|QMF_GRAYED|QMF_INACTIVE)))
					return (Menu_ActivateItem( m, item ));
			break;
	}

	return sound;
}

/*
=================
Menu_Cache
=================
*/
void Menu_Cache( void )
{
	int i;
	uis.charset[0]			= trap_R_RegisterShaderNoMip( "gfx/2d/default_font" );
	uis.charset[1]			= trap_R_RegisterShaderNoMip( "gfx/2d/default_font1" );
	uis.charset[2]			= trap_R_RegisterShaderNoMip( "gfx/2d/default_font2" );
	uis.cursor          = trap_R_RegisterShaderNoMip( "menu/art/3_cursor2" );
	uis.corner          = trap_R_RegisterShaderNoMip( "corner" );
	uis.rb_on           = trap_R_RegisterShaderNoMip( "menu/art/switch_on" );
	uis.rb_off          = trap_R_RegisterShaderNoMip( "menu/art/switch_off" );

	uis.whiteShader = trap_R_RegisterShaderNoMip( "white" );
	uis.menuBlack		= trap_R_RegisterShaderNoMip( "menu/art/blacktrans" );
	uis.menuWallpapers = trap_R_RegisterShaderNoMip( "menu/animbg" );
	
	uis.menuLoadingIcon = trap_R_RegisterShaderNoMip( "menu/art/loading" );

	menu_in_sound	= trap_S_RegisterSound( "sound/misc/menu1.wav", qfalse );
	menu_move_sound	= trap_S_RegisterSound( "sound/misc/menu2.wav", qfalse );
	menu_out_sound	= trap_S_RegisterSound( "sound/misc/menu3.wav", qfalse );
	menu_buzz_sound	= trap_S_RegisterSound( "sound/misc/menu4.wav", qfalse );
	weaponChangeSound	= trap_S_RegisterSound( "sound/weapons/change.wav", qfalse );

	// need a nonzero sound, make an empty sound for this
	menu_null_sound = -1;

	sliderBar = trap_R_RegisterShaderNoMip( "menu/art/slider2" );
	sliderButton_0 = trap_R_RegisterShaderNoMip( "menu/art/sliderbutt_0" );
	sliderButton_1 = trap_R_RegisterShaderNoMip( "menu/art/sliderbutt_1" );
}

//Copyright (C) 1999-2005 Id Software, Inc.
//
#include "ui_local.h"

/*
===================
MField_Draw

Handles horizontal scrolling and cursor blinking
x, y, are in pixels
===================
*/
void MField_Draw( mfield_t *edit, int x, int y, int style, vec4_t color ) {
	int		len;
	int		charw;
	int		drawLen;
	int		prestep;
	int		cursorChar;
	char	str[MAX_STRING_CHARS];

	drawLen = edit->widthInChars;
	len     = strlen( edit->buffer ) + 1;

	// guarantee that cursor will be visible
	if ( len <= drawLen ) {
		prestep = 0;
	} else {
		if ( edit->scroll + drawLen > len ) {
			edit->scroll = len - drawLen;
			if ( edit->scroll < 0 ) {
				edit->scroll = 0;
			}
		}
		prestep = edit->scroll;
	}

	if ( prestep + drawLen > len ) {
		drawLen = len - prestep;
	}

	// extract <drawLen> characters from the field at <prestep>
	if ( drawLen >= MAX_STRING_CHARS ) {
		trap_Error( "drawLen >= MAX_STRING_CHARS" );
	}
	memcpy( str, edit->buffer + prestep, drawLen );
	str[ drawLen ] = 0;

	UI_DrawString( x, y, str, style, color );

	// draw the cursor
	if (!(style & UI_PULSE)) {
		return;
	}

	if ( trap_Key_GetOverstrikeMode() ) {
		cursorChar = 11;
	} else {
		cursorChar = 10;
	}

	style &= ~UI_PULSE;
	style |= UI_BLINK;

	if (style & UI_SMALLFONT)
	{
		charw =	SMALLCHAR_WIDTH;
	}
	else if (style & UI_GIANTFONT)
	{
		charw =	GIANTCHAR_WIDTH;
	}
	else if (style & UI_TINYFONT)
	{
		charw =	TINYCHAR_WIDTH;
	}
	else
	{
		charw =	BIGCHAR_WIDTH;
	}

	if (style & UI_CENTER)
	{
		len = strlen(str);
		x = x - len*charw/2;
	}
	else if (style & UI_RIGHT)
	{
		len = strlen(str);
		x = x - len*charw;
	}
	
	UI_DrawChar( x + ( edit->cursor - prestep ) * charw, y, cursorChar, style & ~(UI_CENTER|UI_RIGHT), color );
}

/*
===================
MField_DrawCustom

Handles horizontal scrolling and cursor blinking
x, y, are in pixels
===================
*/
void MField_DrawCustom( mfield_t *edit, int x, int y, int style, vec4_t color, float csize ) {
	int		len;
	int		charw;
	int		drawLen;
	int		prestep;
	int		cursorChar;
	char	str[MAX_STRING_CHARS];

	drawLen = edit->widthInChars;
	len     = strlen( edit->buffer ) + 1;

	// guarantee that cursor will be visible
	if ( len <= drawLen ) {
		prestep = 0;
	} else {
		if ( edit->scroll + drawLen > len ) {
			edit->scroll = len - drawLen;
			if ( edit->scroll < 0 ) {
				edit->scroll = 0;
			}
		}
		prestep = edit->scroll;
	}

	if ( prestep + drawLen > len ) {
		drawLen = len - prestep;
	}

	// extract <drawLen> characters from the field at <prestep>
	if ( drawLen >= MAX_STRING_CHARS ) {
		trap_Error( "drawLen >= MAX_STRING_CHARS" );
	}
	memcpy( str, edit->buffer + prestep, drawLen );
	str[ drawLen ] = 0;

	UI_DrawStringCustom( x, y, str, style, color, csize, 512 );

	// draw the cursor
	if (!(style & UI_PULSE)) {
		return;
	}

	if ( trap_Key_GetOverstrikeMode() ) {
		cursorChar = 11;
	} else {
		cursorChar = 10;
	}

	style &= ~UI_PULSE;
	style |= UI_BLINK;

	if (style & UI_SMALLFONT)
	{
		charw =	SMALLCHAR_WIDTH;
	}
	else if (style & UI_TINYFONT)
	{
		charw =	TINYCHAR_WIDTH;
	}
	else if (style & UI_GIANTFONT)
	{
		charw =	GIANTCHAR_WIDTH;
	}
	else
	{
		charw =	BIGCHAR_WIDTH;
	}

	if (style & UI_CENTER)
	{
		len = strlen(str);
		x = x - len*charw/2;
	}
	else if (style & UI_RIGHT)
	{
		len = strlen(str);
		x = x - len*charw;
	}
	
	UI_DrawCharCustom( x + ( edit->cursor - prestep ) * charw*csize, y, cursorChar, style & ~(UI_CENTER|UI_RIGHT), color, csize );
}

/*
================
MField_Paste
================
*/
void MField_Paste( mfield_t *edit ) {
	char	pasteBuffer[64];
	int		pasteLen, i;

	trap_GetClipboardData( pasteBuffer, 64 );

	// send as if typed, so insert / overstrike works properly
	pasteLen = strlen( pasteBuffer );
	for ( i = 0 ; i < pasteLen ; i++ ) {
		MField_CharEvent( edit, pasteBuffer[i] );
	}
}

/*
=================
MField_KeyDownEvent

Performs the basic line editing functions for the console,
in-game talk, and menu fields

Key events are used for non-printable characters, others are gotten from char events.
=================
*/
void MField_KeyDownEvent( mfield_t *edit, int key ) {
	int		len;

	// shift-insert is paste
	if ( ( ( key == K_INS ) || ( key == K_KP_INS ) ) && trap_Key_IsDown( K_SHIFT ) ) {
		MField_Paste( edit );
		return;
	}

	len = strlen( edit->buffer );

	if ( key == K_DEL || key == K_KP_DEL ) {
		if ( edit->cursor < len ) {
			memmove( edit->buffer + edit->cursor, 
				edit->buffer + edit->cursor + 1, len - edit->cursor );
		}
		return;
	}

	if ( key == K_RIGHTARROW || key == K_KP_RIGHTARROW ) 
	{
		if ( edit->cursor < len ) {
			edit->cursor++;
		}
		if ( edit->cursor >= edit->scroll + edit->widthInChars && edit->cursor <= len )
		{
			edit->scroll++;
		}
		return;
	}

	if ( key == K_LEFTARROW || key == K_KP_LEFTARROW ) 
	{
		if ( edit->cursor > 0 ) {
			edit->cursor--;
		}
		if ( edit->cursor < edit->scroll )
		{
			edit->scroll--;
		}
		return;
	}

	if ( key == K_HOME || key == K_KP_HOME || ( tolower(key) == 'a' && trap_Key_IsDown( K_CTRL ) ) ) {
		edit->cursor = 0;
		edit->scroll = 0;
		return;
	}

	if ( key == K_END || key == K_KP_END || ( tolower(key) == 'e' && trap_Key_IsDown( K_CTRL ) ) ) {
		edit->cursor = len;
		edit->scroll = len - edit->widthInChars + 1;
		if (edit->scroll < 0)
			edit->scroll = 0;
		return;
	}

	if ( key == K_INS || key == K_KP_INS ) {
		trap_Key_SetOverstrikeMode( !trap_Key_GetOverstrikeMode() );
		return;
	}
}

/*
==================
MField_CharEvent
==================
*/
void MField_CharEvent( mfield_t *edit, int ch ) {
	int		len;

	if ( ch == 'v' - 'a' + 1 ) {	// ctrl-v is paste
		MField_Paste( edit );
		return;
	}

	if ( ch == 'c' - 'a' + 1 ) {	// ctrl-c clears the field
		MField_Clear( edit );
		return;
	}

	len = strlen( edit->buffer );

	if ( ch == 'h' - 'a' + 1 )	{	// ctrl-h is backspace
		if ( edit->cursor > 0 ) {
			memmove( edit->buffer + edit->cursor - 1, 
				edit->buffer + edit->cursor, len + 1 - edit->cursor );
			edit->cursor--;
			if ( edit->cursor < edit->scroll )
			{
				edit->scroll--;
			}
		}
		return;
	}

	if ( ch == 'a' - 'a' + 1 ) {	// ctrl-a is home
		edit->cursor = 0;
		edit->scroll = 0;
		return;
	}

	if ( ch == 'e' - 'a' + 1 ) {	// ctrl-e is end
		edit->cursor = len;
		edit->scroll = edit->cursor - edit->widthInChars + 1;
		if (edit->scroll < 0)
			edit->scroll = 0;
		return;
	}

	//
	// ignore any other non printable chars
	//
	if ( ch == -48 ) {
		return;
	}

	if ( !trap_Key_GetOverstrikeMode() ) {	
		if ((edit->cursor == MAX_EDIT_LINE - 1) || (edit->maxchars && edit->cursor >= edit->maxchars))
			return;
	} else {
		// insert mode
		if (( len == MAX_EDIT_LINE - 1 ) || (edit->maxchars && len >= edit->maxchars))
			return;
		memmove( edit->buffer + edit->cursor + 1, edit->buffer + edit->cursor, len + 1 - edit->cursor );
	}

	edit->buffer[edit->cursor] = ch;
	if (!edit->maxchars || edit->cursor < edit->maxchars-1)
		edit->cursor++;

	if ( edit->cursor >= edit->widthInChars )
	{
		edit->scroll++;
	}

	if ( edit->cursor == len + 1) {
		edit->buffer[edit->cursor] = 0;
	}
}

/*
==================
MField_Clear
==================
*/
void MField_Clear( mfield_t *edit ) {
	edit->buffer[0] = 0;
	edit->cursor = 0;
	edit->scroll = 0;
}

/*
==================
MenuField_Init
==================
*/
void MenuField_Init( menufield_s* m ) {
	int	l;
	int	w;
	int	h;

	MField_Clear( &m->field );

	if (m->generic.flags & QMF_SMALLFONT)
	{
		w = SMALLCHAR_WIDTH;
		h = SMALLCHAR_HEIGHT;
	}
	else
	{
		w = BIGCHAR_WIDTH;
		h = BIGCHAR_HEIGHT;
	}	

	if (m->generic.name) {
		l = (strlenru( m->generic.name )+1) * w;		
	}
	else {
		l = 0;
	}

	m->generic.left   = m->generic.x - l;
	m->generic.top    = m->generic.y;
	m->generic.right  = m->generic.x + w + m->field.widthInChars*w;
	m->generic.bottom = m->generic.y + h;
}

/*
==================
MenuField_Draw
==================
*/
void MenuField_Draw( menufield_s *f )
{
	int		x;
	int		y;
	int		w;
	int		h;
	int		style;
	qboolean focus;
	float	*color;

	x =	f->generic.x;
	y =	f->generic.y;

	if (f->generic.flags & QMF_SMALLFONT)
	{
		w = SMALLCHAR_WIDTH;
		h = SMALLCHAR_HEIGHT;
		style = UI_SMALLFONT;
	}
	else
	{
		w = BIGCHAR_WIDTH;
		h = BIGCHAR_HEIGHT;
		style = UI_BIGFONT;
	}	

	if (Menu_ItemAtCursor( f->generic.parent ) == f) {
		focus = qtrue;
		style |= UI_PULSE;
	}
	else {
		focus = qfalse;
	}

	if (f->generic.flags & QMF_GRAYED)
		color = text_color_disabled;
	else if (focus)
		color = color_highlight;
	else
		if(!f->color){
		color = text_color_normal;
		} else {
		color = f->color;
		}

	if ( focus )
	{
		// draw cursor
		UI_DrawChar( x, y, 13, UI_CENTER|UI_BLINK|style, color);
	}

	if ( f->generic.name ) {
		UI_DrawString( x - w, y, f->generic.name, style|UI_RIGHT, color );
	}

	MField_Draw( &f->field, x + w, y, style, color );
}

/*
==================
MenuField_Key
==================
*/
sfxHandle_t MenuField_Key( menufield_s* m, int* key )
{
	int keycode;
	static int lastKeypress = 0;

	keycode = *key;

	switch ( keycode )
	{
		case K_KP_ENTER:
		case K_ENTER:
		case K_MOUSE1:
			break;
		case K_JOY1:
		case K_JOY2:
		case K_JOY3:
		case K_JOY4:
			// have enter go to next cursor point
			*key = K_TAB;
			break;

		case K_TAB:
		case K_KP_DOWNARROW:
		case K_DOWNARROW:
		case K_KP_UPARROW:
		case K_UPARROW:
			break;

		default:
			if ( keycode & K_CHAR_FLAG )
			{
				keycode &= ~K_CHAR_FLAG;

				if ((m->generic.flags & QMF_UPPERCASE) && Q_islower( keycode ))
					keycode -= 'a' - 'A';
				else if ((m->generic.flags & QMF_LOWERCASE) && Q_isupper( keycode ))
					keycode -= 'A' - 'a';
				else if ((m->generic.flags & QMF_NUMBERSONLY) && Q_isalpha( keycode ))
					return (menu_buzz_sound);

				MField_CharEvent( &m->field, keycode);
			}
			else
				MField_KeyDownEvent( &m->field, keycode );
			break;
	}
	lastKeypress = uis.realtime;

	return (0);
}
