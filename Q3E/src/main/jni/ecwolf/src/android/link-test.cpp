// This is here so that we can link to libecwolf.so and determine if there will
// be any linker errors prior to running.

int main(int argc, char* argv[])
{
	extern int main_android(int, char*[]);
	return main_android(argc, argv);
}
