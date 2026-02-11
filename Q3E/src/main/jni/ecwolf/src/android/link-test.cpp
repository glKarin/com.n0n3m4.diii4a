// This is here so that we can link to libecwolf.so and determine if there will
// be any linker errors prior to running.

int main(int argc, char* argv[])
{
	extern int WL_Main(int, char*[]);
	return WL_Main(argc, argv);
}
