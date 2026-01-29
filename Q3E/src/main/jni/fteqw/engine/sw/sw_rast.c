#include "quakedef.h"
#ifdef SWQUAKE
#include "sw.h"
#include "gl_draw.h"
#include "shader.h"
#include "renderque.h"
#include "glquake.h"

#if __STDC_VERSION__ >= 199901L
	//no need to do anything
#elif defined(_MSC_VER)
	#define restrict __restrict
#else
	#define restrict
#endif

#define ZI_MAX	0xffff

/*
Our software rendering basically works like this:

main thread builds command:
	command contains vertex data in the command block
	main thread runs the vertex programs (much like q3) and performs matrix transforms (much like d3d)

worker threads read each command sequentially:
	clip to viewport

division of labour between worker threads works by interlacing.
each thread gets a different set of scanlines to render.
we can also trivially implement interlacing with this method

*/

cvar_t sw_interlace = CVAR("sw_interlace", "0");
cvar_t sw_vthread = CVAR("sw_vthread", "0");
cvar_t sw_fthreads = CVAR("sw_fthreads", "0");

struct workqueue_s commandqueue;
struct workqueue_s spanqueue;

static void WT_Triangle(swthread_t *th, swimage_t *img, swvert_t *v1, swvert_t *v2, swvert_t *v3)
{
	//affine vs correct:
	//to correct perspective, divide interpolants by z.
	//per pixel, divide by interpolated 1 (actually 1/z)

	unsigned int tpix;
#if 1
	#define PERSPECTIVE(v) (v>>16)
#else
	#define PERSPECTIVE(v) (v/zi)
	#define SPAN_ZI
#endif
#define SPAN_ST

#define SPAN_Z
#define PLOT_PIXEL(o) \
	{	\
		if (*zb >= z)	\
		{	\
			*zb = z;	\
		tpix = img->data[	\
					((unsigned)PERSPECTIVE(s)&img->pwidthmask)	\
					+ (((unsigned)PERSPECTIVE(t)&img->pheightmask) * img->pitch)	\
				];	\
		if (tpix&0xff000000) \
			o = tpix; \
		}	\
	}

#ifdef MSVCWORKSPROPERLY
#include "sw_spans.h"
#else
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

	//l=value on left
	//ld=change per y (on left)
	//d=change per x
	int xl,xld, xr,xrd;
#ifdef SPAN_ST
	int sl,sld, sd;
	int tl,tld, td;
#endif
#ifdef SPAN_ZI
	int zil, zild, zid;
#endif
#ifdef SPAN_Z
	int zl,zld, zd;
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
	if (v1->scoord[1] > v2->scoord[1])
	{
		vt = v1;
		v1 = v2;
		v2 = vt;
	}
	if (v1->scoord[1] > v3->scoord[1])
	{
		vt = v1;
		v1 = v3;
		v3 = vt;
	}
	if (v2->scoord[1] > v3->scoord[1])
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
			if (v[i]->scoord[0] < 0 || v[i]->scoord[0] > th->vpwidth)
				return;
			if (v[i]->scoord[1] < 0 || v[i]->scoord[1] > th->vpheight)
				return;
			if (v[i]->zicoord < 0)
				return;
		}

		for (i = 0; i < 2; i++)
		{
			if (v[i]->scoord[1] > v[i+1]->scoord[1])
				return;
		}
	}

	fdx1 = v2->scoord[0] - v1->scoord[0];
	fdy1 = v2->scoord[1] - v1->scoord[1];

	fdx2 = v3->scoord[0] - v1->scoord[0];
	fdy2 = v3->scoord[1] - v1->scoord[1];

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
	d1 = (v2->tccoord[0] - v1->tccoord[0])*(img->pwidth<<16);
	d2 = (v3->tccoord[0] - v1->tccoord[0])*(img->pwidth<<16);
	sld = fdx1*d2 - fdx2*d1;
	sd = fdy2*d1 - fdy1*d2;

	d1 = (v2->tccoord[1] - v1->tccoord[1])*(img->pheight<<16);
	d2 = (v3->tccoord[1] - v1->tccoord[1])*(img->pheight<<16);
	tld = fdx1*d2 - fdx2*d1;
	td = fdy2*d1 - fdy1*d2;
#endif
#ifdef SPAN_ZI
	d1 = (1<<16);
	d2 = (1<<16);
	zild = 0;//fdx1*d2 - fdx2*d1;
	zid = 0;//fdy2*d1 - fdy1*d2;
#endif
#ifdef SPAN_Z
	d1 = (v2->zicoord - v1->zicoord)*(1<<16);
	d2 = (v3->zicoord - v1->zicoord)*(1<<16);
	zld = fdx1*d2 - fdx2*d1;
	zd = fdy2*d1 - fdy1*d2;
#endif

	ti = img->data;

	y = v1->scoord[1];

	for (secondhalf = 0; secondhalf <= 1; secondhalf++)
	{
		if (secondhalf)
		{
		//	return;
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
#ifdef SPAN_ZI
				zil += zild*interlace;
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
				sld -= (((long long)sd*xld)>>16);
				tld -= (((long long)td*xld)>>16);
#endif
#ifdef SPAN_ZI
				zild -= (((long long)zid*xld)>>16);
#endif
#ifdef SPAN_Z
				zld -= (((long long)zd*xld)>>16);
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
			numspans = v3->scoord[1] - y;
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
			numspans = v2->scoord[1] - y;
		}

		if (recalcside & 1)
		{
			dx = (vlb->scoord[0] - vlt->scoord[0]);
			dy = (vlb->scoord[1] - vlt->scoord[1]);
			if (dy > 0)
				xld = (dx<<16) / dy;
			else
				xld = 0;
			xl = (int)vlt->scoord[0]<<16;

#ifdef SPAN_ST
			sl = vlt->tccoord[0] * (img->pwidth<<16);
			sld = sld + (((long long)sd*xld+32767)>>16);
			tl = vlt->tccoord[1] * (img->pheight<<16);
			tld = tld + (((long long)td*xld+32767)>>16);
#endif
#ifdef SPAN_ZI
			zil = (1<<16);///vlt->zicoord;
			zild = zild + (((long long)zid*xld)>>16);
#endif
#ifdef SPAN_Z
			zl = vlt->zicoord * (1<<16);
			zld = zld + (((long long)zd*xld)>>16);
#endif
		}

		if (recalcside & 2)
		{
			dx = (vrb->scoord[0] - vrt->scoord[0]);
			dy = (vrb->scoord[1] - vrt->scoord[1]);
			if (dy)
				xrd = (dx<<16) / dy;
			else
				xrd = 0;
			xr = (int)vrt->scoord[0]<<16;
		}

		if (y + numspans > th->vpheight)
			numspans = th->vpheight - y;

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
#ifdef SPAN_ZI
			zil += zild*interlace;
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
#ifdef SPAN_ZI
			,zil += zild*th->interlacemod
#endif
#ifdef SPAN_Z
			,zl += zld*th->interlacemod
#endif
			)
		{
#ifdef SPAN_ST
			unsigned int s = sl;
			unsigned int t = tl;
#endif
#ifdef SPAN_ZI
			unsigned int zi = zil;
#else
			const unsigned int zi = (1<<16);
#endif
#ifdef SPAN_Z
			unsigned int z = zl;
			unsigned int *restrict zb = th->vpdbuf + y * th->vpwidth + (xl>>16);
#endif

			spanlen = (xr - xl)>>16;
			outbuf = vplout + (xl>>16);

			while(spanlen-->0)
			{
				PLOT_PIXEL(*outbuf);
				outbuf++;

#ifdef SPAN_ST
				s += sd;
				t += td;
#endif
#ifdef SPAN_ZI
				zi += zid;
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
#endif
	
}

static void WT_Clip_Top(swthread_t *th, swvert_t *out, swvert_t *in, swvert_t *result)
{
	float frac;
	frac =	(0 - in->scoord[1]) /
			(float)(out->scoord[1] - in->scoord[1]);
	Vector2Interpolate(in->scoord, frac, out->scoord, result->scoord);
	FloatInterpolate(in->zicoord, frac, out->zicoord, result->zicoord);
	result->scoord[1] = 0;
	Vector2Interpolate(in->tccoord, frac, out->tccoord, result->tccoord);
}
static void WT_Clip_Bottom(swthread_t *th, swvert_t *out, swvert_t *in, swvert_t *result)
{
	float frac;
	frac =	((th->vpheight) - in->scoord[1]) /
			(float)(out->scoord[1] - in->scoord[1]);
	Vector2Interpolate(in->scoord, frac, out->scoord, result->scoord);
	FloatInterpolate(in->zicoord, frac, out->zicoord, result->zicoord);
	result->scoord[1] = th->vpheight;
	Vector2Interpolate(in->tccoord, frac, out->tccoord, result->tccoord);
}
static void WT_Clip_Left(swthread_t *th, swvert_t *out, swvert_t *in, swvert_t *result)
{
	float frac;
	frac =	(0 - in->scoord[0]) /
			(float)(out->scoord[0] - in->scoord[0]);
	Vector2Interpolate(in->scoord, frac, out->scoord, result->scoord);
	FloatInterpolate(in->zicoord, frac, out->zicoord, result->zicoord);
	result->scoord[0] = 0;
	Vector2Interpolate(in->tccoord, frac, out->tccoord, result->tccoord);
}
static void WT_Clip_Right(swthread_t *th, swvert_t *out, swvert_t *in, swvert_t *result)
{
	float frac;
	frac =	((th->vpwidth) - in->scoord[0]) /
			(float)(out->scoord[0] - in->scoord[0]);
	Vector2Interpolate(in->scoord, frac, out->scoord, result->scoord);
	FloatInterpolate(in->zicoord, frac, out->zicoord, result->zicoord);
	result->scoord[0] = th->vpwidth;
	Vector2Interpolate(in->tccoord, frac, out->tccoord, result->tccoord);
}
static void WT_Clip_Near(swthread_t *th, swvert_t *out, swvert_t *in, swvert_t *result)
{
	float nearclip = 0;
	double frac;
	frac =	(nearclip - in->zicoord) /
			(out->zicoord - in->zicoord);
	VectorInterpolate(in->vcoord, frac, out->vcoord, result->vcoord);
	FloatInterpolate(in->zicoord, frac, out->zicoord, result->zicoord);
	result->zicoord = nearclip;
	Vector2Interpolate(in->tccoord, frac, out->tccoord, result->tccoord);
}

static void WT_Clip_Far(swthread_t *th, swvert_t *out, swvert_t *in, swvert_t *result)
{
	float farclip = 1;
	double frac;
	frac =	(farclip - in->zicoord) /
			(out->zicoord - in->zicoord);
	VectorInterpolate(in->vcoord, frac, out->vcoord, result->vcoord);
	FloatInterpolate(in->zicoord, frac, out->zicoord, result->zicoord);
	result->zicoord = farclip;
	Vector2Interpolate(in->tccoord, frac, out->tccoord, result->tccoord);
}

static int WT_ClipPoly(swthread_t *th, int incount, swvert_t *inv, swvert_t *outv, int flag, void (*clip)(swthread_t *th, swvert_t *out, swvert_t *in, swvert_t *result))
{
	int p, c;
	int result = 0;
	int pf, cf;
	if (incount < 3)
		return 0;

	for (p = incount - 1, c = 0; c < incount; p = c, c++)
	{
		pf = inv[p].clipflags & flag;
		cf = inv[c].clipflags & flag;

		if (pf && cf)
			continue;	//both clipped, skip it now
		if (pf ^ cf)
		{
			//crossed... emit a new vertex on the boundary
			if (cf)	//new is offscreen
				clip(th, &inv[c], &inv[p], &outv[result]);
			else
				clip(th, &inv[p], &inv[c], &outv[result]);
			outv[result].clipflags = 0;

			if (outv[result].scoord[0] < 0)
				outv[result].clipflags |= CLIP_LEFT_FLAG;
			if (outv[result].scoord[0] > th->vpwidth)
				outv[result].clipflags |= CLIP_RIGHT_FLAG;
			if (outv[result].scoord[1] < 0)
				outv[result].clipflags |= CLIP_TOP_FLAG;
			if (outv[result].scoord[1] > th->vpheight)
				outv[result].clipflags |= CLIP_BOTTOM_FLAG;

			if (outv[result].zicoord < 0)
				outv[result].clipflags |= CLIP_NEAR_FLAG;
			if (outv[result].zicoord > ZI_MAX)
				outv[result].clipflags |= CLIP_FAR_FLAG;

			result++;
		}
		if (!cf)
		{
			outv[result] = inv[c];
			result++;
		}
	}
	return result;
}

//transform the vertex and calculate its final position.
static int WT_TransformVertXY(swthread_t *th, swvert_t *v)
{
	int result = 0;
	vec4_t tr;
	Matrix4x4_CM_Transform34(th->u.matrix, v->vcoord, tr);
	if (tr[3] != 1)
	{
		tr[0] /= tr[3];
		tr[1] /= tr[3];
		tr[2] /= tr[3];
	}

	v->scoord[0] = (tr[0]+1)/2 * th->vpwidth;
	if (v->scoord[0] < 0)
		result |= CLIP_LEFT_FLAG;
	if (v->scoord[0] > th->vpwidth)
		result |= CLIP_RIGHT_FLAG;

	v->scoord[1] = (tr[1]+1)/2 * th->vpheight;
	if (v->scoord[1] < 0)
		result |= CLIP_TOP_FLAG;
	if (v->scoord[1] > th->vpheight)
		result |= CLIP_BOTTOM_FLAG;

	v->clipflags = result;

	return result;
}

static void WT_ClipTriangle(swthread_t *th, swimage_t *img, swvert_t *v1, swvert_t *v2, swvert_t *v3)
{
	unsigned int cflags;
	swvert_t final[2][64];
	int list = 0;
	int i;
	int count;

	//check the near/far planes.
	v1->zicoord = DotProduct(v1->vcoord, th->u.viewplane) - th->u.viewplane[3];
	if (v1->zicoord < 0) v1->clipflags = CLIP_NEAR_FLAG; else if (v1->zicoord >= ZI_MAX) v1->clipflags = CLIP_FAR_FLAG; else v1->clipflags = 0;
	v2->zicoord = DotProduct(v2->vcoord, th->u.viewplane) - th->u.viewplane[3];
	if (v2->zicoord < 0) v2->clipflags = CLIP_NEAR_FLAG; else if (v2->zicoord >= ZI_MAX) v2->clipflags = CLIP_FAR_FLAG; else v2->clipflags = 0;
	v3->zicoord = DotProduct(v3->vcoord, th->u.viewplane) - th->u.viewplane[3];
	if (v3->zicoord < 0) v3->clipflags = CLIP_NEAR_FLAG; else if (v3->zicoord >= ZI_MAX) v3->clipflags = CLIP_FAR_FLAG; else v3->clipflags = 0;

	if (v1->clipflags & v2->clipflags & v3->clipflags)
		return;	//all verticies are off at least one plane
	cflags = v1->clipflags | v2->clipflags | v3->clipflags;

	if (0)//!cflags)
	{
		//figure out the final 2d positions
		cflags = 0;
		for (i = 0; i < count; i++)
			cflags |= WT_TransformVertXY(th, &final[list][i]);

	}
	else
	{
		final[list][0] = *v1;
		final[list][1] = *v2;
		final[list][2] = *v3;
		count = 3;

		//clip to the screen
		if (cflags & CLIP_NEAR_FLAG)
		{
//			return;
			count = WT_ClipPoly(th, count, final[list], final[list^1], CLIP_NEAR_FLAG, WT_Clip_Near);
			list ^= 1;
		}
		if (cflags & CLIP_FAR_FLAG)
		{
			count = WT_ClipPoly(th, count, final[list], final[list^1], CLIP_FAR_FLAG, WT_Clip_Far);
			list ^= 1;
		}

		//figure out the final 2d positions
		cflags = 0;
		for (i = 0; i < count; i++)
			cflags |= WT_TransformVertXY(th, &final[list][i]);
	}

	//and clip those by the screen (instead of by plane, to try to prevent crashes)
	if (cflags & CLIP_TOP_FLAG)
	{
		count = WT_ClipPoly(th, count, final[list], final[list^1], CLIP_TOP_FLAG, WT_Clip_Top);
		list ^= 1;
	}
	if (cflags & CLIP_BOTTOM_FLAG)
	{
		count = WT_ClipPoly(th, count, final[list], final[list^1], CLIP_BOTTOM_FLAG, WT_Clip_Bottom);
		list ^= 1;
	}
	if (cflags & CLIP_LEFT_FLAG)
	{
		count = WT_ClipPoly(th, count, final[list], final[list^1], CLIP_LEFT_FLAG, WT_Clip_Left);
		list ^= 1;
	}
	if (cflags & CLIP_RIGHT_FLAG)
	{
		count = WT_ClipPoly(th, count, final[list], final[list^1], CLIP_RIGHT_FLAG, WT_Clip_Right);
		list ^= 1;
	}

	//draw the damn thing. FIXME: generate spans and push to a fragment thread.
	for (i = 2; i < count; i++)
	{
		WT_Triangle(th, img, &final[list][0], &final[list][i-1], &final[list][i]);
	}
}

void WQ_ClearBuffer(swthread_t *t, unsigned int *mbuf, qintptr_t stride, unsigned int clearval)
{
	int y;
	int x;
	unsigned int *buf;

	for (y = t->interlaceline; y < t->vpheight; y += t->interlacemod)
	{
		buf = mbuf + stride*y;
		for (x = 0; x < (t->vpwidth & ~15);)
		{
			buf[x++] = clearval;
			buf[x++] = clearval;
			buf[x++] = clearval;
			buf[x++] = clearval;
			buf[x++] = clearval;
			buf[x++] = clearval;
			buf[x++] = clearval;
			buf[x++] = clearval;
			buf[x++] = clearval;
			buf[x++] = clearval;
			buf[x++] = clearval;
			buf[x++] = clearval;
			buf[x++] = clearval;
			buf[x++] = clearval;
			buf[x++] = clearval;
			buf[x++] = clearval;
		}
		for (; x < t->vpwidth; )
			buf[x++] = clearval;
	}
}

qboolean WT_HandleCommand(swthread_t *t, wqcom_t *com)
{
	index_t *idx;
	int i;
	switch(com->com.command)
	{
	case WTC_DIE:
		t->readpoint += com->com.cmdsize;
		return 1;
	case WTC_NOOP:
		break;
	case WTC_NEWFRAME:
		break;
	case WTC_UNIFORMS:
		memcpy(&t->u, &com->uniforms.u, sizeof(t->u));
		break;
	case WTC_VIEWPORT:
		t->vpcbuf = com->viewport.cbuf;
		t->vpdbuf = com->viewport.dbuf;
		t->vpwidth = com->viewport.width;
		t->vpheight = com->viewport.height;
		t->vpcstride = com->viewport.stride;
		if (!t->wq->numthreads)
		{
			t->interlacemod = com->viewport.interlace;	//this many vthreads
			t->interlaceline = com->viewport.framenum%com->viewport.interlace;	//this vthread
		}
		else
		{
			t->interlacemod = t->wq->numthreads*com->viewport.interlace;	//this many vthreads
			t->interlaceline = (t->threadnum*com->viewport.interlace) + (com->viewport.framenum%com->viewport.interlace);	//this vthread
		}

		if (com->viewport.clearcolour)
		{
			WQ_ClearBuffer(t, t->vpcbuf, t->vpcstride, 0);
		}
		if (com->viewport.cleardepth)
		{
			WQ_ClearBuffer(t, t->vpdbuf, t->vpwidth, ~0u);
		}
		break;
	case WTC_TRIFAN:
		for (i = 2; i < com->trifan.numverts; i++)
		{
			WT_ClipTriangle(t, com->trifan.texture, &com->trifan.verts[0], &com->trifan.verts[i-1], &com->trifan.verts[i]);
		}
		break;
	case WTC_TRISOUP:
		idx = (index_t*)(com->trisoup.verts + com->trisoup.numverts);
		for (i = 0; i < com->trisoup.numidx; i+=3, idx+=3)
		{
			WT_ClipTriangle(t, com->trisoup.texture, &com->trisoup.verts[idx[0]], &com->trisoup.verts[idx[1]], &com->trisoup.verts[idx[2]]);
		}
		break;
	case WTC_SPANS:
		break;
	default:
		Sys_Printf("Unknown render command!\n");
		break;
	}
	t->readpoint += com->com.cmdsize;
	return false;
}

int WT_Main(void *ptr)
{
	wqcom_t *com;
	swthread_t *t = ptr;
	for(;;)
	{
		if (t->readpoint == t->wq->pos)
		{
			Sys_Sleep(0);
			continue;
		}
		com = (wqcom_t*)&t->wq->queue[t->readpoint & WQ_MASK];
		if (WT_HandleCommand(t, com))
			break;
	}
	return 0;
}
void SWRast_EndCommand(struct workqueue_s *wq, wqcom_t *com)
{
	wq->pos += com->com.cmdsize;

	if (!wq->numthreads)
	{
		//immediate mode
		WT_HandleCommand(wq->swthreads, com);
	}
}
wqcom_t *SWRast_BeginCommand(struct workqueue_s *wq, int cmdtype, unsigned int size)
{
	wqcom_t *com;
	//round the command size up, so we always have space for a noop/wrap if needed
	size = (size + sizeof(com->align)) & ~(sizeof(com->align)-1);

	//generate a noop if we don't have enough space for the command
	if ((wq->pos&WQ_MASK) + size > WQ_SIZE)
	{
//		SWRast_Sync();
		com = (wqcom_t *)&wq->queue[wq->pos&WQ_MASK];
		com->com.cmdsize = WQ_SIZE - wq->pos&WQ_MASK;
		com->com.command = WTC_NOOP;
		SWRast_EndCommand(wq, com);
	}

	com = (wqcom_t *)&wq->queue[wq->pos&WQ_MASK];
	com->com.cmdsize = size;
	com->com.command = cmdtype;

	return com;
}
void SWRast_Sync(struct workqueue_s *wq)
{
	int i;
	swthread_t *t;

	for (i = 0; i < wq->numthreads; i++)
	{
		t = &wq->swthreads[i];
		while (t->readpoint != wq->pos)
			;
	}

	//all worker threads are up to speed
}
void SWRast_CreateThreadPool(struct workqueue_s *wq, int numthreads)
{
	int i = 0;
	swthread_t *t;
	wq->pos = 0;
	numthreads = ((numthreads > WQ_MAXTHREADS)?WQ_MAXTHREADS:numthreads);
#ifdef MULTITHREAD
	for (i = 0; i < numthreads; i++)
	{
		t = &wq->swthreads[i];
		t->threadnum = i;
		t->thread = Sys_CreateThread("swrast", WT_Main, t, THREADP_NORMAL, 0);
		if (!t->thread)
			break;
	}
#else
	numthreads = 0;
#endif
	wq->numthreads = i;

	if (i == 0)
		numthreads = 1;
	else
		numthreads = i;
	for (i = 0; i < numthreads; i++)
	{
		wq->swthreads[i].readpoint = wq->pos;
		wq->swthreads[i].wq = wq;
	}
}
void SWRast_TerminateThreadPool(struct workqueue_s *wq)
{
	int i;
	wqcom_t *com = SWRast_BeginCommand(wq, WTC_DIE, sizeof(com->com));
	SWRast_EndCommand(wq, com);
#ifdef MULTITHREAD
	for (i = 0; i < wq->numthreads; i++)
	{
		Sys_WaitOnThread(wq->swthreads[i].thread);
	}
#endif
	wq->numthreads = 0;
}













void SW_Draw_Init(void)
{
	R2D_Init();

	R_InitFlashblends();
}
void SW_Draw_Shutdown(void)
{
	R2D_Shutdown();
}
void SW_R_Init(void)
{
	SWRast_CreateThreadPool(&commandqueue, sw_vthread.ival?1:0);
	sw_vthread.modified = true;
}
void SW_R_DeInit(void)
{
	SWRast_TerminateThreadPool(&commandqueue);
}
void SW_R_RenderView(void)
{
	extern cvar_t gl_screenangle;
	extern cvar_t gl_mindist;
	vec3_t newa;
	int tmpvisents = cl_numvisedicts;	/*world rendering is allowed to add additional ents, but we don't want to keep them for recursive views*/
	if (!cl.worldmodel || (!cl.worldmodel->nodes && cl.worldmodel->type != mod_heightmap))
		r_refdef.flags |= RDF_NOWORLDMODEL;

	//no fbos here
	vid.fbvwidth = vid.width;
	vid.fbvheight = vid.height;
	vid.fbpwidth = vid.pixelwidth;
	vid.fbpheight = vid.pixelheight;

	{
		//figure out the viewport that we should be using.
		int x = floor(r_refdef.vrect.x * (float)vid.fbpwidth/(float)vid.width);
		int x2 = ceil((r_refdef.vrect.x + r_refdef.vrect.width) * (float)vid.fbpwidth/(float)vid.width);
		int y = floor(r_refdef.vrect.y * (float)vid.fbpheight/(float)vid.height);
		int y2 = ceil((r_refdef.vrect.y + r_refdef.vrect.height) * (float)vid.fbpheight/(float)vid.height);
		int w = x2 - x;
		int h = y2 - y;
		r_refdef.pxrect.x = x;
		r_refdef.pxrect.y = y;
		r_refdef.pxrect.width = w;
		r_refdef.pxrect.height = h;
		r_refdef.pxrect.maxheight = vid.fbpheight;
	}

	AngleVectors (r_refdef.viewangles, vpn, vright, vup);
	VectorCopy (r_refdef.vieworg, r_origin);
	if (r_refdef.useperspective)
		Matrix4x4_CM_Projection_Inf(r_refdef.m_projection_std, r_refdef.fov_x, r_refdef.fov_y, r_refdef.mindist, false);
	else
		Matrix4x4_CM_Orthographic(r_refdef.m_projection_std, -r_refdef.fov_x/2, r_refdef.fov_x/2, -r_refdef.fov_y/2, r_refdef.fov_y/2, r_refdef.mindist, r_refdef.maxdist>=1?r_refdef.maxdist:9999);
	VectorCopy(r_refdef.viewangles, newa);
	newa[0] = r_refdef.viewangles[0];
	newa[1] = r_refdef.viewangles[1];
	newa[2] = r_refdef.viewangles[2] + gl_screenangle.value;
	Matrix4x4_CM_ModelViewMatrix(r_refdef.m_view, newa, r_refdef.vieworg);

	R_SetFrustum (r_refdef.m_projection_std, r_refdef.m_view);

	RQ_BeginFrame();

	Surf_DrawWorld ();		// adds static entities to the list

	S_ExtraUpdate ();	// don't let sound get messed up if going slow

//	R_DrawDecals();

	R_RenderDlights ();

	RQ_RenderBatchClear();

	cl_numvisedicts = tmpvisents;
}
qboolean SW_SCR_UpdateScreen(void)
{
	wqcom_t *com;

	extern cvar_t gl_screenangle;
	float w = vid.width, h = vid.height;

	r_refdef.time = realtime;

	SWBE_Set2D();

	SWRast_Sync(&commandqueue);
	SWRast_Sync(&spanqueue);
	SW_VID_SwapBuffers();
	if (sw_vthread.modified)
	{
		SWRast_TerminateThreadPool(&commandqueue);
		SWRast_CreateThreadPool(&commandqueue, sw_vthread.ival?1:0);
		sw_vthread.modified = false;
	}
	if (sw_fthreads.modified)
	{
		SWRast_TerminateThreadPool(&spanqueue);
		SWRast_CreateThreadPool(&spanqueue, sw_fthreads.ival);
		sw_fthreads.modified = false;
	}

	com = SWRast_BeginCommand(&commandqueue, WTC_VIEWPORT, sizeof(com->viewport));
	com->viewport.interlace = bound(0, sw_interlace.ival, 15)+1;
	com->viewport.clearcolour = r_clear.ival;
	com->viewport.cleardepth = true;
	SW_VID_UpdateViewport(com);
	SWRast_EndCommand(&commandqueue, com);

	Shader_DoReload();

	R2D_Font_Changed();

	//FIXME: playfilm/editor+q3ui
	SCR_SetUpToDrawConsole ();

	if (cls.state == ca_active)
	{
		if (!CSQC_DrawView())
			V_RenderView (false);

		R2D_BrightenScreen();
	}

	SCR_DrawTwoDimensional(0, 0);

	V_UpdatePalette (false);
	return true;
}

void SW_VBO_Begin(vbobctx_t *ctx, size_t maxsize)
{
}
void SW_VBO_Data(vbobctx_t *ctx, void *data, size_t size, vboarray_t *varray)
{
}
void SW_VBO_Finish(vbobctx_t *ctx, void *edata, size_t esize, vboarray_t *earray, void **vbomem, void **ebomem)
{
}
void SW_VBO_Destroy(vboarray_t *vearray, void *mem)
{
}
void SWBE_Scissor(srect_t *rect)
{
}

rendererinfo_t swrendererinfo =
{
	"Software Renderer",
	{
		"sw",
		"Software",
		"SoftRast",
	},
	QR_SOFTWARE,

	SW_Draw_Init,
	SW_Draw_Shutdown,

	SW_UpdateFiltering,
	SW_LoadTextureMips,
	SW_DestroyTexture,

	SW_R_Init,
	SW_R_DeInit,
	SW_R_RenderView,

	SW_VID_Init,
	SW_VID_DeInit,
	SW_VID_SwapBuffers,
	SW_VID_ApplyGammaRamps,
	NULL,
	NULL,
	NULL,
	SW_VID_SetWindowCaption,
	SW_VID_GetRGBInfo,

	SW_SCR_UpdateScreen,

	SWBE_SelectMode,
	SWBE_DrawMesh_List,
	SWBE_DrawMesh_Single,
	SWBE_SubmitBatch,
	SWBE_GetTempBatch,
	SWBE_DrawWorld,
	SWBE_Init,
	SWBE_GenBrushModelVBO,
	SWBE_ClearVBO,
	SWBE_UploadAllLightmaps,
	SWBE_SelectEntity,
	SWBE_SelectDLight,
	SWBE_Scissor,
	SWBE_LightCullModel,

	SW_VBO_Begin,
	SW_VBO_Data,
	SW_VBO_Finish,
	SW_VBO_Destroy,

	SWBE_RenderToTextureUpdate2d,

	"no more"
};
#endif
