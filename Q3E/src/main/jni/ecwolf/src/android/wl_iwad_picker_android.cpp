#include <jni.h>

#include "wl_iwad.h"

extern "C"
{
	extern JNIEnv* env_;
}

int I_PickIWad_Android (WadStuff *wads, int numwads, bool showwin, int defaultiwad)
{
	jobjectArray jwads = env_->NewObjectArray(numwads, env_->FindClass("java/lang/String"), NULL);
	for(int i = 0;i < numwads;++i)
	{
		FString entry;
		entry.Format("%s (%s)", wads[i].Name.GetChars(), wads[i].Extension.GetChars());

		env_->SetObjectArrayElement(jwads, i, env_->NewStringUTF(entry));
	}

	jclass NativeLibClass = env_->FindClass("com/beloko/idtech/wolf3d/NativeLib");
	jmethodID pickIWadMethod = env_->GetStaticMethodID(NativeLibClass, "pickIWad", "([Ljava/lang/String;I)I");

	jint tmp = defaultiwad;
	return env_->CallStaticIntMethod(NativeLibClass, pickIWadMethod, jwads, tmp);
}
