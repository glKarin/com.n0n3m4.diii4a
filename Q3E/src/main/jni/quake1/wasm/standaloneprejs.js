//current command in ascii decimal
let currentcmd = [0,0,0] 
let currentfile = "";
const sleep = ms => new Promise(r => setTimeout(r, ms));

Module['print'] = function(text) { console.log(text); }

Module['preRun'] = function()
{
	function stdin() { return 10 };
	var stdout = null;
	var stderr = null;
	FS.init(stdin, stdout, stderr);
	FS.mount(IDBFS, {}, "/home/web_user/");
	FS.symlink("/home/web_user", "/save");
}

Module['noInitialRun'] = true

document.addEventListener('click', (ev) => {
	console.log("event is captured only once.");
	args = []
	if(window.location.href.indexOf("file://") > -1)
	{
		try
		{
			args = args.concat(prompt("Enter command line arguments").split(" "))
		}
		catch (error)
		{
			console.log("Error: ", error);
			console.log("Failed to concat extra arguments (likely passed nothing for the argument)")
		}
	}
	else
	{
		parms = new URLSearchParams(window.location.search);
		try
		{
			args = args.concat(parms.get("args").split(" "))
		}
		catch (error)
		{
			console.log("Error: ", error);
			console.log("Failed to concat extra arguments (likely passed nothing for the argument)")
		}
	}

	FS.syncfs(true, function()
	{
		if(FS.analyzePath("/preload/runhere").exists)
		{
			FS.symlink("/preload", "/home/web_user/games");
			args = args.concat(["-basedir", "/home/web_user/games"])
		}
		else
		{
			args = args.concat(["-basedir", "/home/web_user/"])
		}

		Module.callMain(args);
	});
}, { once: true });
