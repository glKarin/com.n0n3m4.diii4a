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
#include "precompiled.h"

#include "BitArray.h"

#include "../tests/testing.h"

template<class TWord> static void TestStress(int size, int queries) {
	idList<bool> check;
	idBitArray<TWord> bitset;

	check.SetNum(size);
	check.FillZero();
	bitset.Init(size);
	bitset.SetBitsSameAll(false);

	auto Compare = [&]() -> void {
		CHECK(bitset.Num() == check.Num());
		for (int i = 0; i < check.Num(); i++)
			CHECK(bitset.GetBit(i) == (check[i] ? 1 : 0));
	};
	Compare();

	idRandom rnd;
	for (int q = 0; q < queries; q++) {
		int currType = -1;

		if (size > 0 && rnd.RandomFloat() < 0.7) {
			int type = rnd.RandomInt() % 5;
			int idx = rnd.RandomInt() % size;

			if (type == 0) {
				bool ok = true;
				ok &= (check[idx] ? 1 : 0) == bitset.GetBit(idx);
				ok &= check[idx] == !!bitset.IfBit(idx);
				if (!ok)
					CHECK(ok);
			}
			else if (type == 1) {
				bitset.SetBitFalse(idx);
				check[idx] = false;
			}
			else if (type == 2) {
				bitset.SetBitTrue(idx);
				check[idx] = true;
			}
			else if (type == 3) {
				bitset.InvertBit(idx);
				check[idx] = !check[idx];
			}
			else if (type == 4) {
				bool value = !(rnd.RandomInt() % 2);
				bitset.SetBit(idx, value);
				check[idx] = value;
			}
			currType = type;
		}
		else {
			int type = rnd.RandomInt() % 5;
			int beg = rnd.RandomInt() % (size + 1);
			int end = rnd.RandomInt() % (size + 1);
			if (beg > end) idSwap(beg, end);

			else if (type == 0) {
				bitset.SetBitsFalse(beg, end);
				for (int i = beg; i < end; i++) check[i] = false;
			}
			else if (type == 1) {
				bitset.SetBitsTrue(beg, end);
				for (int i = beg; i < end; i++) check[i] = true;
			}
			else if (type == 2) {
				bitset.InvertBits(beg, end);
				for (int i = beg; i < end; i++) check[i] = !check[i];
			}
			else if (type == 3) {
				bool value = !(rnd.RandomInt() % 2);
				bitset.SetBitsSame(beg, end, value);
				for (int i = beg; i < end; i++) check[i] = value;
			}
			else if (type == 4) {
				int len = rnd.RandomInt() % (sizeof(TWord) * 8);
				end = idMath::Imin(end, beg + len);
				len = end - beg;
				TWord values = 0;
				values <<= 15;
				values ^= rnd.RandomInt();
				values <<= 15;
				values ^= rnd.RandomInt();
				values <<= 15;
				values ^= rnd.RandomInt();
				values <<= 15;
				values ^= rnd.RandomInt();
				values <<= 15;
				values ^= rnd.RandomInt();
				values &= (1ULL << len) - 1;
				bitset.SetBitsWord(beg, end, values);
				for (int i = beg; i < end; i++) check[i] = (values >> (i - beg)) & 1;
			}
			currType = 10 + type;
		}

		// very slow, but allows to see which call broke it
		//Compare();
	}
	Compare();
}

TEST_CASE("idBitArray: stress"
) {
	static const int MAX_SIZE = 100;
	static const int QUERY_NUM = 10000;
	for (int size = 0; size <= MAX_SIZE; size++) {
		TestStress<uint8_t>(size, QUERY_NUM);
		TestStress<uint16_t>(size, QUERY_NUM);
		TestStress<uint32_t>(size, QUERY_NUM);
		TestStress<uint64_t>(size, QUERY_NUM);
	}
}
