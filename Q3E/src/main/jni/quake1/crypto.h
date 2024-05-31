/*
Copyright (C) 2010-2015 Rudolf Polzer (divVerent)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef CRYPTO_H
#define CRYPTO_H

#include <stddef.h>
#include "qtypes.h"
struct lhnetaddress_s;

extern struct cvar_s crypto_developer;
extern struct cvar_s crypto_aeslevel;
#define ENCRYPTION_REQUIRED (crypto_aeslevel.integer >= 3)

extern int crypto_keyfp_recommended_length; // applies to LOCAL IDs, and to ALL keys

#define CRYPTO_HEADERSIZE 31
// AES case causes 16 to 31 bytes overhead
// SHA256 case causes 16 bytes overhead as we truncate to 128bit

#define FP64_SIZE 44
#define DHKEY_SIZE 16

typedef struct
{
	unsigned char dhkey[DHKEY_SIZE]; // shared key, not NUL terminated
	char client_idfp[FP64_SIZE+1];
	char client_keyfp[FP64_SIZE+1];
	qbool client_issigned;
	char server_idfp[FP64_SIZE+1];
	char server_keyfp[FP64_SIZE+1];
	qbool server_issigned;
	qbool authenticated;
	qbool use_aes;
	void *data;
}
crypto_t;

void Crypto_Init(void);
void Crypto_Init_Commands(void);
void Crypto_LoadKeys(void); // NOTE: when this is called, the SV_LockThreadMutex MUST be active
void Crypto_Shutdown(void);
qbool Crypto_Available(void);
void sha256(unsigned char *out, const unsigned char *in, int n); // may ONLY be called if Crypto_Available()
const void *Crypto_EncryptPacket(crypto_t *crypto, const void *data_src, size_t len_src, void *data_dst, size_t *len_dst, size_t len);
const void *Crypto_DecryptPacket(crypto_t *crypto, const void *data_src, size_t len_src, void *data_dst, size_t *len_dst, size_t len);
#define CRYPTO_NOMATCH 0        // process as usual (packet was not used)
#define CRYPTO_MATCH 1          // process as usual (packet was used)
#define CRYPTO_DISCARD 2        // discard this packet
#define CRYPTO_REPLACE 3        // make the buffer the current packet
int Crypto_ClientParsePacket(const char *data_in, size_t len_in, char *data_out, size_t *len_out, lhnetaddress_t *peeraddress, const char *peeraddressstring);
int Crypto_ServerParsePacket(const char *data_in, size_t len_in, char *data_out, size_t *len_out, lhnetaddress_t *peeraddress);

// if len_out is nonzero, the packet is to be sent to the client

qbool Crypto_ServerAppendToChallenge(const char *data_in, size_t len_in, char *data_out, size_t *len_out, size_t maxlen);
crypto_t *Crypto_ServerGetInstance(struct lhnetaddress_s *peeraddress);
qbool Crypto_FinishInstance(crypto_t *out, crypto_t *in); // also clears allocated memory, and frees the instance received by ServerGetInstance
const char *Crypto_GetInfoResponseDataString(void);

// retrieves a host key for an address (can be exposed to menuqc, or used by the engine to look up stored keys e.g. for server bookmarking)
// pointers may be NULL
qbool Crypto_RetrieveHostKey(struct lhnetaddress_s *peeraddress, int *keyid, char *keyfp, size_t keyfplen, char *idfp, size_t idfplen, int *aeslevel, qbool *issigned);
int Crypto_RetrieveLocalKey(int keyid, char *keyfp, size_t keyfplen, char *idfp, size_t idfplen, qbool *issigned); // return value: -1 if more to come, +1 if valid, 0 if end of list

size_t Crypto_SignData(const void *data, size_t datasize, int keyid, void *signed_data, size_t signed_size);
size_t Crypto_SignDataDetached(const void *data, size_t datasize, int keyid, void *signed_data, size_t signed_size);

// netconn protocol:
//   non-crypto:
//     getchallenge                                            >
//                                                             < challenge
//     connect                                                 >
//                                                             < accept (or: reject)
//   crypto:
//     getchallenge                                            >
//                                                             < challenge SP <challenge> NUL vlen <size> d0pk <fingerprints I can auth to> NUL NUL <other fingerprints I accept>
//
//     IF serverfp:
//     d0pk\cnt\0\challenge\<challenge>\aeslevel\<level> NUL <serverfp> NUL <clientfp>
//                                                             >
//                                                               check if client would get accepted; if not, do "reject" now
//     require non-control packets to be encrypted               require non-control packets to be encrypted
//     do not send anything yet                                  do not send anything yet
//     RESET to serverfp                                         RESET to serverfp
//                                                               d0_blind_id_authenticate_with_private_id_start() = 1
//                                                             < d0pk\cnt\1\aes\<aesenabled> NUL *startdata*
//     d0_blind_id_authenticate_with_private_id_challenge() = 1
//     d0pk\cnt\2 NUL *challengedata*                          >
//                                                               d0_blind_id_authenticate_with_private_id_response() = 0
//                                                             < d0pk\cnt\3 NUL *responsedata*
//     d0_blind_id_authenticate_with_private_id_verify() = 1
//     store server's fingerprint NOW
//     d0_blind_id_sessionkey_public_id() = 1                    d0_blind_id_sessionkey_public_id() = 1
//
//     IF clientfp AND NOT serverfp:
//     RESET to clientfp                                         RESET to clientfp
//     d0_blind_id_authenticate_with_private_id_start() = 1
//     d0pk\cnt\0\challenge\<challenge>\aeslevel\<level> NUL NUL <clientfp> NUL *startdata*
//                                                             >
//                                                               check if client would get accepted; if not, do "reject" now
//     require non-control packets to be encrypted               require non-control packets to be encrypted
//                                                               d0_blind_id_authenticate_with_private_id_challenge() = 1
//                                                             < d0pk\cnt\5\aes\<aesenabled> NUL *challengedata*
//
//     IF clientfp AND serverfp:
//     RESET to clientfp                                         RESET to clientfp
//     d0_blind_id_authenticate_with_private_id_start() = 1
//     d0pk\cnt\4 NUL *startdata*                              >
//                                                               d0_blind_id_authenticate_with_private_id_challenge() = 1
//                                                             < d0pk\cnt\5 NUL *challengedata*
//
//     IF clientfp:
//     d0_blind_id_authenticate_with_private_id_response() = 0
//     d0pk\cnt\6 NUL *responsedata*                           >
//                                                               d0_blind_id_authenticate_with_private_id_verify() = 1
//                                                               store client's fingerprint NOW
//     d0_blind_id_sessionkey_public_id() = 1                    d0_blind_id_sessionkey_public_id() = 1
//     note: the ... is the "connect" message, except without the challenge. Reinterpret as regular connect message on server side
//
//     enforce encrypted transmission (key is XOR of the two DH keys)
//
//     IF clientfp:
//                                                             < challenge (mere sync message)
//
//     connect\...                                             >
//                                                             < accept (ALWAYS accept if connection is encrypted, ignore challenge as it had been checked before)
//
//     commence with ingame protocol

// in short:
//   server:
//     getchallenge NUL d0_blind_id: reply with challenge with added fingerprints
//     cnt=0: IF server will auth, cnt=1, ELSE cnt=5
//     cnt=2: cnt=3
//     cnt=4: cnt=5
//     cnt=6: send "challenge"
//   client:
//     challenge with added fingerprints: cnt=0; if client will auth but not server, append client auth start
//     cnt=1: cnt=2
//     cnt=3: IF client will auth, cnt=4, ELSE rewrite as "challenge"
//     cnt=5: cnt=6, server will continue by sending "challenge" (let's avoid sending two packets as response to one)
// other change:
//   accept empty "challenge", and challenge-less connect in case crypto protocol has executed and finished
//   statusResponse and infoResponse get an added d0_blind_id key that lists
//   the keys the server can auth with and to in key@ca SPACE key@ca notation
//   any d0pk\ message has an appended "id" parameter; messages with an unexpected "id" are ignored to prevent errors from multiple concurrent auth runs


// comparison to OTR:
// - encryption: yes
// - authentication: yes
// - deniability: no (attacker requires the temporary session key to prove you
//   have sent a specific message, the private key itself does not suffice), no
//   measures are taken to provide forgeability to even provide deniability
//   against an attacker who knows the temporary session key, as using CTR mode
//   for the encryption - which, together with deriving the MAC key from the
//   encryption key, and MACing the ciphertexts instead of the plaintexts,
//   would provide forgeability and thus deniability - requires longer
//   encrypted packets and deniability was not a goal of this, as we may e.g.
//   reserve the right to capture packet dumps + extra state info to prove a
//   client/server has sent specific packets to prove cheating)
// - perfect forward secrecy: yes (session key is derived via DH key exchange)

#endif
