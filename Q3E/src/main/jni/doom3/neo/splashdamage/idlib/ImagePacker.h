// Copyright (C) 2007 Id Software, Inc.
//


class sdImagePackerNode;

struct sdSubImage {
	int x;
	int y;
	int width;
	int height;

	ID_INLINE bool Overlaps( const sdSubImage& other ) const {
		if ( &other == this ) {
			return true;
		}

		int other_x2 = other.x + other.width - 1;
		int other_y2 = other.y + other.height - 1;

		int x2 = x + width - 1;
		int y2 = y + height - 1;

		if ( other_x2 < x || other_y2 < y || other.x > x2 || other.y > y2 ) {
			return false;
		}

		return true;
	}
};

/************************************************************************/
/* This just does the logic of packing images, copying the actual data  */
/* and all that needs to be implemented by the client.					*/
/* The size of the packed image will always be a power of two.			*/
/************************************************************************/
class sdImagePacker {

public:

	sdImagePacker () {
		root = NULL;
	}

	// The initial size of the packed image
	sdImagePacker ( int width, int height );
	sdImagePacker( const sdImagePacker &other );

	~sdImagePacker ();

	sdImagePacker &operator= ( const sdImagePacker &other );

	// Returns the rectangle of the small image in the big packed image
	// If expandIfFull is true the image size will be increased if no space is
	// free to account for the rectangle
	sdSubImage PackImage( int width, int height, bool expandIfFull = true );
	sdSubImage PackImage( sdSubImage &image );

	// Returns the size of the big image so far
	int GetWidth( void );
	int GetHeight( void );

	// For debug
	void DrawTree( byte *image, int width, int height );

private:

	void DrawTree_R( sdImagePackerNode *node, byte *image, int width, int height );
	sdImagePackerNode	*root;
	int usedWidth;
	int usedHeight;
};