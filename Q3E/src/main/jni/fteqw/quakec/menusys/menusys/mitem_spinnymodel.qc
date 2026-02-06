/*
renderscene stuff is always available in csqc.
If the engine supports DP_QC_RENDER_SCENE then its meant to be available in menuqc too.
In practise, DP advertises this to menuqc even though it has never officially supported it.
At the time of writing, FTE's dev builds can do it (and only advertise the extension in builds that try to support it).

Note: the basemenu mod does not make use of this in menuqc if only because its vaugely trying to support both engines, and its easier to just use an ifdef.

The mitem_spinnymodel item just shows a rotating model centered on the z axis. Simple as that.
For the sake of fun gimmicks, you can specify frame information with firstframe and framecount.
Additionally, you can cause it to switch to a different animation when it faces towards the camera with shootframe and shootframes.
This style of animation shouldn't really be considered very complex, but might help give a small demo of the basic concept of animation.
*/
/*
Note - DP Bugs:
1: 	viewport positions are interpreted in physical pixels in DP.
  	there is no reliable way to know how many physical pixels there actually are.
	custom viewports are thus near unusable.
2:	failure to revert the viewport to default afterwards fucks over any 2d stuff drawn afterwards.
	including the console.
3:	bloom on worldless models draws the world.
4:	lighting is applied on models even if there's no world.
	enabling rtlights really fucks everything up.
5:	gamma/contrast/brightness are applied to the 3d view. even if there's no world.
	this results in horrible squares if these settings are used.
6:	DP_QC_RENDER_SCENE is advertised in menuqc, but renderscene is not supported there at all.
7:	avoid the use of cltime. DP doesn't support it.

probably others. it really wouldn't surprise me.
*/

//helper function to work around a DP bug.
static vector(vector v) vtodpp =
{
#ifndef CSQC_SIMPLE
	//so fucking disgustingly ugly.
	if (dp_workarounds)
	{
#pragma warning disable F333
		v_x *= cvar("vid_width") / cvar("vid_conwidth");
		v_y *= cvar("vid_height") / cvar("vid_conheight");
#pragma warning enable F333
	}
#endif
	return v;
};
//make sure the fields are all defined if we're in menuqc or something
noref .vector origin;
noref .vector angles;
noref .vector mins;
noref .vector maxs;
noref .string model;
noref .float frame, frame2, lerpfrac, renderflags;
float frametime;
class mitem_spinnymodel : mitem
{
	float zbias;
	float firstframe;
	float framecount;
	float shootframe;
	float shootframes;
	float rotatespeed;
	
#ifndef EXT_CSQC
	virtual void(vector pos) item_draw = {};
#else

	//might as well use the class as the entity that is drawn.
	//it might end up clobbering any fields used for multiple things...
	virtual void(vector pos) item_draw =
	{
#ifndef CSQC
		if (!checkbuiltin(clearscene) || !checkbuiltin(setviewprop) || !checkbuiltin(addentity) || !checkbuiltin(renderscene))
		{
			drawstring(pos, "Renderscene unsupported", [8,8], [1,1,1],1,0);
			return;
		}
		__using VF_ORIGIN, VF_ANGLES, VF_MIN, VF_SIZE, VF_FOV, VF_AFOV;
#endif
		vector orgbias;
		orgbias = '0 0 0';
		if (dp_workarounds)
			__using(getviewprop) orgbias = (vector)getviewprop(VF_ORIGIN);	//DP still lights the entity even if the world isn't drawn. this results in inconsistant/buggy light levels. there's nothing we can do about that other than stopping it from being completely black.
		origin = orgbias;
		origin_z += zbias;
	
//		angles_y = cltime*90;//frametime*90;
		angles_y += frametime*rotatespeed;

		clearscene(); //wipe the scene, and apply the default rendering values.
		setviewprop(VF_MIN, vtodpp(pos)); //min pos
		setviewprop(VF_SIZE, vtodpp(item_size));	//size=maxpos-minpos
		if (dp_workarounds)
			setviewprop(VF_FOV, [25, atan(item_size_y/(item_size_x/tan(25/360*6.28))) * 360/6.28]); //set an explicit fov. this thing had better be square. DP doesn't support VF_AFOV
		else
			setviewprop(VF_AFOV, 25); //set the aproximate fov (ie: engine takes care of aspect ratio).
#ifdef CSQC
		setproperty(VF_DRAWENGINESBAR, FALSE);
		setproperty(VF_DRAWWORLD, FALSE);
		setproperty(VF_DRAWCROSSHAIR, FALSE);
#endif
		setviewprop(VF_ORIGIN, orgbias + '-128 0 0');	//look towards it.
		setviewprop(VF_ANGLES, '0 0 0');
		
		//animate it a bit
		lerpfrac -= frametime*10;
		while(lerpfrac < 0)
		{
			lerpfrac += 1;
			frame2 = frame;
			frame += 1;
			if (angles_y >= 170 && shootframes)
			{
				if (frame == shootframe+shootframes)
				{	//reached the last frame. clear the shooting 'flag' and go back to idle
					frame = firstframe;
					angles_y -= 360;
				}
				else if (frame < shootframe || frame >= shootframe+shootframes)
					frame = shootframe;	//we were still idle, apparently.
			}
			else
			{
				if (frame < firstframe || frame >= firstframe+framecount)
					frame = firstframe;
			}
		}
		
		addentity(this);
		renderscene();
		if (dp_workarounds)
		{	//dp fucks up 2d stuff if we don't explicitly restore the 3d view to fullscreen
			setviewprop(VF_MIN, vtodpp('0 0 0'));
			setviewprop(VF_SIZE, vtodpp(ui.screensize));
		}
	};
	
	virtual void(vector pos) dp_draw =
	{
	};

	void() mitem_spinnymodel =
	{
		if (!checkbuiltin(renderscene))
		{
//			item_draw = dp_draw;
//			return;
		}

#ifndef CSQC
		if (checkbuiltin(precache_model) && checkbuiltin(setmodel))
		{
			precache_model(item_text);
			setmodel(this, item_text);	//use the size information from the engine, woo for unreliability.
		}
#endif
		zbias += (mins_z - maxs_z)/2 - mins_z; //center the model on its z axis, so the whole thing is visible.
		frame = firstframe;
		if (!angles_y && !rotatespeed)
			rotatespeed=90;

	};
#endif
};

