
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "prof.h"
#include "prof_internal.h"

#ifdef _WIN32
#pragma warning(disable:4305; disable:4244)
#endif

// use factor to compute a glow amount
int get_colors(float factor,
                       float text_color_ret[3],
                       float glow_color_ret[3],
                       float *glow_alpha_ret)
{
   const float GLOW_RANGE = 0.5f;
   const float GLOW_ALPHA_MAX = 0.5f;
   float glow_alpha;
   int i;
   float hot[3] = {1, 1.0, 0.9};
   float cold[3] = {0.15, 0.9, 0.15};

   float glow_cold[3] = {0.5f, 0.5f, 0};
   float glow_hot[3] = {1.0f, 1.0f, 0};

   if (factor < 0) factor = 0;
   if (factor > 1) factor = 1;

   for (i=0; i < 3; ++i)
      text_color_ret[i] = cold[i] + (hot[i] - cold[i]) * factor;

   // Figure out whether to start up the glow as well.
   glow_alpha = (factor - GLOW_RANGE) / (1 - GLOW_RANGE);
   if (glow_alpha < 0) {
      *glow_alpha_ret = 0;
      return 0;
   }

   for (i=0; i < 3; ++i)
      glow_color_ret[i] = glow_cold[i] + (glow_hot[i] - glow_cold[i]) * factor;

   *glow_alpha_ret = glow_alpha * GLOW_ALPHA_MAX;
   return 1;
}

//static void draw_rectangle(float x0, float y0, float x1, float y1)
//{
//   // FACE_CULL is disabled so winding doesn't matter
//   glVertex2f(x0, y0);
//   glVertex2f(x1, y0);
//   glVertex2f(x1, y1);
//   glVertex2f(x0, y1);
//}

extern void draw_rectangle_sdl(float x0, float y0, float x1, float y1);
extern void draw_line_sdl(float x0, float y0, float x1, float y1);

typedef struct
{
   float x0,y0;
   float sx,sy;
} GraphLocation;

//static void graph_func(int id, int x0, int x1, float *values, void *data)
//{
//   GraphLocation *loc = (GraphLocation *) data;
//   int i,r,g,b;
//   float lx = 0.f,ly=0.f;
//
//   // trim out values that are under 0.2 ms to accelerate rendering
//   while (x0 < x1 && (*values < 0.0002f)) { ++x0; ++values; }
//   while (x1 > x0 && (values[x1-1-x0] < 0.0002f)) --x1;
//
//   if (id == 0)
//   {
//	   g_CurrentColour.r = 255;
//	   g_CurrentColour.g = 255;
//	   g_CurrentColour.b = 255;
//	   g_CurrentColour.unused = 128;
//   }
//   else 
//   {
//      //if (x0 == x1) return;
//
//      id = (id >> 8) + id;
//      r = id * 37;
//      g = id * 59;
//      b = id * 45;
//#pragma warning(disable:4761)
//	  g_CurrentColour.r = (r & 127) + 80;
//	  g_CurrentColour.g = (g & 127) + 80;
//	  g_CurrentColour.b = (b & 127) + 80;
//	  g_CurrentColour.unused = 128;
//   }
//
//   if (x0 == x1)
//   {
//      float x,y;
//      x = loc->x0 + x0 * loc->sx;
//      y = loc->y0 + values[0] * loc->sy;
//	  draw_line_sdl(x, loc->y0, x, y);
//
//	  lx = x;
//	  ly = y;
//   }
//   for (i=0; i < x1-x0; ++i) 
//   {
//      float x,y;
//      x = loc->x0 + (i+x0) * loc->sx;
//      y = loc->y0 + values[i] * loc->sy;
//
//	  draw_line_sdl(lx, ly, x, y);
//
//	  lx = x;
//	  ly = y;
//   }
//}

//Prof_extern_C void Prof_draw_graph_gl(float sx, float sy, float x_spacing, float y_spacing)
//{
//#ifdef Prof_ENABLED
//   Prof_Begin(iprof_draw_graph)
//   GraphLocation loc = { sx, sy, x_spacing, y_spacing * 1000 };
//   Prof_graph(128, graph_func, &loc);
//   Prof_End
//#endif
//}


// float to string conversion with sprintf() was
// taking up 10-20% of the Prof_draw time, so I
// wrote a faster float-to-string converter

static char int_to_string[100][4];
static char int_to_string_decimal[100][4];
static char int_to_string_mid_decimal[100][4];

int int_to_string_has_init()
{
	return int_to_string[0][0];
}

void int_to_string_init()
{
	int i;
   for (i=0; i < 100; ++i) 
   {
      sprintf(int_to_string[i], "%d", i);
      sprintf(int_to_string_decimal[i], ".%02d", i);
      sprintf(int_to_string_mid_decimal[i], "%d.%d", i/10, i % 10);
   }
}

static char *formats[5] =
{
   "%.0f",
   "%.1f",
   "%.2f",
   "%.3f",
   "%.4f",
};

void float_to_string(char *buf, float num, int precision)
{
   int x,y;
   switch(precision) {
      case 2:
         if (num < 0 || num >= 100)
            break;
         x = num;
         y = (num - x) * 100;
         strcpy(buf, int_to_string[x]);
         strcat(buf, int_to_string_decimal[y]);
         return;
      case 3:
         if (num < 0 || num >= 10)
            break;
         num *= 10;
         x = num;
         y = (num - x) * 100;
         strcpy(buf, int_to_string_mid_decimal[x]);
         strcat(buf, int_to_string_decimal[y]+1);
         return;
      case 4:
         if (num < 0 || num >= 1)
            break;
         num *= 100;
         x = num;
         y = (num - x) * 100;
         buf[0] = '0';
         strcpy(buf+1, int_to_string_decimal[x]);
         strcat(buf, int_to_string_decimal[y]+1);
         return;
   }
   sprintf(buf, formats[precision], num);
}

//Prof_extern_C void Prof_draw_gl(float sx, float sy,
//                                float full_width, float height,
//                                float line_spacing, int precision,
//    void (*printText)(float x, float y, char *str), float (*textWidth)(char *str))
//{
//#ifdef Prof_ENABLED
//   Prof_Begin(iprof_draw)
//
//   int i,j,n,o;
//   GLuint cull, texture;
//   float backup_sy;
//
//   float field_width = textWidth("5555.55");
//   float name_width  = full_width - field_width * 3;
//   float plus_width  = textWidth("+");
//
//   int max_records;
//
//   Prof_Report *pob;
//
//   if (!int_to_string[0][0]) int_to_string_init();
//
//   if (precision < 1) precision = 1;
//   if (precision > 4) precision = 4;
//
//   // disable face culling to avoid having to get winding correct
//   texture = glIsEnabled(GL_TEXTURE_2D);
//   cull = glIsEnabled(GL_CULL_FACE);
//   if (cull == GL_TRUE) {
//      glDisable(GL_CULL_FACE);
//   }
//
//   pob = Prof_create_report();
//
//   for (i=0; i < NUM_TITLE; ++i) {
//      if (pob->title[i]) {
//         float header_x0 = sx;
//         float header_x1 = header_x0 + full_width;
//
//         if (i == 0)
//            glColor4f(0.1f, 0.3f, 0, 0.85);
//         else
//            glColor4f(0.2f, 0.1f, 0.1f, 0.85);
//
//         glBegin(GL_QUADS);
//         draw_rectangle(header_x0, sy-2, header_x1, sy-line_spacing+2);
//         glEnd();
//
//         if (i == 0) 
//            glColor4f(0.6, 0.4, 0, 0);
//         else
//            glColor4f(0.8f, 0.1f, 0.1f, 0);
//
//         printText(sx+2, sy, pob->title[i]);
//
//         sy += 1.5*line_spacing;
//         height -= abs(line_spacing)*1.5;
//      }
//   }
//
//   max_records = height / abs(line_spacing);
//
//   o = 0;
//   n = pob->num_record;
//   if (n > max_records) n = max_records;
//   if (pob->hilight >= o + n) {
//      o = pob->hilight - n + 1;
//   }
//
//   backup_sy = sy;
//
//   // Draw the background colors for the zone data.
//   glDisable(GL_TEXTURE_2D);
//   glBegin(GL_QUADS);
//
//   glColor4f(0,0,0,0.85);
//   draw_rectangle(sx, sy, sx + full_width, sy - line_spacing);
//   sy += line_spacing;
//
//   for (i = 0; i < n; i++) {
//      float y0, y1;
//
//      if (i & 1) {
//         glColor4f(0.1, 0.1f, 0.2, 0.85);
//      } else {
//         glColor4f(0.1f, 0.1f, 0.3, 0.85);
//      }
//      if (i+o == pob->hilight)
//         glColor4f(0.3f, 0.3f, 0.1f, 0.85);
//
//      y0 = sy;
//      y1 = sy - line_spacing;
//
//      draw_rectangle(sx, y0, sx + full_width, y1);
//      sy += line_spacing;
//   }
//   glEnd();
//
//   sy = backup_sy;
//   glColor4f(0.7,0.7,0.7,0);
//
//   if (pob->header[0])
//      printText(sx+8, sy, pob->header[0]);
//
//   for (j=1; j < NUM_HEADER; ++j)
//      if (pob->header[j])
//         printText(sx + name_width + field_width * (j-1) + 
//            field_width/2 - textWidth(pob->header[j])/2, sy, pob->header[j]);
//
//   sy += line_spacing;
//
//   for (i = 0; i < n; i++) {
//      char buf[256], *b = buf;
//      Prof_Report_Record *r = &pob->record[i+o];
//      float text_color[3], glow_color[3];
//      float glow_alpha;
//      float x = sx + textWidth(" ") * r->indent + plus_width/2;
//      if (r->prefix) {
//         buf[0] = r->prefix;
//         ++b;
//      } else {
//         x += plus_width;
//      }
//      if (r->number)
//         sprintf(b, "%s (%d)", r->name, r->number);
//      else
//         sprintf(b, "%s", r->name);
//      if (get_colors(r->heat, text_color, glow_color, &glow_alpha)) {
//         glColor4f(glow_color[0], glow_color[1], glow_color[2], glow_alpha);
//         printText(x+2, sy-1, buf);
//      }
//      glColor3fv(text_color);
//      printText(x + 1, sy, buf);
//
//      for (j=0; j < NUM_VALUES; ++j) {
//         if (r->value_flag & (1 << j)) {
//            int pad;
//            float_to_string(buf, r->values[j], j == 2 ? 2 : precision);
//            pad = field_width- plus_width - textWidth(buf);
//            if (r->indent) pad += plus_width;
//            printText(sx + pad + name_width + field_width * j, sy, buf);
//         }
//      }
//              
//
//      sy += line_spacing;
//   }
//
//   Prof_free_report(pob);
//
//   if (cull == GL_TRUE)
//      glEnable(GL_CULL_FACE);
//   if (texture == GL_TRUE)
//      glEnable(GL_TEXTURE_2D);
//
//   Prof_End
//#endif
//}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//Prof_extern_C void Prof_draw_sdl(float sx, float sy,
//								float full_width, float height,
//								float line_spacing, int precision,
//								void (*printText)(float x, float y, char *str), float (*textWidth)(char *str))
//{
//#ifdef Prof_ENABLED
//	Prof_Begin(iprof_draw)
//
//	int i,j,n,o;
//	float backup_sy;
//
//	float field_width = textWidth("5555.55");
//	float name_width  = full_width - field_width * 3;
//	float plus_width  = textWidth("+");
//
//	int max_records;
//
//	Prof_Report *pob;
//
//	if (!int_to_string[0][0]) 
//		int_to_string_init();
//
//	if (precision < 1) precision = 1;
//	if (precision > 4) precision = 4;
//
//	pob = Prof_create_report();
//
//	for (i=0; i < NUM_TITLE; ++i) {
//		if (pob->title[i]) {
//			float header_x0 = sx;
//			float header_x1 = header_x0 + full_width;
//
//			if (i == 0)
//			{
//				g_CurrentColour.cdata.m_RGBA[0] = 25;
//				g_CurrentColour.cdata.m_RGBA[1] = 76;
//				g_CurrentColour.cdata.m_RGBA[2] = 0;
//				g_CurrentColour.cdata.m_RGBA[3] = 255;
//			}
//			else
//			{
//				g_CurrentColour.cdata.m_RGBA[0] = 51;
//				g_CurrentColour.cdata.m_RGBA[1] = 25;
//				g_CurrentColour.cdata.m_RGBA[2] = 51;
//				g_CurrentColour.cdata.m_RGBA[3] = 255;
//			}			
//
//			draw_rectangle_sdl(header_x0, sy+2, header_x1, sy+line_spacing+2);
//
//			if (i == 0)
//			{
//				g_CurrentColour.cdata.m_RGBA[0] = 153;
//				g_CurrentColour.cdata.m_RGBA[1] = 102;
//				g_CurrentColour.cdata.m_RGBA[2] = 0;
//				g_CurrentColour.cdata.m_RGBA[3] = 255;
//			}
//			else
//			{
//				g_CurrentColour.cdata.m_RGBA[0] = 204;
//				g_CurrentColour.cdata.m_RGBA[1] = 25;
//				g_CurrentColour.cdata.m_RGBA[2] = 25;
//				g_CurrentColour.cdata.m_RGBA[3] = 255;
//			}	
//
//			printText(sx+2, sy, pob->title[i]);
//
//			sy += 1.5*line_spacing;
//			height -= abs(line_spacing)*1.5;
//		}
//	}
//
//	max_records = height / abs(line_spacing);
//
//	o = 0;
//	n = pob->num_record;
//	if (n > max_records) n = max_records;
//	if (pob->hilight >= o + n) {
//		o = pob->hilight - n + 1;
//	}
//
//	backup_sy = sy;
//
//	// Draw the background colors for the zone data.
//	draw_rectangle_sdl(sx, sy, sx + full_width, sy + line_spacing);
//	sy += line_spacing;
//
//	for (i = 0; i < n; i++) {
//		float y0, y1;
//
//		if (i & 1) 
//		{
//			g_CurrentColour.cdata.m_RGBA[0] = 25;
//			g_CurrentColour.cdata.m_RGBA[1] = 25;
//			g_CurrentColour.cdata.m_RGBA[2] = 51;
//			g_CurrentColour.cdata.m_RGBA[3] = 255;
//		} 
//		else 
//		{
//			g_CurrentColour.cdata.m_RGBA[0] = 25;
//			g_CurrentColour.cdata.m_RGBA[1] = 25;
//			g_CurrentColour.cdata.m_RGBA[2] = 76;
//			g_CurrentColour.cdata.m_RGBA[3] = 255;
//		}
//		if(i+o == pob->hilight)
//		{
//			g_CurrentColour.cdata.m_RGBA[0] = 76;
//			g_CurrentColour.cdata.m_RGBA[1] = 76;
//			g_CurrentColour.cdata.m_RGBA[2] = 25;
//			g_CurrentColour.cdata.m_RGBA[3] = 255;
//		}
//
//		y0 = sy;
//		y1 = sy + line_spacing;
//
//		draw_rectangle_sdl(sx, y0, sx + full_width, y1);
//		sy += line_spacing;
//	}
//
//	sy = backup_sy;
//
//	g_CurrentColour.cdata.m_RGBA[0] = 178;
//	g_CurrentColour.cdata.m_RGBA[1] = 178;
//	g_CurrentColour.cdata.m_RGBA[2] = 178;
//	g_CurrentColour.cdata.m_RGBA[3] = 255;
//
//	if (pob->header[0])
//		printText(sx+8, sy, pob->header[0]);
//
//	for (j=1; j < NUM_HEADER; ++j)
//		if (pob->header[j])
//			printText(sx + name_width + field_width * (j-1) + 
//			field_width/2 - textWidth(pob->header[j])/2, sy, pob->header[j]);
//
//	sy += line_spacing;
//
//	for (i = 0; i < n; i++) 
//	{
//		char buf[256], *b = buf;
//		Prof_Report_Record *r = &pob->record[i+o];
//		float text_color[3], glow_color[3];
//		float glow_alpha;
//		float x = sx + textWidth(" ") * r->indent + plus_width/2;
//		if (r->prefix) 
//		{
//			buf[0] = r->prefix;
//			++b;
//		} 
//		else 
//		{
//			x += plus_width;
//		}
//		if (r->number)
//			sprintf(b, "%s (%d)", r->name, r->number);
//		else
//			sprintf(b, "%s", r->name);
//		if (get_colors(r->heat, text_color, glow_color, &glow_alpha)) 
//		{
//			g_CurrentColour.cdata.m_RGBA[0] = 255.f * glow_color[0];
//			g_CurrentColour.cdata.m_RGBA[1] = 255.f * glow_color[1];
//			g_CurrentColour.cdata.m_RGBA[2] = 255.f * glow_color[2];
//			g_CurrentColour.cdata.m_RGBA[3] = 255.f * glow_alpha;
//
//			printText(x+2, sy-1, buf);
//		}
//
//		g_CurrentColour.cdata.m_RGBA[0] = 255.f * text_color[0];
//		g_CurrentColour.cdata.m_RGBA[1] = 255.f * text_color[1];
//		g_CurrentColour.cdata.m_RGBA[2] = 255.f * text_color[2];
//		g_CurrentColour.cdata.m_RGBA[3] = 255;
//
//		printText(x + 1, sy, buf);
//
//		for (j=0; j < NUM_VALUES; ++j) 
//		{
//			if (r->value_flag & (1 << j)) 
//			{
//				int pad;
//				float_to_string(buf, r->values[j], j == 2 ? 2 : precision);
//				pad = field_width- plus_width - textWidth(buf);
//				if (r->indent) pad += plus_width;
//				printText(sx + pad + name_width + field_width * j, sy, buf);
//			}
//		}
//		sy += line_spacing;
//	}
//
//	Prof_free_report(pob);
//
//	Prof_End
//#endif
//}

//Prof_extern_C void Prof_draw_graph_sdl(float sx, float sy, float x_spacing, float y_spacing)
//{
//#ifdef Prof_ENABLED
//	Prof_Begin(iprof_draw_graph)
//	GraphLocation loc = { sx, sy, x_spacing, y_spacing * 1000 };
//	Prof_graph(128, graph_func, &loc);
//	Prof_End
//#endif
//}
