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
#include "HashMap.h"


#include "../tests/testing.h"

TEST_CASE("idHashMap<int int>: simple"
) {
	idHashMap<int, int> table;
	const auto &ctable = table;

	// {}
	CHECK(ctable.Num() == 0);
	CHECK(ctable.CellsNum() == 0);
	CHECK(ctable.IsDead() == true);
	CHECK(ctable.Ptr() == nullptr);
	CHECK(table.Ptr() == nullptr);
	CHECK(ctable.Find(42) == nullptr);
	//yeah, that's counter-intuitive, but ANY query forces table to allocate buffer
	CHECK(ctable.IsDead() == false);
	CHECK(ctable.Get(123) == 0);
	CHECK(ctable.Get(123, 666) == 666);
	CHECK(table.Remove(123) == false);

	table[7] = 42;

	// {7: 42}
	CHECK(ctable.Num() == 1);
	CHECK(ctable.CellsNum() >= 1);
	CHECK(ctable.IsDead() == false);
	CHECK(ctable.Ptr() != nullptr);
	CHECK(table.Ptr() == ctable.Ptr());
	CHECK(ctable.Find(42) == nullptr);
	CHECK(ctable.Find(7) != nullptr);
	CHECK(ctable.Find(7)->value == 42);
	CHECK(ctable.Get(123) == 0);
	CHECK(ctable.Get(123, 666) == 666);
	CHECK(ctable.Get(7) == 42);
	CHECK(ctable.Get(7, 666) == 42);

	table[0] = 12345;
	CHECK(table.Get(123456789) == 0);
	table[13];

	// {7: 42, 0: 12345, 13: 0}
	CHECK(ctable.Num() == 3);
	CHECK(ctable.Find(99) == nullptr);
	CHECK(ctable.Find(7) != nullptr);
	CHECK(ctable.Find(7)->value == 42);
	CHECK(ctable.Get(0) == 12345);
	CHECK(ctable.Get(0, 666) == 12345);
	CHECK(ctable.Get(13) == 0);
	CHECK(ctable.Get(13, 666) == 0);
	CHECK(ctable.Get(10) == 0);
	CHECK(ctable.Get(10, 666) == 666);
	CHECK(table.Remove(10) == false);

	CHECK(table.Remove(13) == true);
	// {7: 42, 0: 12345}
	CHECK(ctable.Num() == 2);
	CHECK(ctable.Get(7) == 42);
	CHECK(ctable.Get(0) == 12345);
	CHECK(table.Set(13, 1313) == true);
	// {7: 42, 0: 12345, 13: 1313}
	CHECK(table.Get(13) == 1313);
	CHECK(table.Get(0) == 12345);
	CHECK(table.Set(13, 3131) == false);
	// {7: 42, 0: 12345, 13: 3131}
	CHECK(table.Get(13) == 3131);
	CHECK(table.Get(7) == 42);
	CHECK(table.Set(45, 450) == true);
	// {7: 42, 0: 12345, 13: 3131, 45: 450}
	CHECK(table.Set(45, 54) == false);
	// {7: 42, 0: 12345, 13: 3131, 45: 54}

	CHECK(table.Remove(0) == true);
	// {7: 42, 13: 3131, 45: 54}
	CHECK(table.Remove(100) == false);
	{
	auto &cell = table.FindCell(100);
	cell.key = 100;
	cell.value = 10000;
	table.CellAdded(cell);
	}
	// {7: 42, 13: 3131, 45: 54, 100: 10000}
	CHECK(table.Num() == 4);

	idStrList strings;
	for (int i = 0; i < ctable.CellsNum(); i++) {
		const auto &elem = ctable.Ptr()[i];
		if (ctable.IsEmpty(elem))
			continue;
		strings.AddGrow(va("[%d: %d]", elem.key, elem.value));
	}
	strings.Sort();
	idStr str = "";
	for (int i = 0; i < strings.Num(); i++)
		str += strings[i];
	CHECK(str == "[100: 10000][13: 3131][45: 54][7: 42]");

	{
	auto &cell = table.FindCell(45);
	CHECK(table.IsEmpty(cell) == false);
	CHECK(cell.key == 45);
	CHECK(cell.value == 54);
	auto &cell2 = table.FindCell(455555);
	CHECK(table.IsEmpty(cell2) == true);
	CHECK(cell2.key == INT_MIN);
	auto &cell3 = table.FindCell(45);
	table.CellRemove(cell3);
	// {7: 42, 13: 3131, 100: 10000}
	CHECK(table.Num() == 3);
	}

	CHECK(table.AddIfNew(386, 680) == true);
	// {7: 42, 13: 3131, 100: 10000, 386, 680}
	CHECK(table.Num() == 4);
	CHECK(table.Get(386) == 680);
	CHECK(table.AddIfNew(386, 860) == false);
	CHECK(table.Get(386) == 680);
	CHECK(table.AddIfNew(386, 111) == false);
	CHECK(table.Get(386) == 680);
	CHECK(table.AddIfNew(101, 10101) == true);
	// {7: 42, 13: 3131, 100: 10000, 386, 680, 101: 10101}
	CHECK(table.Get(101) == 10101);

	int oldCells = table.CellsNum();
	auto oldPtr = table.Ptr();
	table.Clear();
	// {}
	CHECK(table.Num() == 0);
	CHECK(table.CellsNum() == oldCells);
	CHECK(table.Ptr() == oldPtr);
	CHECK(table.IsDead() == false);
	table.AddIfNew(1, 1);
	// {1: 1}
	CHECK(table.CellsNum() == oldCells);
	CHECK(table.Ptr() == oldPtr);
	for (int i = 0; i < 10; i++)
		table[i] = i*i;
	CHECK(table.Num() == 10);
	CHECK((0.0 + table.Num()) / table.CellsNum() <= (0.0 + HASHMAP_LOADFACTOR_DEFAULT) / 256);
	// {...}

	table.ClearFree();
	// {}
	CHECK(table.Num() == 0);
	CHECK(table.CellsNum() == 0);
	CHECK(table.Ptr() == nullptr);
	CHECK(table.IsDead() == true);
	CHECK(table.Set(123456789, 987654321) == true);
	// {123456789: 987654321}
	CHECK(table.AddIfNew(111, 222) == true);
	// {123456789: 987654321, 111: 222}
	CHECK(table.Num() == 2);
	CHECK(table.Get(0) == 0);
	CHECK(table.FindCell(0).key == INT_MIN);
}

TEST_CASE("idHashMap<int int>: lifetime"
) {
	idHashMap<int, int> a;
	const auto &aref = a;
	for (int i = 0; i < 100; i++)
		a.Set(i, i*i);

	{	//copy constructor
		idHashMap<int, int> b = aref;
		a.ClearFree();
		idHashMap<int, int> c = b;
		CHECK(c.Num() == 100);
		for (int i = 0; i < 100; i++) {
			CHECK(c.Get(i) == i*i);
			CHECK(c.Get(i + 100) == 0);
		}
		idHashMap<int, int> d = a;
		CHECK(a.IsDead() == true);
		CHECK(d.IsDead() == true);
		CHECK(d.Num() == 0);
	}

	a.Clear();
	for (int i = 0; i < 100; i++)
		a.Set(i, i*i);

	{	//copy assignment
		idHashMap<int, int> d;
		d = aref;
		CHECK(d.Num() == 100);
		for (int i = 0; i < 100; i++) {
			CHECK(d.Get(i) == i*i);
			CHECK(d.Get(i + 100) == 0);
		}
		d.Clear();
		for (int i = 0; i < 1000; i++)
			d.Set(i + 100, i + 200);
		CHECK(d.Num() == 1000);
		for (int i = 0; i < 1000; i++)
			CHECK(d.Get(i + 100) == i + 200);
		d = a;
		for (int q = 0; q < 2; q++) {
			CHECK(d.Num() == 100);
			for (int i = 0; i < 100; i++) {
				CHECK(d.Get(i) == i*i);
				CHECK(d.Get(i + 100) == 0);
			}
			d = d;
		}
		idHashMap<int, int> b = d;
		d.ClearFree();
		b = d;
		CHECK(b.IsDead() == true);
		CHECK(b.Num() == 0);
		d = b;
		CHECK(d.IsDead() == true);
		CHECK(d.Num() == 0);
	}

	a.Clear();
	for (int i = 0; i < 100; i++)
		a.Set(i, i*i);

	{	//Swap
		idHashMap<int, int> b;
		for (int i = 0; i < 70; i += 3)
			b.Set(i, i^5);
		auto aptr = a.Ptr();
		auto bptr = b.Ptr();
		b.Swap(a);
		CHECK(b.Num() == 100);
		for (int i = 0; i < 100; i++) {
			CHECK(b.Get(i) == i*i);
			CHECK(b.Get(i + 100) == 0);
		}
		CHECK(a.Num() == 24);
		for (int i = 0; i < 70; i+=3)
			CHECK(a.Get(i) == (i^5));
		CHECK(a.Ptr() == bptr);
		CHECK(b.Ptr() == aptr);
	}

	//Reserve
	a.Reserve(5);
	CHECK(a.Num() == 24);
	CHECK(a.CellsNum() > 24);
	for (int i = 0; i < 70; i+=3)
		CHECK(a.Get(i) == (i^5));
	a.Reserve(8000);
	CHECK(a.Num() == 24);
	CHECK(a.CellsNum() * (HASHMAP_LOADFACTOR_DEFAULT / 256.0) >= 8000);
	auto aptr = a.Ptr();
	a.Clear();
	for (int i = 0; i < 8000; i++)
		a.Set(i, i*i);
	CHECK(a.Num() == 8000);
	CHECK(a.Ptr() == aptr);

	a.ClearFree();
	a.Reserve(0);
	CHECK(a.Get(17) == 0);
	a.Set(5, 6);
	CHECK(a.Get(17) == 0);
	a.ClearFree();
	a.Reserve(1);
	CHECK(a.Get(17) == 0);
	a.Set(5, 6);
	CHECK(a.Get(17) == 0);
}

TEST_CASE("idHashMap: types"
) {
	CHECK(idHashDefaultEmpty<uint64>::Get() == UINT64_MAX);
	CHECK(idHashDefaultEmpty<uint32>::Get() == UINT32_MAX);
	CHECK(idHashDefaultEmpty<uint16>::Get() == UINT16_MAX);
	CHECK(idHashDefaultEmpty<uint8>::Get() == UINT8_MAX);
	CHECK(idHashDefaultEmpty<int64>::Get() == INT64_MIN);
	CHECK(idHashDefaultEmpty<int32>::Get() == INT32_MIN);
	CHECK(idHashDefaultEmpty<int16>::Get() == INT16_MIN);
	CHECK(idHashDefaultEmpty<int8>::Get() == INT8_MIN);
	CHECK(ptrdiff_t(idHashDefaultEmpty<void*>::Get()) == ptrdiff_t(-1));
	CHECK(ptrdiff_t(idHashDefaultEmpty<int*>::Get()) == ptrdiff_t(-1));

	idHashMap<uint64, uint8> H_u64_u8;
	H_u64_u8[0xDEADBEEFDADAB0DAU] = 0xCCU;
	CHECK(H_u64_u8.Get(0xDEADBEEFDADAB0DAU) == 0xCCU);
	idHashMap<uint32, uint16> H_u32_u16;
	H_u32_u16.Set(0xFEFEFEFEU, 32000);
	CHECK(H_u32_u16.Get(0xFEFEFEFEU) == 32000);
	idHashMap<uint16, uint32> H_u16_u32;
	H_u16_u32.AddIfNew(40000U, 123123123);
	CHECK(H_u16_u32.Get(40000U) == 123123123);
	idHashMap<uint8, uint64> H_u8_u64;
	H_u8_u64.AddIfNew(0xCDU, uint64(1e+18));
	CHECK(H_u8_u64.Get(0xCDU) == uint64(1e+18));

	idHashMap<int64, int8> H_i64_i8;
	H_i64_i8[-1000000000000000000LL] = 't';
	CHECK(H_i64_i8.Get(-1000000000000000000LL) == 't');
	idHashMap<int32, int16> H_i32_i16;
	H_i32_i16.Set(-123123123, -20000);
	CHECK(H_i32_i16.Get(-123123123) == -20000);
	idHashMap<int16, int32> H_i16_i32;
	H_i16_i32.AddIfNew(-20000, -123123123);
	CHECK(H_i16_i32.Get(-20000) == -123123123);
	idHashMap<int8, int64> H_i8_i64;
	H_i8_i64.AddIfNew('a', int64(-1e+18));
	CHECK(H_i8_i64.Get('a') == int64(-1e+18));

	int temp = 13;
	idHashMap<void*, int> H_vptr;
	H_vptr.AddIfNew(&temp, 123);
	idHashMap<int*, int> H_iptr;
	H_iptr.AddIfNew(&temp, 123);
}

struct EqualMod {
	int MOD;
	int empty;
	EqualMod(int m = 0, int e = INT_MIN) : MOD(m), empty(e) {}
	bool operator() (int a, int b) const {
		if (a == empty) return b == empty;
		if (b == empty) return a == empty;
		return (a - b) % MOD == 0;
	}
};
struct HashMod {
	int MOD;
	HashMod(int m = 0) : MOD(m) {}
	int operator() (int a) const {
		int res = (int64(a) * 0xDEADBEEFU) % MOD;
		if (res < 0) res += MOD;
		return res;
	}
};
TEST_CASE("idHashMap<int int>: custom"
) {
	typedef idHashMap<int, int, HashMod, EqualMod> ModMap;

	ModMap tmod1000;
	tmod1000.SetEmpty(-666);
	tmod1000.SetFunctions(1000, EqualMod(1000, -666));
	tmod1000.SetLoadFactor(0.1f);

	CHECK(tmod1000.FindCell(0).key == -666);
	CHECK(tmod1000.AddIfNew(666, 1) == true);
	CHECK(tmod1000.AddIfNew(334, 3) == true);
	CHECK(tmod1000.AddIfNew(-1666, 2) == false);
	CHECK(tmod1000.AddIfNew(1666, 200) == false);
	CHECK(tmod1000.AddIfNew(333, 3) == true);
	CHECK(tmod1000.AddIfNew(1000333, 3) == false);
	CHECK(tmod1000.Num() == 3);
	CHECK(tmod1000.CellsNum() >= 10 * tmod1000.Num());

	ModMap tmod13;
	tmod13.SetFunctions(13, 13);
	CHECK(tmod13.FindCell(123456789).key == INT_MIN);
	for (int i = 0; i < 30; i++)
		tmod13.AddIfNew(i, i);
	CHECK(tmod13.Num() == 13);
	CHECK(tmod13.Find(123456789) != nullptr);

	//copy constructor copies empty & funcs & maxload
	ModMap tmod1000copy = tmod1000;
	CHECK(tmod1000copy.FindCell(13).key == -666);
	CHECK(tmod1000copy.Get(1334) == 3);
	tmod1000copy.ClearFree();
	for (int i = 0; i < 5; i++)
		tmod1000copy.AddIfNew(i, 3*i);
	CHECK(tmod1000copy.CellsNum() >= 10 * tmod1000copy.Num());

	//copy assignment copies empty & funcs & maxload
	ModMap tmp = tmod13;
	CHECK(tmp.Get(17) == 4);
	tmp = tmod1000;
	CHECK(tmp.Get(1334) == 3);
	CHECK(tmp.FindCell(1111).key == -666);
	tmp.ClearFree();
	for (int i = 0; i < 5; i++)
		tmp.AddIfNew(i, 3*i);
	CHECK(tmp.CellsNum() >= 10 * tmp.Num());

	//swap changes empty & funcs & maxload
	ModMap tmod13copy = tmod1000;
	tmod1000copy = tmod13;
	tmod13copy.Swap(tmod1000copy);
	tmod1000copy.ClearFree();
	tmod13copy.ClearFree();
	CHECK(tmod1000copy.FindCell(0).key == -666);
	CHECK(tmod13copy.FindCell(0).key == INT_MIN);
	for (int i = 0; i < 1000; i++) {
		tmod1000copy[i] = i;
		tmod13copy[i] = i;
	}
	CHECK(tmod1000copy.Num() == 1000);
	CHECK(tmod13copy.Num() == 13);
	CHECK(tmod1000copy.CellsNum() >= 10 * tmod1000copy.Num());
	CHECK(tmod13copy.CellsNum() <= 3 * tmod13copy.Num());
}

TEST_CASE("idHashMap<idStr int>: simple"
) {
	idHashMap<idStr, int> hmap;
	hmap["hello"] = 7;
	CHECK(hmap.AddIfNew("hi", 17) == true);
	CHECK(hmap.AddIfNew("hi", 34) == false);
	CHECK(hmap.Set("nope", 13) == true);
	CHECK(hmap.Set("nope", 31) == false);
	CHECK(hmap.Set("", -1) == true);

	CHECK(hmap.Num() == 4);
	CHECK(hmap.Get("") == -1);
	idStr q(12345);
	CHECK(hmap.Get(q) == 0);
	CHECK(hmap.Get("nope") == 31);
	CHECK(hmap.Get("hi") == 17);
	q = "hello";
	CHECK(hmap.Get(q) == 7);
}

TEST_CASE("idHashMap<ptr int>: simple"
) {
	char buffer[] = "0123456789";

	idHashMap<char*, int> hmap;
	hmap[buffer + 3] = 7;
	CHECK(hmap.AddIfNew(buffer + 7, 17) == true);
	CHECK(hmap.AddIfNew(buffer + 7, 34) == false);
	CHECK(hmap.Set(buffer + 2, 13) == true);
	CHECK(hmap.Set(buffer + 2, 31) == false);
	CHECK(hmap.Set(nullptr, -1) == true);

	CHECK(hmap.Num() == 4);
	CHECK(hmap.Get(nullptr) == -1);
	char tmp;
	CHECK(hmap.Get(&tmp) == 0);
	CHECK(hmap.Get(buffer + 2) == 31);
	CHECK(hmap.Get(buffer + 7) == 17);
	CHECK(hmap.Get(buffer + 3) == 7);
}

TEST_CASE("idHashDict: simple"
) {
	idHashMapDict hmap;
	hmap["hello"] = "7";
	CHECK(hmap.AddIfNew("hi", "17") == true);
	CHECK(hmap.AddIfNew("Hi", "34") == false);
	CHECK(hmap.Set("nope", "13") == true);
	CHECK(hmap.Set("NOPE", "31") == false);
	CHECK(hmap.Set("", "-1") == true);

	CHECK(hmap.Num() == 4);
	CHECK(hmap.Get("") == "-1");
	idStr q(12345);
	CHECK(hmap.Get(q, "missing") == "missing");
	CHECK(hmap.Get("NopE") == "31");
	CHECK(hmap.Get("hI") == "17");
	q = "HelLO";
	CHECK(hmap.Get(q) == "7");
}
