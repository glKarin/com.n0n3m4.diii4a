//SHA.h 

#ifndef __SHA_H__ 
#define __SHA_H__ 

//Typical DISCLAIMER: 
//The code in this project is Copyright (C) 2003 by George Anescu. You have the right to 
//use and distribute the code in any way you see fit as long as this paragraph is included 
//with the distribution. No warranties or claims are made as to the validity of the 
//information and code contained herein, so use it at your own risk.

class CSHA
{ 
public: 
	enum { SHA256=0 };
	//CONSTRUCTOR 
	CSHA(int iMethod=SHA256);
	//Update context to reflect the concatenation of another buffer of bytes. 
	void AddData(char const* pcData, unsigned int iDataLength);
	//Final wrapup - pad to 64-byte boundary with the bit pattern  
	//1 0*(64-bit count of bits processed, MSB-first) 
	void FinalDigest(char* pcDigest); 
	//Reset current operation in order to prepare a new one 
	void Reset(); 
 
private: 
	bool m_bAddData;
	//Transformation Function 
	void Transform(); 
	//The Method 
	int m_iMethod; 
	enum { BLOCKSIZE = 64, BLOCKSIZE2 = BLOCKSIZE<<1 };
	//For 32 bits Integers 
	enum { SHA160LENGTH=5, SHA256LENGTH=8 }; 
	//Context Variables 
	unsigned int m_auiBuf[SHA256LENGTH]; //Maximum for SHA256 
	unsigned int m_auiBits[2]; 
	unsigned char m_aucIn[BLOCKSIZE2]; //128 bytes for SHA384, SHA512 
	//Internal auxiliary static functions 
	static unsigned int CircularShift(unsigned int uiBits, unsigned int uiWord); 
	static unsigned int CH(unsigned int x, unsigned int y, unsigned int z); 
	static unsigned int MAJ(unsigned int x, unsigned int y, unsigned int z); 
	static unsigned int SIG0(unsigned int x); 
	static unsigned int SIG1(unsigned int x); 
	static unsigned int sig0(unsigned int x); 
	static unsigned int sig1(unsigned int x); 
	static void Bytes2Word(unsigned char const* pcBytes, unsigned int& ruiWord); 
	static void Word2Bytes(unsigned int const& ruiWord, unsigned char* pcBytes); 
	static const unsigned int sm_K256[64]; 
	static const unsigned int sm_H256[SHA256LENGTH]; 
 }; 
 
inline unsigned int CSHA::CircularShift(unsigned int uiBits, unsigned int uiWord) 
{ 
	return (uiWord << uiBits) | (uiWord >> (32-uiBits)); 
} 
 
inline unsigned int CSHA::CH(unsigned int x, unsigned int y, unsigned int z) 
{ 
	return ((x&(y^z))^z); 
} 
 
inline unsigned int CSHA::MAJ(unsigned int x, unsigned int y, unsigned int z) 
{ 
	return (((x|y)&z)|(x&y)); 
} 
 
inline unsigned int CSHA::SIG0(unsigned int x) 
{ 
	return ((x >> 2)|(x << 30)) ^ ((x >> 13)|(x << 19)) ^ ((x >> 22)|(x << 10)); 
} 
 
inline unsigned int CSHA::SIG1(unsigned int x) 
{ 
	return ((x >> 6)|(x << 26)) ^ ((x >> 11)|(x << 21)) ^ ((x >> 25)|(x << 7)); 
} 
 
inline unsigned int CSHA::sig0(unsigned int x) 
{ 
	return ((x >> 7)|(x << 25)) ^ ((x >> 18)|(x << 14)) ^ (x >> 3); 
} 
 
inline unsigned int CSHA::sig1(unsigned int x) 
{ 
	return ((x >> 17)|(x << 15)) ^ ((x >> 19)|(x << 13)) ^ (x >> 10); 
} 
 
inline void CSHA::Bytes2Word(unsigned char const* pcBytes, unsigned int& ruiWord) 
{ 
	ruiWord = (unsigned int)*(pcBytes+3) | (unsigned int)(*(pcBytes+2)<<8) | 
		(unsigned int)(*(pcBytes+1)<<16) | (unsigned int)(*pcBytes<<24); 
} 
 
inline void CSHA::Word2Bytes(unsigned int const& ruiWord, unsigned char* pcBytes) 
{ 
	pcBytes += 3; 
	*pcBytes = ruiWord & 0xff; 
	*--pcBytes = (ruiWord>>8) & 0xff; 
	*--pcBytes = (ruiWord>>16) & 0xff; 
	*--pcBytes = (ruiWord>>24) & 0xff; 
} 
 
#endif // __SHA_H__ 
