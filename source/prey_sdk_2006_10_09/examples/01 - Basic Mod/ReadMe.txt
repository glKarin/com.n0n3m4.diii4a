This example demonstrates a simple mod which adds a console command to print some text.

Fire up the codebase and open up SysCmds.cpp. Around line 2730, you will see some text that looks like this:

	cmdSystem->AddCommand( "centerview",			Cmd_CenterView_f,			CMD_FL_GAME,				"centers the view" );
	cmdSystem->AddCommand( "god",					Cmd_God_f,					CMD_FL_GAME|CMD_FL_CHEAT,	"enables god mode" );
	cmdSystem->AddCommand( "notarget",				Cmd_Notarget_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"disables the player as a target" );

Let's turn that into:

	cmdSystem->AddCommand( "centerview",			Cmd_CenterView_f,			CMD_FL_GAME,				"centers the view" );
	//mymod begin
	cmdSystem->AddCommand( "myCommand",				Cmd_MyCommand_f,			CMD_FL_GAME,				"my brand new command" );
	//mymod end
	cmdSystem->AddCommand( "god",					Cmd_God_f,					CMD_FL_GAME|CMD_FL_CHEAT,	"enables god mode" );
	cmdSystem->AddCommand( "notarget",				Cmd_Notarget_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"disables the player as a target" );

Now let's go to line 458, above the void Cmd_God_f( const idCmdArgs &args ) function, and let's add:

//mymod begin
void Cmd_MyCommand_f( const idCmdArgs &args ) {
	gameLocal.Printf("I am a banana!\n");
}
//mymod end

Now compile, and you should end up with a ./releasedll/gamex86.dll file. Using a program like WinRAR or WinZip,
take that file and put it in a zip file. Now you will want to create a file called "binary.conf". The contents
of that file should look like this:

// add flags for supported operating systems in this pak
// one id per line
// name the file binary.conf and place it in the game pak
// 0 win-x86
// 1 mac-ppc
// 2 linux-x86
0

Add that binary.conf file into the zip file with the gamex86.dll. You should now have a zip file with those two
files in it. Rename the zip file to game00.pk4, and head over to your Prey folder. Under your Prey folder you will
have a base folder. Likewise, you want to create your mod folder next to base (not in base!), so let's make a folder
called mymod, and put the game00.pk4 we just made in there. Now you should have a file called something like:
C:\Program Files\Prey\mymod\game00.pk4
So, in the mymod folder, create a file called description.txt. In that file, just put something like:
My Mod!
And save it. Now start Prey up. In the lower-right of the main menu you'll see the "Mods" option. Click it, and
you should see your "My Mod!" mod on the list. Click it and then click "Load MOD". Now bring down the console
(ctrl+alt+~), and type myCommand and hit enter. If all is well, your message will be displayed.

For an example of the final package, check out the mymod folder (in the same folder as this text file).


Rich Whitehouse
( http://www.telefragged.com/thefatal/ )
