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

#ifndef RAW_VECTOR_H
#define RAW_VECTOR_H

/**
 * std::vector-like raw buffer class.
 * Contains raw bytes, uses C allocation routines.
 * Grows exponentially, never shrinks.
 * No initialization or checks are done.
 * If malloc error happens then Error() is called
 * Only the most necessary methods are implemented.
 */
class CRawVector {
	static const int INITIAL_CAPACITY = 16;
public:
	CRawVector();
	~CRawVector();

	ID_INLINE int size() const { return m_Size; }
	ID_INLINE char &operator[] (int i) { return m_Pointer[i]; }
	ID_INLINE const char &operator[] (int i) const { return m_Pointer[i]; }
	ID_INLINE char *data() { return m_Pointer; }
	ID_INLINE const char *data() const { return m_Pointer; }

	inline void resize(int newSize)
	{
		if (newSize > m_Capacity) reallocate(newSize);
		m_Size = newSize;
	}
	inline void reserve(int newSize)
	{
		if (newSize > m_Capacity) reallocate(newSize);
	}

	//note: does not free memory!
	void clear();

private:
	///Pointer to data (allocated with malloc).
	char *m_Pointer;
	///Size of vector (number of bytes).
	int m_Size;
	///Size of allocated buffer.
	int m_Capacity;

	///Reallocate the memory buffer
	void reallocate(int newSize);

	//Non-copyable
	CRawVector(const CRawVector &other);
	CRawVector &operator= (const CRawVector &other);
};

namespace std {
	void swap(CRawVector &a, CRawVector &b);
}

#endif
