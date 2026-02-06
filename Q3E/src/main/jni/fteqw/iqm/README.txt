FTE's fork of Lee Salzman's commandline IQM exporter, based upon 'IQM Developer Kit 2015-08-03'.
there is no blender integration - export your files to a supported format first.

Main changes:
now utilises command files instead of needing the weird commandline stuff (although that should still mostly work).
additional mesh properties and multiple mesh files, providing for proper hitmesh support, as well as some other things.
more verbose text output, additional shader prefixes.
bone renaming+regrouping
animation unpacking, for qc mods that still insist on animating the lame way.

Supported import formats:
.md5mesh
.md5anim
.iqe
.smd
.fbx
.obj

Unless you're doing complex stuff like any of the above, there's probably not all that much difference. There may be some commandline behaviour differences.


Command File Format:
	output <FILENAME> - specifies the output file name. you should only have one of each type of output.
	output_qmdl <FILENAME> - specifies which file to write a quake1-format model. May only occur once.
	output_md16 <FILENAME> - specifies the filename to write a quakeforge 16-bit md16 model to (a upgraded variation of quake's format). May only occur once.
	output_md3 <FILENAME> - specifies the filename to write a quake3 md3 file to. May only occur once.
	exec <FILENAME> - exec the specified command file, before parsing the rest of the current file.
	hitbox <BODY NUM> <BONE NAME> <MIN POS VECTOR> <MAX POS VECTOR> - generates a hitmesh as a bbox centered around the bone in the base pose (the hitbox will rotate/move with animations). The bodynum will be visible to gamecode, and may merge with other hitboxes with the same group.
	modelflags <NAME OR HEX> - enables the specified bit in the iqm header. supported names include q1_rocket, q1_grenade, q1_gib, q1_rotate, q1_tracer1, q1_zomgib, q1_tracer2, q1_tracer3
	<MESH PROPERTY> - defined below and applied as the defaults to the following import lines as well as mesh lines.
	mesh <NAME> [MESH PROPERTIES LIST] - provides overrides for a single named mesh (properties used will be those as they're already defined, if not otherwise listed).
	bone <SOURCENAME> [rename <NEWNAME>] [group <GROUPNUM>] - provides bone renaming and grouping. try to avoid renaming two bones to the same resulting name... groups may have limitations if a parent/child relationship cannot be honoured. lowest group numbers come before higher groups. by default bones will inherit their group from their parent.
	<IMPORT PROPERTY> - defined below and applied as the defaults to the following import lines.
	import <FILENAME> [IMPORT PROPERTIES] [MESH PROPERTIES] - imports the meshes and animations from the specified file.

Mesh Properties:
	contents <NAMES OR 0xBITS> - 'body' or 'empty' are the two that are most likely to be used. 'solid' may also be desired, or possibly also 'corpse'.
	surfaceflags <NAMES OR 0xBITS> - 'q3_nodraw/fte_nodraw'
	body <NUMBER> - this is the 'body' value reported to gamecode when a trace hits this surface.
	geomset <GEOMGROUP> <GEOMID> - by configuring this, you can have models display different body parts for different character models.
	lodrange <MINDIST> <MAXDIST> - not yet implemented by the engine. 0 for both is the default.

Anim/Import Properties:
	name <NAME> - the imported animations will be assigned this name. May be problematic if the imported file(s) share the same name, so try to avoid using this at global scope.
	fps <RATE> - framerate for any imported animations.
	loop - flags animations as looping.
	clamp - disables looping.
	unpack - seperates each pose of the animations into a seperate single-pose animation for compat with q1 or q2-style model animations.
	pack - disables unpacking again.
	nomesh <1|0> - discards all meshed from the affected files.
	noanim <1|0> - discards animations from the affected files, does not disclude the base pose.
	materialprefix <PREFIX/> - provides a text prefix on the material name, which should make it easier for editors while still honouring shader paths.
	start <FIRSTPOSE> - the fist pose to import.
	end <LASTPOSE> - the last pose to import.
	rotate <PITCH> <YAW> <ROLL> - rotates the model
	scale <SCALER> - rescales the model
	origin <X> <Y> <Z> - moves the thing
	event [ANIM,]<POSE[.FRAC]> <EVENTCODE> <"EVENTDATA"> - embeds event info within the animation, for stuff like footsteps. How this is used depends on the engine... If used at global scope, can be reset with 'event reset' in order to not apply to later files.
	
