all: linux32 linux64
linux32:
	gcc -fvisibility=hidden -shared -fPIC -m32 -lstdc++ -Llib32 -llibberkelium plugapi.cpp ../plugin.c ../qvm_api.c -oberkeliumx86.so -I.

linux64:
	gcc -fvisibility=hidden -shared -fPIC -m64 -lstdc++ -Llib64 -llibberkelium plugapi.cpp ../plugin.c ../qvm_api.c -oberkeliumamd.so -I.
