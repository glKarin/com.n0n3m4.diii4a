/*
this file is expected to be #included as the body of a real function
to define create a new pixel shader, define PLOT_PIXEL(outval) at the top of your function and you're good to go

//modifiers:
SPAN_ST - interpolates S+T across the span. access with 'sc' and 'tc'
		affine... no perspective correction.


*/

{
	swvert_t *vt;
	int y;
	int secondhalf;
	int xl,xld, xr,xrd;
#ifdef SPAN_ST
	float sl,sld, sd;
	float tl,tld, td;
#endif
#ifdef SPAN_Z
	unsigned int zl,zld, zd;
#endif
	unsigned int *restrict outbuf;
	unsigned int *restrict ti;
	int i;
	const swvert_t *vlt,*vlb,*vrt,*vrb;
	int spanlen;
	int numspans;
	unsigned int *vplout;
	int dx, dy;
	int recalcside;
	int interlace;

	float fdx1,fdy1,fdx2,fdy2,fz,d1,d2;

	if (!img)
		return;

	/*we basically render a diamond
	that is, the single triangle is split into two triangles, outwards towards the midpoint and inwards to the final position.
	*/

	/*reorder the verticies for height*/
	if (v1->vcoord[1] > v2->vcoord[1])
	{
		vt = v1;
		v1 = v2;
		v2 = vt;
	}
	if (v1->vcoord[1] > v3->vcoord[1])
	{
		vt = v1;
		v1 = v3;
		v3 = vt;
	}
	if (v2->vcoord[1] > v3->vcoord[1])
	{
		vt = v3;
		v3 = v2;
		v2 = vt;
	}

	{
		const swvert_t *v[3];

		v[0] = v1;
		v[1] = v2;
		v[2] = v3;

		//reject triangles with any point offscreen, for now
		for (i = 0; i < 3; i++)
		{
			if (v[i]->vcoord[0] < 0 || v[i]->vcoord[0] >= th->vpwidth)
				return;
			if (v[i]->vcoord[1] < 0 || v[i]->vcoord[1] >= th->vpheight)
				return;
			if (v[i]->vcoord[2] < 0)
				return;
		}

		for (i = 0; i < 2; i++)
		{
			if (v[i]->vcoord[1] > v[i+1]->vcoord[1])
				return;
		}
	}

	fdx1 = v2->vcoord[0] - v1->vcoord[0];
	fdy1 = v2->vcoord[1] - v1->vcoord[1];

	fdx2 = v3->vcoord[0] - v1->vcoord[0];
	fdy2 = v3->vcoord[1] - v1->vcoord[1];

	fz = fdx1*fdy2 - fdx2*fdy1;

	if (fz == 0)
	{
		//weird angle...
		return;
	}

	fz = 1.0 / fz;
	fdx1 *= fz;
	fdy1 *= fz;
	fdx2 *= fz;
	fdy2 *= fz;

#ifdef SPAN_ST	//affine
	d1 = v2->tccoord[0] - v1->tccoord[0];
	d2 = v3->tccoord[0] - v1->tccoord[0];
	sld = fdx1*d2 - fdx2*d1;
	sd = fdy2*d1 - fdy1*d2;

	d1 = v2->tccoord[1] - v1->tccoord[1];
	d2 = v3->tccoord[1] - v1->tccoord[1];
	tld = fdx1*d2 - fdx2*d1;
	td = fdy2*d1 - fdy1*d2;
#endif
#ifdef SPAN_Z
	d1 = (v2->vcoord[2] - v1->vcoord[2])*UINT_MAX;
	d2 = (v3->vcoord[2] - v1->vcoord[2])*UINT_MAX;
	zld = fdx1*d2 - fdx2*d1;
	zd = fdy2*d1 - fdy1*d2;
#endif

	ti = img->data;

	y = v1->vcoord[1];

	for (secondhalf = 0; secondhalf <= 1; secondhalf++)
	{
		if (secondhalf)
		{
			if (numspans < 0)
			{
				interlace = -numspans;
				y+=interlace;
				numspans-=interlace;

				xl += xld*interlace;
				xr += xrd*interlace;
				vplout += th->vpcstride*interlace;

#ifdef SPAN_ST
				sl += sld*interlace;
				tl += tld*interlace;
#endif
#ifdef SPAN_Z
				zl += zld*interlace;
#endif
			}

			/*v2->v3*/
			if (fz <= 0)
			{
				vlt = v2;
				//vrt == v1;
				vlb = v3;
				//vrb == v3;

				recalcside = 1;

#ifdef SPAN_ST
				sld -= sd*xld/(float)(1<<16);
				tld -= td*xld/(float)(1<<16);
#endif
#ifdef SPAN_Z
				zld -= zd*xld/(float)(1<<16);
#endif
			}
			else
			{
				//vlt == v1;
				vrt = v2;
				///vlb == v3;
				vrb = v3;

				recalcside = 2;
			}

			//flip the triangle to keep it facing the screen (we swapped the verts almost randomly)
			numspans = v3->vcoord[1] - y;
		}
		else
		{
			vlt = v1;
			vrt = v1;
			/*v1->v2*/
			if (fz < 0)
			{
				vlb = v2;
				vrb = v3;
			}
			else
			{
				vlb = v3;
				vrb = v2;
			}
			recalcside = 3;

			//flip the triangle to keep it facing the screen (we swapped the verts almost randomly)
			numspans = v2->vcoord[1] - y;
		}

		if (recalcside & 1)
		{
			dx = (vlb->vcoord[0] - vlt->vcoord[0]);
			dy = (vlb->vcoord[1] - vlt->vcoord[1]);
			if (dy > 0)
				xld = (dx<<16) / dy;
			else
				xld = 0;
			xl = (int)vlt->vcoord[0]<<16;

#ifdef SPAN_ST
			sl = vlt->tccoord[0];
			sld = sld + sd*xld/(float)(1<<16);
			tl = vlt->tccoord[1];
			tld = tld + td*xld/(float)(1<<16);
#endif
#ifdef SPAN_Z
			zl = vlt->vcoord[2]*UINT_MAX;
			zld = zld + zd*xld/(float)(1<<16);
#endif
		}

		if (recalcside & 2)
		{
			dx = (vrb->vcoord[0] - vrt->vcoord[0]);
			dy = (vrb->vcoord[1] - vrt->vcoord[1]);
			if (dy)
				xrd = (dx<<16) / dy;
			else
				xrd = 0;
			xr = (int)vrt->vcoord[0]<<16;
		}



		if (y + numspans >= th->vpheight)
			numspans = th->vpheight - y - 1;

		if (numspans <= 0)
			continue;


		vplout = th->vpcbuf + y * th->vpcstride;	//this is a pointer to the left of the viewport buffer.

		interlace = ((y + th->interlaceline) % th->interlacemod);
		if (interlace)
		{
			if (interlace > numspans)
			{
				interlace = numspans;
				y+=interlace;
			}
			else
			{
				y+=interlace;
				numspans-=interlace;
			}
			xl += xld*interlace;
			xr += xrd*interlace;
			vplout += th->vpcstride*interlace;

#ifdef SPAN_ST
			sl += sld*interlace;
			tl += tld*interlace;
#endif
#ifdef SPAN_Z
			zl += zld*interlace;
#endif
		}

		for (; numspans > 0; 
			numspans -= th->interlacemod
			,xl += xld*th->interlacemod
			,xr += xrd*th->interlacemod
			,vplout += th->vpcstride*th->interlacemod
			,y += th->interlacemod

#ifdef SPAN_ST
			,sl += sld*th->interlacemod
			,tl += tld*th->interlacemod
#endif
#ifdef SPAN_Z
			,zl += zld*th->interlacemod
#endif
			)
		{
#ifdef SPAN_ST
			float s = sl;
			float t = tl;
#endif
#ifdef SPAN_Z
			unsigned int z = zl;
			unsigned int *restrict zb = th->vpdbuf + y * th->vpwidth + (xl>>16);
#endif

			spanlen = (xr - xl)>>16;
			outbuf = vplout + (xl>>16);

			while(spanlen-->=0)
			{
				PLOT_PIXEL(*outbuf);
				outbuf++;

#ifdef SPAN_ST
				s += sd;
				t += td;
#endif
#ifdef SPAN_Z
				z += zd;
				zb++;
#endif
			}
		}
	}
}

#undef SPAN_ST
#undef PLOT_PIXEL
