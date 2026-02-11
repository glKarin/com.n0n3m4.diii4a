
mergeInto(LibraryManager.library,
{
	//generic handles array
	//yeah, I hope you don't use-after-free. hopefully that sort of thing will be detected on systems with easier non-mangled debuggers.
//	$FTEH__deps: [],
//	$FTEH: {
//		h: [],
//		f: {}
//	},

	//FIXME: split+merge by \n
	emscriptenfte_print : function(msg)
	{
		FTEC.linebuffer += UTF8ToString(msg);
		for(;;)
		{
			nl = FTEC.linebuffer.indexOf("\n");
			if (nl == -1)
				break;
			console.log(FTEC.linebuffer.substring(0, nl));
			FTEC.linebuffer = FTEC.linebuffer.substring(nl+1);
		}
	},
	emscriptenfte_alert : function(msg)
	{
		msg = UTF8ToString(msg);
		console.log(msg);
		alert(msg);
	},
	
	emscriptenfte_window_location : function(msg)
	{
		msg = UTF8ToString(msg);
		console.log("Redirecting page to " + msg);
		window.location = msg;
	},

//	emscriptenfte_handle_alloc__deps : ['$FTEH'],
	emscriptenfte_handle_alloc : function(h)
	{
		for (var i = 0; FTEH.h.length; i+=1)
		{
			if (FTEH.h[i] == null)
			{
				FTEH.h[i] = h;
				return i;
			}
		}
		i = FTEH.h.length;
		FTEH.h[i] = h;
		return i;
	},

	//temp files
	emscriptenfte_buf_createfromarraybuf__deps : ['emscriptenfte_handle_alloc'],
	emscriptenfte_buf_createfromarraybuf : function(buf)
	{
		buf = new Uint8Array(buf);
		var len = buf.length;
		var b = {h:-1, r:1, l:len,m:len,d:buf, n:null};
		b.h = _emscriptenfte_handle_alloc(b);
		return b.h;
	},

	$FTEC__deps : ['emscriptenfte_buf_createfromarraybuf'],
	$FTEC:
	{
		ctxwarned:0,
		pointerislocked:0,
		pointerwantlock:0,
		clipboard:"",
		linebuffer:'',
		localstorefailure:false,
		dovsync:false,
		wakelock:null,
		xrsupport:-1,
		xrsession:null,
		xrframe:null,
		referenceSpace:null,
		w: -1,
		h: -1,
		donecb:0,
		evcb: {
			resize:0,
			mouse:0,
			button:0,
			key:0,
			loadfile:0,
			cbufaddtext:0,
			jbutton:0,
			jaxis:0,
			jorientation:0,
			wantfullscreen:0,
			frame:0
		},

		loadurl : function(url, mime, arraybuf)
		{
			if (FTEC.evcb.loadfile != 0)
			{
				let handle = -1;
				if (arraybuf !== undefined)
					handle = _emscriptenfte_buf_createfromarraybuf(arraybuf);	
				let blen = lengthBytesUTF8(url)+1;
				let urlptr = _malloc(blen);
				stringToUTF8(url, urlptr, blen);
				blen = lengthBytesUTF8(mime)+1;
				let mimeptr = _malloc(blen);
				stringToUTF8(mime, mimeptr,blen);
				{{{makeDynCall('viii','FTEC.evcb.loadfile')}}}(urlptr, mimeptr, handle);
				_free(mimeptr);
				_free(urlptr);
				window.focus();
			}
		},
		cbufadd : function(command)
		{
			if (FTEC.evcb.cbufaddtext != 0)
			{
				let handle = -1;
				let blen = lengthBytesUTF8(command)+1;
				let ptr = _malloc(blen);
				stringToUTF8(command, ptr, blen);
				{{{makeDynCall('vi','FTEC.evcb.cbufaddtext')}}}(ptr);
				_free(ptr);
				window.focus();
			}
		},

		step : function(timestamp)
		{
			if (FTEC.aborted)
				return;

			//do this first in the hope that it'll make firefox a smidge smoother.
			if (FTEC.dovsync)
				window.requestAnimationFrame(FTEC.step);
			else
				setTimeout(FTEC.step, 0, performance.now());

			if (FTEC.xrsession)
				return;	//keep ticking, cos its safer. :\

			try	//this try is needed to handle Host_EndGame properly.
			{
				FTEC.dovsync = {{{makeDynCall('if','FTEC.evcb.frame')}}}(timestamp);
			}
			catch(err)
			{
				console.log(err);
			}
		},
		doxrframe : function(timestamp, frame)
		{
			if (FTEC.aborted || FTEC.xrsession == null)
				return;

			//do this first in the hope that it'll make firefox a smidge smoother.
			FTEC.xrsession.requestAnimationFrame(FTEC.doxrframe);

			FTEC.xrframe = frame;
			try	//this try is needed to handle Host_EndGame properly.
			{
				FTEC.dovsync = {{{makeDynCall('if','FTEC.evcb.frame')}}}(timestamp);
			}
			catch(err)
			{
				console.log(err);
			}
			FTEC.xrframe = null;
		},

		handleevent : function(event)
		{
			switch(event.type)
			{
				case 'message':
					console.log(event);
					console.log(event.data);
					FTEC.loadurl(event.data.url, event.data.cmd, undefined);
					break;
				case 'resize':
					if (FTEC.evcb.resize != 0)
					{
						{{{makeDynCall('vii','FTEC.evcb.resize')}}}(Module['canvas'].width, Module['canvas'].height);
					}
					break;
				case 'mousemove':
					if (FTEC.evcb.mouse != 0)
					{
						if (Browser.pointerLock)
						{
							if (typeof event.movementX === 'undefined')
							{
								event.movementX = event.mozMovementX;
								event.movementY = event.mozMovementY;
							}
							if (typeof event.movementX === 'undefined')
							{
								event.movementX = event.webkitMovementX;
								event.movementY = event.webkitMovementY;
							}
							{{{makeDynCall('viiffff','FTEC.evcb.mouse')}}}(0, false, event.movementX, event.movementY, 0, 0);
						}
						else
						{
							var rect = Module['canvas'].getBoundingClientRect();
							{{{makeDynCall('viiffff','FTEC.evcb.mouse')}}}(0, true, (event.clientX - rect.left)*(Module['canvas'].width/rect.width), (event.clientY - rect.top)*(Module['canvas'].height/rect.height), 0, 0);
						}
					}
					break;
				case 'mousedown':
					window.focus();
					//Mozilla docs say do the pointerlock request first...
					//older browsers only allowed pointer lock when fullscreen. maybe it'll need two clicks. sucks to be you.
					if (FTEC.pointerwantlock != 0 && FTEC.pointerislocked == 0)
					{
						var v;
						try
						{
							FTEC.pointerislocked = -1;  //don't repeat the request on every click. firefox has a fit at that, so require the mouse to leave the element or something before we retry.
							v = Module['canvas'].requestPointerLock({unadjustedMovement: true});
							if (v !== undefined)
							{	//fuck sake, this is chrome being shitty.
								//this is all bullshit.
								//requestPointerLock spec does not return a promise. but chrome does it anyway, and returns its errors that way. and it eerrors a LOT, in system-specific ways, resulting in pointer locks failing entirely.
								v.catch((e)=>
								{
									if (e.name == "NotSupportedError")
									{
										Module['canvas'].requestPointerLock().then(()=>{
											console.log("Shitty browser forces mouse accel. Expect a shit experience.");
										}).catch(()=>{
											console.log("Your defective browser forces can't handle mouse look. Expect a truely dire experience. Give up now.");
										});
									}
									else
										console.log("Your defective browser forces can't handle mouse look. Expect a truely dire experience. Give up now.");
								});
							}
						}
						catch(e)
						{
							try {
								Module['canvas'].requestPointerLock();
								console.log("Your shitty browser doesn't support disabling mouse acceleration.");
							}
							catch(e)
							{
								console.log("Your shitty browser doesn't support mouse grabs.");
								FTEC.pointerislocked = -1;  //don't repeat the request on every click. firefox has a fit at that, so require the mouse to leave the element or something before we retry.
							}
						}
					}
					//older browsers need fullscreen in order for requestPointerLock to work. Which seems to be deprecated cos of how shitty an experience it is whenever you hit escape to load a menu or w/e
					//newer browsers can still break pointer locks when alt-tabbing, even without breaking fullscreen, so lets spam requests for it. enjoy.
					if (!document.fullscreenElement)
						if (FTEC.evcb.wantfullscreen != 0)
							if ({{{makeDynCall('i','FTEC.evcb.wantfullscreen')}}}())
							{
								try
								{
									Module['canvas'].requestFullscreen();
								}
								catch(e)
								{
									console.log("requestFullscreen:");
									console.log(e);
								}
							}
					//fallthrough
				case 'mouseup':
					if (FTEC.evcb.button != 0)
					{
						{{{makeDynCall('viii','FTEC.evcb.button')}}}(0, event.type=='mousedown', event.button);
						event.preventDefault();
					}
					break;
				case 'mousewheel':
				case 'wheel':
					if (FTEC.evcb.button != 0)
					{
						{{{makeDynCall('viii','FTEC.evcb.button')}}}(0, 2, event.deltaY);
						event.preventDefault();
					}
					break;
				case 'mouseout':
					if (FTEC.evcb.button != 0)
					{
						for (let i = 0; i < 8; i++)
							{{{makeDynCall('viii','FTEC.evcb.button')}}}(0, false, i);
					}
					if (FTEC.pointerislocked == -1)
						FTEC.pointerislocked = 0;
					break;
				case 'visibilitychange':
					try{
						if (!FTEC.wakelock && navigator.wakeLock && document.visibilityState === "visible")
							navigator.wakeLock.request("screen").then((value)=>{FTEC.wakelock = value;value.addEventListener("release", ()=>{FTEC.wakelock=null;});}).catch(()=>{});
					}catch(e){console.log(e);}
					break;
				case 'focus':
				case 'blur':
					{{{makeDynCall('iiiii','FTEC.evcb.key')}}}(0, false, 16, 0); //shift
					{{{makeDynCall('iiiii','FTEC.evcb.key')}}}(0, false, 17, 0); //alt
					{{{makeDynCall('iiiii','FTEC.evcb.key')}}}(0, false, 18, 0); //ctrl
					if (FTEC.pointerislocked == -1)
						FTEC.pointerislocked = 0;
					break;
				case 'keypress':
					if (FTEC.evcb.key != 0)
					{
						if (event.charCode >= 122 && event.charCode <= 123)	//no f11/f12
							break;
						{{{makeDynCall('iiiii','FTEC.evcb.key')}}}(0, 1, 0, event.charCode);
						{{{makeDynCall('iiiii','FTEC.evcb.key')}}}(0, 0, 0, event.charCode);
						event.preventDefault();
						event.stopPropagation();
					}
					break;
				case 'keydown':
				case 'keyup':
					//122 is 'toggle fullscreen'.
					//we don't steal that because its impossible to leave it again once used.
					if (FTEC.evcb.key != 0 && event.keyCode != 122)
					{
						const codepoint = event.key.codePointAt(1)?0:event.key.codePointAt(0); // only if its a single codepoint - none of this 'Return' nonsense.
						if (codepoint < ' ') codepoint = 0; //don't give a codepoint for c0 chars - like tab.
						if ({{{makeDynCall('iiiii','FTEC.evcb.key')}}}(0, event.type=='keydown', event.keyCode, codepoint))
							event.preventDefault();
					}
					break;
				case 'touchstart':
				case 'touchend':
				case 'touchcancel':
				case 'touchleave':
				case 'touchmove':
					event.preventDefault();
					const touches = event.changedTouches;
					for (let i = 0; i < touches.length; i++)
					{
						const t = touches[i];
						if (FTEC.evcb.mouse)
							{{{makeDynCall('viiffff','FTEC.evcb.mouse')}}}(t.identifier+1, true, t.pageX, t.pageY, 0, Math.sqrt(t.radiusX*t.radiusX+t.radiusY*t.radiusY));
						if (FTEC.evcb.button)
						{
							if (event.type == 'touchstart')
								{{{makeDynCall('viii','FTEC.evcb.button')}}}(t.identifier+1, 1, -1);
							else if (event.type != 'touchmove')	//cancel/end/leave...
								{{{makeDynCall('viii','FTEC.evcb.button')}}}(t.identifier+1, 0, -1);
						}
					}
					break;
				case 'dragenter':
				case 'dragover':
					event.stopPropagation();
					event.preventDefault();
					break;
				case 'drop':
					event.stopPropagation();
					event.preventDefault();
					let files = event.dataTransfer.files;
					for (let i = 0; i < files.length; i++)
					{
						const file = files[i];
						const reader = new FileReader();
						reader.onload = function(evt)
						{
							FTEC.loadurl(files[i].name, "", evt.target.result);
						};
						reader.readAsArrayBuffer(file);
					}
					break;
				case 'gamepadconnected':
					{
						const gp = event.gamepad;
						if (FTEH.gamepads === undefined)
							FTEH.gamepads = [];
						FTEH.gamepads[gp.index] = gp;
						console.log("Gamepad connected at index %d: %s. %d buttons, %d axes.", gp.index, gp.id, gp.buttons.length, gp.axes.length);
					}
					break;
				case 'gamepaddisconnected':
					{
						const gp = event.gamepad;
						delete FTEH.gamepads[gp.index];
						if (FTEC.evcb.jaxis)	//try and clear out the axis when released.
							for (let j = 0; j < 6; j+=1)
								{{{makeDynCall('viifi','FTEC.evcb.jaxis')}}}(gp.index, j, 0, true);
						if (FTEC.evcb.jbutton)	//try and clear out the axis when released.
							for (let j = 0; j < 32+4; j+=1)
								{{{makeDynCall('viiii','FTEC.evcb.jbutton')}}}(gp.index, j, 0, true);
						console.log("Gamepad disconnected from index %d: %s", gp.index, gp.id);
					}
					break;
				case 'pointerlockerror':
				case 'pointerlockchange':
				case 'mozpointerlockchange':
				case 'webkitpointerlockchange':
					FTEC.pointerislocked =	document.pointerLockElement === Module['canvas'] ||
											document.mozPointerLockElement === Module['canvas'] ||
											document.webkitPointerLockElement === Module['canvas'];
//					console.log("Pointer lock now " + FTEC.pointerislocked);
					break;
					
				case 'beforeunload':
					event.preventDefault();
					return 'quit this game like everything else?';
				default:
					console.log(event);
					break;
			}
		}
	},
	emscriptenfte_updatepointerlock : function(wantlock, softcursor)
	{
		FTEC.pointerwantlock = wantlock;
		//we can only apply locks when we're clicked, but should be able to unlock any time.
		if (wantlock == 0 && FTEC.pointerislocked != 0)
		{
			document.exitPointerLock =	document.exitPointerLock    ||
										document.mozExitPointerLock ||
										document.webkitExitPointerLock;
			FTEC.pointerislocked = 0;
			if (document.exitPointerLock)
				document.exitPointerLock();
		}
		if (softcursor)
			Module.canvas.style.cursor = "none";	//hide the cursor, we'll do a soft-cursor when one is needed.
		else
			Module.canvas.style.cursor = "default";	//restore the cursor
	},
	emscriptenfte_polljoyevents : function()
	{
		//with events, we can do unplug stuff properly.
		//otherwise hot unplug might be buggy.
		let gamepads;
//		if (FTEH.gamepads !== undefined)
//			gamepads = FTEH.gamepads;
//		else
		try
		{
			gamepads = navigator.getGamepads ? navigator.getGamepads() : [];
		}
		catch(e){}

		if (gamepads !== undefined)
		{
			for (let i = 0; i < gamepads.length; i+=1)
			{
				const gp = gamepads[i];
				if (gp === undefined)
					continue;
				if (gp == null)
					continue;	//xbox controllers tend to have 4 and exactly 4. on the plus side indexes won't change.
				for (let j = 0; j < gp.buttons.length; j+=1)
				{
					const b = gp.buttons[j];
					let p;
					if (typeof(b) == "object")
						p = b.pressed || (b.value > 0.5);	//.value is a fractional thing. oh well.
					else
						p = b > 0.5;	//old chrome bug

					if (b.lastframe != p)
					{	//cache it to avoid spam
						b.lastframe = p;
						{{{makeDynCall('viiii','FTEC.evcb.jbutton')}}}(gp.index, j, p, gp.mapping=="standard");
					}
				}
				for (let j = 0; j < gp.axes.length; j+=1)
					{{{makeDynCall('viifi','FTEC.evcb.jaxis')}}}(gp.index, j, gp.axes[j], gp.mapping=="standard");
			}
		}

		if (FTEC.xrsession != null && FTEC.xrframe != null && FTEC.referenceSpace != null)
		{
			//try and figure out the head angles according to where we're told the eyes are
			let count = 0;
			let org={x:0,y:0,z:0}, quat={x:0,y:0,z:0,w:0};
			const pose = FTEC.xrframe.getViewerPose(FTEC.referenceSpace);
			if (pose)
			{
				for (let view of pose.views)
				{
					org.x += view.transform.position.x;
					org.y += view.transform.position.y;
					org.z += view.transform.position.z;
					quat.x += view.transform.orientation.x;
					quat.y += view.transform.orientation.y;
					quat.z += view.transform.orientation.z;
					quat.w += view.transform.orientation.w;
					count++;
				}
				if (count)
				{
					org.x /= count;
					org.y /= count;
					org.z /= count;
					quat.x /= count;
					quat.y /= count;
					quat.z /= count;
					quat.w /= count;

//const idx=-3;
//console.log("jorientation dev:" + idx + " org:"+org.x+","+org.y+","+org.z+" quat:"+quat.x+","+quat.y+","+quat.z+","+quat.w);
					{{{makeDynCall('vifffffff','FTEC.evcb.jorientation')}}}(-3, org.x,org.y,org.z, quat.x,quat.y,quat.z,quat.w);
				}
			}

			for (const is of FTEC.xrsession.inputSources)
			{
				if (is === undefined || is == null)
					continue;

				//webxr doesn't really do indexes, so make em up from hands..
				let idx;
				if (is.handedness == "right")	//is.targetRayMode=="tracked-pointer"
					idx = -1;
				else if (is.handedness == "left")	//is.targetRayMode=="tracked-pointer"
					idx = -2;
				//else if (is.handedness == "head")	//is.targetRayMode=="???"... handled above.
				//	idx = -3;
				else if (is.handedness == "none" && (is.targetRayMode=="gaze" || is.targetRayMode=="screen"))
					idx = -4;
				else
					continue;	//wut?

				//tell the engine its orientation.
				const targetRayPose = FTEC.xrframe.getPose(is.targetRaySpace, FTEC.referenceSpace);
				if (targetRayPose)
				{
					const org = targetRayPose.transform.position;
					const quat = targetRayPose.transform.orientation;
//console.log("jorientation dev:" + idx + " org:"+org.x+","+org.y+","+org.z+" quat:"+quat.x+","+quat.y+","+quat.z+","+quat.w);
					{{{makeDynCall('vifffffff','FTEC.evcb.jorientation')}}}(idx, org.x,org.y,org.z, quat.x,quat.y,quat.z,quat.w);
				}

				//if it has a usable gamepad then use it.
				const gp = is.gamepad;
				if (gp == null)
					continue;
				if (gp.mapping != "xr-standard")
					continue;

				for (let j = 0; j < gp.buttons.length; j+=1)
				{
					const b = gp.buttons[j];
					let p;
					p = b.pressed;	//.value is a fractional thing. oh well.

					if (b.lastframe != p)
					{	//cache it to avoid spam
						b.lastframe = p;
//console.log("jbutton dev:" + idx + " btn:"+j+" dn:"+p+" mapping:"+gp.mapping);
						{{{makeDynCall('viiii','FTEC.evcb.jbutton')}}}(idx, j, p, gp.mapping=="standard");
					}
				}
				for (let j = 0; j < gp.axes.length; j+=1)
				{
//console.log("jaxis dev:" + idx + " axis:"+j+" val:"+gp.axes[j]+" mapping:"+gp.mapping);
					{{{makeDynCall('viifi','FTEC.evcb.jaxis')}}}(idx, j, gp.axes[j], gp.mapping=="standard");
				}
			}
		}
	},
	emscriptenfte_setupcanvas__deps: ['$FTEC', '$Browser', 'emscriptenfte_buf_createfromarraybuf'],
	emscriptenfte_setupcanvas : function(nw,nh,evresize,evmouse,evmbutton,evkey,evfile,evcbufadd,evjbutton,evjaxis,evjorientation,evwantfullscreen)
	{
		try
		{
		FTEC.evcb.resize = evresize;
		FTEC.evcb.mouse = evmouse;
		FTEC.evcb.button = evmbutton;
		FTEC.evcb.key = evkey;
		FTEC.evcb.loadfile = evfile;
		FTEC.evcb.cbufaddtext = evcbufadd;
		FTEC.evcb.jbutton = evjbutton;
		FTEC.evcb.jaxis = evjaxis;
		FTEC.evcb.jorientation = evjorientation;
		FTEC.evcb.wantfullscreen = evwantfullscreen;

		if ('GamepadEvent' in window)
			FTEH.gamepads = [];	//don't bother ever trying to poll if we can use gamepad events. this will hopefully avoid weirdness.

		if (!FTEC.donecb)
		{
			FTEC.donecb = 1;
			var events = ['mousedown', 'mouseup', 'mousemove', 'wheel', 'mousewheel', 'mouseout', 
						'keypress', 'keydown', 'keyup', 
						'touchstart', 'touchend', 'touchcancel', 'touchleave', 'touchmove',
						'dragenter', 'dragover', 'drop',
						'message', 'resize',
						'pointerlockerror', 'pointerlockchange', 'mozpointerlockchange', 'webkitpointerlockchange',
						'focus', 'blur'];   //try to fix alt-tab
			events.forEach(function(event)
			{
				Module['canvas'].addEventListener(event, FTEC.handleevent, {capture:true, passive:false});
			});

			var docevents = ['keypress', 'keydown', 'keyup',
							'pointerlockerror', 'pointerlockchange', 'mozpointerlockchange', 'webkitpointerlockchange', 'visibilitychange'];
			docevents.forEach(function(event)
			{
				document.addEventListener(event, FTEC.handleevent, true);
			});

			var windowevents = ['message','gamepadconnected', 'gamepaddisconnected', 'beforeunload', 'focus', 'blur'];
			windowevents.forEach(function(event)
			{
				window.addEventListener(event, FTEC.handleevent, true);
			});

//			Browser.resizeListeners.push(function(w, h) {
//				FTEC.handleevent({
//					type: 'resize',
//				});
//			});
		}
		var ctx = Browser.createContext(Module['canvas'], true, true, {});
		if (ctx == null)
		{
			var msg = "Unable to set up webgl context.\n\nPlease use a browser that supports it and has it enabled\nYour graphics drivers may also be blacklisted, so try updating those too. woo, might as well update your entire operating system while you're at it.\nIt'll be expensive, but hey, its YOUR money, not mine.\nYou can probably just disable the blacklist, but please don't moan at me when your computer blows up, seriously, make sure those drivers are not too buggy.\nI knew a guy once. True story. Boring, but true.\nYou're probably missing out on something right now. Don't you just hate it when that happens?\nMeh, its probably just tinkertoys, right?\n\nYou know, you could always try Internet Explorer, you never know, hell might have frozen over.\nDon't worry, I wasn't serious.\n\nTum te tum. Did you get it working yet?\nDude, fix it already.\n\nThis message was brought to you by Sleep Deprivation, sponsoring quake since I don't know when";
			if (FTEC.ctxwarned == 0)
			{
				FTEC.ctxwarned = 1;
				console.log(msg);
				alert(msg);
			}
			return 0;
		}
//		Browser.setCanvasSize(nw, nh, false);

		window.onresize = function()
		{
			let scale = window.devicePixelRatio;	//urgh. haxx.
			if (scale <= 0)
				scale = 1;
			//emscripten's browser library will revert sizes wrongly or something when we're fullscreen, so make sure that doesn't happen.
//			if (Browser.isFullScreen)
//			{
//				Browser.windowedWidth = window.innerWidth;
//				Browser.windowedHeight = window.innerHeight;
//			}
//			else
			{
				let rect = Module['canvas'].getBoundingClientRect();
				Browser.setCanvasSize(rect.width*scale, rect.height*scale, false);
			}
			if (FTEC.evcb.resize != 0)
				{{{makeDynCall('viif','FTEC.evcb.resize')}}}(Module['canvas'].width, Module['canvas'].height, scale);
		};
		window.onresize();

		if (FTEC.evcb.hashchange)
		{
			window.onhashchange = function()
			{
				FTEC.loadurl(location.hash.substring(1), "", undefined);
			};
		}

		//try to grab the mouse if we can
		try{
			_emscriptenfte_updatepointerlock(false, false);
		}catch(e){console.log(e);}

		//stop the screen from turning off.

		try{
			if (!FTEC.wakelock && navigator.wakeLock)
				navigator.wakeLock.request("screen").then((value)=>{FTEC.wakelock = value;value.addEventListener("release", ()=>{FTEC.wakelock=null;});}).catch(()=>{});
		}catch(e){console.log(e);}

		} catch(e)
		{
		console.log(e);
		}

		FTEC.xrsupport = 0;
		if (navigator.xr)
		{
			function rechecksupport()
			{
				navigator.xr.isSessionSupported("inline",		{}).then((works) => {FTEC.xrsupport = ((FTEC.xrsupport&~1)<<0)|(works<<0);});
				navigator.xr.isSessionSupported("immersive-vr",	{}).then((works) => {FTEC.xrsupport = ((FTEC.xrsupport&~1)<<1)|(works<<1);});
				navigator.xr.isSessionSupported("immersive-ar",	{}).then((works) => {FTEC.xrsupport = ((FTEC.xrsupport&~1)<<2)|(works<<2);});
			};

			//check it now
			rechecksupport();

			//and keep it up to date.
			navigator.xr.addEventListener('devicechange', (e)=>{rechecksupport();});
		}

		return 1;
	},
	emscriptenfte_xr_issupported : function()
	{
		if (navigator.xr)
			return FTEC.xrsupport;
		return 0;
	},
	emscriptenfte_xr_isactive : function()
	{
		var ret = 0;
		if (FTEC.xrsession!=null)
		{
			ret |= 1;

			const pose = FTEC.xrframe.getViewerPose(FTEC.referenceSpace);
			if (pose.views.length)
				ret |= 2;
		}
		return ret;
	},
	emscriptenfte_xr_setup : function(mode)
	{
		if (mode == -3)
			mode = ((FTEC.xrsession!=null)?-1:-2);
		if (mode == -2)
		{	//pick some suitable mode
			if (FTEC.xrsupport & (1<<2))
				mode = 2;	//ar
			else if (FTEC.xrsupport & (1<<1))
				mode = 1;	//vr
			else if (FTEC.xrsupport & (1<<0))
				mode = 0;	//inline
		}
		if (mode == -1)	//kill any current session.
			_emscriptenfte_xr_shutdown();
		else if (FTEC.xrsession == null && navigator.xr && mode >= 0 && mode < 3)
		{
			const modes = ["inline", "immersive-vr", "immersive-ar"];
			navigator.xr.requestSession(modes[mode], {optionalFeatures:["local"]}).then((session) => {
				FTEC.xrsession = session;

				session.addEventListener('end', ()=>{console.log("Session ended"); _emscriptenfte_xr_shutdown();});
				session.addEventListener('inputsourcechange', (e)=>{console.log("inputsourcechange", e);});
				session.addEventListener('select', (e)=>{console.log("select", e);});
				session.addEventListener('selectstart', (e)=>{console.log("selectstart", e);});
				session.addEventListener('selectend', (e)=>{console.log("selectend", e);});
				session.addEventListener('squeeze', (e)=>{console.log("squeeze", e);});
				session.addEventListener('squeezestart', (e)=>{console.log("squeezestart", e);});
				session.addEventListener('squeezeend', (e)=>{console.log("squeezeend", e);});

				/*Module['canvas'].addEventListener("webglcontextlost", (event) => {
					event.canceled = true;
					console.log("webglcontextlost");
				});
				Module['canvas'].addEventListener("webglcontextrestored", (event) => {
					console.log("webglcontextrestored");
				});*/

				var madecompatible = function()
				{
					session.updateRenderState({baseLayer: new XRWebGLLayer(session, Module.ctx), depthFar:8192, depthNear:1});

					session.requestReferenceSpace("local").then((refspace) => {
						FTEC.referenceSpace = refspace;
						FTEC.xrsession.requestAnimationFrame(FTEC.doxrframe);
					}).catch((e)=>{	//fall back to viewer if that failed.
						session.requestReferenceSpace("viewer").then((refspace) => {
							FTEC.referenceSpace = refspace;
							FTEC.xrsession.requestAnimationFrame(FTEC.doxrframe);
						}).catch((e)=>{console.error("requestReferenceSpace",e); _emscriptenfte_xr_shutdown();});	//and just in case...
					});
				};

				if (mode == 0)
					madecompatible();	//chrome88+ throws a hissy fit if we makeXRCompatible on an inline context.
				else
					Module.ctx.makeXRCompatible().then(madecompatible).catch((e)=>{console.error("makeXRCompatible", e);_emscriptenfte_xr_shutdown();});
			}).catch((e)=>{console.error("requestSession", e);_emscriptenfte_xr_shutdown();});
			return true;
        }
        return false;   //not really success, more that it can't possibly work.
	},
	emscriptenfte_xr_geteyeinfo : function(maxeyes, eyeptr)
	{
		if (!FTEC.xrframe || !FTEC.xrsession)
			return 0;	//nope.

		const pose = FTEC.xrframe.getViewerPose(FTEC.referenceSpace);
		if (!pose)
			return 0;	//nope.
		const layer = FTEC.xrsession.renderState.baseLayer;
		var e = 0;

		if (!FTEC.xrfbo)	//make the fbo handle available to the C code
			FTEC.xrfbo = GL.framebuffers.length;	//alloc a new one.
		GL.framebuffers[FTEC.xrfbo] = layer.framebuffer;	//update it so the engine can actually use it...

		eyeptr>>=2;
		for (const view of pose.views)
		{
			if (e == maxeyes)
				break;
			const viewport = layer.getViewport(view);

			HEAP32[eyeptr+0] = FTEC.xrfbo;
			eyeptr+=1;

			HEAP32[eyeptr+0] = viewport.x;
			HEAP32[eyeptr+1] = viewport.y;
			HEAP32[eyeptr+2] = viewport.width;
			HEAP32[eyeptr+3] = viewport.height;
			eyeptr+=4;

			HEAPF32[eyeptr+ 0] = view.projectionMatrix[0];
			HEAPF32[eyeptr+ 1] = view.projectionMatrix[1];
			HEAPF32[eyeptr+ 2] = view.projectionMatrix[2];
			HEAPF32[eyeptr+ 3] = view.projectionMatrix[3];
			HEAPF32[eyeptr+ 4] = view.projectionMatrix[4];
			HEAPF32[eyeptr+ 5] = view.projectionMatrix[5];
			HEAPF32[eyeptr+ 6] = view.projectionMatrix[6];
			HEAPF32[eyeptr+ 7] = view.projectionMatrix[7];
			HEAPF32[eyeptr+ 8] = view.projectionMatrix[8];
			HEAPF32[eyeptr+ 9] = view.projectionMatrix[9];
			HEAPF32[eyeptr+10] = view.projectionMatrix[10];
			HEAPF32[eyeptr+11] = view.projectionMatrix[11];
			HEAPF32[eyeptr+12] = view.projectionMatrix[12];
			HEAPF32[eyeptr+13] = view.projectionMatrix[13];
			HEAPF32[eyeptr+14] = view.projectionMatrix[14];
			HEAPF32[eyeptr+15] = view.projectionMatrix[15];
			eyeptr+=16;

			HEAPF32[eyeptr+ 0] = view.transform.matrix[0];
			HEAPF32[eyeptr+ 1] = view.transform.matrix[1];
			HEAPF32[eyeptr+ 2] = view.transform.matrix[2];
			HEAPF32[eyeptr+ 3] = view.transform.matrix[3];
			HEAPF32[eyeptr+ 4] = view.transform.matrix[4];
			HEAPF32[eyeptr+ 5] = view.transform.matrix[5];
			HEAPF32[eyeptr+ 6] = view.transform.matrix[6];
			HEAPF32[eyeptr+ 7] = view.transform.matrix[7];
			HEAPF32[eyeptr+ 8] = view.transform.matrix[8];
			HEAPF32[eyeptr+ 9] = view.transform.matrix[9];
			HEAPF32[eyeptr+10] = view.transform.matrix[10];
			HEAPF32[eyeptr+11] = view.transform.matrix[11];
			HEAPF32[eyeptr+12] = view.transform.matrix[12];
			HEAPF32[eyeptr+13] = view.transform.matrix[13];
			HEAPF32[eyeptr+14] = view.transform.matrix[14];
			HEAPF32[eyeptr+15] = view.transform.matrix[15];
			eyeptr+=16;

			e++;
		}
		return e;
	},
	emscriptenfte_xr_shutdown : function()
	{
		if (FTEC.xrfbo)
		{	//destroy that handle
			GL.framebuffers[FTEC.xrfbo] = null;
			FTEC.xrfbo = 0;
		}

		if (FTEC.xrsession)
		{
			FTEC.xrsession.end();
			FTEC.xrsession = null;
		}
		FTEC.xrframe = null;
	},
	emscriptenfte_settitle : function(txt)
	{
		document.title = UTF8ToString(txt);
	},
	emscriptenfte_abortmainloop : function(fname, fatal)
	{
		fname = UTF8ToString(fname);
		if (fatal)
			FTEC.aborted = true;
		if (Module['stackTrace'])
			throw 'oh noes! something bad happened in ' + fname + '!\n' + Module['stackTrace']();
		throw 'oh noes! something bad happened!\n';
	},

	emscriptenfte_setupmainloop__deps: ['$FTEC'],
	emscriptenfte_setupmainloop : function(fnc)
	{
		Module['noExitRuntime'] = true;
		FTEC.aborted = fnc==0;

		Module["sched"] = FTEC.step;	//this is stupid.

		FTEC.evcb.frame = fnc;
		if (fnc)
		{
			//don't start it instantly, so we can distinguish between types of errors (emscripten sucks!).
			setTimeout(FTEC.step, 1, performance.now());
		}
		else if (Module["close"])
			Module["close"]();
		else if (Module["quiturl"])
			document.location.replace(Module["quiturl"]);
		else
		{
			try
			{
				if (history.length == 1)
					window.close();
				else
					history.back();	//can't close. go back to the previous page though.
			}
			catch(e)
			{
			}
		}
		//else kill it?
	},

	emscriptenfte_ticks_ms : function()
	{	//milliseconds...
		return Date.now();
	},
	emscriptenfte_uptime_ms : function()
	{	//milliseconds...
		return performance.now();
	},

	emscriptenfte_openfile : function()
	{
		if (FTEC.evcb.loadfile != 0)
		{
			window.showOpenFilePicker(
				{	types:[
						{
							description: "Packages",
							accept:{"text/*":[".pk3", ".pak", ".pk4", ".zip"]}
						},
						{
							description: "Maps",
							accept:{"text/*":[".bsp.gz", ".bsp", ".map", ".hmp"]}
						},
						{
							description: "Demos",
							accept:{"application/*":[".mvd.gz", ".qwd.gz", ".dem.gz", ".mvd", ".qwd", ".dem"]}	//dm2?
						},
						{
							description: "QuakeTV Info",
							accept:{"application/*":[".qtv"]}
						},
						{
							description: "FTE Manifest",
							accept:{"text/*":[".fmf"]}
						},
						//model formats?... nah, too many/weird. they can always
						//audio formats?	eww
						//image formats?	double eww!
						{
							description: "Configs",
							accept:{"text/*":[".cfg", ".rc"]}
						}],
					excludeAcceptAllOption:false,	//let em pick anything. we actually support more than listed here (and bitrot...)
					id:"openfile",	//remember the dir we were in for the next invocation
					multiple:true	//does this make sense? not for a demo but does for *.pak or *.bsp
				}).then((r)=>
				{
					for (let i of r)
					{
						i.getFile().then((f)=>
						{
							f.arrayBuffer().then((arraybuf)=>
							{
								if (FTEC.evcb.loadfile != 0)
								{
									const handle = _emscriptenfte_buf_createfromarraybuf(arraybuf);
									let blen = lengthBytesUTF8(i.name)+1;
									let urlptr = _malloc(blen);
									stringToUTF8(f.name, urlptr, blen);
									blen = lengthBytesUTF8(f.type)+1;
									let mimeptr = _malloc(blen);
									stringToUTF8(f.type, mimeptr,blen);
									{{{makeDynCall('viii','FTEC.evcb.loadfile')}}}(urlptr, mimeptr, handle);
									_free(mimeptr);
									_free(urlptr);
									window.focus();
								}
							});
						}).catch((e)=>{console.error("getFile() failed:");console.error(e);});
					}
				}).catch((e)=>{console.log("showOpenFilePicker() aborted", e);});
		}
	},

	emscriptenfte_buf_create__deps : ['emscriptenfte_handle_alloc'],
	emscriptenfte_buf_create : function()
	{
		var b = {h:-1, r:1, l:0,m:4096,d:new Uint8Array(4096), n:null};
		b.h = _emscriptenfte_handle_alloc(b);
		return b.h;
	},
	//filesystem emulation
	emscriptenfte_buf_open__deps : ['emscriptenfte_buf_create'],
	emscriptenfte_buf_open : function(name, createifneeded)
	{
		name = UTF8ToString(name);
		var f = FTEH.f[name];
		var r = -1;
		if (f == null)
		{
			if (!FTEC.localstorefailure)
			{
				try
				{
					if (localStorage && createifneeded != 2)
					{
						var str = localStorage.getItem(name);
						if (str != null)
						{
		//					console.log('read file '+name+': ' + str);

							var len = str.length;
							var buf = new Uint8Array(len);
							for (var i = 0; i < len; i++)
								buf[i] = str.charCodeAt(i);

							var b = {h:-1, r:2, l:len,m:len,d:buf, n:name};
							r = b.h = _emscriptenfte_handle_alloc(b);
							FTEH.f[name] = b;
							return b.h;
						}
					}
				}
				catch(e)
				{
					console.log('exception while trying to read local storage for ' + name);
					console.log(e);
					console.log('disabling further attempts to access local storage');
					FTEC.localstorefailure = true;
				}
			}

			if (createifneeded)
				r = _emscriptenfte_buf_create();
			if (r != -1)
			{
				f = FTEH.h[r];
				f.r+=1;
				f.n = name;
				FTEH.f[name] = f;
				if (FTEH.f[name] != f || f.n != name)
					console.log('error creating file '+name);
			}
		}
		else
		{
			f.r+=1;
			r = f.h;
		}
		if (f != null && createifneeded == 2)
			f.l = 0;  //truncate it.
		return r;
	},
	emscriptenfte_buf_rename : function(oldname, newname)
	{
		oldname = UTF8ToString(oldname);
		newname = UTF8ToString(newname);
		var f = FTEH.f[oldname];
		if (f == null)
			return 0;
		if (FTEH.f[newname] != null)
			return 0;
		FTEH.f[newname] = f;
		delete FTEH.f[oldname];
		f.n = newname;

		if (Module['cache'])
		{
			Module['cache'].match("/_/"+oldname).then((oldresp)=>{
				if (oldresp === undefined)
					Module['cache'].delete("/_/"+newname);	//'overwrite' the new name.
				else
				{
					Module['cache'].put("/_/"+newname, oldresp);
					Module['cache'].delete("/_/"+oldname);
				}
			}).catch(()=>{
				Module['cache'].delete("/_/"+oldname);
				Module['cache'].delete("/_/"+newname);	//if we're overwriting with a file we don't have then just wipe the old one.
			});
		}
		return 1;
	},
	emscriptenfte_buf_delete : function(name)
	{
		name = UTF8ToString(name);
		var f = FTEH.f[name];
		if (f)
		{
			delete FTEH.f[name];
			f.n = null;
			_emscriptenfte_buf_release(f.h);

			if (Module['cache'])
				Module['cache'].delete("/_/"+name);
			return 1;
		}
		return 0;
	},
	emscritenfte_buf_enumerate : function(cb, ctx, sz)
	{
		var n = Object.keys(FTEH.f);
		var c = n.length, i;
		for (i = 0; i < c; i++)
		{
			stringToUTF8(n[i], ctx, sz);
			{{{makeDynCall('vii','cb')}}}(ctx, FTEH.f[n[i]].l);
		}
	},
	emscriptenfte_buf_pushtolocalstore : function(handle)
	{
		var b = FTEH.h[handle];
		if (b == null)
		{
			Module.printError('emscriptenfte_buf_pushtolocalstore with invalid handle');
			return;
		}
		if (b.n == null)
			return;
		var data = b.d;
		var len = b.l;
		try
		{
			if (b.n.endsWith(".pak") || b.n.endsWith(".pk3") || b.n.endsWith(".kpf") || b.n.indexOf("/dlcache/")>0)
			{
				if (Module['cache'])
				{
					console.log("Saving "+b.n+" to cache ("+len+" bytes).");
					Module['cache'].put("/_/"+b.n, new Response(data, {"headers":{"Content-Type":"application/octet-stream", "Content-Length":len}}));
				}
				else
					console.log("cache not available");
			}
			else if (localStorage)
			{
				var foo = "";
				//use a divide and conquer implementation instead for speed?
				for (var i = 0; i < len; i++)
					foo += String.fromCharCode(data[i]);
				localStorage.setItem(b.n, foo);
			}
			else
				console.log('local storage not supported');
		}
		catch (e)
		{
			console.log('exception while trying to save ' + b.n, e);
		}
	},
	emscriptenfte_buf_release : function(handle)
	{
		var b = FTEH.h[handle];
		if (b == null)
		{
			Module.printError('emscriptenfte_buf_release with invalid handle');
			return;
		}
		b.r -= 1;
		if (b.r == 0)
		{
			if (b.n != null)
				delete FTEH.f[b.n];
			delete FTEH.h[handle];
			b.d = null;
		}
	},
	emscriptenfte_buf_getsize : function(handle)
	{
		var b = FTEH.h[handle];
		return b.l;
	},
	emscriptenfte_buf_read : function(handle, offset, data, len)
	{
		var b = FTEH.h[handle];
		if (offset+len > b.l)	//clamp the read
			len = b.l - offset;
		if (len < 0)
		{
			len = 0;
			if (offset+len >= b.l)
				return -1;
		}
		HEAPU8.set(b.d.subarray(offset, offset+len), data);
		return len;
	},
	emscriptenfte_buf_write : function(handle, offset, data, len)
	{
		var b = FTEH.h[handle];
		if (len < 0)
			len = 0;
		if (offset+len > b.m)
		{	//extend it if needed.
			b.m = offset + len + 4095;
			b.m = b.m & ~4095;
			var nd = new Uint8Array(b.m);
			nd.set(b.d, 0);
			b.d = nd;
		}
		b.d.set(HEAPU8.subarray(data, data+len), offset);
		if (offset + len > b.l)
			b.l = offset + len;
		return len;
	},

	emscriptenfte_ws_connect__deps: ['emscriptenfte_handle_alloc'],
	emscriptenfte_ws_connect : function(brokerurl, protocolname)
	{
		var _url = UTF8ToString(brokerurl);
		var _protocol = UTF8ToString(protocolname);
		var s = {ws:null, inq:[], err:0, con:0};
		try {
			s.ws = new WebSocket(_url, _protocol);
		} catch(err) { console.log(err); }
		if (s.ws === undefined)
			return -1;
		if (s.ws == null)
			return -1;
		s.ws.binaryType = 'arraybuffer';
		s.ws.onerror = function(event) {s.con = 0; s.err = 1;};
		s.ws.onclose = function(event) {s.con = 0; s.err = 1;};
		s.ws.onopen = function(event) {s.con = 1;};
		s.ws.onmessage = function(event)
			{
				assert(typeof event.data !== 'string' && event.data.byteLength, 'websocket data is not usable');
				s.inq.push(new Uint8Array(event.data));
			};

		return _emscriptenfte_handle_alloc(s);
	},
	emscriptenfte_ws_close : function(sockid)
	{
		var s = FTEH.h[sockid];
		if (s === undefined)
			return -1;

		s.callcb = null;

		if (s.ws != null)
		{
			s.ws.close();
			s.ws = null;	//make sure to avoid circular references
		}

		if (s.pc != null)
		{
			s.pc.close();
			s.pc = null;	//make sure to avoid circular references
		}

		if (s.broker != null)
		{
			s.broker.close();
			s.broker = null;	//make sure to avoid circular references
		}
		delete FTEH.h[sockid];	//socked is no longer accessible.
		return 0;
	},
	//separate call allows for more sane flood control when fragmentation is involved.
	emscriptenfte_ws_cansend : function(sockid, extra, maxpending)
	{
		var s = FTEH.h[sockid];
		if (s === undefined)
			return 1;	//go on punk, make my day.
		return ((s.ws.bufferedAmount+extra) < maxpending);
	},
	emscriptenfte_ws_send : function(sockid, data, len)
	{
		var s = FTEH.h[sockid];
		if (s === undefined)
			return -1;
		if (s.con == 0)
			return 0; //not connected yet
		if (s.err != 0)
			return -1;
		if (len == 0)
			return 0; //...
		s.ws.send(HEAPU8.subarray(data, data+len));
		return len;
	},
	emscriptenfte_ws_recv : function(sockid, data, len)
	{
		var s = FTEH.h[sockid];
		if (s === undefined)
			return -1;
		var inp = s.inq.shift();
		if (inp)
		{
			if (inp.length > len)
				inp.length = len;
			HEAPU8.set(inp, data);
			return inp.length;
		}
		if (s.err)
			return -1;
		return 0;
	},

	emscriptenfte_rtc_create__deps: ['emscriptenfte_handle_alloc'],
	emscriptenfte_rtc_create : function(clientside, ctxp, ctxi, callback, pcconfig)
	{
		try {
			pcconfig = JSON.parse(UTF8ToString(pcconfig));
		} catch(err) {pcconfig = {};}

		var dcconfig = {ordered: false, maxRetransmits: 0, reliable:false};

		var s = {pc:null, ws:null, inq:[], err:0, con:0, isclient:clientside, callcb:
			function(evtype,stringdata)
			{	//private helper

//console.log("emscriptenfte_rtc_create callback: " + evtype);

				var stringlen = (stringdata.length*3)+1;
				var dataptr = _malloc(stringlen);
				stringToUTF8(stringdata, dataptr, stringlen);
				{{{makeDynCall('viiii','callback')}}}(ctxp,ctxi,evtype,dataptr);
				_free(dataptr);
			}
		};

		if (RTCPeerConnection === undefined)
		{	//IE or something.
			console.log("RTCPeerConnection undefined");
			return -1;
		}

		s.pc = new RTCPeerConnection(pcconfig);
		if (s.pc === undefined)
		{
			console.log("webrtc failed to create RTCPeerConnection");
			return -1;
		}

//create the dataconnection
		s.ws = s.pc.createDataChannel('quake', dcconfig);
		s.ws.binaryType = 'arraybuffer';
		s.ws.onclose = function(event)
			{
				s.con = 0;
				s.err = 1;
			};
		s.ws.onopen = function(event)
			{
				s.con = 1;
			};
		s.ws.onmessage = function(event)
			{
				assert(typeof event.data !== 'string' && event.data.byteLength);
				s.inq.push(new Uint8Array(event.data));
			};

		s.pc.onicecandidate = function(e)
			{
				var desc;
				if (1)
					desc = JSON.stringify(e.candidate);
				else
					desc = e.candidate.candidate;
				if (desc == null)
					return;	//no more...
				s.callcb(4, desc);
			};
		s.pc.ondatachannel = function(e)
			{
				s.recvchan = e.channel;
				s.recvchan.binaryType = 'arraybuffer';
				s.recvchan.onmessage = s.ws.onmessage;
			};
		s.pc.onconnectionstatechange = function(e)
			{
//console.log(s.pc.connectionState);
//console.log(e);
				switch (s.pc.connectionState)
				{
				//case "new":
				//case "checking":
				//case "connected":
				case "disconnected":
					s.err = 1;
					break;
				case "closed":
					s.con = 0;
					s.err = 1;
					break;
				case "failed":
					s.err = 1;
					break;
				default:
					break;
				}
			};

		if (clientside)
		{
			s.pc.createOffer().then(
				function(desc)
				{
					s.pc.setLocalDescription(desc);

					if (1)
						desc = JSON.stringify(desc);
					else
						desc = desc.sdp;

					s.callcb(3, desc);
				},
				function(event)
				{
					s.err = 1;
				}
			);
		}

		return _emscriptenfte_handle_alloc(s);
	},
	emscriptenfte_rtc_offer : function(sockid, offer, offertype)
	{
		var desc;
		var s = FTEH.h[sockid];
		offer = UTF8ToString(offer);
		offertype = UTF8ToString(offertype);
		if (s === undefined)
			return -1;

		try
		{
			try
			{
				desc = JSON.parse(offer);
			}
			catch(e)
			{
				desc = {sdp:offer, type:offertype};
			}

			s.pc.setRemoteDescription(desc).then(() =>
					{
						if (!s.isclient)
						{	//server must give a response.
							s.pc.createAnswer().then(
								function(desc)
								{
									s.pc.setLocalDescription(desc);

									if (1)
										desc = JSON.stringify(desc);
									else
										desc = desc.sdp;

									s.callcb(3, desc);
								},
								function(event)
								{
									s.err = 1;
								}
							);
						}
					}, err =>
					{
						console.log(desc);
						console.log(err);
					});
		} catch(err) { console.log(err); }

	},
	emscriptenfte_rtc_candidate : function(sockid, offer)
	{
		var s = FTEH.h[sockid];
		offer = UTF8ToString(offer);
		if (s === undefined)
			return -1;

		try	//don't screw up if the peer is trying to screw with us.
		{
			var desc;
			try
			{
				desc = JSON.parse(offer);
			}
			catch(e)
			{
				desc = {candidate:offer, sdpMid:null, sdpMLineIndex:0};
			}
			s.pc.addIceCandidate(desc);
		} catch(err) { console.log(err); }
		return 0;
	},

	emscriptenfte_async_wget_data2 : function(url, postdata, postlen, postmimetype, ctx, onload, onerror, onprogress)
	{
		var _url = UTF8ToString(url);
		var http = new XMLHttpRequest();
		try
		{
			if (postdata)
			{
				http.open('POST', _url, true);

				if (postmimetype)
					http.setRequestHeader("Content-Type", UTF8ToString(postmimetype));
				else
					http.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
			}
			else
			{
				http.open('GET', _url, true);
			}
		}
		catch(e)
		{
			if (onerror)
				{{{makeDynCall('vii','onerror')}}}(ctx, 404);
			return;
		}
		http.responseType = 'arraybuffer';

		http.onload = function(e)
		{
			if (http.status == 200)
			{
				if (onload)
					{{{makeDynCall('vii','onload')}}}(ctx, _emscriptenfte_buf_createfromarraybuf(http.response));
			}
			else
			{
				if (onerror)
					{{{makeDynCall('vii','onerror')}}}(ctx, http.status);
			}
		};

		http.onerror = function(e)
		{
			//Note: Unfortunately it is not possible to distinguish between dns, network, certificate, or CORS errors (other than viewing the browser's log).
			//      This is apparently intentional to prevent sites probing lans - cors will make them all seem dead and thus uninteresting targets.
			if (onerror)
				{{{makeDynCall('vii','onerror')}}}(ctx, 0);
		};

		http.onprogress = function(e)
		{
			if (onprogress)
				{{{makeDynCall('viii','onprogress')}}}(ctx, e.loaded, e.total);
		};

		try	//ffs
		{
			if (postdata)
			{
				http.send(HEAPU8.subarray(postdata, postdata+postlen));
			}
			else
			{
				http.send(null);
			}
		}
		catch(e)
		{
			console.log(e);
			http.onerror(e);
		}
	},

	emscriptenfte_al_loadaudiofile : function(buf, dataptr, datasize)
	{
		var ctx = AL;
		//match emscripten's openal support.
		if (!buf)
			return;

		var albuf = AL.buffers[buf];
		AL.buffers[buf] = null; //alIsBuffer will report it as invalid now

		try
		{
			//its async, so it needs its own copy of an arraybuffer, not just a view.
			var abuf = new ArrayBuffer(datasize);
			var rbuf = new Uint8Array(abuf);
			rbuf.set(HEAPU8.subarray(dataptr, dataptr+datasize));
			AL.currentCtx.audioCtx.decodeAudioData(abuf,
					function(buffer)
					{
						//Warning: This depends upon emscripten's specific implementation of alBufferData
						albuf.bytesPerSample = 2;
						albuf.channels = 1;
						albuf.length = buffer.length;
						albuf.frequency = buffer.sampleRate;
						albuf.audioBuf = buffer;

						ctx.buffers[buf] = albuf;	//and its valid again!
					},
					function()
					{
						console.log("Audio Callback failed!");
						ctx.buffers[buf] = albuf;
					}
				);
		}
		catch (e)
		{
			console.log("unable to decode audio data");
			console.log(e);
			ctx.buffers[buf] = albuf;
		}
	},
	emscriptenfte_pcm_loadaudiofile : function(ctx, callback, dataptr, datasize, snd_speed)
	{
		const successcb = function(buffer)
		{
			const frames = buffer.length;
			const chans = buffer.numberOfChannels;
			const rate = buffer.sampleRate;
			const outptr = _malloc(2*frames*chans);
			if (!outptr)
				return; //it went away?
			const dst = HEAP16.subarray(outptr>>1, (outptr+2*frames*chans)>>1);

			/*if (buffer.numberOfChannels == 1)
				dst.set(buffer.getChannelData(0));
			else*/ for (let c = 0; c < buffer.numberOfChannels; c++)
			{
				const src = buffer.getChannelData(c);
				for (let f = 0; f < frames; f++)
					dst[f*chans+c] = 16384*src[f];	//docs imply it should be 32767 but I'm getting unhealthy clipping
			}
			{{{makeDynCall('viiif','callback')}}}(ctx, outptr, frames, chans, rate);
			_free(outptr);
		};
		const failurecb = function(buffer)
		{
			{{{makeDynCall('viiif','callback')}}}(ctx, 0, 0, 0, 0);
		};

		try{

			const abuf = new ArrayBuffer(datasize);
			const rbuf = new Uint8Array(abuf);
			rbuf.set(HEAPU8.subarray(dataptr, dataptr+datasize));
			const ac = new AudioContext({sampleRate:snd_speed});
			ac.decodeAudioData(abuf, successcb, failurecb);	//do the decode
		
			return true;
		}
		catch(e)
		{
			console.log("emscriptenfte_pcm_loadaudiofile failure :(");
		}
		return false;
	},

	emscriptenfte_gl_loadtexturefile : function(texid, widthptr, heightptr, dataptr, datasize, fname, dopremul, genmips)
	{
		function encode64(data) {
			var BASE = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
			var PAD = '=';
			var ret = '';
			var leftchar = 0;
			var leftbits = 0;
			for (var i = 0; i < data.length; i++) {
				leftchar = (leftchar << 8) | data[i];
				leftbits += 8;
				while (leftbits >= 6) {
					var curr = (leftchar >> (leftbits-6)) & 0x3f;
					leftbits -= 6;
					ret += BASE[curr];
				}
			}
			if (leftbits == 2) {
				ret += BASE[(leftchar&3) << 4];
				ret += PAD + PAD;
			} else if (leftbits == 4) {
				ret += BASE[(leftchar&0xf) << 2];
				ret += PAD;
			}
			return ret;
		}

		//make sure the texture is defined before its loaded, so we get no errors
		GLctx.texImage2D(GLctx.TEXTURE_2D, 0, GLctx.RGBA, 1,1,0,GLctx.RGBA, GLctx.UNSIGNED_BYTE, null);

		var img = new Image();
		var gltex = GL.textures[texid];
		img.name = UTF8ToString(fname);
		img.onload = function()
		{
			if (img.width < 1 || img.height < 1)
			{
				console.log("emscriptenfte_gl_loadtexturefile("+img.name+"): bad image size\n");
				return;
			}
			var oldtex = GLctx.getParameter(GLctx.TEXTURE_BINDING_2D);	//blurgh, try to avoid breaking anything in this unexpected event.
			GLctx.bindTexture(GLctx.TEXTURE_2D, gltex);
			if (dopremul)
				GLctx.pixelStorei(GLctx.UNPACK_PREMULTIPLY_ALPHA_WEBGL, true);
			GLctx.texImage2D(GLctx.TEXTURE_2D, 0, GLctx.RGBA, GLctx.RGBA, GLctx.UNSIGNED_BYTE, img);
			if (dopremul)
				GLctx.pixelStorei(GLctx.UNPACK_PREMULTIPLY_ALPHA_WEBGL, false);
			if (genmips)
				GLctx.generateMipmap(GLctx.TEXTURE_2D);
			GLctx.bindTexture(GLctx.TEXTURE_2D, oldtex);
		};
		img.crossorigin = true;
		img.src = "data:image/png;base64," + encode64(HEAPU8.subarray(dataptr, dataptr+datasize));	//png... jpeg... browsers don't seem to actually care
	},

	Sys_Clipboard_PasteText: function(cbt, callback, ctx)
	{
		if (cbt != 0)
			return;	//don't do selections.

		let docallback = function(text)
		{
			FTEC.clipboard = text;
			try{
				let stringlen = (text.length*3)+1;
				let dataptr = _malloc(stringlen);
				stringToUTF8(text, dataptr, stringlen);
				{{{makeDynCall('vii','callback')}}}(ctx, dataptr);
				_free(dataptr);
			}catch(e){
			}
		};

		//try pasting. if it fails then use our internal string.
		try
		{
			navigator.clipboard.readText()
				.then(docallback)
				.catch((e)=>{docallback(FTEC.clipboard)});
		}
		catch(e)
		{	//clipboard API not supported at all.
			console.log(e);	//happens in firefox. lets print it so we know WHY its failing.
			docallback(FTEC.clipboard);
		}
	},
	Sys_SaveClipboard: function(cbt, text)
	{
		if (cbt != 0)
			return;	//don't do selections.

		FTEC.clipboard = UTF8ToString(text);

		try
		{
			//try and copy it to the system clipboard too.
			navigator.clipboard.writeText(FTEC.clipboard);
		}
		catch {}
	}
});

