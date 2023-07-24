// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __IDLIB_HANDLE_H__
#define __IDLIB_HANDLE_H__

namespace sdUtility {
	/*
	============
	sdHandle
	============
	*/
	template< class T, T invalidValue >
	class sdHandle {
	public:
		typedef T valueType_t;

		sdHandle() :
		value ( invalidValue ) {}

		sdHandle( const T& rhs ) :
		value ( rhs ) {}

		sdHandle& operator=( const T& rhs ) {
			value = rhs;
			return *this;
		}

		bool operator==( const sdHandle< T, invalidValue >& rhs ) {
			return value == rhs.value;
		}

		bool operator!() {
			return !IsValid();
		}

		bool IsValid() const { 
			return value != INVALID_VALUE;
		}

		void Release() { 
			value = INVALID_VALUE;
		}

		operator T() const {
			return value;
		}

	private:
		T value;
		static T INVALID_VALUE;		
	};
	template< class T, T invalidValue > T sdHandle< T, invalidValue >::INVALID_VALUE = invalidValue;
}

#endif // __IDLIB_HANDLE_H__
