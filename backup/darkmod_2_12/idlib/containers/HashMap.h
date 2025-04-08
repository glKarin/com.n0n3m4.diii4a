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

#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#include <type_traits>

//used for key/value pairs in idHashMap
//(OdKeyValue already exists: it is used in idDict, strings-only)
template<class Key, class Value> struct idKeyVal {
	Key key;
	Value value;
};

struct idEquality {
	template<class A, class B> ID_FORCE_INLINE bool operator() (const A &a, const B &b) const {
		return a == b;
	}
};
struct idEqualityStrI {
	ID_INLINE bool operator() (const char *a, const char *b) const {
		return idStr::Icmp(a, b) == 0;
	}
};

//fallback hash function: use std::hash
//note that it might be very bad hash function e.g. for integers!
template<class Key, class = void> struct idHashFunction {
	uint32 operator()(const Key &key) const {
		return (uint32)std::hash<Key>()(key);
	}
};
//hash function for integers
template<class Key> struct idHashFunction<Key, std::enable_if_t<
	std::is_integral<Key>::value
>> {
	static const uint32 MULTIPLIER = 2654435769U;	//golden ratio * 2^32
	ID_FORCE_INLINE uint32 operator()(Key key) const {
		//note: depend only on lower 32 bits, all the higher ones are ignored
		//it is a flaw, but I doubt we would ever get caught by this
		return MULTIPLIER * uint32(key);
	}
};
//hash function for pointers --- same as for integers
template<class Key> struct idHashFunction<Key, std::enable_if_t<
	std::is_pointer<Key>::value
>> {
	ID_FORCE_INLINE uint32 operator()(Key key) const {
		return idHashFunction<size_t>()(size_t(key));
	}
};
//hash function for idStr (case-sensitive)
template<class Key> struct idHashFunction<Key, std::enable_if_t<
	std::is_same<Key, idStr>::value
>> {
	uint32 operator()(const Key &key) const {
		uint64 hash = idStr::HashPoly64(key.c_str());
		return idHashFunction<uint64>()(hash);
	}
};
//hash function for idStr (case-INsensitive)
struct idHashFunctionStrI {
	ID_INLINE uint32 operator()(const char *key) const {
		uint64 hash = idStr::IHashPoly64(key);
		return idHashFunction<uint64>()(hash);
	}
};

//idHashMap uses default-constructed value to denote "empty" cells by default
template<class Key, class = void> struct idHashDefaultEmpty {
	static Key Get() { return Key(); }
};
//minimum value (signed bit set) is default "empty" value in idHashMap for signed integers
template<class Key> struct idHashDefaultEmpty<Key, std::enable_if_t<
	std::is_integral<Key>::value && std::is_signed<Key>::value
>> {
	static Key Get() { return Key(-1) << (8 * sizeof(Key) - 1); }
};
//-1 (all bits set) is default "empty" value in idHashMap for unsigned integers (and pointers)
template<class Key> struct idHashDefaultEmpty<Key, std::enable_if_t<
	(std::is_integral<Key>::value && !std::is_signed<Key>::value) || std::is_pointer<Key>::value
>> {
	static Key Get() { return Key(-1); }
};
//default "empty" value in idHashMap is idStr with one special character (which should not be in strings)
template<class Key> struct idHashDefaultEmpty<Key, std::enable_if_t<
	std::is_same<Key, idStr>::value
>> {
	static Key Get() { return idStr("\5"); }
};

#define HASHMAP_LOADFACTOR_DEFAULT 192	//load factor 75%

#ifdef _DEBUG
	//enable statistics (e.g. number of queries and collisions) in debug build
	#define HASHMAP_STATINC(name) stat_##name++
#else
	#define HASHMAP_STATINC(name) ((void)0)
#endif



/**
 * Hash table storing key/value pairs for arbitrary types.
 * Can be used as generic hash map (operator[] compatible with std::map included).
 * One value of "key" type must be set as "empty": such empty key must not be passed to any method (except SetEmpty).
 * Suitable for high-performance usage, e.g. used in renderer's "interaction table".
 * Note: all allocated items are always kept in constructed state, just like in idList.
 *
 * This hash table is rather sensitive to bad hash functions.
 * As of now, the following types of key are explicitly supported:
 *   + integers and pointers
 *   + idStr case-sensitive
 *   + idStr case-INsensitive --- see idHashMapIStr
 * In other cases, you might need to implement your own hash function and equality operator.
 * If you do so, please check that collision ratio (stat_Collision / stat_FindCell) does not exceed 3-5.
 *
 * The implementation is similar to google dense map:
 *   1. open addressing: one array to store everything, no linked lists
 *   2. linear probing: neighbor elements are checked on collision
 *   3. backshift removal: NO tombstones mess
 *   4. size is power-of-two
 *   5. upper bits of hash are used for cell index (like in Knuth hash)
 */
template<
	class Key,
	class Value,
	class HashFunction = idHashFunction<Key>,
	class EqualFunction = idEquality
>
class idHashMap {
public:
	typedef idKeyVal<Key, Value> Elem;

	~idHashMap() {
		ClearFree();
	}
	idHashMap()
		: table(nullptr)
		, size(0)
		, count(0)
		, empty(idHashDefaultEmpty<Key>::Get())
		, maxLoad(HASHMAP_LOADFACTOR_DEFAULT)
		, shift(33)
	{}
	idHashMap(const idHashMap &src)	{
		table = nullptr;
		operator=(src);
	}
	void operator= (const idHashMap &src) {
		if (this == &src)
			return;
		ClearFree();
		size = src.size;
		count = src.count;
		empty = src.empty;
		maxLoad = src.maxLoad;
		shift = src.shift;
		hashFunc = src.hashFunc;
		equalFunc = src.equalFunc;
		if (size > 0) {
			table = new Elem[size];
			for (int i = 0; i < size; i++)
				table[i] = src.table[i];
		}
	}
	void Swap(idHashMap &other) {
		idSwap(table, other.table);
		idSwap(size, other.size);
		idSwap(count, other.count);
		idSwap(empty, other.empty);
		idSwap(maxLoad, other.maxLoad);
		idSwap(shift, other.shift);
		idSwap(hashFunc, other.hashFunc);
		idSwap(equalFunc, other.equalFunc);
	}

	//set special "empty" value (frees all resources)
	void SetEmpty(const Key &_empty) {
		if (table)
			ClearFree();
		empty = _empty;
	}
	//set maximum load factor before reallocation (frees all resources)
	void SetLoadFactor(float ratio) {
		if (table)
			ClearFree();
		maxLoad = idMath::ClampInt(1, 255, ratio * 256);
	}
	//set hash function and equality --- in case they contain some data (frees all resources)
	void SetFunctions(const HashFunction &hf, const EqualFunction &eqf = EqualFunction()) {
		if (table)
			ClearFree();
		hashFunc = hf;
		equalFunc = eqf;
	}

	void Reserve(int number, bool cellsNumber = false) {
		if (!cellsNumber)
			number = (uint64(number) << 8) / maxLoad;
		number++;	//ensure one excessive empty cell
		int bits = 1;
		while ((1 << bits) < number)
			bits++;
		int wantedShift = 32 - bits;
		if (wantedShift < shift)
			Reallocate(wantedShift);
	}

	void Clear() {
		count = 0;
		for (int i = 0; i < size; i++)
			table[i].key = empty;
	}
	void ClearFree() {
		size = 0;
		count = 0;
		shift = 33;
		delete[] table;
		table = nullptr;
	}

	int Num() const { return count; }
	int CellsNum() const { return size; }
	//returns true if hash table is not allocated
	//note: dead map is always empty, but empty map is not necessarily dead
	bool IsDead() const { return table == nullptr; }

	//[low-level] can be used in conjunction with IsEmpty to iterate over all elements
	//BEWARE: order of iteration depends on hash function and contained keys
	//if keys are pointers, then iteration order will be non-deterministic!
	const Elem *Ptr() const { return table; }
	Elem *Ptr() { return table; }

	//[low-level] returns reference to the table cell with given key
	//if the returned cell is empty (you can check it with IsEmpty), then key is absent in the table
	ID_FORCE_INLINE Elem &FindCell(const Key &key) {
		if (table == nullptr)
			Reallocate(31);	//2 cells
		HASHMAP_STATINC(FindCell);
		int size1 = size - 1;
		uint32 hash = hashFunc(key);
		for (int curr = hash >> shift; ; curr = (curr + 1) & size1) {
			if (equalFunc(table[curr].key, key) || equalFunc(table[curr].key, empty))
				return table[curr];
			HASHMAP_STATINC(Collision);
		}
	}
	//[low-level] checks whether specified cell is empty
	ID_FORCE_INLINE bool IsEmpty(const Elem &elem) const {
		return equalFunc(elem.key, empty);
	}
	//[low-level] removes specified element (usually found by FindCell)
	//it shifts elements back through the table (i.e. backshift paradigm instead of tombstones)
	void CellRemove(Elem &elem) {
		int last = &elem - table;
		int size1 = size - 1;
		for (int curr = (last + 1) & size1; ; curr = (curr + 1) & size1) {
			if (equalFunc(table[curr].key, empty))
				break;
			uint32 hash = hashFunc(table[curr].key);
			int wanted = (hash >> shift);
			unsigned offset = unsigned(wanted - last - 1) & size1;
			unsigned passed = unsigned(curr - last) & size1;
			if (offset < passed)
				continue;
			table[last] = table[curr];
			last = curr;
			HASHMAP_STATINC(Backshift);
		}
		table[last].key = empty;
		count--;
		HASHMAP_STATINC(CellRemove);
	}
	//[low-level] commits addition of new element at specified call
	//before calling this, you must find empty cell using FindCell, and write new key/value into it
	//may resize the whole table if load factor exceeds limit
	bool CellAdded(Elem &elem) {
		assert(equalFunc(elem.key, empty) == false);
		count++;
		if (count > ((uint64(size) * maxLoad) >> 8)) {
			Reallocate(shift - 1);
			return true;
		}
		HASHMAP_STATINC(CellAdded);
		return false;
	}

	//returns pointer to the element with given key, or NULL if not present
	//note: you may change value, but you must not change key!
	Elem *Find(const Key &key) const {
		Elem &cell = const_cast<idHashMap*>(this)->FindCell(key);
		if (IsEmpty(cell))
			return nullptr;
		return &cell;
	}
	//returns value assigned to specified key, or passed "defaultValue" if not found
	const Value &Get(const Key &key, const Value &defaultValue = Value()) const {
		Elem &cell = const_cast<idHashMap*>(this)->FindCell(key);
		if (IsEmpty(cell))
			return defaultValue;
		return cell.value;
	}

	//returns reference to the value assigned to specified key
	//if not present, then inserts the key with default-constructed value beforehand
	//equivalent to convenient std::map::operator[]
	//prefer methods Get/Set/AddIfNew over operator[], unless operator[] makes your code simpler
	Value &operator[](const Key &key) {
		Elem &cell = FindCell(key);
		if (IsEmpty(cell)) {
			cell.key = key;
			cell.value = Value();
			if (CellAdded(cell))	//search again if table was reallocated
				return FindCell(key).value;
		}
		return cell.value;
	}
	//assign specified value for the given key, overwrite value if already present
	//returns true if key was NOT present
	bool Set(const Key &key, const Value &value) {
		Elem &cell = FindCell(key);
		if (IsEmpty(cell)) {
			cell.key = key;
			cell.value = value;
			CellAdded(cell);
			return true;
		}
		cell.value = value;
		return false;
	}
	//assign specified value for the given key, but only if such key is not present yet
	//returns true if key was NOT present
	bool AddIfNew(const Key &key, const Value &value) {
		Elem &cell = FindCell(key);
		if (IsEmpty(cell)) {
			cell.key = key;
			cell.value = value;
			CellAdded(cell);
			return true;
		}
		return false;
	}

	//remove element with specified key if it exists
	//returns true if such element was removed
	bool Remove(const Key &key) {
		Elem &cell = FindCell(key);
		if (IsEmpty(cell))
			return false;
		CellRemove(cell);
		return true;
	}

private:
	void Reallocate(int wantedShift) {
		assert(wantedShift >= 1 && wantedShift <= 31);
		byte oldShift = wantedShift;
		int oldSize = (1 << (32 - wantedShift));
		Elem *oldTable = new Elem[oldSize];
		for (int i = 0; i < oldSize; i++)
			oldTable[i].key = empty;
		int oldCount = 0;

		idSwap(table, oldTable);
		idSwap(size, oldSize);
		idSwap(count, oldCount);
		idSwap(shift, oldShift);

		for (int i = 0; i < oldSize; i++) {
			if (equalFunc(oldTable[i].key, empty))
				continue;
			Elem &place = FindCell(oldTable[i].key);
			place = oldTable[i];
			CellAdded(place);
			assert(count <= oldCount);
		}
		assert(count == oldCount);

		delete[] oldTable;
	}

	Elem *table;
	int size;
	int count;
	Key empty;		//special "empty" key
	byte maxLoad;	//K in [1..255], factor is K/256;
	byte shift;		//32 - log2(size)
	HashFunction hashFunc;
	EqualFunction equalFunc;

#ifdef _DEBUG
	//can be used to check quality of hash function or to detect typical usage patterns
	//  average probes per query: 1.0 + stat_Collision / stat_FindCell
	//  ratio of additions: stat_CellAdded / stat_FindCell
	//  ratio of removals: stat_CellRemove / stat_FindCell
	int stat_FindCell = 0;		//number of queries: #searches + #additions + #removals
	int stat_Collision = 0;		//number of unsuccessful probes
	int stat_CellAdded = 0;		//number of additions
	int stat_CellRemove = 0;	//number of removals
	int stat_Backshift = 0;		//number of element moves during removal
#endif
};

#undef HASHMAP_STATINC

//idHashMap<idStr, *> with keys compared case-INsensitively
template<class Value> using idHashMapIStr = idHashMap<idStr, Value, idHashFunctionStrI, idEqualityStrI>;
//idHashMap<idStr, idStr> with keys compared case-INsensitively (similar to idDict)
using idHashMapDict = idHashMapIStr<idStr>;

#endif
