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
/********************************************************
* 
* CLASS DESCRIPTION: 
* CMatrixSq is a container for square matrices (n x n)
*
**********************************************************/

#ifndef MATRIXSQ_H
#define MATRIXSQ_H

#include "DarkModGlobals.h"
#include <algorithm>

template <class Type>
class CMatrixSq
{
	std::size_t m_Dim;
	std::vector<Type> m_Data;

public:
	// Default constructor
	CMatrixSq();

	// Constructor taking the dimension as argument
	explicit CMatrixSq(std::size_t dim);

	virtual ~CMatrixSq<Type>();

	/**
	* Initialize the square matrix to hold dim elements.
	* NOTE: A 1x1 matrix has dimension 1, NOT zero!
	**/
	bool Init(std::size_t dim);

	void Clear();

	/** 
	 * greebo: Fills the whole matrix with the given value.
	 * Does not change matrix dimensions.
	 */
	void Fill(const Type& src);

	/**
	* Set the appropriate entry to the provided type
	*
	* The following applies to derived class CMatRUT only:
	* NOTE: The indices provided here are those of a normal matrix
	* HOWEVER, columns must be greater than rows due to matrix being
	* right upper triangular (entries with row > col are empty)
	* 
	* CsndPropLoader handles this with another function that reverses the
	* indices if row is greater than column.
	**/
	bool Set(std::size_t row, std::size_t col, const Type& src);

	/**
	* Returns a pointer to the entry for the given 2d indices
	* Again gives an error if row > col, because this entry is empty.
	**/
	const Type& Get(std::size_t row, std::size_t col) const;

	const Type &operator()(std::size_t row, std::size_t col) const {
		return m_Data[row * m_Dim + col];
	}
	Type &operator()(std::size_t row, std::size_t col) {
		return m_Data[row * m_Dim + col];
	}

	/**
	* Returns the dimension (eg, returns 3 for 3x3 matrix)
	**/
	int Dim();

	// STL-like size operator to retrieve the matrix dimension
	std::size_t size() const;
	
	/**
	* Returns true if the private member matrix has been deleted
	**/
	bool IsCleared();

	/**
	* The following two functions save/restore the matrix to/from a
	* save file.
	**/
	void Save(idSaveGame *savefile) const;
	void Restore(idRestoreGame *savefile);
};

/********************************************************
* CLASS DESCRIPTION: 
*
* CMatRUT : CMatrixSq - Right upper triangular matrix
* (the matrix diagonals and lower 'triangle' are all zero 
*  and not stored or accessed)
*
* Derived class of CMatrixSq.
**********************************************************/

template <class Type>
class CMatRUT : 
	public CMatrixSq<Type>
{
public:
	CMatRUT() : 
		CMatrixSq<Type>()
	{}

	explicit CMatRUT(std::size_t dim) : 
		CMatrixSq<Type>(dim)
	{}

	/**
	* Works like CMatrixSq::Set, except it automatically reverses the indices
	* if row > column
	**/
	bool SetRev(std::size_t row, std::size_t col, Type &src);

	/**
	* Works like CMatrixSq::Get, except it automatically reverses the indices
	* if row > column
	**/
	const Type& GetRev(std::size_t row, std::size_t col) const;
};


/****************************************************************
* Begin CMatrixSq Implementation:
*****************************************************************/

// Default constructor
template <class Type>
inline CMatrixSq<Type>::CMatrixSq()
{
	DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("CMatrixSq constructor called, set vars.\r" );
	m_Dim = 0;
}

template <class Type>
inline CMatrixSq<Type>::CMatrixSq( std::size_t dim )
{
	Init(dim);
}

template <class Type>
inline CMatrixSq<Type>::~CMatrixSq()
{
	Clear();
}

template <class Type>
inline void CMatrixSq<Type>::Fill(const Type& src)
{
	std::fill(m_Data.begin(), m_Data.end(), src);
}

template <class Type>
inline bool CMatrixSq<Type>::Set(std::size_t row, std::size_t col, const Type &src )
{
	// Check bounds and resize if necessary
	if (row >= m_Dim || col >= m_Dim) {
		// Resize and preserve values
		std::size_t largerDim = (row > col) ? row + 1 : col + 1;

		std::vector<Type> newData(largerDim * largerDim);
		for (std::size_t r = 0; r < m_Dim; r++)
			std::copy_n(m_Data.begin() + m_Dim * r, m_Dim, newData.begin() + largerDim * r);

		m_Dim = largerDim;
		m_Data = std::move(newData);
	}

	// Assignment
	(*this)(row, col) = src;

	return true; // always succeeds
}

template <class Type>
inline const Type& CMatrixSq<Type>::Get(std::size_t row, std::size_t col) const
{
	assert(row < m_Dim && col < m_Dim);

	return (*this)(row, col);
}

template <class Type>
inline int CMatrixSq<Type>::Dim()
{
	return static_cast<int>(m_Dim);
}

template <class Type>
inline std::size_t CMatrixSq<Type>::size() const
{
	return m_Dim;
}

template <class Type>
inline bool CMatrixSq<Type>::IsCleared( void )
{
	return (m_Dim == 0);
}
	
template <class Type>
inline bool CMatrixSq<Type>::Init( std::size_t dim )
{
	DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("Initializing matrix of dimension %d with %d elements.\r", static_cast<int>(dim));

	m_Dim = dim;
	m_Data.assign(dim * dim, 0);

	return true;
}

template <class Type>
inline void CMatrixSq<Type>::Clear( void )
{
	m_Dim = 0;
	m_Data.clear();
}

template<class Type> static inline void SaveMatrixElement( idSaveGame *savefile, const Type &elem ) {
	elem.Save(savefile);
}
template<class Type> static inline void RestoreMatrixElement( idRestoreGame *savefile, Type &elem ) {
	elem.Restore(savefile);
}
static inline void SaveMatrixElement( idSaveGame *savefile, const int &elem ) {
	savefile->WriteInt(elem);
}
static inline void RestoreMatrixElement( idRestoreGame *savefile, int &elem ) {
	savefile->ReadInt(elem);
}
static inline void SaveMatrixElement( idSaveGame *savefile, const float &elem ) {
	savefile->WriteFloat(elem);
}
static inline void RestoreMatrixElement( idRestoreGame *savefile, float &elem ) {
	savefile->ReadFloat(elem);
}

template<class Type>
inline void CMatrixSq<Type>::Save( idSaveGame *savefile ) const
{
	savefile->WriteUnsignedInt(static_cast<unsigned int>(size()));

	for (std::size_t i = 0; i < size(); ++i) 
	{
		for (std::size_t j = 0; j < size(); ++j) 
		{
			SaveMatrixElement(savefile, (*this)(i,j));
		}
	}
}

template<class Type>
inline void CMatrixSq<Type>::Restore( idRestoreGame *savefile )
{
	unsigned int sz = 0;
	savefile->ReadUnsignedInt(sz);

	// Resize the matrix accordingly
	m_Dim = sz;
	m_Data.assign(m_Dim * m_Dim, 0);

	for (std::size_t i = 0; i < size(); ++i) 
	{
		for (std::size_t j = 0; j < size(); ++j) 
		{
			RestoreMatrixElement(savefile, (*this)(i,j));
		}
	}
}

template <class Type>
inline const Type& CMatRUT<Type>::GetRev(std::size_t row, std::size_t col) const
{
	if (row > col)
	{
		std::size_t temp = row;
		row = col;
		col = temp;
	}

	return CMatrixSq<Type>::Get(row, col);
}

template <class Type>
inline bool CMatRUT<Type>::SetRev(std::size_t row, std::size_t col, Type& src )
{
	if( row > col )
	{
		std::size_t temp = row;
		row = col;
		col = temp;
	}

	CMatrixSq<Type>::Set(row, col, src);

	return true;
}

#endif
