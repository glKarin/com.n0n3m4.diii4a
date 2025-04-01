/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#pragma once

#include <type_traits>
#include <stdlib.h>

/**
 * Array of boolean values packed in a bitset.
 */
template<class TWord> class idBitArray {
public:
	static_assert(std::is_unsigned<TWord>::value, "BitArray only works with unsigned word");
	static const int WordBytes = sizeof(TWord);
	static const int LogWordBytes = (
		WordBytes == 1 ? 0 :
		WordBytes == 2 ? 1 :
		WordBytes == 4 ? 2 :
		WordBytes == 8 ? 3 :
	-1);
	static const int WordBits = WordBytes * 8;
	static const int LogWordBits = LogWordBytes + 3;
	static const TWord W1 = TWord(1);


	static ID_FORCE_INLINE int NumWordsForBits(int num) {
		// note: if num % WB = 0, then we need one padding word for segment operations
		return (num >> LogWordBits) + 1;
	}
	static ID_FORCE_INLINE int CapacityInWords(int num) {
		return (num << LogWordBits) - 1;
	}

	static ID_FORCE_INLINE TWord IfBit(const TWord *arr, int index) {
		return ( arr[index >> LogWordBits] ) & ( W1 << ( index & (WordBits - 1) ) );
	}
	static ID_FORCE_INLINE int GetBit(const TWord *arr, int index) {
		return ( ( arr[index >> LogWordBits] ) >> ( index & (WordBits - 1) ) ) & W1;

	}
	static ID_FORCE_INLINE void SetBitTrue(TWord *arr, int index) {
		arr[index >> LogWordBits] |= W1 << ( index & (WordBits - 1) );
	}
	static ID_FORCE_INLINE void SetBitFalse(TWord *arr, int index) {
		arr[index >> LogWordBits] &= ~( W1 << ( index & (WordBits - 1) ) );
	}
	static ID_FORCE_INLINE void InvertBit(TWord *arr, int index) {
		arr[index >> LogWordBits] ^= W1 << ( index & (WordBits - 1) );
	}
	static ID_FORCE_INLINE void SetBit(TWord *arr, int index, int value) {
		assert(value == 0 || value == 1);
		TWord &x = arr[index >> LogWordBits];
		int bit = index & (WordBits - 1);
		x &= ~(W1 << bit);
		x |= TWord(value) << bit;
	}

	static void SetBitsTrue(TWord *arr, int beg, int end) {
		assert(beg <= end);
		int begBlock = beg >> LogWordBits;
		int endBlock = end >> LogWordBits;
		int begRem = beg & (WordBits - 1);
		int endRem = end & (WordBits - 1);
		TWord begMask = 0 - (W1 << begRem);
		TWord endMask = (W1 << endRem) - 1;
		if (begBlock == endBlock) {
			arr[begBlock] |= begMask & endMask;
		}
		else {
			arr[begBlock] |= begMask;
			arr[endBlock] |= endMask;
			for (int i = begBlock + 1; i < endBlock; i++)
				arr[i] = ~TWord(0);
		}
	}
	static void SetBitsFalse(TWord *arr, int beg, int end) {
		assert(beg <= end);
		int begBlock = beg >> LogWordBits;
		int endBlock = end >> LogWordBits;
		int begRem = beg & (WordBits - 1);
		int endRem = end & (WordBits - 1);
		TWord begMask = (W1 << begRem) - 1;
		TWord endMask = 0 - (W1 << endRem);
		if (begBlock == endBlock) {
			arr[begBlock] &= begMask | endMask;
		}
		else {
			arr[begBlock] &= begMask;
			arr[endBlock] &= endMask;
			for (int i = begBlock + 1; i < endBlock; i++)
				arr[i] = 0;
		}
	}
	static void InvertBits(TWord *arr, int beg, int end) {
		assert(beg <= end);
		int begBlock = beg >> LogWordBits;
		int endBlock = end >> LogWordBits;
		int begRem = beg & (WordBits - 1);
		int endRem = end & (WordBits - 1);
		TWord begMask = (W1 << begRem) - 1;
		TWord endMask = (W1 << endRem) - 1;
		arr[begBlock] ^= begMask;
		arr[endBlock] ^= endMask;
		for (int i = begBlock; i < endBlock; i++)
			arr[i] = ~arr[i];
	}
	static void SetBitsSame(TWord *arr, int beg, int end, int value) {
		assert(value == 0 || value == 1);
		value ? SetBitsTrue(arr, beg, end) : SetBitsFalse(arr, beg, end);
	}
	static void SetBitsWord(TWord *arr, int beg, int end, TWord value) {
		int len = end - beg;
		assert(unsigned(len) < unsigned(WordBits) && value < (W1 << len));
		int begBlock = beg >> LogWordBits;
		int endBlock = end >> LogWordBits;
		int begRem = beg & (WordBits - 1);
		int endRem = end & (WordBits - 1);
		TWord begMask = (W1 << begRem) - 1;
		TWord endMask = 0 - (W1 << endRem);
		if (begBlock == endBlock) {
			arr[begBlock] &= begMask | endMask;
			arr[begBlock] |= (value << begRem);
		}
		else {
			arr[begBlock] &= begMask;
			arr[endBlock] &= endMask;
			arr[begBlock] |= value << begRem;
			arr[endBlock] |= value >> (WordBits - begRem);
		}
	}

	// you can combine above functions with any idList-like container
	// or you can use the methods below for simple cases, when size is known at time of initialization

	~idBitArray() {
		free(list);
	}
	idBitArray() {
		// note: this object is invalid (see NumWordsForBits)
		// don't use any methods on it!
		num = cap = -1;
		list = nullptr;
	}
	void Init(int numBits) {
		free(list);
		int numWords = NumWordsForBits(numBits);
		num = numBits;
		cap = CapacityInWords(numWords);
		assert(num <= cap);
		list = (TWord*)malloc(numWords * sizeof(TWord));
	}
	// non-copyable for now
	idBitArray(const idBitArray &x) = delete;
	idBitArray& operator=(const idBitArray &x) = delete;

	void SetBitsSameAll(int value) {
		memset(list, value ? -1 : 0, (num + 7) >> 3);
	}

	void AddGrow(int value) {
		if (num == cap) {
			int numWords = NumWordsForBits(cap) * 2 + 1;
			assert(numWords > 0 && CapacityInWords(numWords) > cap);
			cap = CapacityInWords(numWords);
			list = (TWord*)realloc(list, numWords * sizeof(TWord));
			num = idMath::Imax(num, 0);
		}
		assert(num < cap);
		int idx = num++;
		SetBit(idx, value);
	}

	ID_FORCE_INLINE int Num() const {
		return num;
	}
	ID_FORCE_INLINE TWord *Ptr() {
		return list;
	}
	ID_FORCE_INLINE const TWord *Ptr() const {
		return list;
	}

	ID_FORCE_INLINE TWord IfBit(int index) const {
		assert(unsigned(index) < unsigned(num));
		return IfBit(list, index);
	}
	ID_FORCE_INLINE int GetBit(int index) const {
		assert(unsigned(index) < unsigned(num));
		return GetBit(list, index);
	}
	ID_FORCE_INLINE void SetBitTrue(int index) {
		assert(unsigned(index) < unsigned(num));
		SetBitTrue(list, index);
	}
	ID_FORCE_INLINE void SetBitFalse(int index) {
		assert(unsigned(index) < unsigned(num));
		SetBitFalse(list, index);
	}
	ID_FORCE_INLINE void InvertBit(int index) {
		assert(unsigned(index) < unsigned(num));
		InvertBit(list, index);
	}
	ID_FORCE_INLINE void SetBit(int index, int value) {
		assert(unsigned(index) < unsigned(num));
		SetBit(list, index, value);
	}
	ID_FORCE_INLINE void SetBitsTrue(int beg, int end) {
		assert(beg >= 0 && end <= num);
		SetBitsTrue(list, beg, end);
	}
	ID_FORCE_INLINE void SetBitsFalse(int beg, int end) {
		assert(beg >= 0 && end <= num);
		SetBitsFalse(list, beg, end);
	}
	ID_FORCE_INLINE void InvertBits(int beg, int end) {
		assert(beg >= 0 && end <= num);
		InvertBits(list, beg, end);
	}
	ID_FORCE_INLINE void SetBitsSame(int beg, int end, int value) {
		assert(beg >= 0 && end <= num);
		SetBitsSame(list, beg, end, value);
	}
	ID_FORCE_INLINE void SetBitsWord(int beg, int end, TWord values) {
		assert(beg >= 0 && end <= num);
		SetBitsWord(list, beg, end, values);
	}

private:
	TWord *list;
	int num;		// logical number of bits!
	int cap;		// maximum allowed value of "num" without growth
};

typedef idBitArray<size_t> idBitArrayDefault;
