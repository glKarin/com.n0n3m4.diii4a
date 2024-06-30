// Those select the specific syn123 functions, without
// this, the native wrapper is used, hopefully matching off_t.
//#define _FILE_OFFSET_BITS 64
//#define _FILE_OFFSET_BITS 32
#include <syn123.h>

#include <stdio.h>

const long arate = 8000;
const long brate = 48000;
const off_t ins = 12637;
const off_t outs = 75822;

int main()
{
	off_t outs2 = syn123_resample_total(arate,brate,ins);
	off_t ins2  = syn123_resample_intotal(arate,brate,outs);
	int err = 0;
	if(outs2 != outs && ++err)
		fprintf(stderr, "total mismatch: %ld != %ld\n"
		,	(long)outs2, (long)outs );
	if(ins2 != ins && ++err)
		fprintf(stderr, "intotal mismatch: %ld != %ld\n"
		,	(long)ins2, (long)ins );
	printf("%s\n", err ? "FAIL" : "PASS");
	return err;
}
