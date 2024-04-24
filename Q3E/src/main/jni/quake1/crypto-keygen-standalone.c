#define _GNU_SOURCE

#include <d0_blind_id/d0_blind_id.h>

#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

// BEGIN stuff shared with crypto.c
#define FOURCC_D0PK (('d' << 0) | ('0' << 8) | ('p' << 16) | ('k' << 24))
#define FOURCC_D0SK (('d' << 0) | ('0' << 8) | ('s' << 16) | ('k' << 24))
#define FOURCC_D0PI (('d' << 0) | ('0' << 8) | ('p' << 16) | ('i' << 24))
#define FOURCC_D0SI (('d' << 0) | ('0' << 8) | ('s' << 16) | ('i' << 24))
#define FOURCC_D0IQ (('d' << 0) | ('0' << 8) | ('i' << 16) | ('q' << 24))
#define FOURCC_D0IR (('d' << 0) | ('0' << 8) | ('i' << 16) | ('r' << 24))
#define FOURCC_D0ER (('d' << 0) | ('0' << 8) | ('e' << 16) | ('r' << 24))
#define FOURCC_D0IC (('d' << 0) | ('0' << 8) | ('i' << 16) | ('c' << 24))

static unsigned long Crypto_LittleLong(const char *data)
{
	return
		((unsigned char) data[0]) |
		(((unsigned char) data[1]) << 8) |
		(((unsigned char) data[2]) << 16) |
		(((unsigned char) data[3]) << 24);
}

static void Crypto_UnLittleLong(char *data, unsigned long l)
{
	data[0] = l & 0xFF;
	data[1] = (l >> 8) & 0xFF;
	data[2] = (l >> 16) & 0xFF;
	data[3] = (l >> 24) & 0xFF;
}

static size_t Crypto_ParsePack(const char *buf, size_t len, unsigned long header, const char **lumps, size_t *lumpsize, size_t nlumps)
{
	size_t i;
	size_t pos;
	pos = 0;
	if(header)
	{
		if(len < 4)
			return 0;
		if(Crypto_LittleLong(buf) != header)
			return 0;
		pos += 4;
	}
	for(i = 0; i < nlumps; ++i)
	{
		if(pos + 4 > len)
			return 0;
		lumpsize[i] = Crypto_LittleLong(&buf[pos]);
		pos += 4;
		if(pos + lumpsize[i] > len)
			return 0;
		lumps[i] = &buf[pos];
		pos += lumpsize[i];
	}
	return pos;
}

static size_t Crypto_UnParsePack(char *buf, size_t len, unsigned long header, const char *const *lumps, const size_t *lumpsize, size_t nlumps)
{
	size_t i;
	size_t pos;
	pos = 0;
	if(header)
	{
		if(len < 4)
			return 0;
		Crypto_UnLittleLong(buf, header);
		pos += 4;
	}
	for(i = 0; i < nlumps; ++i)
	{
		if(pos + 4 + lumpsize[i] > len)
			return 0;
		Crypto_UnLittleLong(&buf[pos], lumpsize[i]);
		pos += 4;
		memcpy(&buf[pos], lumps[i], lumpsize[i]);
		pos += lumpsize[i];
	}
	return pos;
}

void file2buf(const char *fn, char **data, size_t *datasize)
{
	FILE *f;
	*data = NULL;
	*datasize = 0;
	size_t n = 0, dn = 0;
	if(!strncmp(fn, "/dev/fd/", 8))
		f = fdopen(atoi(fn + 8), "rb");
	else
		f = fopen(fn, "rb");
	if(!f)
	{
		return;
	}
	for(;;)
	{
		*data = realloc(*data, *datasize += 8192);
		if(!*data)
		{
			*datasize = 0;
			fclose(f);
			return;
		}
		dn = fread(*data + n, 1, *datasize - n, f);
		if(!dn)
			break;
		n += dn;
	}
	fclose(f);
	*datasize = n;
}

int buf2file(const char *fn, const char *data, size_t n)
{
	FILE *f;
	if(!strncmp(fn, "/dev/fd/", 8))
		f = fdopen(atoi(fn + 8), "wb");
	else
		f = fopen(fn, "wb");
	if(!f)
		return 0;
	n = fwrite(data, n, 1, f);
	if(fclose(f) || !n)
		return 0;
	return 1;
}

void file2lumps(const char *fn, unsigned long header, const char **lumps, size_t *lumpsize, size_t nlumps)
{
	char *buf;
	size_t n;
	file2buf(fn, &buf, &n);
	if(!buf)
	{
		fprintf(stderr, "could not open %s\n", fn);
		exit(1);
	}
	if(!Crypto_ParsePack(buf, n, header, lumps, lumpsize, nlumps))
	{
		fprintf(stderr, "could not parse %s as %c%c%c%c (%d lumps expected)\n", fn, (int) header & 0xFF, (int) (header >> 8) & 0xFF, (int) (header >> 16) & 0xFF, (int) (header >> 24) & 0xFF, (int) nlumps);
		free(buf);
		exit(1);
	}
	free(buf);
}

mode_t umask_save;
void lumps2file(const char *fn, unsigned long header, const char *const *lumps, size_t *lumpsize, size_t nlumps, D0_BOOL private)
{
	char buf[65536];
	size_t n;
	if(private)
		umask(umask_save | 0077);
	else
		umask(umask_save);
	if(!(n = Crypto_UnParsePack(buf, sizeof(buf), header, lumps, lumpsize, nlumps)))
	{
		fprintf(stderr, "could not unparse for %s\n", fn);
		exit(1);
	}
	if(!buf2file(fn, buf, n))
	{
		fprintf(stderr, "could not write %s\n", fn);
		exit(1);
	}
}

void USAGE(const char *me)
{
	printf("Usage:\n"
			"%s [-F] [-b bits] [-n progress-denominator] [-x prefix] [-X infix] [-C] -o private.d0sk\n"
			"%s -P private.d0sk -o public.d0pk\n"
			"%s [-n progress-denominator] [-x prefix] [-X infix] [-C] -p public.d0pk -o idkey-unsigned.d0si\n"
			"%s -p public.d0pk -I idkey-unsigned.d0si -o request.d0iq -O camouflage.d0ic\n"
			"%s -P private.d0sk -j request.d0iq -o response.d0ir\n"
			"%s -p public.d0pk -I idkey-unsigned.d0si -c camouflage.d0ic -J response.d0ir -o idkey.d0si\n"
			"%s -P private.d0sk -I idkey-unsigned.d0si -o idkey.d0si\n"
			"%s -I idkey.d0si -o id.d0pi\n"
			"%s -p public.d0pk\n"
			"%s -P private.d0sk\n"
			"%s -p public.d0pk -i id.d0pi\n"
			"%s -p public.d0pk -I idkey.d0si\n"
			"%s -0 -p public.d0pk -I idkey.d0si\n"
			"%s -0 -p public.d0pk\n"
			"%s -p public.d0pk -I idkey.d0si -d file-to-sign.dat -o file-signed.dat\n"
			"%s -p public.d0pk -s file-signed.dat -o file-content.dat [-O id.d0pi]\n"
			"%s -p public.d0pk -I idkey.d0si -d file-to-sign.dat -O signature.dat\n"
			"%s -p public.d0pk -d file-to-sign.dat -s signature.dat [-O id.d0pi]\n",
			me, me, me, me, me, me, me, me, me, me, me, me, me, me, me, me, me, me
		   );
}

unsigned int seconds;
unsigned int generated;
unsigned int ntasks = 1;
double generated_offset;
double guesscount;
double guessfactor;
void print_generated(int signo)
{
	(void) signo;
	++seconds;
	if(generated >= 1000000000)
	{
		generated_offset += generated;
		generated = 0;
	}
	fprintf(stderr, "Generated: %.0f (about %.0f, %.1f/s, about %.2f hours for %.0f)\n",
		// nasty and dishonest hack:
		// we are adjusting the values "back", so the total count is
		// divided by guessfactor (as the check function is called
		// guessfactor as often as it would be if no fastreject were
		// done)
		// so the values indicate the relative speed of fastreject vs
		// normal!
		(generated + generated_offset) / guessfactor,
		(generated + generated_offset) * ntasks / guessfactor,
		(generated + generated_offset) * ntasks / (guessfactor * seconds),
		guesscount * ((guessfactor * seconds) / (generated + generated_offset) / ntasks) / 3600.0,
		guesscount);
	alarm(1);
}

#define CHECK(x) if(!(x)) { fprintf(stderr, "error exit: error returned by %s\n", #x); exit(2); }

const char *prefix = NULL, *infix = NULL;
size_t prefixlen = 0;
int ignorecase;
typedef D0_BOOL (*fingerprint_func) (const d0_blind_id_t *ctx, char *outbuf, size_t *outbuflen);
D0_BOOL fastreject(const d0_blind_id_t *ctx, void *pass)
{
	static char fp64[513]; size_t fp64size = 512;
	CHECK(((fingerprint_func) pass)(ctx, fp64, &fp64size));
	++generated;
	if(ignorecase)
	{
		if(prefixlen)
			if(strncasecmp(fp64, prefix, prefixlen))
				return 1;
		if(infix)
		{
			fp64[fp64size] = 0;
			if(!strcasestr(fp64, infix))
				return 1;
		}
	}
	else
	{
		if(prefixlen)
			if(memcmp(fp64, prefix, prefixlen))
				return 1;
		if(infix)
		{
			fp64[fp64size] = 0;
			if(!strstr(fp64, infix))
				return 1;
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	int opt;
	size_t lumpsize[2];
	const char *lumps[2];
	char *databuf_in; size_t databufsize_in;
	char *databuf_out; size_t databufsize_out;
	char *databuf_sig; size_t databufsize_sig;
	char lumps_w0[65536];
	char lumps_w1[65536];
	const char *pubkeyfile = NULL, *privkeyfile = NULL, *pubidfile = NULL, *prividfile = NULL, *idreqfile = NULL, *idresfile = NULL, *outfile = NULL, *outfile2 = NULL, *camouflagefile = NULL, *datafile = NULL, *sigfile = NULL;
	char fp64[513]; size_t fp64size = 512;
	int mask = 0;
	int bits = 1024;
	int i;
	D0_BOOL do_fastreject = 1;
	d0_blind_id_t *ctx;
	if(!d0_blind_id_INITIALIZE())
	{
		fprintf(stderr, "could not initialize\n");
		exit(1);
	}

	umask_save = umask(0022);

	ctx = d0_blind_id_new();
	while((opt = getopt(argc, argv, "d:s:p:P:i:I:j:J:o:O:c:b:x:X:y:Fn:C0")) != -1)
	{
		switch(opt)
		{
			case 'C':
				ignorecase = 1;
				break;
			case 'n':
				ntasks = atoi(optarg);
				break;
			case 'b':
				bits = atoi(optarg);
				break;
			case 'p': // d0pk = <pubkey> <modulus>
				pubkeyfile = optarg;
				mask |= 1;
				break;
			case 'P': // d0sk = <privkey> <modulus>
				privkeyfile = optarg;
				mask |= 2;
				break;
			case 'i': // d0pi = <pubid>
				pubidfile = optarg;
				mask |= 4;
				break;
			case 'I': // d0si = <privid>
				prividfile = optarg;
				mask |= 8;
				break;
			case 'j': // d0iq = <req>
				idreqfile = optarg;
				mask |= 0x10;
				break;
			case 'J': // d0ir = <resp>
				idresfile = optarg;
				mask |= 0x20;
				break;
			case 'o':
				outfile = optarg;
				mask |= 0x40;
				break;
			case 'O':
				outfile2 = optarg;
				mask |= 0x80;
				break;
			case 'c':
				camouflagefile = optarg;
				mask |= 0x100;
				break;
			case 'x':
				prefix = optarg;
				prefixlen = strlen(prefix);
				break;
			case '0':
				// test mode
				mask |= 0x200;
				break;
			case 'd':
				datafile = optarg;
				mask |= 0x400;
				break;
			case 's':
				sigfile = optarg;
				mask |= 0x800;
				break;
			case 'X':
				infix = optarg;
				break;
			case 'F':
				do_fastreject = 0;
				break;
			default:
				USAGE(*argv);
				return 1;
		}
	}

	// fastreject is a slight slowdown when rejecting nothing at all
	if(!infix && !prefixlen)
		do_fastreject = 0;

	guesscount = pow(64.0, prefixlen);
	if(infix)
		guesscount /= (1 - pow(1 - pow(1/64.0, strlen(infix)), 44 - prefixlen - strlen(infix)));
	// 44 chars; prefix is assumed to not match the infix (although it theoretically could)
	// 43'th char however is always '=' and does not count
	if(ignorecase)
	{
		if(infix)
			for(i = 0; infix[i]; ++i)
				if(toupper(infix[i]) != tolower(infix[i]))
					guesscount /= 2;
		for(i = 0; i < (int)prefixlen; ++i)
			if(toupper(prefix[i]) != tolower(prefix[i]))
				guesscount /= 2;
	}

	if(do_fastreject)
	{
		// fastreject: reject function gets called about log(2^bits) times more often
		guessfactor = bits * log(2) / 2;
		// so guess function gets called guesscount * guessfactor times, and it tests as many valid keys as guesscount
	}

	if(mask & 1)
	{
		file2lumps(pubkeyfile, FOURCC_D0PK, lumps, lumpsize, 2);
		if(!d0_blind_id_read_public_key(ctx, lumps[0], lumpsize[0]))
		{
			fprintf(stderr, "could not decode public key\n");
			exit(1);
		}
		if(!d0_blind_id_read_private_id_modulus(ctx, lumps[1], lumpsize[1]))
		{
			fprintf(stderr, "could not decode modulus\n");
			exit(1);
		}
	}
	else if(mask & 2)
	{
		file2lumps(privkeyfile, FOURCC_D0SK, lumps, lumpsize, 2);
		if(!d0_blind_id_read_private_key(ctx, lumps[0], lumpsize[0]))
		{
			fprintf(stderr, "could not decode private key\n");
			exit(1);
		}
		if(!d0_blind_id_read_private_id_modulus(ctx, lumps[1], lumpsize[1]))
		{
			fprintf(stderr, "could not decode modulus\n");
			exit(1);
		}
	}

	if(mask & 4)
	{
		file2lumps(pubidfile, FOURCC_D0PI, lumps, lumpsize, 1);
		if(!d0_blind_id_read_public_id(ctx, lumps[0], lumpsize[0]))
		{
			fprintf(stderr, "could not decode public ID\n");
			exit(1);
		}
	}
	if(mask & 8)
	{
		file2lumps(prividfile, FOURCC_D0SI, lumps, lumpsize, 1);
		if(!d0_blind_id_read_private_id(ctx, lumps[0], lumpsize[0]))
		{
			fprintf(stderr, "could not decode private ID\n");
			exit(1);
		}
	}

	if(mask & 0x10)
	{
		file2lumps(idreqfile, FOURCC_D0IQ, lumps, lumpsize, 1);
		lumpsize[1] = sizeof(lumps_w1);
		lumps[1] = lumps_w1;
		if(!d0_blind_id_answer_private_id_request(ctx, lumps[0], lumpsize[0], lumps_w1, &lumpsize[1]))
		{
			fprintf(stderr, "could not answer private ID request\n");
			exit(1);
		}
	}
	else if((mask & 0x120) == 0x120)
	{
		file2lumps(camouflagefile, FOURCC_D0IC, lumps, lumpsize, 1);
		if(!d0_blind_id_read_private_id_request_camouflage(ctx, lumps[0], lumpsize[0]))
		{
			fprintf(stderr, "could not decode camouflage\n");
			exit(1);
		}

		file2lumps(idresfile, FOURCC_D0IR, lumps, lumpsize, 1);
		if(!d0_blind_id_finish_private_id_request(ctx, lumps[0], lumpsize[0]))
		{
			fprintf(stderr, "could not finish private ID request\n");
			exit(1);
		}
	}

	if(mask & 0x400)
	{
		file2buf(datafile, &databuf_in, &databufsize_in);
		if(!databuf_in)
		{
			fprintf(stderr, "could not decode data\n");
			exit(1);
		}
	}

	if(mask & 0x800)
	{
		file2buf(sigfile, &databuf_sig, &databufsize_sig);
		if(!databuf_sig)
		{
			fprintf(stderr, "could not decode signature\n");
			exit(1);
		}
	}

	switch(mask)
	{
		// modes of operation:
		case 0x40:
			//   nothing -> private key file (incl modulus), print fingerprint
			generated = 0;
			generated_offset = 0;
			seconds = 0;
			signal(SIGALRM, print_generated);
			alarm(1);
			if(do_fastreject)
			{
				CHECK(d0_blind_id_generate_private_key_fastreject(ctx, bits, fastreject, d0_blind_id_fingerprint64_public_key));
			}
			else
			{
				guessfactor = 1; // no fastreject here
				do
				{
					CHECK(d0_blind_id_generate_private_key(ctx, bits));
				}
				while(fastreject(ctx, d0_blind_id_fingerprint64_public_key));
			}
			alarm(0);
			signal(SIGALRM, NULL);
			CHECK(d0_blind_id_generate_private_id_modulus(ctx));
			lumps[0] = lumps_w0;
			lumpsize[0] = sizeof(lumps_w0);
			lumps[1] = lumps_w1;
			lumpsize[1] = sizeof(lumps_w1);
			CHECK(d0_blind_id_write_private_key(ctx, lumps_w0, &lumpsize[0]));
			CHECK(d0_blind_id_write_private_id_modulus(ctx, lumps_w1, &lumpsize[1]));
			lumps2file(outfile, FOURCC_D0SK, lumps, lumpsize, 2, 1);
			break;
		case 0x42:
			//   private key file -> public key file (incl modulus)
			lumps[0] = lumps_w0;
			lumpsize[0] = sizeof(lumps_w0);
			lumps[1] = lumps_w1;
			lumpsize[1] = sizeof(lumps_w1);
			CHECK(d0_blind_id_write_public_key(ctx, lumps_w0, &lumpsize[0]));
			CHECK(d0_blind_id_write_private_id_modulus(ctx, lumps_w1, &lumpsize[1]));
			lumps2file(outfile, FOURCC_D0PK, lumps, lumpsize, 2, 0);
			break;
		case 0x41:
			//   public key file -> unsigned private ID file
			generated = 0;
			generated_offset = 0;
			seconds = 0;
			signal(SIGALRM, print_generated);
			alarm(1);
			guessfactor = 1; // no fastreject here
			do
			{
				CHECK(d0_blind_id_generate_private_id_start(ctx));
			}
			while(fastreject(ctx, d0_blind_id_fingerprint64_public_id));
			alarm(0);
			signal(SIGALRM, 0);
			lumps[0] = lumps_w0;
			lumpsize[0] = sizeof(lumps_w0);
			CHECK(d0_blind_id_write_private_id(ctx, lumps_w0, &lumpsize[0]));
			lumps2file(outfile, FOURCC_D0SI, lumps, lumpsize, 1, 1);
			break;
		case 0xC9:
			//   public key file, unsigned private ID file -> ID request file and camouflage file
			lumps[0] = lumps_w0;
			lumpsize[0] = sizeof(lumps_w0);
			CHECK(d0_blind_id_generate_private_id_request(ctx, lumps_w0, &lumpsize[0]));
			lumps2file(outfile, FOURCC_D0IQ, lumps, lumpsize, 1, 0);
			lumpsize[0] = sizeof(lumps_w0);
			CHECK(d0_blind_id_write_private_id_request_camouflage(ctx, lumps_w0, &lumpsize[0]));
			lumps2file(outfile2, FOURCC_D0IC, lumps, lumpsize, 1, 1);
			break;
		case 0x52:
			//   private key file, ID request file -> ID response file
			lumps2file(outfile, FOURCC_D0IR, lumps+1, lumpsize+1, 1, 0);
			break;
		case 0x169:
			//   public key file, ID response file, private ID file -> signed private ID file
			lumps[0] = lumps_w0;
			lumpsize[0] = sizeof(lumps_w0);
			CHECK(d0_blind_id_write_private_id(ctx, lumps_w0, &lumpsize[0]));
			lumps2file(outfile, FOURCC_D0SI, lumps, lumpsize, 1, 1);
			break;
		case 0x4A:
			//   private key file, private ID file -> signed private ID file
			{
				char buf[65536]; size_t bufsize;
				char buf2[65536]; size_t buf2size;
				D0_BOOL status;
				d0_blind_id_t *ctx2 = d0_blind_id_new();
				CHECK(d0_blind_id_copy(ctx2, ctx));
				bufsize = sizeof(buf);
				CHECK(d0_blind_id_authenticate_with_private_id_start(ctx, 1, 1, "hello world", 11, buf, &bufsize));
				buf2size = sizeof(buf2);
				CHECK(d0_blind_id_authenticate_with_private_id_challenge(ctx2, 1, 1, buf, bufsize, buf2, &buf2size, &status));
				bufsize = sizeof(buf);
				CHECK(d0_blind_id_authenticate_with_private_id_response(ctx, buf2, buf2size, buf, &bufsize));
				buf2size = sizeof(buf2);
				CHECK(d0_blind_id_authenticate_with_private_id_verify(ctx2, buf, bufsize, buf2, &buf2size, &status));
				CHECK(status == 0);
				CHECK(d0_blind_id_authenticate_with_private_id_generate_missing_signature(ctx2));
				lumps[0] = lumps_w0;
				lumpsize[0] = sizeof(lumps_w0);
				CHECK(d0_blind_id_write_private_id(ctx2, lumps_w0, &lumpsize[0]));
				lumps2file(outfile, FOURCC_D0SI, lumps, lumpsize, 1, 1);
			}
			break;
		case 0x48:
			//   private ID file -> public ID file
			lumps[0] = lumps_w0;
			lumpsize[0] = sizeof(lumps_w0);
			CHECK(d0_blind_id_write_public_id(ctx, lumps_w0, &lumpsize[0]));
			lumps2file(outfile, FOURCC_D0PI, lumps, lumpsize, 1, 0);
			break;
		case 0x01:
		case 0x02:
			//   public/private key file -> fingerprint
			CHECK(d0_blind_id_fingerprint64_public_key(ctx, fp64, &fp64size));
			printf("%.*s\n", (int)fp64size, fp64);
			break;
		case 0x05:
		case 0x09:
			//   public/private ID file -> fingerprint
			CHECK(d0_blind_id_fingerprint64_public_id(ctx, fp64, &fp64size));
			printf("%.*s\n", (int)fp64size, fp64);
			break;
		case 0x449:
			//   public key, private ID, data -> signed data
			databufsize_out = databufsize_in + 8192;
			databuf_out = malloc(databufsize_out);
			CHECK(d0_blind_id_sign_with_private_id_sign(ctx, 1, 0, databuf_in, databufsize_in, databuf_out, &databufsize_out));
			buf2file(outfile, databuf_out, databufsize_out);
			break;
		case 0x489:
			//   public key, private ID, data -> signature
			databufsize_out = databufsize_in + 8192;
			databuf_out = malloc(databufsize_out);
			CHECK(d0_blind_id_sign_with_private_id_sign_detached(ctx, 1, 0, databuf_in, databufsize_in, databuf_out, &databufsize_out));
			buf2file(outfile2, databuf_out, databufsize_out);
			break;
		case 0x841:
		case 0x8C1:
			//   public key, signed data -> data, optional public ID
			{
				D0_BOOL status;
				databufsize_out = databufsize_sig;
				databuf_out = malloc(databufsize_out);
				CHECK(d0_blind_id_sign_with_private_id_verify(ctx, 1, 0, databuf_sig, databufsize_sig, databuf_out, &databufsize_out, &status));
				CHECK(d0_blind_id_fingerprint64_public_id(ctx, fp64, &fp64size));
				printf("%d\n", (int)status);
				printf("%.*s\n", (int)fp64size, fp64);
				buf2file(outfile, databuf_out, databufsize_out);

				if(outfile2)
				{
					lumps[0] = lumps_w0;
					lumpsize[0] = sizeof(lumps_w0);
					lumps[1] = lumps_w1;
					lumpsize[1] = sizeof(lumps_w1);
					CHECK(d0_blind_id_write_public_key(ctx, lumps_w0, &lumpsize[0]));
					CHECK(d0_blind_id_write_private_id_modulus(ctx, lumps_w1, &lumpsize[1]));
					lumps2file(outfile2, FOURCC_D0PK, lumps, lumpsize, 2, 0);
				}
			}
			break;
		case 0xC01:
		case 0xC81:
			//   public key, signature, signed data -> optional public ID
			{
				D0_BOOL status;
				CHECK(d0_blind_id_sign_with_private_id_verify_detached(ctx, 1, 0, databuf_sig, databufsize_sig, databuf_in, databufsize_in, &status));
				CHECK(d0_blind_id_fingerprint64_public_id(ctx, fp64, &fp64size));
				printf("%d\n", (int)status);
				printf("%.*s\n", (int)fp64size, fp64);

				if(outfile2)
				{
					lumps[0] = lumps_w0;
					lumpsize[0] = sizeof(lumps_w0);
					lumps[1] = lumps_w1;
					lumpsize[1] = sizeof(lumps_w1);
					CHECK(d0_blind_id_write_public_key(ctx, lumps_w0, &lumpsize[0]));
					CHECK(d0_blind_id_write_private_id_modulus(ctx, lumps_w1, &lumpsize[1]));
					lumps2file(outfile2, FOURCC_D0PK, lumps, lumpsize, 2, 0);
				}
			}
			break;
/*
		case 0x09:
			//   public key, private ID file -> test whether key is properly signed
			{
				char buf[65536]; size_t bufsize;
				char buf2[65536]; size_t buf2size;
				D0_BOOL status;
				d0_blind_id_t *ctx2 = d0_blind_id_new();
				CHECK(d0_blind_id_copy(ctx2, ctx));
				bufsize = sizeof(buf);
				CHECK(d0_blind_id_authenticate_with_private_id_start(ctx, 1, 1, "hello world", 11, buf, &bufsize));
				buf2size = sizeof(buf2);
				CHECK(d0_blind_id_authenticate_with_private_id_challenge(ctx2, 1, 1, buf, bufsize, buf2, &buf2size, &status));
				bufsize = sizeof(buf);
				CHECK(d0_blind_id_authenticate_with_private_id_response(ctx, buf2, buf2size, buf, &bufsize));
				buf2size = sizeof(buf2);
				CHECK(d0_blind_id_authenticate_with_private_id_verify(ctx2, buf, bufsize, buf2, &buf2size, &status));
				if(status)
					printf("OK\n");
				else
					printf("EPIC FAIL\n");
			}
			break;
*/
		case 0x209:
			// protocol client
			{
				char hexbuf[131073];
				const char hex[] = "0123456789abcdef";
				char buf[65536]; size_t bufsize;
				char buf2[65536]; size_t buf2size;
				bufsize = sizeof(buf);
				CHECK(d0_blind_id_authenticate_with_private_id_start(ctx, 1, 1, "hello world", 11, buf, &bufsize));
				for(i = 0; i < (int)bufsize; ++i)
					sprintf(&hexbuf[2*i], "%02x", (unsigned char)buf[i]);
				printf("%s\n", hexbuf);
				fgets(hexbuf, sizeof(hexbuf), stdin);
				buf2size = strlen(hexbuf) / 2;
				for(i = 0; i < (int)buf2size; ++i)
					buf2[i] = ((strchr(hex, hexbuf[2*i]) - hex) << 4) | (strchr(hex, hexbuf[2*i+1]) - hex);
				bufsize = sizeof(buf);
				CHECK(d0_blind_id_authenticate_with_private_id_response(ctx, buf2, buf2size, buf, &bufsize));
				for(i = 0; i < (int)bufsize; ++i)
					sprintf(&hexbuf[2*i], "%02x", (unsigned char)buf[i]);
				printf("%s\n", hexbuf);
			}
			break;
		case 0x201:
			// protocol server
			{
				char hexbuf[131073];
				const char hex[] = "0123456789abcdef";
				char buf[65536]; size_t bufsize;
				char buf2[65536]; size_t buf2size;
				D0_BOOL status;
				fgets(hexbuf, sizeof(hexbuf), stdin);
				buf2size = strlen(hexbuf) / 2;
				for(i = 0; i < (int)buf2size; ++i)
					buf2[i] = ((strchr(hex, hexbuf[2*i]) - hex) << 4) | (strchr(hex, hexbuf[2*i+1]) - hex);
				bufsize = sizeof(buf);
				CHECK(d0_blind_id_authenticate_with_private_id_challenge(ctx, 1, 1, buf2, buf2size, buf, &bufsize, &status));
				for(i = 0; i < (int)bufsize; ++i)
					sprintf(&hexbuf[2*i], "%02x", (unsigned char)buf[i]);
				printf("%s\n", hexbuf);
				fgets(hexbuf, sizeof(hexbuf), stdin);
				buf2size = strlen(hexbuf) / 2;
				for(i = 0; i < (int)buf2size; ++i)
					buf2[i] = ((strchr(hex, hexbuf[2*i]) - hex) << 4) | (strchr(hex, hexbuf[2*i+1]) - hex);
				bufsize = sizeof(buf);
				CHECK(d0_blind_id_authenticate_with_private_id_verify(ctx, buf2, buf2size, buf, &bufsize, &status));
				printf("verify status: %d\n", status);

				CHECK(d0_blind_id_fingerprint64_public_id(ctx, fp64, &fp64size));
				printf("%.*s\n", (int)fp64size, fp64);
			}
			break;
		default:
			USAGE(*argv);
			exit(1);
			break;
	}
	d0_blind_id_SHUTDOWN();
	return 0;
}
