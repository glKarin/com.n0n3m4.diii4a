#include <cstring>
#include <SHA.h>

//SHA256 TEST VALUES:
//1)One-Block Message "abc"
//"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
//
//2)"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
//"248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"
//
//3)Multi-Block Message "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"
//"CF5B16A778AF8380036CE59E7B0492370B249B11E8F07A51AFAC45037AFEE9D1"
//
//4)Long Message "a" 1,000,000 times
//"cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"


int main( int argc, char **argv )
{
	char result[32];

	auto sha = CSHA();
	sha.AddData( "abc", 3 );
	sha.FinalDigest( result );

	if( memcmp( result, "\xba\x78\x16\xbf\x8f\x01\xcf\xea\x41\x41\x40\xde\x5d\xae\x22\x23\xb0\x03\x61\xa3\x96\x17\x7a\x9c\xb4\x10\xff\x61\xf2\x00\x15\xad", 32 ))
		return 1;

	sha.Reset();
	sha.AddData( "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56 );
	sha.FinalDigest( result );

	if( memcmp( result, "\x24\x8d\x6a\x61\xd2\x06\x38\xb8\xe5\xc0\x26\x93\x0c\x3e\x60\x39\xa3\x3c\xe4\x59\x64\xff\x21\x67\xf6\xec\xed\xd4\x19\xdb\x06\xc1", 32 ))
		return 2;

	sha.Reset();
	sha.AddData( "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu", 112 );
	sha.FinalDigest( result );

	if( memcmp( result, "\xcf\x5b\x16\xa7\x78\xaf\x83\x80\x03\x6c\xe5\x9e\x7b\x04\x92\x37\x0b\x24\x9b\x11\xe8\xf0\x7a\x51\xaf\xac\x45\x03\x7a\xfe\xe9\xd1", 32 ))
		return 3;

	sha.Reset();
	for( int i = 0; i < 1000000; i++ )
		sha.AddData( "a", 1 );
	sha.FinalDigest( result );

	if( memcmp( result, "\xcd\xc7\x6e\x5c\x99\x14\xfb\x92\x81\xa1\xc7\xe2\x84\xd7\x3e\x67\xf1\x80\x9a\x48\xa4\x97\x20\x0e\x04\x6d\x39\xcc\xc7\x11\x2c\xd0", 32 ))
		return 4;


	return 0;
}
