#ifndef Prof_INC_PROF_H
#define Prof_INC_PROF_H

#ifdef WIN32
//#define Prof_ENABLED
#endif

#include "prof_gather.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
	union cdatatype
	{
		unsigned char		m_RGBA[4];
		unsigned int		m_RGBAi;
	} cdata;
} prof_color;


/*
 *  Prof_update
 *
 *  Pass in true (1) to accumulate history info; pass
 *  in false (0) to throw away the current frame's data
 */
extern void Prof_update(int record);

/*
 *  Prof_draw_gl -- display the current report via OpenGL
 *
 *  You must provide a callable text-printing function.
 *  Put the opengl state into a 2d rendering mode.
 *
 *  Parameters:
 *    <sx,sy>         --  location where top line is drawn
 *    <width, height> --  total size of display (if too small, text will overprint)
 *    line_spacing    --  how much to move sy by after each line; use a
 *                        negative value if y decreases down the screen
 *    precision       --  decimal places of precision for time data, 1..4 (try 2)
 *    print_text      --  function to display a line of text starting at the
 *                        given coordinate; best if 0,1..9 are fixed-width
 *    text_width      --  a function that computes the pixel-width of
 *                        a given string before printing. you can fake with a
 *                        simple approximation of width('0')*strlen(str)
 *
 *  to avoid overprinting, you can make print_text truncate long strings
 */
//extern void Prof_draw_gl(float sx, float sy,
//                         float width, float height,
//                         float line_spacing,
//                         int precision,
//                         void (*print_text)(float x, float y, char *str),
//                         float (*text_width)(char *str));

/*
 *  Parameters
 *    <sx, sy>      --  origin of the graph--location of (0,0)
 *    x_spacing     --  screenspace size of each history sample; e.g.
 *                         2.0 pixels
 *    y_spacing     --  screenspace size of one millisecond of time;
 *                         for an app with max of 20ms in any one zone,
 *                         8.0 would produce a 160-pixel tall display,
 *                         assuming screenspace is in pixels
 */
//extern void Prof_draw_graph_gl(float sx, float sy,
//                               float x_spacing, float y_spacing);

extern void Prof_draw_sdl(float sx, float sy,
						 float width, float height,
						 float line_spacing,
						 int precision,
						 void (*print_text)(float x, float y, char *str),
						 float (*text_width)(char *str));

/*
*  Parameters
*    <sx, sy>      --  origin of the graph--location of (0,0)
*    x_spacing     --  screenspace size of each history sample; e.g.
*                         2.0 pixels
*    y_spacing     --  screenspace size of one millisecond of time;
*                         for an app with max of 20ms in any one zone,
*                         8.0 would produce a 160-pixel tall display,
*                         assuming screenspace is in pixels
*/
extern void Prof_draw_graph_sdl(float sx, float sy,
							   float x_spacing, float y_spacing);
typedef enum
{
   Prof_SELF_TIME,
   Prof_HIERARCHICAL_TIME,
   Prof_CALL_GRAPH,
   Prof_Last_Mode = Prof_CALL_GRAPH
} Prof_Report_Mode;

extern Prof_Report_Mode Prof_get_report_mode();
extern void Prof_set_report_mode(Prof_Report_Mode e);
extern void Prof_move_cursor(int delta);
extern void Prof_select(void);
extern void Prof_select_parent(void);
extern void Prof_move_frame(int delta);

extern void Prof_set_smoothing(int smoothing_mode);
extern void Prof_set_frame(int frame);
extern void Prof_set_cursor(int line);

typedef enum
{
   Prof_FLATTEN_RECURSION,
   Prof_SPREAD_RECURSION
} Prof_Recursion_Mode;

extern void Prof_set_recursion(Prof_Recursion_Mode e);

#ifdef __cplusplus
}
#endif

#endif // Prof_INC_PROF_H

