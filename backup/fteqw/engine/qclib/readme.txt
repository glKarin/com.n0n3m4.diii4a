Readme for the FTE QCLib

This library is a library for running QuakeC gamecode. It does not provide any builtins itself.

Features:
	* Multiple library instances, enabling server qc, client qc, and menu qc. There is no maximum instance limit other than memory.

	* Addons, for running multiple progs in any individual instance.

	* Field reassignment, allowing a single engine to support multiple subtly different QC APIs. Also makes additional fields easier.

	* Step-by-step debugging. Requires a text editor of some form, however. A printout of the current line is also useful of course.

	* 64bit support. All strings, globals, and fields are allocated in a consecutive addressable section of memory. This also allows pointers and secure access (not implemented yet, but should be relativly easy bar builtins, which are your responsability).

	* Multiple 'threads'. The library allows a builtin to make a duplicate of the current execution state, or to wipe the current state. This allows sleep commands and fork commands. How handy.

	* Integrated QC compiler. FTEQCC comes as part of qclib. By setting up an interface with a specific value, you can cause it to always run, or run only if it detects a source change.

	* Support for different sorts of progs. Namly Hexen2's, kkqwsv's bigprogs, and FTE's extended format with extra opcodes and possibly fully 32bit offsets. The use of kkqwsv's progs is not recommended - this might be removed at some point.




Quirks:
	* don't use multiple instances of fteqcc at the same time. Compilation will fail.
	* 64bit support requires all strings to be allocated by qclib itself, achivable via a method call. Compatability requires a certain ammount of caution.
	* a fair number of methods are obsolete.
	* An overuse of pointers in the API. There are some macros which you can use to hide some of the dereferences.
	* kkqwsv progs are not reliable. Do not try saving the game. Avoid letting your users know of support.
	
	* Builtin structures are different from original quake. You'll need to convert the arguments to qclib style. This change was required for both multiple instances as well as addon support. It should be straightforward enough.
	* Entity fields are accessed via a pointer from the edict_t structure. This was required to place entity fields within the 64bit accessable section. Changing a . to a -> is not a major issue though. However, there are a lot. do a find and replace of ->v. to ->v->
	* FTE's entities are numbers not pointers. This fact is not made into a big feature as it's kinda incompatable with standard quake. Please do not use numbers directly to refer to ents but instead use the EDICT_TO_PROGS macro which will give protection. This is consistant with standard quake.


Basic usage:
	* refer to test.c for a sample on how to set up the library.
	* refer to progslib.h for the things that I've forgotten to mention.
	* Call the InitProgs function to get a handle to the instance. It takes a parameter which should be set up with some fields. You'll require ReadFile, FileSize, Abort and printf for basic execution.
	* Call the configure function to say how much memory to use, and how many progs/addons to support.
	* Load your progs via LoadProgs. Use a crc of 0 to use any. Otherwise progs will be rejected if it doesn't match. Give it a list of progs-specific builtins too. :)
	* Before calling the spawn builtin, call the InitEnts method. It's parameter stating how many maximum entities to spawn. Using a really large quantity is not much of an issue, as they are allocated as required.
	* Before calling InitEnts, you can tell the VM which fields your engine uses (state all basic ones or none). This will place the entity fields in the same order as your engine expects for entvars_t.
	* Obtain pointers to globals, or just use the globals structure directly.
	* Call the ExecuteProgram method to start execution.
	* Call the FindFunction method to find a function to run in the first place.
	* Call the 'globals' method to retrieve a pointer to the globals (you should always use PR_CURRENT here). Set the parameters with the G_INT/G_FLOAT macros and friends. Use OFS_PARM0 - OFS_PARM7 to set params before calling or read inside a builtin. Use OFS_RETURN to read the return value. These macros are hard coded to use a 'pr_globals' symbol, so avoid renaming builtin parameter names.
	* Ask me on IRC when it all starts keeling over.
	* These are the C files that form qclib: pr_edict.c pr_exec.c pr_multi.c initlib.c qcc_pr_comp.c qcc_pr_lex.c qccmain.c qcc_cmdlib.c comprout.c hash.c qcd_main.c qcdecomp.c
