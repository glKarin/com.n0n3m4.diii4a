// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __PROPERTIES_IMPL_H__
#define __PROPERTIES_IMPL_H__

namespace sdProperties {
	/*
	============
	sdPropertyValue< T >::operator==
	============
	*/
	template< class T > ID_INLINE bool sdPropertyValue< T >::operator==( const sdPropertyValue< T >& rhs ) const {
		return( rhs.value == value );
	}

	/*
	============
	sdPropertyValue< T >::operator==
	============
	*/
	template< class T > ID_INLINE bool sdPropertyValue< T >::operator==( const T& rhs ) const {
		return( rhs == value );
	}

	/*
	============
	sdPropertyValue< T >::operator!=
	============
	*/
	template< class T > ID_INLINE bool sdPropertyValue< T >::operator!=( const sdPropertyValue< T >& rhs ) const {
		return( rhs.value != value );
	}

	/*
	============
	sdPropertyValue< T >::operator!=
	============
	*/
	template< class T > ID_INLINE bool sdPropertyValue< T >::operator!=( const T& rhs ) const {
		return( rhs != value );
	}

	/*
	============
	sdPropertyValue< T >::operator=
	============
	*/
	template< class T > ID_INLINE sdPropertyValue< T >& sdPropertyValue< T >::operator=( const sdPropertyValue< T >& rhs ) {
		if( &rhs != this ) {
			*this = rhs.value;
		}
		return *this;
	}

	/*
	============
	operator==
	============
	*/
	template<class T, class U> ID_INLINE bool operator==( sdPropertyValue< T >& lhs, const U& rhs ) {
		return lhs.GetValue() == rhs;
	}

	/*
	============
	operator==
	============
	*/
	template<class T, class U> ID_INLINE bool operator==( const U& lhs, sdPropertyValue< T >& rhs ) {
		return lhs == rhs.GetValue();
	}


	/*
	============
	sdFromString
	============
	*/
	ID_INLINE bool sdFromString( int& value, const char* str ) {
		int i;
		if( sscanf( str, "%i", &i ) != 1 ) {
			assert( 0 );
			value = 0;
			return false;
		} else {
			value = i;
			return true;
		}		
	}

	/*
	============
	sdFromString
	============
	*/
	ID_INLINE void sdFromString( unsigned int& value, const char* str ) {
		unsigned int i;
		if( sscanf( str, "%u", &i ) != 1 ) {
			assert( 0 );
			value = 0;
		} else {
			value = i;
		}		
	}

	/*
	============
	sdFromString
	============
	*/
	ID_INLINE void sdFromString( float& value, const char* str ) {
		float f;
		if( sscanf( str, "%g", &f ) != 1 ) {
			assert( 0 );
			value = 0.0f;
		} else {
			value = f;
		}		
	}

	/*
	============
	sdFromString
	============
	*/
	ID_INLINE void sdFromString( idVec2& value, const char* str ) {
		idVec2 vec;
		if( sscanf( str, "%g %g", &vec.x, &vec.y ) != 2 ) {
			assert( 0 );
			value = vec2_origin;
		} else {
			value = vec;
		}		
	}

	/*
	============
	sdFromString
	============
	*/
	ID_INLINE void sdFromString( idVec3& value, const char* str ) {
		idVec3 vec;
		if( sscanf( str, "%g %g %g", &vec.x, &vec.y, &vec.z ) != 3 ) {
			assert( 0 );
			value = vec3_origin;
		} else {
			value = vec;
		}
	}

	/*
	============
	sdFromString
	============
	*/
	ID_INLINE void sdFromString( idAngles& value, const char* str ) {
		idAngles vec;
		if( sscanf( str, "%g %g %g", &vec.pitch, &vec.yaw, &vec.roll ) != 3 ) {
			assert( 0 );
			value.Zero();
		} else {
			value = vec;
		}
	}

	/*
	============
	sdFromString
	============
	*/
	ID_INLINE void sdFromString( idVec4& value, const char* str ) {
		idVec4 vec;
		if( sscanf( str, "%g %g %g %g", &vec.x, &vec.y, &vec.z, &vec.w ) != 4 ) {
			assert( 0 );
			value = vec4_origin;
		} else {
			value = vec;
		}
	}

	/*
	============
	sdFromString
	============
	*/
	ID_INLINE void sdFromString( sdColor3& value, const char* str ) {
		sdColor3 c;
		if( sscanf( str, "%g %g %g", &c.r, &c.g, &c.b ) != 3 ) {
			assert( 0 );
			value = sdColor3::white;
		} else {
			value = c;
		}
	}

	/*
	============
	sdFromString
	============
	*/
	ID_INLINE void sdFromString( sdColor4& value, const char* str ) {
		sdColor4 c;
		if( sscanf( str, "%g %g %g %g", &c.r, &c.g, &c.b, &c.a ) != 4 ) {
			assert( 0 );
			value = sdColor4::white;
		} else {
			value = c;
		}
	}

	/*
	============
	sdFromString
	============
	*/
	ID_INLINE void sdFromString( bool& value, const char* str ) {
		int i;
		if( sscanf( str, "%i", &i ) != 1 ) {
			assert( 0 );
			value = false;
		} else {
			value = ( i != 0 );
		}		
	}

	/*
	============
	sdFromString
	============
	*/
	ID_INLINE void sdFromString( idStr& value, const char* str ) {
		value = str;		
	}

	/*
	============
	sdFromString
	============
	*/
	template< class T >
	ID_INLINE void sdFromString( sdPropertyValue< T >& value, const char* str ) {
		T v;
		sdFromString( v, str );
		value = v;
	}

	/*
	============
	sdEnforceNumericFormat
	============
	*/
	ID_INLINE idStr sdEnforceNumericFormat( const int numItems, const char* input ) {		
		idStr retStr;
		idToken token;

		idLexer src( input, idStr::Length( input ), "sdEnforceNumericFormat" );
		
		int readItems = 0;
		while( src.ReadToken( &token ) ) {
			if( token == "-" ) {
				retStr += token;
				continue;
			}

			if( token.IsNumeric() ) {				
				retStr += token;
				if( token != "-" && token != "." ) {
					retStr += " ";
				}
			} else {
				retStr += "0 ";
			}

			++readItems;
		}

		if( numItems != readItems || !src.EndOfFile() ) {
			assert( 0 );
			retStr.Clear();
			for( int i = 0; i < numItems - 1; i++ ) {
				retStr += "0 ";
			}
			retStr += "0";
		}

		retStr.StripTrailing( ' ' );
		return retStr;		
	}


	/*
	============
	sdFromString
	============
	*/
	ID_INLINE void sdFromString( sdProperty& value, const char* str ) {
		if( !value.IsValid() ) {
			assert( !"sdFromString:: Invalid property" );
			return;
		}

		switch( value.GetValueType() ) {
			case PT_STRING:	*value.value.stringValue = str; break;
			case PT_WSTRING: *value.value.wstringValue = va( L"%hs", str ); break;
			case PT_INT:	sdFromString( *value.value.intValue,		sdEnforceNumericFormat( sdPropertyTraits< int >::dimension, str ) ); break;
			case PT_FLOAT:	sdFromString( *value.value.floatValue,		sdEnforceNumericFormat( sdPropertyTraits< float >::dimension, str ) ); break;
			case PT_BOOL:	sdFromString( *value.value.boolValue, 		sdEnforceNumericFormat( sdPropertyTraits< bool >::dimension, str ) ); break;
			case PT_VEC2:	sdFromString( *value.value.vec2Value, 		sdEnforceNumericFormat( sdPropertyTraits< idVec2 >::dimension, str ) );	break;
			case PT_VEC3:	sdFromString( *value.value.vec3Value, 		sdEnforceNumericFormat( sdPropertyTraits< idVec3 >::dimension, str ) ); break;			
			case PT_VEC4:	sdFromString( *value.value.vec4Value, 		sdEnforceNumericFormat( sdPropertyTraits< idVec4 >::dimension, str ) ); break;
			case PT_COLOR3:	sdFromString( *value.value.color3Value, 	sdEnforceNumericFormat( sdPropertyTraits< sdColor3 >::dimension, str ) ); break;
			case PT_COLOR4:	sdFromString( *value.value.color4Value, 	sdEnforceNumericFormat( sdPropertyTraits< sdColor4 >::dimension, str ) ); break;
			case PT_ANGLES:	sdFromString( *value.value.anglesValue, 	sdEnforceNumericFormat( sdPropertyTraits< idAngles >::dimension, str ) ); break;
			default:
				assert( !"sdFromString:: Invalid property type" );
				break;
		}
	}

	/*
	============
	sdToString
	============
	*/
	template< class T >
	ID_INLINE idStr sdToString( const sdPropertyValue< T >& value ) {
		return value.GetValue().ToString();
	}

	/*
	============
	sdToString
	============
	*/
	ID_INLINE idStr sdToString( const sdPropertyValue< int >& value ) {
		return va( "%i", value.GetValue() );
	}

	/*
	============
	sdToString
	============
	*/
	ID_INLINE idStr sdToString( const sdPropertyValue< float >& value ) {
		return va( "%g", value.GetValue() );
	}

	/*
	============
	sdToString
	============
	*/
	ID_INLINE idStr sdToString( const sdPropertyValue< bool >& value ) {
		return va( "%i", value.GetValue() ? 1 : 0 );
	}

	/*
	============
	sdToString
	============
	*/
	ID_INLINE idStr sdToString( const sdProperty& value ) {
		if( !value.IsValid() ) {
			assert( !"sdToString:: Invalid property" );
			return "";
		}

		switch( value.GetValueType() ) {
			case PT_STRING:		return value.value.stringValue->GetValue();
			case PT_WSTRING:	return va( "%ls", value.value.wstringValue->GetValue().c_str() );
			case PT_INT:		return sdToString( *value.value.intValue	);
			case PT_FLOAT:		return sdToString( *value.value.floatValue	);
			case PT_BOOL:		return sdToString( *value.value.boolValue 	);
			case PT_VEC2:		return sdToString( *value.value.vec2Value 	);
			case PT_VEC3:		return sdToString( *value.value.vec3Value 	);
			case PT_VEC4:		return sdToString( *value.value.vec4Value 	);
			case PT_COLOR3:		return sdToString( *value.value.color3Value );
			case PT_COLOR4:		return sdToString( *value.value.color4Value );
			case PT_ANGLES:		return sdToString( *value.value.anglesValue );
			default:
				assert( !"sdFromString:: Invalid property type" );				
		}
		return "";
	}

#if defined( MACOS_X )
    template< typename T >
    struct isNaN_impl {
        bool operator()(T) { return false; }
    };
    
    template<> 
    struct isNaN_impl<float> {
        bool operator()(float x) { return x != x; }
    };

    template<> 
    struct isNaN_impl<double> {
        bool operator()(double x) { return x != x; }
    };
    
    template< typename T > bool isNaN( T x ) {
        static isNaN_impl<T> impl;
        return impl(x);
    }
#endif
    
	/*
	============
	sdPropertyValue< T >::SetIndex	
	============
	*/
	template< class T >
	template< typename ST >
	ID_INLINE void sdPropertyValue< T >::SetIndex( int index, ST newValue ) {
		assert( !Traits::integral && index >= 0 && ( Traits::dimension == 0 || index < Traits::dimension ));

#if defined( MACOS_X )
        // Avoid a crash due to infinite recursion if newValue is NaN.
		// TTimo - no indication this would happen with final/released assets. leaving for mac build only
        if ( newValue != value[ index ] && !( isNaN( newValue ) && isNaN( value[ index] ) ) ) {
#else
		if ( newValue != value[ index ] ) {
#endif
			if( onValidate.Num() ) {
				T tempValue = value;
				if( !Validate( tempValue )) {
					return;
				}
			}

			const T oldValue = value;
			value[ index ] = newValue;
			if ( flags.callbackEnabled ) {
				for( int i = 0; i < onChange.Num(); i++ ) {
					if( onChange[ i ].IsValid() ) {
						onChange[ i ]( oldValue, value );
					}					
				}
			}
		}
	}

	/*
	============
	sdPropertyValue< T >::operator=
	============
	*/
	template< class T > ID_INLINE sdPropertyValue< T >& sdPropertyValue< T >::operator=( typename Traits::ConstParameter rhs ) {
		if( flags.readOnly ) {
			return *this;
		}

		if( rhs != value ) {
			if( !Validate( rhs )) {
				return *this;
			}

			const T oldValue = value;
			value = rhs;
			if ( flags.callbackEnabled ) {
				for( int i = 0; i < onChange.Num(); i++ ) {
					if( onChange[ i ].IsValid() ) {
						onChange[ i ]( oldValue, value );
					}
				}
			}
		}
		return *this;
	}

	template<> ID_INLINE void sdPropertyValue< idStr >::Set( const char* rhs ) {
		if( flags.readOnly ) {
			return;
		}

		if( value.Cmp( rhs ) ) {
			if( !Validate( rhs )) {
				return;
			}

			const idStr oldValue = value;
			value = rhs;
			if ( flags.callbackEnabled ) {
				for( int i = 0; i < onChange.Num(); i++ ) {
					if( onChange[ i ].IsValid() ) {
						onChange[ i ]( oldValue, value );
					}
				}
			}
		}
	}


	/*
	============
	operator|=
	============
	*/
	template<class T, class U> ID_INLINE sdPropertyValue< T >& operator|=( sdPropertyValue< T >& lhs, const U& rhs ) {
		return ( lhs = ( lhs.GetValue() | rhs ));
	}

	/*
	============
	operator&=
	============
	*/
	template<class T, class U> ID_INLINE sdPropertyValue< T >& operator&=( sdPropertyValue< T >& lhs, const U& rhs ) {
		return ( lhs = ( lhs.GetValue() & rhs ));
	}

	/*
	============
	operator^=
	============
	*/
	template<class T, class U> ID_INLINE sdPropertyValue< T >& operator^=( sdPropertyValue< T >& lhs, const U& rhs ) {
		return ( lhs = ( lhs.GetValue() ^ rhs ));
	}

	/*
	============
	operator+=
	============
	*/
	template<class T, class U> ID_INLINE sdPropertyValue< T >& operator+=( sdPropertyValue< T >& lhs, const U& rhs ) {
		return ( lhs = ( lhs.GetValue() + rhs ));
	}

	/*
	============
	operator-=
	============
	*/
	template<class T, class U> ID_INLINE sdPropertyValue< T >& operator-=( sdPropertyValue< T >& lhs, const U& rhs ) {
		return ( lhs = ( lhs.GetValue() - rhs ));
	}

	/*
	============
	operator*=
	============
	*/
	template<class T, class U> ID_INLINE sdPropertyValue< T >& operator*=( sdPropertyValue< T >& lhs, const U& rhs ) {
		return ( lhs = ( lhs.GetValue() * rhs ));
	}

	/*
	============
	operator/=
	============
	*/
	template<class T, class U> ID_INLINE sdPropertyValue< T >& operator/=( sdPropertyValue< T >& lhs, const U& rhs ) {
		return ( lhs = ( lhs.GetValue() / rhs ));
	}

	/*
	============
	operator++
	============
	*/
	template<class T> ID_INLINE sdPropertyValue< T > operator++( sdPropertyValue< T >& lhs, int ) {
		sdPropertyValue< T > temp = lhs;
		lhs = lhs.GetValue() + 1;
		return temp;
	}

	/*
	============
	operator--
	============
	*/
	template<class T> ID_INLINE sdPropertyValue< T > operator--( sdPropertyValue< T >& lhs, int ) {
		sdPropertyValue< T > temp = lhs;
		lhs = lhs.GetValue() - 1;
		return temp;
	}

	/*
	============
	operator++
	============
	*/
	template<class T, class U> ID_INLINE sdPropertyValue< T >& operator++( sdPropertyValue< T >& lhs ) {	
		return ( lhs = ( lhs + 1 ));
	}

	/*
	============
	operator--
	============
	*/
	template<class T, class U> ID_INLINE sdPropertyValue< T >& operator--( sdPropertyValue< T >& lhs ) {
		return ( lhs = ( lhs - 1 ));
	}

	// convenience global operators for integer types
	/*
	============
	sdPropertyValue< T >::operator &
	============
	*/
	template< class T > ID_INLINE int operator&( const sdPropertyValue< T >& lhs, const int rhs ) { 
		return lhs.GetValue() & rhs;
	}

	/*
	============
	sdPropertyValue< T >::operator &
	============
	*/
	template< class T > ID_INLINE int operator&( const int lhs, const sdPropertyValue< T >& rhs ) { 
		return lhs & rhs.GetValue();
	}

	/*
	============
	sdPropertyValue< T >::operator |
	============
	*/
	template< class T > ID_INLINE int operator|( const sdPropertyValue< T >& lhs, const int rhs ) { 
		return lhs.GetValue() | rhs;
	}

	/*
	============
	sdPropertyValue< T >::operator |
	============
	*/
	template< class T > ID_INLINE int operator|( const int lhs, const sdPropertyValue< T >& rhs ) { 
		return lhs | rhs.GetValue();
	}

	/*
	============
	sdPropertyValue< T >::operator ^
	============
	*/
	template< class T > ID_INLINE int operator^( const sdPropertyValue< T >& lhs, const int rhs ) { 
		return lhs.GetValue() ^ rhs;
	}

	/*
	============
	sdPropertyValue< T >::operator ^
	============
	*/
	template< class T > ID_INLINE int operator^( const int lhs, const sdPropertyValue< T >& rhs ) { 
		return lhs ^ rhs.GetValue();
	}

	/*
	============
	sdPropertyValue< T >::operator %
	============
	*/
	template< class T > ID_INLINE int operator%( const sdPropertyValue< T >& lhs, const int rhs ) { 
		return lhs.GetValue() % rhs;
	}

	/*
	============
	sdPropertyValue< T >::operator %
	============
	*/
	template< class T > ID_INLINE int operator%( const int lhs, const sdPropertyValue< T >& rhs ) { 
		return lhs % rhs.GetValue();
	}

	/*
	============
	sdPropertyValue< T >::Get
	============
	*/
	template< class T > ID_INLINE sdPropertyValue< T >::operator typename sdPropertyValue< T >::Traits::ConstReference() const { 
		return value; 
	}

	/*
	============
	sdPropertyValue< T >::Get
	============
	*/
	template< class T > ID_INLINE typename sdPropertyValue< T >::Traits::ConstReference sdPropertyValue< T >::GetValue() const { 
		return value; 
	}

	/*
	============
	sdPropertyValue< T >::AddOnChangeHandler
	============
	*/
	template< class T >
	ID_INLINE CallbackHandle sdPropertyValue< T >::AddOnChangeHandler( CallbackTarget newCallback ) {
		int index = 0;
		while( index < onChange.Num() && onChange[ index ].IsValid() ) {
			++index;
		}
		if( index >= onChange.Num() ) {
			index = onChange.Append( newCallback );
		} else {
			onChange[ index ] = newCallback;
		}
		assert( onChange.Num() );
		return index;
	}

	/*
	============
	sdPropertyValue< T >::RemoveOnChangeHandler
	============
	*/
	template< class T >
	ID_INLINE void sdPropertyValue< T >::RemoveOnChangeHandler( CallbackHandle& handle ) {
		if( handle.IsValid() && handle < onChange.Num() ) {
			onChange[ handle ].Release();
		} else {
			assert( 0 );
		}
		handle.Release();
	}

	/*
	============
	sdPropertyValue< T >::AddValidator
	============
	*/
	template< class T >
	ID_INLINE CallbackHandle sdPropertyValue< T >::AddValidator( ValidatorCallbackTarget newCallback ) {
		int index = 0;
		while( index < onValidate.Num() && onValidate[ index ].IsValid() ) {
			++index;
		}
		if( index >= onValidate.Num() ) {
			index = onValidate.Append( newCallback );
		} else {
			onValidate[ index ] = newCallback;
		}
		assert( onValidate.Num() );
		return index;
	}

	/*
	============
	sdPropertyValue< T >::RemoveValidator
	============
	*/
	template< class T >
	ID_INLINE void sdPropertyValue< T >::RemoveValidator( CallbackHandle& handle ) {
		if( handle.IsValid() && handle < onValidate.Num() ) {
			onValidate[ handle ].Release();
		} else {
			assert( 0 );
		}
		handle.Release();
	}


	/*
	============
	sdPropertyValue< T >::Validate
	============
	*/
	template< class T >
	ID_INLINE bool sdPropertyValue< T >::Validate( typename Traits::ConstParameter rhs ) const {
		if( !IsValidationEnabled() ) {
			return true;
		}
		for( int i = 0; i < onValidate.Num(); i++ ) {
			if( onValidate[ i ].IsValid() && !onValidate[ i ]( rhs ) ) {
				return false;
			}
		}
		return true;
	}

	/*
	============
	sdPropertyHandler::Remove
	============
	*/
	ID_INLINE void sdPropertyHandler::Remove( const char* name ) {
		propertyAllocator.Free( GetProperty( name, PT_INVALID ) );
		properties.Remove( name ); 
	}

	/*
	============
	sdPropertyHandler::Num
	============
	*/
	ID_INLINE int sdPropertyHandler::Num() const {
		return properties.Num();
	}

	/*
	============
	sdPropertyHandler::GetProperty
	============
	*/
	ID_INLINE sdPropertyHandler::propertyPair_t	sdPropertyHandler::GetProperty( int index ) const { 
		return *properties.FindIndex( index );
	}

	/*
	============
	sdPropertyHandler::GetProperty
	============
	*/
	ID_INLINE sdProperty*	sdPropertyHandler::GetProperty( const char* name, ePropertyType type, bool warnIfNotFound ) const {
		const char* canonicalName = MakeCanonical( name );
		
		propertyHashMap_t::ConstIterator iter = properties.Find( canonicalName );
		if ( iter != properties.End() && ( type == PT_INVALID || iter->second->valueType == type ) ) {
			return iter->second;
		}

		if( warnIfNotFound ) {
			common->Warning( "Property '%s' of type '%s' not found", canonicalName, sdProperty::TypeToString( type ));
		}
		return NULL;
	}

	/*
	============
	sdPropertyHandler::SetCallbacksEnabled
	============
	*/
	ID_INLINE void sdPropertyHandler::SetCallbacksEnabled( bool callbacksEnabled ) {
		for ( int i = 0; i < properties.Num(); i++ ) {
			propertyHashMap_t::Iterator iter = properties.FindIndex( i );
			if ( iter->second->IsValid() ) {
				iter->second->value.baseValue->SetCallbackEnabled( callbacksEnabled );
			}
		}
	}

	/*
	============
	sdPropertyHandler::SetReadOnly
	============
	*/
	ID_INLINE void sdPropertyHandler::SetReadOnly( bool readOnly ) {
		for ( int i = 0; i < properties.Num(); i++ ) {
			propertyHashMap_t::Iterator iter = properties.FindIndex( i );
			if( iter->second->IsValid() ) {
				iter->second->value.baseValue->SetReadOnly( readOnly );
			}
		}
	}

	/*
	============
	sdPropertyHandler::MakeCanonical
	============
	*/
	ID_INLINE const char* sdPropertyHandler::MakeCanonical( const char* name ) const {
		canonicalName = name;
		canonicalName.ToLower();
		return canonicalName.c_str();
	}

	/*
	============
	CountForPropertyType
	============
	*/
	ID_INLINE int CountForPropertyType( sdProperties::ePropertyType type ) {
		switch ( type ) {
		case sdProperties::PT_VEC4:		/* fall through */
		case sdProperties::PT_COLOR4:	return 4;
		case sdProperties::PT_VEC3:		/* fall through */
		case sdProperties::PT_ANGLES:	/* fall through */
		case sdProperties::PT_COLOR3:	return 3;
		case sdProperties::PT_VEC2:		return 2;
		case sdProperties::PT_FLOAT:	return 1;
		case sdProperties::PT_STRING:	return 1;
		case sdProperties::PT_WSTRING:	return 1;
		case sdProperties::PT_INT:		return 1;
		default:						return -1;
		}
	}
}

/*
============
sdTypeFromString
============
*/
template< class Result > 
class sdTypeFromString {
public:
	sdTypeFromString( const char* input ) {	
		sdProperties::sdFromString( output, input );
	}
	sdTypeFromString( const wchar_t* input ) {	
		sdProperties::sdFromString( output, va( "%ls", input ) );
	}

	operator Result() {
		return output;
	}

private:
	Result output;
};

/*
============
operator==
============
*/
template< class T > ID_INLINE bool operator==( const T& lhs, const sdProperties::sdPropertyValue< T >& rhs ) {
	return rhs.GetValue() == lhs;
}

/*
============
operator!=
============
*/
template< class T > ID_INLINE bool operator!=( const T& lhs, const sdProperties::sdPropertyValue< T >& rhs ) {
	return rhs.GetValue() != lhs;
}

#endif // __PROPERTIES_IMPL_H__

