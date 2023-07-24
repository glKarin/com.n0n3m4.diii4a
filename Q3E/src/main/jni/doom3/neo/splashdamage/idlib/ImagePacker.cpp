// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#include "ImagePacker.h"

class sdImagePackerNode {
public:
	sdImagePackerNode () {
		children[0] = children[1] = NULL;
	//	imageIndex = -1;
		isStuffed = false;
	}

	sdImagePackerNode ( const sdImagePackerNode &other ) {
		rect = other.rect;
		isStuffed = other.isStuffed;
		if ( other.children[0] ) {
			children[0] = new sdImagePackerNode( *other.children[0] );
		} else {
			children[0] = NULL;
		}
		if ( other.children[1] ) {
			children[1] = new sdImagePackerNode( *other.children[1] );
		} else {
			children[1] = NULL;
		}
	}

	~sdImagePackerNode () {
		delete children[0];
		delete children[1];
	}

	sdImagePackerNode *Insert( const sdSubImage &image );
	void StuffUnderRect( const sdSubImage &image );

	sdImagePackerNode	*children[2];
	sdSubImage			rect;
	//int					imageIndex;
	bool				isStuffed;
};

sdImagePackerNode* sdImagePackerNode::Insert( const sdSubImage &image ) {
	
	if ( children[0] ) {
		sdImagePackerNode*newNode = children[0]->Insert( image );
		if ( newNode ) return newNode;

		return children[1]->Insert( image );
	} else {
		// If this leaf is already filled image data we can't do anything here
		if ( isStuffed ) {
			return NULL;
		}

		// If there is not enough space we will expand at the highest level by doubling the image so just
		// return null for now
		if ( ( rect.width < image.width ) || ( rect.height < image.height ) ) { 
			// Try again with flipped image dimensions
			if ( image.x >= 0 /* && tryFlipping */ ) {
				sdSubImage flip;
				flip.x = -1;
				flip.y = -1;
				flip.width = image.height;
				flip.height = image.width;
				return Insert( flip );
			} else {
				return NULL;
			}
		}

		// Exact match
		if ( ( rect.width == image.width ) && ( rect.height == image.height ) ) { 
			isStuffed = true;
			return this;
		}

		// Split
		children[0] = new sdImagePackerNode;
		children[1] = new sdImagePackerNode;

		// Slit along the axis that will leave the biggest continuous part, with a preference for horizontal splits if equal
		int dw = rect.width - image.width;
		int dh = rect.height - image.height;

		if ( dw > dh ) {
			// Vertical split
			children[0]->rect.x = rect.x;
			children[0]->rect.y = rect.y;
			children[0]->rect.width = image.width;
			children[0]->rect.height = rect.height;

			children[1]->rect.x = rect.x + image.width;
			children[1]->rect.y = rect.y;
			children[1]->rect.width = rect.width - image.width;
			children[1]->rect.height = rect.height;
		} else {
			// Horizontal split
			children[0]->rect.x = rect.x;
			children[0]->rect.y = rect.y;
			children[0]->rect.width = rect.width;
			children[0]->rect.height = image.height;

			children[1]->rect.x = rect.x;
			children[1]->rect.y = rect.y + image.height;
			children[1]->rect.width = rect.width;
			children[1]->rect.height = rect.height - image.height;
		}

		// children[0] is the best fitting now so insert it there
		return children[0]->Insert( image );
	}
}

void sdImagePackerNode::StuffUnderRect( const sdSubImage &image ) {

	if ( !rect.Overlaps( image ) ) {
		return;
	}

	if ( !children[0] ) {
		// stuff leaves
		isStuffed = true;
	} else {
		children[0]->StuffUnderRect( image );
		children[1]->StuffUnderRect( image );
	}

}

sdImagePacker::sdImagePacker( int width, int height ) {
	usedWidth = usedHeight = 0;
	root = new sdImagePackerNode;
	root->rect.x = 0;
	root->rect.width = idMath::CeilPowerOfTwo( width );
	root->rect.y = 0;
	root->rect.height = idMath::CeilPowerOfTwo( height );	

}

sdImagePacker::~sdImagePacker(  ) {
	usedWidth = usedHeight = 0;
	delete root;
}

sdImagePacker::sdImagePacker( const sdImagePacker &other ) {
	root = new sdImagePackerNode( *other.root );
	usedWidth = other.usedWidth;
	usedHeight = other.usedHeight;
}


sdImagePacker &sdImagePacker::operator= ( const sdImagePacker &other ) {
	delete root;
	root = new sdImagePackerNode( *other.root );
	usedWidth = other.usedWidth;
	usedHeight = other.usedHeight;
	return *this;
}

sdSubImage sdImagePacker::PackImage( int width, int height, bool expandIfFull ) {
	
	// First time, create a root node
	if ( !root ) {
		root = new sdImagePackerNode;
		root->rect.x = 0;
		root->rect.width = idMath::CeilPowerOfTwo( width );
		root->rect.y = 0;
		root->rect.height = idMath::CeilPowerOfTwo( height );
	}
	
	// Insert and optionally enlarge the image
	sdSubImage arg;
	arg.x = arg.y = 0;
	arg.width = width;
	arg.height = height;
	
	sdImagePackerNode *resultNode = root->Insert( arg );

	if ( !resultNode ) {
		// Merge the borders as a last effort and try again
		sdImagePackerNode *newRoot = new sdImagePackerNode;
		sdImagePackerNode *newChild1 = new sdImagePackerNode;
		newRoot->rect = root->rect;
		newChild1->rect = root->rect;

		int dw = root->rect.width - usedWidth;
		int dh = root->rect.height - usedHeight;

		if ( dh > dw ) {
			newChild1->rect.y = usedHeight;
			newChild1->rect.height = newRoot->rect.height - usedHeight;
			root->StuffUnderRect( newChild1->rect ); // Dont let the holes in root use this...
			root->rect.height = usedHeight;

			newRoot->children[0] = root;
			newRoot->children[1] = newChild1;
			root = newRoot;

//			common->Printf( "Recovered %i x %i block\n", newChild1->rect.width, newChild1->rect.height );
		} else {
			newChild1->rect.x = usedWidth;
			newChild1->rect.width = newRoot->rect.width - usedWidth;
			root->StuffUnderRect( newChild1->rect ); // Dont let the holes in root use this...
			root->rect.width = usedWidth;
			
			newRoot->children[0] = root;
			newRoot->children[1] = newChild1;
			root = newRoot;

//			common->Printf( "Recovered %i x %i block\n", newChild1->rect.width, newChild1->rect.height );
		}

		resultNode = root->Insert( arg );
	}

	// Expand if needed
	while ( expandIfFull && !resultNode ) {
		
		bool isHorizontal = false;
		// Is the topmost split a horizontal or a vertical one? 
		if ( root->children[0] && ( root->children[0]->rect.x == root->children[1]->rect.x ) ) {
			isHorizontal = true;
		}

		if ( isHorizontal ) {
			// double with and make root a vertical split
			sdImagePackerNode *newRoot = new sdImagePackerNode;
			sdImagePackerNode *newChild1 = new sdImagePackerNode;

			newRoot->rect = root->rect;
			newRoot->rect.width *= 2;
            newChild1->rect = root->rect;
			newChild1->rect.x = root->rect.x + root->rect.width;

			newRoot->children[0] = root;
			newRoot->children[1] = newChild1;
			root = newRoot;
		} else {
			// double height and make root a horizontal split
			sdImagePackerNode *newRoot = new sdImagePackerNode;
			sdImagePackerNode *newChild1 = new sdImagePackerNode;

			newRoot->rect = root->rect;
			newRoot->rect.height *= 2;
			newChild1->rect = root->rect;
			newChild1->rect.y = root->rect.y + root->rect.height;

			newRoot->children[0] = root;
			newRoot->children[1] = newChild1;
			root = newRoot;
		}

		// Try inserting in the new expanded tree
		resultNode = root->Insert( arg );
	}

	// Return the inserted rect or the empty rect if no suitable location was found
	sdSubImage result;
	if ( resultNode ) {
		result = resultNode->rect;
		if ( (result.x + result.width) > usedWidth ) {
			usedWidth = result.x + result.width;
		}
		if ( (result.y + result.height) > usedHeight ) {
			usedHeight = result.y + result.height;
		}
	} else {
		result.x = result.y = result.width = result.height = 0;
	}
	return result;
}


static void DrawRect( byte *data, int width, int height, sdSubImage rect ) {
	byte *pixel;
	int x2 = rect.x + rect.width - 1;
	int y2 = rect.y + rect.height - 1;

	for ( int i=rect.x; i<=x2; i++ ) {
		pixel = data + ( rect.y*width+i )*4;
		pixel[0] = 255; pixel[1] = pixel[2] = pixel[3] = 0; 
		pixel = data + ( y2*width+i )*4;
		pixel[0] = 255; pixel[1] = pixel[2] = pixel[3] = 0; 
	}

	for ( int j=rect.y; j<=y2; j++ ) {
		pixel = data + ( j*width+rect.x )*4;
		pixel[0] = 255; pixel[1] = pixel[2] = pixel[3] = 0; 
		pixel = data + ( j*width+x2 )*4;
		pixel[0] = 255; pixel[1] = pixel[2] = pixel[3] = 0; 
	}
}

static void CheckerRect( byte *data, int width, int height, sdSubImage rect ) {
	byte *pixel;
	int x2 = rect.x + rect.width - 1;
	int y2 = rect.y + rect.height - 1;

	for ( int i=rect.x; i<=x2; i++ ) {
		for ( int j=rect.y; j<=y2; j++ ) {
			pixel = data + ( j*width+i )*4;
			if ( ((j/5) + (i/5)) & 1 ) {
				pixel[2] = 255; pixel[0] = pixel[1] = pixel[3] = 0; 
			}
		}
	}
}

void sdImagePacker::DrawTree( byte *image, int width, int height ) {
	assert( width >= GetWidth() );
	assert( height >= GetHeight() );

	DrawTree_R( root, image, width, height );
}

void sdImagePacker::DrawTree_R( sdImagePackerNode *node, byte *image, int width, int height ) {
	if ( node->isStuffed ) CheckerRect( image, width, height, node->rect );
	DrawRect( image, width, height, node->rect );

	if ( node->children[0] ) DrawTree_R( node->children[0], image, width, height );
	if ( node->children[1] ) DrawTree_R( node->children[1], image, width, height );
}



sdSubImage sdImagePacker::PackImage( sdSubImage &image ) {
	return PackImage( image.width, image.height );
}

int sdImagePacker::GetWidth( void ) {
	if ( !root ) return 0;
	return root->rect.width;
}

int sdImagePacker::GetHeight( void ) {
	if ( !root ) return 0;
	return root->rect.height;
}
