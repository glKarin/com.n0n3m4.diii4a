/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

class CLightRectangle {
public:
  // the rectangle coordinates
  PIX lr_pixMinU;
  PIX lr_pixMinV;
  PIX lr_pixSizeU;
  PIX lr_pixSizeV;
  // point on the plane closest to the light
  FLOAT lr_fpixHotU;
  FLOAT lr_fpixHotV;
  FLOAT lr_fLightPlaneDistance;
};
