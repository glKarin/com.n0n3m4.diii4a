// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __DEVICECONTEXTHELPER_H__
#define __DEVICECONTEXTHELPER_H__

class sdTextDimensionHelper {
public:
			sdTextDimensionHelper( void );
			~sdTextDimensionHelper( void );
	
	void	Init( const wchar_t* text, const int textLength, const sdBounds2D& rect, unsigned int flags, const qhandle_t font, const int pointSize, idList< int >* lineBreaks = NULL );

	int		GetAdvance( const int index ) const;
	float	GetWidth( const int startIndex, const int endIndex ) const;

	int		GetTextWidth() const { return width; }
	int		GetTextHeight() const { return height; }
	int		GetLineHeight() const { return lineHeight; }

	int		ToVirtualScreenSize( const int size ) const;
	float	ToVirtualScreenSizeFloat( const int size ) const;

private:
	static const int BASE_BUFFER = 256;
	float	scale;
	int		width;
	int		height;
	int		lineHeight;
	int*	advances;
	int		advancesBase[ BASE_BUFFER ];
	int		textLength;
};


/*
============
sdTextDimensionHelper::sdTextDimensionHelper
============
*/
ID_INLINE sdTextDimensionHelper::sdTextDimensionHelper( void ) :
	scale( 1.0f ),
	width( 0 ),
	height( 0 ),
	lineHeight( 0 ),
	advances( &advancesBase[ 0 ] ),
	textLength( 0 ) {
}



/*
============
sdTextDimensionHelper::~sdTextDimensionHelper
============
*/
ID_INLINE sdTextDimensionHelper::~sdTextDimensionHelper( void ) {
	if( advances != &advancesBase[ 0 ] ) {
		Mem_Free( advances );
	}
}


/*
============
sdTextDimensionHelper::Init
============
*/
ID_INLINE void sdTextDimensionHelper::Init( const wchar_t* text, const int textLength, const sdBounds2D& rect, unsigned int flags, const qhandle_t font, const int pointSize, idList< int >* lineBreaks ) {
	if( lineBreaks != NULL ) {
		lineBreaks->SetNum( 0, false );
	}	

	if ( textLength == 0 ) {
		memset( advances, 0, this->textLength * sizeof( int ) );
		this->textLength = 0;		
		return;
	}

	if( advances != NULL && textLength > this->textLength && advances != &advancesBase[ 0 ] ) {
		Mem_Free( advances );
		advances = &advancesBase[ 0 ];
	}

	if( textLength > BASE_BUFFER ) {
		advances = static_cast<	int* >( Mem_Alloc( textLength * sizeof( int ) ) );
	}	

	deviceContext->GetTextDimensions( text, rect, flags, font, pointSize, width, height, &scale, &advances, lineBreaks );
	
	int numLines = ( lineBreaks != NULL ) ? lineBreaks->Num() : 0;

	// a trailing empty line isn't included in the total drawn height
	if( text[ textLength - 1 ] == L'\n' ) {
		numLines--;
	}

	lineHeight = idMath::Ftoi( idMath::Ceil( static_cast< float >( height ) / ( numLines + 1 ) ) );
	this->textLength = textLength;
}



/*
============
sdTextDimensionHelper::GetAdvance
============
*/
ID_INLINE int sdTextDimensionHelper::GetAdvance( const int index ) const {
	if ( advances == NULL ) {
		return 0;
	}

	return advances[index];
}

/*
============
sdTextDimensionHelper::GetWidth
============
*/
ID_INLINE float sdTextDimensionHelper::GetWidth( const int startIndex, const int endIndex ) const {
	if ( advances == NULL || textLength == 0 ) {
		return 0.0f;
	}

	int width = 0;
	for ( int i = startIndex; i <= endIndex && i < textLength ; i++ ) {
		width += advances[i];
	}
	return ToVirtualScreenSize( width );
}


/*
============
sdTextDimensionHelper::ToVirtualScreenSize
============
*/
ID_INLINE int sdTextDimensionHelper::ToVirtualScreenSize( const int size ) const {
	return idMath::Ftoi( idMath::Ceil( ( size >> 6 ) / scale ) );
}

/*
============
sdTextDimensionHelper::ToVirtualScreenSizeFloat
============
*/
ID_INLINE float sdTextDimensionHelper::ToVirtualScreenSizeFloat( const int size ) const {
	return ( size >> 6 ) / scale;
}


#endif /* !__DEVICECONTEXTHELPER_H__ */
