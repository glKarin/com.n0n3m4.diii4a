//SHA.cpp
#include "SHA.h"
#include <cstring>

using namespace std;

const unsigned int CSHA::sm_K256[64] =
{
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

const unsigned int CSHA::sm_H256[SHA256LENGTH] =
{
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

//CONSTRUCTOR
CSHA::CSHA(int iMethod) : m_iMethod( iMethod )
{
	Reset();
}

//Update context to reflect the concatenation of another buffer of bytes.
void CSHA::AddData(char const* pcData, unsigned int iDataLength)
{
	unsigned int uiT;
    switch(m_iMethod)
    {
        case SHA256:
            {
                //Update bitcount
                uiT = m_auiBits[0];
                if((m_auiBits[0] = uiT + ((unsigned int)iDataLength << 3)) < uiT)
                    m_auiBits[1]++; //Carry from low to high
                m_auiBits[1] += iDataLength >> 29;
                uiT = (uiT >> 3) & (BLOCKSIZE-1); //Bytes already
                //Handle any leading odd-sized chunks
                if(uiT != 0)
                {
                    unsigned char* puc = (unsigned char*)m_aucIn + uiT;
                    uiT = BLOCKSIZE - uiT;
                    if(iDataLength < uiT)
                    {
                        memcpy(puc, pcData, iDataLength);
                        return;
                    }
                    memcpy(puc, pcData, uiT);
                    Transform();
                    pcData += uiT;
                    iDataLength -= uiT;
                }
                //Process data in 64-byte chunks
                while(iDataLength >= BLOCKSIZE)
                {
                    memcpy(m_aucIn, pcData, BLOCKSIZE);
                    Transform();
                    pcData += BLOCKSIZE;
                    iDataLength -= BLOCKSIZE;
                }
                //Handle any remaining bytes of data
                memcpy(m_aucIn, pcData, iDataLength);
            }
			break;
	}
}

//Final wrapup - pad to 64-byte boundary with the bit pattern
//1 0*(64-bit count of bits processed, MSB-first)
void CSHA::FinalDigest(char* pcDigest)
{
    //Is the User's responsability to ensure that pcDigest is properly allocated 20, 32,
    //48 or 64 bytes, depending on the method
    switch(m_iMethod)
    {
        case SHA256:
            {
                unsigned int uiCount;
                unsigned char *puc;
                //Compute number of bytes mod 64
                uiCount = (m_auiBits[0] >> 3) & (BLOCKSIZE-1);
                //Set the first char of padding to 0x80. This is safe since there is
                //always at least one byte free
                puc = m_aucIn + uiCount;
                *puc++ = 0x80;
                //Bytes of padding needed to make 64 bytes
                uiCount = BLOCKSIZE - uiCount - 1;
                //Pad out to 56 mod 64
                if(uiCount < 8)
                {
                    //Two lots of padding: Pad the first block to 64 bytes
                    memset(puc, 0, uiCount);
                    Transform();
                    //Now fill the next block with 56 bytes
                    memset(m_aucIn, 0, BLOCKSIZE-8);
                }
                else
                {
                    //Pad block to 56 bytes
                    memset(puc, 0, uiCount - 8);
                }
                //Append length in bits and transform
                Word2Bytes(m_auiBits[1], &m_aucIn[BLOCKSIZE-8]);
                Word2Bytes(m_auiBits[0], &m_aucIn[BLOCKSIZE-4]);
                Transform();
                switch(m_iMethod)
                {
                    case SHA256:
                        {
                            for(int i=0; i<SHA256LENGTH; i++,pcDigest+=4)
                                Word2Bytes(m_auiBuf[i], reinterpret_cast<unsigned char*>(pcDigest));
                        }
                        break;
                }
            }
			break;
    }
    //Reinitialize
    Reset();
}

//Reset current operation in order to prepare a new one
void CSHA::Reset()
{
    //Reinitialize
    switch(m_iMethod)
    {
        case SHA256:
            {
                for(int i=0; i<SHA256LENGTH; i++)
                    m_auiBuf[i] = sm_H256[i];
                m_auiBits[0] = 0;
                m_auiBits[1] = 0;
            }
			break;
	}
}

//The core of the SHA algorithm, this alters an existing SHA hash to
//reflect the addition of 16 longwords of new data.
void CSHA::Transform()
{
    switch(m_iMethod)
    {
        case SHA256:
            {
                //Expansion of m_aucIn
                unsigned char* pucIn = m_aucIn;
                unsigned int auiW[64];
                int i;
                for(i=0; i<16; i++,pucIn+=4)
                    Bytes2Word(pucIn, auiW[i]);
                for(i=16; i<64; i++)
                    auiW[i] = sig1(auiW[i-2]) + auiW[i-7] + sig0(auiW[i-15]) + auiW[i-16];
                //OR
                //for(i=0; i<48; i++)
                //  auiW[i+16] = sig1(auiW[i+14]) + auiW[i+9] + sig0(auiW[i+1]) + auiW[i];
                unsigned int a, b, c, d, e, f, g, h, t;
                a = m_auiBuf[0];
                b = m_auiBuf[1];
                c = m_auiBuf[2];
                d = m_auiBuf[3];
                e = m_auiBuf[4];
                f = m_auiBuf[5];
                g = m_auiBuf[6];
                h = m_auiBuf[7];
                t = h + SIG1(e) + CH(e, f, g) + sm_K256[0] + auiW[0]; h = t + SIG0(a) + MAJ(a, b, c); d += t;
                t = g + SIG1(d) + CH(d, e, f) + sm_K256[1] + auiW[1]; g = t + SIG0(h) + MAJ(h, a, b); c += t;
                t = f + SIG1(c) + CH(c, d, e) + sm_K256[2] + auiW[2]; f = t + SIG0(g) + MAJ(g, h, a); b += t;
                t = e + SIG1(b) + CH(b, c, d) + sm_K256[3] + auiW[3]; e = t + SIG0(f) + MAJ(f, g, h); a += t;
                t = d + SIG1(a) + CH(a, b, c) + sm_K256[4] + auiW[4]; d = t + SIG0(e) + MAJ(e, f, g); h += t;
                t = c + SIG1(h) + CH(h, a, b) + sm_K256[5] + auiW[5]; c = t + SIG0(d) + MAJ(d, e, f); g += t;
                t = b + SIG1(g) + CH(g, h, a) + sm_K256[6] + auiW[6]; b = t + SIG0(c) + MAJ(c, d, e); f += t;
                t = a + SIG1(f) + CH(f, g, h) + sm_K256[7] + auiW[7]; a = t + SIG0(b) + MAJ(b, c, d); e += t;
                //
                t = h + SIG1(e) + CH(e, f, g) + sm_K256[8] + auiW[8]; h = t + SIG0(a) + MAJ(a, b, c); d += t;
                t = g + SIG1(d) + CH(d, e, f) + sm_K256[9] + auiW[9]; g = t + SIG0(h) + MAJ(h, a, b); c += t;
                t = f + SIG1(c) + CH(c, d, e) + sm_K256[10] + auiW[10]; f = t + SIG0(g) + MAJ(g, h, a); b += t;
                t = e + SIG1(b) + CH(b, c, d) + sm_K256[11] + auiW[11]; e = t + SIG0(f) + MAJ(f, g, h); a += t;
                t = d + SIG1(a) + CH(a, b, c) + sm_K256[12] + auiW[12]; d = t + SIG0(e) + MAJ(e, f, g); h += t;
                t = c + SIG1(h) + CH(h, a, b) + sm_K256[13] + auiW[13]; c = t + SIG0(d) + MAJ(d, e, f); g += t;
                t = b + SIG1(g) + CH(g, h, a) + sm_K256[14] + auiW[14]; b = t + SIG0(c) + MAJ(c, d, e); f += t;
                t = a + SIG1(f) + CH(f, g, h) + sm_K256[15] + auiW[15]; a = t + SIG0(b) + MAJ(b, c, d); e += t;
                //
                t = h + SIG1(e) + CH(e, f, g) + sm_K256[16] + auiW[16]; h = t + SIG0(a) + MAJ(a, b, c); d += t;
                t = g + SIG1(d) + CH(d, e, f) + sm_K256[17] + auiW[17]; g = t + SIG0(h) + MAJ(h, a, b); c += t;
                t = f + SIG1(c) + CH(c, d, e) + sm_K256[18] + auiW[18]; f = t + SIG0(g) + MAJ(g, h, a); b += t;
                t = e + SIG1(b) + CH(b, c, d) + sm_K256[19] + auiW[19]; e = t + SIG0(f) + MAJ(f, g, h); a += t;
                t = d + SIG1(a) + CH(a, b, c) + sm_K256[20] + auiW[20]; d = t + SIG0(e) + MAJ(e, f, g); h += t;
                t = c + SIG1(h) + CH(h, a, b) + sm_K256[21] + auiW[21]; c = t + SIG0(d) + MAJ(d, e, f); g += t;
                t = b + SIG1(g) + CH(g, h, a) + sm_K256[22] + auiW[22]; b = t + SIG0(c) + MAJ(c, d, e); f += t;
                t = a + SIG1(f) + CH(f, g, h) + sm_K256[23] + auiW[23]; a = t + SIG0(b) + MAJ(b, c, d); e += t;
                //
                t = h + SIG1(e) + CH(e, f, g) + sm_K256[24] + auiW[24]; h = t + SIG0(a) + MAJ(a, b, c); d += t;
                t = g + SIG1(d) + CH(d, e, f) + sm_K256[25] + auiW[25]; g = t + SIG0(h) + MAJ(h, a, b); c += t;
                t = f + SIG1(c) + CH(c, d, e) + sm_K256[26] + auiW[26]; f = t + SIG0(g) + MAJ(g, h, a); b += t;
                t = e + SIG1(b) + CH(b, c, d) + sm_K256[27] + auiW[27]; e = t + SIG0(f) + MAJ(f, g, h); a += t;
                t = d + SIG1(a) + CH(a, b, c) + sm_K256[28] + auiW[28]; d = t + SIG0(e) + MAJ(e, f, g); h += t;
                t = c + SIG1(h) + CH(h, a, b) + sm_K256[29] + auiW[29]; c = t + SIG0(d) + MAJ(d, e, f); g += t;
                t = b + SIG1(g) + CH(g, h, a) + sm_K256[30] + auiW[30]; b = t + SIG0(c) + MAJ(c, d, e); f += t;
                t = a + SIG1(f) + CH(f, g, h) + sm_K256[31] + auiW[31]; a = t + SIG0(b) + MAJ(b, c, d); e += t;
                //
                t = h + SIG1(e) + CH(e, f, g) + sm_K256[32] + auiW[32]; h = t + SIG0(a) + MAJ(a, b, c); d += t;
                t = g + SIG1(d) + CH(d, e, f) + sm_K256[33] + auiW[33]; g = t + SIG0(h) + MAJ(h, a, b); c += t;
                t = f + SIG1(c) + CH(c, d, e) + sm_K256[34] + auiW[34]; f = t + SIG0(g) + MAJ(g, h, a); b += t;
                t = e + SIG1(b) + CH(b, c, d) + sm_K256[35] + auiW[35]; e = t + SIG0(f) + MAJ(f, g, h); a += t;
                t = d + SIG1(a) + CH(a, b, c) + sm_K256[36] + auiW[36]; d = t + SIG0(e) + MAJ(e, f, g); h += t;
                t = c + SIG1(h) + CH(h, a, b) + sm_K256[37] + auiW[37]; c = t + SIG0(d) + MAJ(d, e, f); g += t;
                t = b + SIG1(g) + CH(g, h, a) + sm_K256[38] + auiW[38]; b = t + SIG0(c) + MAJ(c, d, e); f += t;
                t = a + SIG1(f) + CH(f, g, h) + sm_K256[39] + auiW[39]; a = t + SIG0(b) + MAJ(b, c, d); e += t;
                //
                t = h + SIG1(e) + CH(e, f, g) + sm_K256[40] + auiW[40]; h = t + SIG0(a) + MAJ(a, b, c); d += t;
                t = g + SIG1(d) + CH(d, e, f) + sm_K256[41] + auiW[41]; g = t + SIG0(h) + MAJ(h, a, b); c += t;
                t = f + SIG1(c) + CH(c, d, e) + sm_K256[42] + auiW[42]; f = t + SIG0(g) + MAJ(g, h, a); b += t;
                t = e + SIG1(b) + CH(b, c, d) + sm_K256[43] + auiW[43]; e = t + SIG0(f) + MAJ(f, g, h); a += t;
                t = d + SIG1(a) + CH(a, b, c) + sm_K256[44] + auiW[44]; d = t + SIG0(e) + MAJ(e, f, g); h += t;
                t = c + SIG1(h) + CH(h, a, b) + sm_K256[45] + auiW[45]; c = t + SIG0(d) + MAJ(d, e, f); g += t;
                t = b + SIG1(g) + CH(g, h, a) + sm_K256[46] + auiW[46]; b = t + SIG0(c) + MAJ(c, d, e); f += t;
                t = a + SIG1(f) + CH(f, g, h) + sm_K256[47] + auiW[47]; a = t + SIG0(b) + MAJ(b, c, d); e += t;
                //
                t = h + SIG1(e) + CH(e, f, g) + sm_K256[48] + auiW[48]; h = t + SIG0(a) + MAJ(a, b, c); d += t;
                t = g + SIG1(d) + CH(d, e, f) + sm_K256[49] + auiW[49]; g = t + SIG0(h) + MAJ(h, a, b); c += t;
                t = f + SIG1(c) + CH(c, d, e) + sm_K256[50] + auiW[50]; f = t + SIG0(g) + MAJ(g, h, a); b += t;
                t = e + SIG1(b) + CH(b, c, d) + sm_K256[51] + auiW[51]; e = t + SIG0(f) + MAJ(f, g, h); a += t;
                t = d + SIG1(a) + CH(a, b, c) + sm_K256[52] + auiW[52]; d = t + SIG0(e) + MAJ(e, f, g); h += t;
                t = c + SIG1(h) + CH(h, a, b) + sm_K256[53] + auiW[53]; c = t + SIG0(d) + MAJ(d, e, f); g += t;
                t = b + SIG1(g) + CH(g, h, a) + sm_K256[54] + auiW[54]; b = t + SIG0(c) + MAJ(c, d, e); f += t;
                t = a + SIG1(f) + CH(f, g, h) + sm_K256[55] + auiW[55]; a = t + SIG0(b) + MAJ(b, c, d); e += t;
                //
                t = h + SIG1(e) + CH(e, f, g) + sm_K256[56] + auiW[56]; h = t + SIG0(a) + MAJ(a, b, c); d += t;
                t = g + SIG1(d) + CH(d, e, f) + sm_K256[57] + auiW[57]; g = t + SIG0(h) + MAJ(h, a, b); c += t;
                t = f + SIG1(c) + CH(c, d, e) + sm_K256[58] + auiW[58]; f = t + SIG0(g) + MAJ(g, h, a); b += t;
                t = e + SIG1(b) + CH(b, c, d) + sm_K256[59] + auiW[59]; e = t + SIG0(f) + MAJ(f, g, h); a += t;
                t = d + SIG1(a) + CH(a, b, c) + sm_K256[60] + auiW[60]; d = t + SIG0(e) + MAJ(e, f, g); h += t;
                t = c + SIG1(h) + CH(h, a, b) + sm_K256[61] + auiW[61]; c = t + SIG0(d) + MAJ(d, e, f); g += t;
                t = b + SIG1(g) + CH(g, h, a) + sm_K256[62] + auiW[62]; b = t + SIG0(c) + MAJ(c, d, e); f += t;
                t = a + SIG1(f) + CH(f, g, h) + sm_K256[63] + auiW[63]; a = t + SIG0(b) + MAJ(b, c, d); e += t;
                //
                //OR
                /*
                unsigned int a, b, c, d, e, f, g, h, t1, t2;
                a = m_auiBuf[0];
                b = m_auiBuf[1];
                c = m_auiBuf[2];
                d = m_auiBuf[3];
                e = m_auiBuf[4];
                f = m_auiBuf[5];
                g = m_auiBuf[6];
                h = m_auiBuf[7];
                //
                for(i=0; i<64; i++)
                {
                    t1 = h + SIG1(e) + CH(e, f, g) + sm_K256[i] + auiW[i];
                    t2 = SIG0(a) + MAJ(a, b, c);
                    h = g;
                    g = f;
                    f = e;
                    e = d+t1;
                    d = c;
                    c = b;
                    b = a;
                    a = t1 + t2;
                }
                */
                m_auiBuf[0] += a;
                m_auiBuf[1] += b;
                m_auiBuf[2] += c;
                m_auiBuf[3] += d;
                m_auiBuf[4] += e;
                m_auiBuf[5] += f;
                m_auiBuf[6] += g;
                m_auiBuf[7] += h;
            }
            break;
    }
}
