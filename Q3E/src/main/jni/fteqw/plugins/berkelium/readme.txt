simple media decoding plugin that decodes web pages instead of videos...
See installation instructions at the end of this file.


This means you can do: playfilm berkelium:http://google.com
And you'll get google.com displayed as if it were a cinematic or so.
The engine will pass mouse+keyboard events to such cinematics so you can interact with it.


You can also use the 'videomap' term within a shader, if you want to put some youtube video on a wall or something. Here's an example of such a shader:
muh_bad
{
	{
		map $lightmap
	}
	{
		videomap berkelium:http://google.com
		blendfunc filter
	}
}


CSQC is able to interact with videos by:
1: find the name of the shader
	string texname = getsurfacetexture(trace_ent, getsurfacenearpoint(trace_ent, trace_endpos));
2: send a key event
	gecko_keyevent(texname, keycode, keydown);
3: move the mouse cursor
	gecko_mousemove(texname, x, y);
	note that the x and y values should be between 0 and 1. Surface resolution is not relevent here.
4: resize the image
	gecko_resize(texname, 1024, 1024);
	if you're using it in 2d, make sure it matches the pixel width of the screen where it'll be displayed. it'll get fuzzy otherwise.
	Size matters. Beware of sites that do not resize images for different resolutions (ie: most of them).
5: you can query the image with:
	vector v = gecko_get_texture_extent(texname);
6: you can purge resources with:
	gecko_destroy(texname);
	this will 'end' the video. when the shader is next displayed it'll reset from the original url.
7: you can change the url with:
	gecko_navigate(texname, "http://fteqw.com");
8: you can send it these navigation commands (in the place of a url). You'll likely want a focus command.
	cmd:refresh
	cmd:transparent
	cmd:focus
	cmd:unfocus
	cmd:opaque
	cmd:stop
	cmd:back
	cmd:forward
	cmd:cut
	cmd:copy
	cmd:paste
	cmd:del
	cmd:selectall


Compiling the plugin for Windows:
In the berkelium 7z file that you should have already downloaded, there'll be an includes and a lib directory. Stick the contents of both of those in the empty doubled berkelium directory on the fte svn.
The msvc project file should now be usable. You will likely want to change the linker target path to get picked up by the engine automatically.

Windows Installation instructions:
Download Berkelium from here: http://berkelium.org/ (http://github.com/sirikata/berkelium/downloads)
You should get a 7z file that contains a bin directory. Copy the contents of that bin directory to your quake directory.
The FTE-specific plugin 'berkeliumx86.dll' should be placed as 'quake/fte/plugins/berkeliumx86.dll'.
