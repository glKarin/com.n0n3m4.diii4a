video encoder/decoder using the ffmpeg avformat/avcodec libraries.

The video decoder plugs into the media decoder functionality on media with an 'av:' or 'avs:' prefix, specifically:
The console command 'playfilm av:foo.mpg' will start playing back $BASEDIR/$GAMEDIR/foo.mpg fullscreen (or from inside paks/pk3s, but make sure seeking is fast, so avoid compression in pk3s...).
The console command 'playfilm avs:c:\foo.mpg' will start playing back c:\foo.mpg fullscreen.
The shader term 'videomap avs:c:\foo.mpg' will play the video upon the shader. This can be used with csqc+drawpic, csqc+beginpolygon, or placed upon walls.
It theoretically supports any file that the avformat/avcodec libraries support, but has no ability to pass arguments, thus playback is likely limited only to files which require no explicit overrides.

The video encoder plugs into the existing capture command. Or something. I don't know. Its all basically junk.
The container type is guessed by the file name used.
avplug_format says which container format to use. If empty, will be guessed based upon file extension. See ffmpeg docs for more info.
avplug_videocodec says which video codec to use. If empty, will be guessed based upon the container type ('libx264' for h264 compression). See ffmpeg docs for a full list.
avplug_videobitrate says what bitrate to encode at. Default 400000.
At the time of writing, audio is not implemented.

To check if the plugin is loaded, use the plug_list command.

For streaming, you can try this:
avplug_format mpegts
avplug_videocodec mpeg4
capture udp://PLAYERIP:1234
You can then run VLC or some such app and tell it to listen on port 1234.
You might be able to find some other protocol/codec that works better for you. Consult the ffmpeg documentation for that, if you can find something that's actually readable.
