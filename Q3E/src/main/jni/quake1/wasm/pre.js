if (!Object.hasOwn(Module, 'arguments')) {
	Module['arguments'] = ['-basedir', '/game'];
}
else {
	Module['arguments'] = ['-basedir', '/game'].concat(Module['arguments']);
}

Module['print'] = function(text) {
	console.log(text);
}

Module['printErr'] = function(text) {
	console.error(text);
}


Module['preRun'] = [
	function()
	{
		function stdin(){
			return '\n';  // Return a newline/line feed character so the user is not prompted for input
		};
		FS.init(stdin, null, null); // null for both stdout and stderr

		function createParentDirectory(filePath) {
			//
			// Split a filePath into parts, create the directory hierarchy
			//
			parts = filePath.split('/');
			for (let i = parts.length - 1; i > 0; i--) {
				localDir = '/game/' + parts.slice(0, -i).join('/')
				try {
					FS.mkdir(localDir);
				}
				catch {
					// Directory already exists
				}
			}
		}

		function startDownload(localPath, remotePath) {
			//
			// Return a promise of a file download
			//
			Module['addRunDependency'](localPath);  // Tell Emscripten about the dependency

			return fetch(remotePath)
				.then(response => {
						return response.arrayBuffer();
				})
				.then(arrayBuffer => {
					const buffer = new Uint8Array(arrayBuffer);
					stream = FS.open("/game/" + localPath, "w");
					FS.write(stream, buffer, 0, buffer.byteLength);
					FS.close(stream);
					console.log("Downloaded " + localPath);
					Module['removeRunDependency'](localPath);  // Tells Emscripten we've finished the download
				});
		}

		function createBaseDir() {
			//
			// Creates the Quake basedir and mounts it to IDBFS
			//
			FS.mkdir('/game');
			//mounts IDBFS to where the game would save
			FS.mount(IDBFS, {}, '/home/web_user/');
		}

		function downloadGameFiles() {
			//
			// Download files specified in the Module.files object
			//
			createBaseDir();

			let downloads = [];
			for (const [localPath, remotePath] of Object.entries(Module.files)) {
				console.log("Downloading " + remotePath + " to " + localPath);

				createParentDirectory(localPath);

				downloads.push(
					startDownload(localPath, remotePath)
				);
			}

			// Wait for downloads to finish, sync the filesystem, start the game
			Promise.all(downloads)
				.then(function(results) {
					FS.syncfs(true, function (err) {  
						assert(!err);
						Module.callMain(Module.arguments);
					});
				});
		}

		downloadGameFiles();
	}
];

Module['noInitialRun'] = true;
