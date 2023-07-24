// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __PROPERTY_TRAITS_H__
#define __PROPERTY_TRAITS_H__

namespace sdProperties {

	typedef enum { PT_INVALID = -1, PT_INT = 0, PT_FLOAT, PT_BOOL, PT_STRING, PT_WSTRING, PT_VEC2, PT_VEC3, PT_VEC4, PT_COLOR3, PT_COLOR4, PT_ANGLES, PT_NUM } ePropertyType;
	extern const char* const propertyTypeStrings[];

	/*
	============
	sdPropertyTypeTraits
	stores the property system's type constants
	============
	*/
	template< class T >
	struct sdPropertyTypeTraits {
		typedef T type;
		typedef enum { tag = PT_INVALID };
	};

	template<>
	struct sdPropertyTypeTraits< int > {
		typedef int type;
		typedef enum { tag = PT_INT };
	};
	template<>
	struct sdPropertyTypeTraits< float > {
		typedef float type;
		typedef enum { tag = PT_FLOAT };
	};
	template<>
	struct sdPropertyTypeTraits< bool > {
		typedef bool type;
		typedef enum { tag = PT_BOOL };
	};
	template<>
	struct sdPropertyTypeTraits< idStr > {
		typedef idStr type;
		typedef enum { tag = PT_STRING };
	};
	template<>
	struct sdPropertyTypeTraits< idWStr > {
		typedef idWStr type;
		typedef enum { tag = PT_WSTRING };
	};
	template<>
	struct sdPropertyTypeTraits< idVec2 > {
		typedef idVec2 type;
		typedef enum { tag = PT_VEC2 };
	};
	template<>
	struct sdPropertyTypeTraits< idVec3 > {
		typedef idVec3 type;
		typedef enum { tag = PT_VEC3 };
	};
	template<>
	struct sdPropertyTypeTraits< idVec4 > {
		typedef idVec4 type;
		typedef enum { tag = PT_VEC4 };
	};
	template<>
	struct sdPropertyTypeTraits< sdColor3 > {
		typedef sdColor3 type;
		typedef enum { tag = PT_COLOR3 };
	};
	template<>
	struct sdPropertyTypeTraits< sdColor4 > {
		typedef sdColor3 type;
		typedef enum { tag = PT_COLOR4 };
	};
	template<>
	struct sdPropertyTypeTraits< idAngles > {
		typedef idAngles type;
		typedef enum { tag = PT_ANGLES };
	};

	/*
	============
	sdPropertyTraits
	store appropriate type information, especially for function parameter types
	a dimension of 0 indicates a dynamically-sized type (like idStr)
	============
	*/
	template< class T >
	struct sdPropertyTraits {
		typedef T type;
		typedef const T ConstType;

		typedef T& Reference;
		typedef const T& ConstReference;

		typedef T* pointer;
		typedef const T* ConstPointer;

		typedef T parameter;
		typedef const T ConstParameter;

		typedef enum { integral = true };
		typedef enum { dimension = 1 };

		typedef sdPropertyTypeTraits< T > TypeTraits;
	};

/*
============
Built-ins
============
*/
#define DECLARE_BUILTIN_PROPERTY_TRAITS( InType )	\
	template<>												\
	struct sdPropertyTraits< Type > {						\
		typedef InType Type;									\
		typedef const Type ConstType;						\
		\
		typedef InType& Reference;							\
		typedef const InType& ConstReference;				\
		\
		typedef InType* Pointer;								\
		typedef const InType* ConstPointer;					\
		\
		typedef Reference Parameter;						\
		typedef ConstReference ConstParameter;			\
		\
		typedef enum { integral = true };					\
		typedef enum { dimension = 1 };						\
		\
		typedef sdPropertyTypeTraits< InType > TypeTraits;	\
	};

#undef DECLARE_BUILTIN_PROPERTY_TRAITS

/*
============
User-defined types
============
*/
#define DECLARE_UDT_PROPERTY_TRAITS( InType, Dimension )		\
	template<>												\
	struct sdPropertyTraits< InType > {						\
		typedef InType Type;									\
		typedef const InType ConstType;						\
		\
		typedef InType& Reference;							\
		typedef const InType& ConstReference;				\
		\
		typedef InType* Pointer;								\
		typedef const InType* ConstPointer;					\
		\
		typedef Reference Parameter;						\
		typedef ConstReference ConstParameter;			\
		\
		typedef enum { integral = false };					\
		typedef enum { dimension = ( Dimension ) };			\
		\
		typedef sdPropertyTypeTraits< InType > TypeTraits;	\
	};

	DECLARE_UDT_PROPERTY_TRAITS( idVec2, 2 )
	DECLARE_UDT_PROPERTY_TRAITS( idVec3, 3 )
	DECLARE_UDT_PROPERTY_TRAITS( idVec4, 4 )
	DECLARE_UDT_PROPERTY_TRAITS( sdColor3, 3 )
	DECLARE_UDT_PROPERTY_TRAITS( sdColor4, 4 )
	DECLARE_UDT_PROPERTY_TRAITS( idAngles, 3 )
	DECLARE_UDT_PROPERTY_TRAITS( idStr, 0 )
	DECLARE_UDT_PROPERTY_TRAITS( idWStr, 0 )

#undef DECLARE_UDT_PROPERTY_TRAITS
}
#endif /* ! __PROPERTY_TRAITS_H__ */
