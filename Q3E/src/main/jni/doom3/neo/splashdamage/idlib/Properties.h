// Copyright (C) 2007 Id Software, Inc.
//


#include "PropertyTraits.h"

#ifndef __PROPERTIES_H__
#define __PROPERTIES_H__

namespace sdProperties {
	/*
	============
	sdPropertyValueBase
	============
	*/
	class sdPropertyValueBase {
	public:

		sdPropertyValueBase() {
			flags.callbackEnabled	= true;
			flags.validationEnabled = true;
			flags.readOnly			= false;			
		}
		
		sdPropertyValueBase( const sdPropertyValueBase& rhs ) {
			flags.callbackEnabled	= rhs.flags.callbackEnabled;
			flags.validationEnabled = rhs.flags.validationEnabled ;
			flags.readOnly			= rhs.flags.readOnly;			
		}

		virtual ~sdPropertyValueBase() {}

		void SetValidationEnabled( bool validationEnabled ) { flags.validationEnabled = validationEnabled; }
		void SetCallbackEnabled( bool callbackEnabled ) 	{ flags.callbackEnabled = callbackEnabled; }
		void SetReadOnly( bool readOnly )					{ flags.readOnly = readOnly; }

		bool IsValidationEnabled() const					{ return flags.validationEnabled; }
		bool IsCallbackEnabled() const						{ return flags.callbackEnabled; }
		bool IsReadOnly() const								{ return flags.readOnly; }

	protected:
		struct flags_t {
			bool callbackEnabled	: 1;
			bool validationEnabled	: 1;
			bool readOnly			: 1;			
		} flags;
	};

	/*
	============
	class sdPropertyValue
	============
	*/
	typedef sdUtility::sdHandle< int, -1 > CallbackHandle;
	template< class T >
	class sdPropertyValue :
		public sdPropertyValueBase {
	public:
		typedef sdPropertyTraits< T > Traits;
		typedef T ValueType;
		typedef sdFunctions::sdCallable< void( typename Traits::ConstParameter, typename Traits::ConstParameter ) > CallbackTarget;
		typedef sdFunctions::sdCallable< bool( typename Traits::ConstParameter ) > ValidatorCallbackTarget;
		typedef sdFunctions::sdCallable< void( typename Traits::ConstParameter ) > Setter;
		typedef sdFunctions::sdCallable< typename Traits::Reference() > Getter;

	private:
		ValueType							value;

	public:		
		sdPropertyValue() {}

		sdPropertyValue( typename Traits::ConstParameter ref ) :
			value( ref ), onChange( 1 ), onValidate( 1 ) {}

		sdPropertyValue( const sdPropertyValue< T >& rhs ) :
			value( rhs.value ), 
			onChange( 1 ),
			onValidate( 1 ),
			sdPropertyValueBase( rhs ) {
		}

		bool 						operator==( const sdPropertyValue< T >& rhs ) const;
		bool 						operator==( const T& rhs ) const;

		bool 						operator!=( const sdPropertyValue< T >& rhs ) const;
		bool 						operator!=( const T& rhs ) const;

		void 						Set( const char* rhs );
		sdPropertyValue< T >& 		operator=( const sdPropertyValue< T >& rhs );
		sdPropertyValue< T >& 		operator=( typename Traits::ConstParameter rhs );

		bool						Validate( typename Traits::ConstParameter rhs ) const;

		template< class ST > void	SetIndex( int index, ST value );

		operator typename sdPropertyValue< T >::Traits::ConstReference() const;
		typename sdPropertyValue< T >::Traits::ConstReference GetValue() const;

		CallbackHandle				AddOnChangeHandler( CallbackTarget newCallback );
		void						RemoveOnChangeHandler( CallbackHandle& handle );
		
		CallbackHandle				AddValidator( ValidatorCallbackTarget newCallback );
		void						RemoveValidator( CallbackHandle& handle );

		void						SetGranularity( int onChange, int onValidate ) {
										this->onChange.SetGranularity( onChange );
										this->onValidate.SetGranularity( onValidate );
									}

		void						NumAttached( int& onChange, int& onValidate ) {
										onChange = this->onChange.Num();
										onValidate = this->onValidate.Num();
									}


	private:		
		idList< CallbackTarget >			onChange;	// these should always be checked, since they can be freed, but still left in the list
		idList< ValidatorCallbackTarget >	onValidate;	// these should always be checked, since they can be freed, but still left in the list
	};

	/*
	============
	class sdProperty
	============
	*/
	class sdProperty {
	public:
		sdProperty() : valueType( PT_INVALID ) {
			value.baseValue = NULL;
		}
		sdProperty( sdPropertyValueBase& value, const ePropertyType& t ) :
			valueType( t ) {
			this->value.baseValue = static_cast< sdPropertyValueBase* >( &value );
		}

		~sdProperty() {}

		union propertyValues_t {
			sdPropertyValueBase*			baseValue;

			sdPropertyValue< idStr >*		stringValue;
			sdPropertyValue< idWStr >*		wstringValue;
			sdPropertyValue< int >*			intValue;
			sdPropertyValue< float >* 		floatValue;
			sdPropertyValue< bool >* 		boolValue;

			sdPropertyValue< idVec2 >* 		vec2Value;
			sdPropertyValue< idVec3 >* 		vec3Value;
			sdPropertyValue< idVec4 >* 		vec4Value;

			sdPropertyValue< sdColor3 >* 	color3Value;
			sdPropertyValue< sdColor4 >* 	color4Value;

			sdPropertyValue< idAngles >* 	anglesValue;
		} value;

		bool IsValid() const				{ return value.baseValue != NULL; }		
		bool IsReadOnly() const 			{ return value.baseValue->IsReadOnly();	}
		const char* TypeToString() const	{ return TypeToString( valueType ); }		
		ePropertyType GetValueType() const	{ return valueType; }

		static const char* TypeToString( int type ) {
			if( type < 0 || type >= PT_NUM ) {
				return "<<INVALID TYPE>>";
			}

			return propertyTypeStrings[ type ];
		}

		void NumAttached( int& onChange, int& onValidate ) const {
			switch( valueType ) {
				case PT_INT:		value.intValue->	NumAttached( onChange, onValidate );	break;
				case PT_FLOAT:		value.floatValue->	NumAttached( onChange, onValidate );	break;
				case PT_BOOL:		value.boolValue->	NumAttached( onChange, onValidate );	break;
				case PT_STRING: 	value.stringValue->	NumAttached( onChange, onValidate );	break;
				case PT_WSTRING:	value.wstringValue->NumAttached( onChange, onValidate );	break;
				case PT_VEC2: 		value.vec2Value->	NumAttached( onChange, onValidate );	break;
				case PT_VEC3: 		value.vec3Value->	NumAttached( onChange, onValidate );	break;
				case PT_VEC4: 		value.vec4Value->	NumAttached( onChange, onValidate );	break;
				case PT_COLOR3:		value.color3Value->	NumAttached( onChange, onValidate );	break;
				case PT_COLOR4:		value.color4Value->	NumAttached( onChange, onValidate );	break;
				case PT_ANGLES: 	value.anglesValue-> NumAttached( onChange, onValidate );	break;
			}		
		}

		void RemoveOnChangeHandler( CallbackHandle handle ) {
			switch( valueType ) {
				case PT_INT:		value.intValue->	RemoveOnChangeHandler( handle );	break;
				case PT_FLOAT:		value.floatValue->	RemoveOnChangeHandler( handle );	break;
				case PT_BOOL:		value.boolValue->	RemoveOnChangeHandler( handle );	break;
				case PT_STRING:		value.stringValue->	RemoveOnChangeHandler( handle );	break;
				case PT_WSTRING:	value.wstringValue->RemoveOnChangeHandler( handle );	break;
				case PT_VEC2: 		value.vec2Value->	RemoveOnChangeHandler( handle );	break;
				case PT_VEC3: 		value.vec3Value->	RemoveOnChangeHandler( handle );	break;
				case PT_VEC4: 		value.vec4Value->	RemoveOnChangeHandler( handle );	break;
				case PT_COLOR3:		value.color3Value->	RemoveOnChangeHandler( handle );	break;
				case PT_COLOR4:		value.color4Value->	RemoveOnChangeHandler( handle );	break;
				case PT_ANGLES:		value.anglesValue-> RemoveOnChangeHandler( handle );	break;
			}
		}		

	private:
		friend class sdPropertyHandler;
		void Init( sdPropertyValueBase& value, const ePropertyType& t ) {
			valueType = t;
			this->value.baseValue = static_cast< sdPropertyValueBase* >( &value );
		}

	private:
		ePropertyType valueType;
	};


	/*
	============
	class sdPropertyHandler
	============
	*/
	class sdPropertyHandler {
	public:
		sdPropertyHandler() {
			properties.InitHash( 16, 16 );
			properties.SetGranularity( 16 );
		}
		~sdPropertyHandler() {
			Clear();
		}
		
		typedef sdHashMapGeneric< idStr, sdProperty*, sdHashCompareStrIcmp > propertyHashMap_t;
		typedef propertyHashMap_t::Pair propertyPair_t;

		template< class T >
		sdProperty*			RegisterProperty( const char* name, sdPropertyValue< T >& value ) {
								typedef sdPropertyTypeTraits< T > typeTraits;

								if( sdProperty* prop = GetProperty( name, static_cast< ePropertyType >( typeTraits::tag ), false )) {
									return prop;
								}
								const char* canonicalName = MakeCanonical( name );
								sdProperty* ret = propertyAllocator.Alloc();
								ret->Init( value, static_cast< ePropertyType >( typeTraits::tag ) );
								properties.Set( canonicalName, ret );
								if ( GetProperty( canonicalName, sdProperties::PT_INVALID, true ) != ret ) {
									assert( false );
								}
								return ret;
							}	

		void				Remove( const char* name );

		const char*			NameForProperty( sdProperty* property ) {
								propertyHashMap_t::Iterator iter = properties.Begin();
								for ( iter; iter != properties.End(); ++iter ) {
									if ( ( *iter ).second == property ) {
										return ( *iter ).first.c_str();
									}
								}
								return NULL;
							}

		void				Clear() {
								propertyHashMap_t::Iterator iter = properties.Begin();
								for ( iter; iter != properties.End(); ++iter ) {
									propertyAllocator.Free( iter->second );
								}
								properties.Clear();
								canonicalName.Clear();
							}


		int					Num() const;
		propertyPair_t		GetProperty( int index ) const;
		sdProperty*			GetProperty( const char* name, ePropertyType type, bool warnIfNotFound = true ) const;

		void				SetCallbacksEnabled( bool callbacksEnabled );
		void				SetReadOnly( bool readOnly );

		static void			Init() {}
		static void			Shutdown() { propertyAllocator.Shutdown(); }

	private:
							sdPropertyHandler( const sdPropertyHandler& rhs );
		sdPropertyHandler&	operator=( const sdPropertyHandler& rhs );
		const char*			MakeCanonical( const char* name ) const;

	private:
		sdHashMapGeneric< idStr, sdProperty*, sdHashCompareStrIcmp >	properties;
		mutable idStr				canonicalName;
		static idBlockAlloc< sdProperty, 64 > propertyAllocator;
	};

	template< class T >
	class sdDisablePropertyCallbacks {
	public:
		sdDisablePropertyCallbacks( sdPropertyValue< T >& property_ ) : property( property_ ) {
			oldState = property.IsCallbackEnabled();
			property.SetCallbackEnabled( false );
		}

		~sdDisablePropertyCallbacks() {
			property.SetCallbackEnabled( oldState );
		}
	private:
		bool oldState;
		sdPropertyValue< T >& property;		
	};
}

typedef sdProperties::sdPropertyValue< bool >		sdBoolProperty;
typedef sdProperties::sdPropertyValue< float >		sdFloatProperty;
typedef sdProperties::sdPropertyValue< int >		sdIntProperty;
typedef sdProperties::sdPropertyValue< idVec2 > 	sdVec2Property;
typedef sdProperties::sdPropertyValue< idVec3 > 	sdVec3Property;
typedef sdProperties::sdPropertyValue< idVec4 >		sdVec4Property;
typedef sdProperties::sdPropertyValue< sdColor3 >	sdColor3Property;
typedef sdProperties::sdPropertyValue< sdColor4 >	sdColor4Property;
typedef sdProperties::sdPropertyValue< idStr >		sdStringProperty;
typedef sdProperties::sdPropertyValue< idWStr >		sdWStringProperty;
typedef sdProperties::sdPropertyValue< idAngles >	sdAnglesProperty;

#endif /* ! __PROPERTIES_H__ */
