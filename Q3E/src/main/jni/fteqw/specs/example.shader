//this file describes fte's extensions to the q3 shader syntax
//it can be used directly as a shader file.


//lets put some glsl on the console (Set gl_console to 'console')
//note that the _glsl postfix is only used if the user has a gfx card that supports glsl.
console_glsl
{
	nopicmip
	nomipmaps

	program shaders/generic.vp shaders/console.fp
	param texture 0 baset	//tell it which samplers it can use
	param time time	//tell it the time
	//param cvarf gl_specularexponant specexponant //example of feeding in the value of a cvar
	//param upper topcol //could be used on entities to read the (player's) shirt colour into a topcol uniform
	//param lower botcol //could be used on entities to read the (player's) lower colour into a botcol uniform
	//param eyepos vieworg //put the render origin into uniform named vieworg
	//param colors colourmod //reads the ent's colourmod and puts it in the colourmod uniform

	{
		map "conback"
		//videomap "video/intro.roq"	//use this line instead in order to feed a roq/avi into the glsl/shader
	}
}


