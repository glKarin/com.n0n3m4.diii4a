#!/bin/bash

touch config.cfg
echo 'unbindall'                                     >> config.cfg
echo 'bind A_BUTTON "+use"'                          >> config.cfg
echo 'bind B_BUTTON "+jump"'                         >> config.cfg
echo 'bind X_BUTTON "+reload"'                       >> config.cfg
echo 'bind Y_BUTTON "+duck"'                         >> config.cfg
echo 'bind L1_BUTTON "+attack2"'                     >> config.cfg
echo 'bind R1_BUTTON "+attack"'                      >> config.cfg
echo 'bind START "escape"'                           >> config.cfg
echo 'bind DPAD_UP "lastinv"'                        >> config.cfg
echo 'bind DPAD_DOWN "impulse 100"'                  >> config.cfg
echo 'bind DPAD_LEFT "invprev"'                      >> config.cfg
echo 'bind DPAD_RIGHT "invnext"'                     >> config.cfg
echo 'bind BACK "chooseteam"'                        >> config.cfg
echo 'gl_vsync "1"'                                  >> config.cfg
echo 'sv_autosave "0"'                               >> config.cfg
echo 'touch_config_file "touch_presets/psvita.cfg"'  >> config.cfg

touch kb_def.lst
echo '"A_BUTTON"		"+use"'                      >> kb_def.lst
echo '"B_BUTTON"		"+jump"'                     >> kb_def.lst
echo '"X_BUTTON"		"+reload"'                   >> kb_def.lst
echo '"Y_BUTTON"		"+duck"'                     >> kb_def.lst
echo '"L1_BUTTON"		"+attack2"'                  >> kb_def.lst
echo '"R1_BUTTON"		"+attack"'                   >> kb_def.lst
echo '"START"			"escape"'                    >> kb_def.lst
echo '"DPAD_UP"			"lastinv"'                   >> kb_def.lst
echo '"DPAD_DOWN"		"impulse 100"'               >> kb_def.lst
echo '"DPAD_LEFT"		"invprev"'                   >> kb_def.lst
echo '"DPAD_RIGHT"		"invnext"'                   >> kb_def.lst
echo '"BACK"			"chooseteam"'                >> kb_def.lst

touch video.cfg
echo 'fullscreen "1"' >> video.cfg
echo 'width "960"'    >> video.cfg
echo 'height "544"'   >> video.cfg
echo 'r_refdll "gl"'  >> video.cfg

touch opengl.cfg
echo 'gl_nosort "1"'  >> opengl.cfg
