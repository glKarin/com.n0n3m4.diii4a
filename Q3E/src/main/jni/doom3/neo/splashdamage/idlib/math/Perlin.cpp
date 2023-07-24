// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

sdPerlin::sdPerlin( float _persistence, int _octaves, float _frequency ) {
	persistence		= _persistence;
	octaves			= _octaves;
	frequency		= _frequency;

	// "no" tiling
	tilex = 0x0FFFFFFF;
	tiley = 0x0FFFFFFF;
	tilez = 0x0FFFFFFF;
}

void sdPerlin::SetTileX( int tile ) {
	tilex = ( int )( tile * frequency );
}

void sdPerlin::SetTileY( int tile ) {
	tiley = ( int )( tile * frequency );
}

void sdPerlin::SetTileZ( int tile ) {
	tilez = ( int )( tile * frequency );
}

float sdPerlin::RawNoise( int x, int y, int z ) {
	int n = x + y * 57 + z * 227;
	n = ( n << 13 ) ^ n;
	//if (alternative)
	//	return (1 - ( ( n * ( n * n * 19417 + 189851) + 4967243) & 4945007) / 3354521.0);
	//else
	return   1 - ( (n *  ( n * n * 15731 + 789221) + 1376312589) & 0x7fffffff ) / 1073741824.0;    
}

/*
float sdPerlin::SmoothedNoise (int x, int y) {
    float corners = ( Noise(x-1, y-1)+Noise(x+1, y-1)+Noise(x-1, y+1)+Noise(x+1, y+1) ) / 16;
    float sides   = ( Noise(x-1, y)  +Noise(x+1, y)  +Noise(x, y-1)  +Noise(x, y+1) ) /  8;
    float center  =  Noise(x, y) / 4;
    return corners + sides + center;
}
*/

/**
	Interpolated 3d noize
*/
float sdPerlin::InterpolatedNoise (float x, float y, float z) {
	int ix, iy, iz;
	float fx, fy, fz;

	ix = static_cast< int >( x );
	iy = static_cast< int >( y );
	iz = static_cast< int >( z );
	
	fx = x - ix;
	fy = y - iy;
	fz = z - iz;

	float v1, v2, v3, v4, v5, v6, v7, v8;
	v1 = RawNoise( ix,     iy,     iz );
	v2 = RawNoise( ix + 1, iy,     iz );
	v3 = RawNoise( ix,     iy + 1, iz );
	v4 = RawNoise( ix + 1, iy + 1, iz  );

	v5 = RawNoise( ix,     iy,     iz + 1 );
	v6 = RawNoise( ix + 1, iy,     iz + 1 );
	v7 = RawNoise( ix,     iy + 1, iz + 1 );
	v8 = RawNoise( ix + 1, iy + 1, iz + 1 );


	float i1, i2, i3, i4;
	i1 = CosineInterp( v1 , v2 , fx );
	i2 = CosineInterp( v3 , v4 , fx );

	i3 = CosineInterp( v5 , v6 , fx );
	i4 = CosineInterp( v7 , v8 , fx );

	float j1, j2;
	j1 = CosineInterp( i1 , i2 , fy );
	j2 = CosineInterp( i3 , i4 , fy );

	return CosineInterp( j1 , j2 , fz );
}

float sdPerlin::NoiseFloat( const idVec3 &pos ) {
      float rez = 0.0f;
	  float frequencyl = frequency;
	  float amplitude = 1.0f;

	  int i;
	  for ( i = 0; i < octaves; i++ ) {
          rez += InterpolatedNoise( pos.x * frequencyl, pos.y * frequencyl, pos.z * frequencyl ) * amplitude;
		  frequencyl *= 2;
		  amplitude *= persistence;
	  }
	  if ( rez > 1.f ) {
		  rez = 1.f;
	  } else if ( rez < -1.f ) {
		  rez = -1.f;
	  }
      return rez;
}







#if 0

bool	sdPerlin2::inited = false;
int		sdPerlin2::p[ ( PERLIN_RANDOM_SAMPLES + 1 ) * 2 ];
idVec3	sdPerlin2::g[ ( PERLIN_RANDOM_SAMPLES + 1 ) * 2 ];

sdPerlin2::sdPerlin2( void ) {
	Init();
}

void sdPerlin2::Init( void ) {
	if ( inited ) {
		return;
	}
	inited = true;

	idVec3 v;
	float s;

	srand( 1 );
	int i, j;
	for ( i = 0; i < PERLIN_RANDOM_SAMPLES; i++ ) {
		do {
			for ( j = 0; j < 3; j++ ) {
				v[ j ] = ( ( rand() % ( PERLIN_RANDOM_SAMPLES * 2 ) ) - PERLIN_RANDOM_SAMPLES ) / static_cast< float >( PERLIN_RANDOM_SAMPLES );
			}
			s = v * v;			
		} while( s > 1.f && s != 0.f );
		s = 1 / sqrt( s );
		for ( j = 0; j < 3; j++ ) {
			g[ i ][ j ] = v[ j ] * s;
		}
	}

	for ( i = 0; i < PERLIN_RANDOM_SAMPLES; i++ ) {
		p[ i ] = i;
	}
	for ( i = PERLIN_RANDOM_SAMPLES; i > 0; i -= 2 ) {
		Swap( p[ i ], p[ rand() % PERLIN_RANDOM_SAMPLES ] );
	}
	for ( i = 0; i < PERLIN_RANDOM_SAMPLES + 2; i++ ) {
		p[ PERLIN_RANDOM_SAMPLES + i ] = p[ i ];
		g[ PERLIN_RANDOM_SAMPLES + i ] = g[ i ];
	}
}

void Perlin_Setup( const float& v, int& b0, int& b1, float& r0, float& r1 ) {
	float t = v + 10000;
	int it = static_cast< int >( t );
	b0 = it & ( PERLIN_RANDOM_SAMPLES - 1 );
	b1 = ( b0 + 1 ) & ( PERLIN_RANDOM_SAMPLES - 1 );
	r0 = t - it;
	r1 = r0 - 1;
}

ID_INLINE float Perlin_At( const idVec3& q, float r0, float r1, float r2 ) {
	return ( q[ 0 ] * r0 ) + ( q[ 1 ] * r1 ) + ( q[ 2 ] * r2 );
}

ID_INLINE float Perlin_SCurve( float t ) {
	return ( t * t * ( 3 - ( 2 * t ) ) );
}

ID_INLINE float Perlin_Lerp( float a, float b, float t ) {
	return ( a + (  t * ( b - a ) ) );
}

float sdPerlin2::Noise( const idVec3& noisePos ) {
	int b00, b01, b11, b10;
	int b0[ 3 ], b1[ 3 ];
	idVec3 r0, r1, s;

	idVec3 vec = noisePos; // * 0.0037972f;

	Perlin_Setup( vec[ 0 ], b0[ 0 ], b1[ 0 ], r0[ 0 ], r1[ 0 ] );
	Perlin_Setup( vec[ 1 ], b0[ 1 ], b1[ 1 ], r0[ 1 ], r1[ 1 ] );
	Perlin_Setup( vec[ 2 ], b0[ 2 ], b1[ 2 ], r0[ 2 ], r1[ 2 ] );

	int i, j;
	i = p[ b0[ 0 ] ];
	j = p[ b1[ 0 ] ];

	b00 = p[ i + b0[ 1 ] ];
	b10 = p[ j + b0[ 1 ] ];
	b01 = p[ i + b1[ 1 ] ];
	b11 = p[ j + b1[ 1 ] ];

	s[ 0 ] = Perlin_SCurve( r0[ 0 ] );
	s[ 1 ] = Perlin_SCurve( r0[ 1 ] );
	s[ 2 ] = Perlin_SCurve( r0[ 2 ] );

	float u, v, a, b, c ,d;

	u = Perlin_At( g[ b00 + b0[ 2 ] ], r0[ 0 ], r0[ 1 ], r0[ 2 ] );
	v = Perlin_At( g[ b10 + b0[ 2 ] ], r1[ 0 ], r0[ 1 ], r0[ 2 ] );
	a = Perlin_Lerp( s[ 0 ], u, v );

	u = Perlin_At( g[ b01 + b0[ 2 ] ], r0[ 0 ], r1[ 1 ], r0[ 2 ] );
	v = Perlin_At( g[ b11 + b0[ 2 ] ], r1[ 0 ], r1[ 1 ], r0[ 2 ] );
	b = Perlin_Lerp( s[ 0 ], u, v );

	c = Perlin_Lerp( s[ 1 ], a, b );


	u = Perlin_At( g[ b00 + b1[ 2 ] ], r0[ 0 ], r0[ 1 ], r1[ 2 ] );
	v = Perlin_At( g[ b10 + b1[ 2 ] ], r1[ 0 ], r0[ 1 ], r1[ 2 ] );
	a = Perlin_Lerp( s[ 0 ], u, v );

	u = Perlin_At( g[ b01 + b1[ 2 ] ], r0[ 0 ], r1[ 1 ], r1[ 2 ] );
	v = Perlin_At( g[ b11 + b1[ 2 ] ], r1[ 0 ], r1[ 1 ], r1[ 2 ] );
	b = Perlin_Lerp( s[ 0 ], u, v );

	d = Perlin_Lerp( s[ 1 ], a, b );

	d = Perlin_Lerp( s[ 2 ], c, d );
	if ( d > 1.f ) {
		d = 1.f;
	} else if ( d < -1.f ) {
		d = -1.f;
	}
	return d;
}

#endif // 0











// TODO: change these preprocessor macros into inline functions

// S curve is (3x^2 - 2x^3) because it's quick to calculate
// though -cos(x * PI) * 0.5 + 0.5 would work too

ID_INLINE float EaseCurve( float t ) {
	return t * t * ( 3.0 - 2.0 * t );
}

ID_INLINE float LinearInterp( float t, float a, float b ) {
	return a + t * ( b - a );
}

ID_INLINE float Dot2( float rx, float ry, float* q ) {
	return rx * q[ 0 ] + ry * q[ 1 ];
}

ID_INLINE float Dot3( float rx, float ry, float rz, float* q ) {
	return rx * q[ 0 ] + ry * q[ 1 ] + rz * q[ 2 ];
}

#define SetupValues( t, axis, g0, g1, d0, d1, pos ) \
	t = pos[axis] + NOISE_LARGE_PWR2; \
	g0 = ((int)t) & NOISE_MOD_MASK; \
	g1 = (g0 + 1) & NOISE_MOD_MASK; \
	d0 = t - (int)t; \
	d1 = d0 - 1.0;

/////////////////////////////////////////////////////////////////////
// return a random float in [-1,1]

ID_INLINE float sdPerlin2::RandNoiseFloat() { 
	return (float)((rand() % (NOISE_WRAP_INDEX + NOISE_WRAP_INDEX)) - 
		NOISE_WRAP_INDEX) / NOISE_WRAP_INDEX;
};

/////////////////////////////////////////////////////////////////////
// convert a 2D vector into unit length

void sdPerlin2::Normalize2d(float vector[2]) {
	float length = sqrt((vector[0] * vector[0]) + (vector[1] * vector[1]));
	vector[0] /= length;
	vector[1] /= length;
}

/////////////////////////////////////////////////////////////////////
// convert a 3D vector into unit length

void sdPerlin2::Normalize3d(float vector[3]) {
	float length = sqrt((vector[0] * vector[0]) + 
		(vector[1] * vector[1]) +
		(vector[2] * vector[2]));
	vector[0] /= length;
	vector[1] /= length;
	vector[2] /= length;
}

/////////////////////////////////////////////////////////////////////
//
// Mnemonics used in the following 3 functions:
//   L = left		(-X direction)
//   R = right  	(+X direction)
//   D = down   	(-Y direction)
//   U = up     	(+Y direction)
//   B = backwards	(-Z direction)
//   F = forwards       (+Z direction)
//
// Not that it matters to the math, but a reader might want to know.
//
// noise1d - create 1-dimensional coherent noise
//   if you want to learn about how noise works, look at this
//   and then look at noise2d.

float sdPerlin2::Noise1d(float pos[1]) {
	int   gridPointL, gridPointR;
	float distFromL, distFromR, sX, t, u, v;

	if ( !initialized ) {
		Reseed();
	}

	// find out neighboring grid points to pos and signed distances from pos to them.
	SetupValues(t, 0, gridPointL, gridPointR, distFromL, distFromR, pos);

	sX = EaseCurve( distFromL );

	// u, v, are the vectors from the grid pts. times the random gradients for the grid points
	// they are actually dot products, but this looks like scalar multiplication
	u = distFromL * gradientTable1d[ permutationTable[ gridPointL ] ];
	v = distFromR * gradientTable1d[ permutationTable[ gridPointR ] ];

	// return the linear interpretation between u and v (0 = u, 1 = v) at sX.
	return LinearInterp( sX, u, v );
}

/////////////////////////////////////////////////////////////////////
// create 2d coherent noise

float sdPerlin2::Noise2d(float pos[2]) {
	int gridPointL, gridPointR, gridPointD, gridPointU;
	int indexLD, indexRD, indexLU, indexRU;
	float distFromL, distFromR, distFromD, distFromU;
	float *q, sX, sY, a, b, t, u, v;
	register int indexL, indexR;

	if ( !initialized ) {
		Reseed();
	}

	// find out neighboring grid points to pos and signed distances from pos to them.
	SetupValues(t, 0, gridPointL, gridPointR, distFromL, distFromR, pos);
	SetupValues(t, 1, gridPointD, gridPointU, distFromD, distFromU, pos);

	// Generate some temporary indexes associated with the left and right grid values
	indexL = permutationTable[ gridPointL ];
	indexR = permutationTable[ gridPointR ];

	// Generate indexes in the permutation table for all 4 corners
	indexLD = permutationTable[ indexL + gridPointD ];
	indexRD = permutationTable[ indexR + gridPointD ];
	indexLU = permutationTable[ indexL + gridPointU ];
	indexRU = permutationTable[ indexR + gridPointU ];

	// Get the s curves at the proper values
	sX = EaseCurve( distFromL );
	sY = EaseCurve( distFromD );

	// Do the dot products for the lower left corner and lower right corners.
	// Interpolate between those dot products value sX to get a.
	q = gradientTable2d[indexLD]; u = Dot2( distFromL, distFromD, q );
	q = gradientTable2d[indexRD]; v = Dot2( distFromR, distFromD, q );
	a = LinearInterp( sX, u, v );

	// Do the dot products for the upper left corner and upper right corners.
	// Interpolate between those dot products at value sX to get b.
	q = gradientTable2d[indexLU]; u = Dot2( distFromL, distFromU, q );
	q = gradientTable2d[indexRU]; v = Dot2( distFromR, distFromU, q );
	b = LinearInterp( sX, u, v );

	// Interpolate between a and b at value sY to get the noise return value.
	return LinearInterp( sY, a, b );
}

/////////////////////////////////////////////////////////////////////
// you guessed it -- 3D coherent noise

float sdPerlin2::Noise3d(float pos[2]) {
	int gridPointL, gridPointR, gridPointD, gridPointU, gridPointB, gridPointF;
	int indexLD, indexLU, indexRD, indexRU;
	float distFromL, distFromR, distFromD, distFromU, distFromB, distFromF;
	float *q, sX, sY, sZ, a, b, c, d, t, u, v;
	register int indexL, indexR;

	if (! initialized) {
		Reseed();
	}

	// find out neighboring grid points to pos and signed distances from pos to them.
	SetupValues(t, 0, gridPointL, gridPointR, distFromL, distFromR, pos);
	SetupValues(t, 1, gridPointD, gridPointU, distFromD, distFromU, pos);
	SetupValues(t, 2, gridPointB, gridPointF, distFromB, distFromF, pos);

	indexL = permutationTable[ gridPointL ];
	indexR = permutationTable[ gridPointR ];

	indexLD = permutationTable[ indexL + gridPointD ];
	indexRD = permutationTable[ indexR + gridPointD ];
	indexLU = permutationTable[ indexL + gridPointU ];
	indexRU = permutationTable[ indexR + gridPointU ];

	sX = EaseCurve( distFromL );
	sY = EaseCurve( distFromD );
	sZ = EaseCurve( distFromB );

	q = gradientTable3d[indexLD+gridPointB]; u = Dot3( distFromL, distFromD, distFromB, q );
	q = gradientTable3d[indexRD+gridPointB]; v = Dot3( distFromR, distFromD, distFromB, q );
	a = LinearInterp( sX, u, v );

	q = gradientTable3d[indexLU+gridPointB]; u = Dot3( distFromL, distFromU, distFromB, q );
	q = gradientTable3d[indexRU+gridPointB]; v = Dot3( distFromR, distFromU, distFromB, q );
	b = LinearInterp( sX, u, v );

	c = LinearInterp( sY, a, b );

	q = gradientTable3d[indexLD+gridPointF]; u = Dot3( distFromL, distFromD, distFromF, q );
	q = gradientTable3d[indexRD+gridPointF]; v = Dot3( distFromR, distFromD, distFromF, q );
	a = LinearInterp( sX, u, v );

	q = gradientTable3d[indexLU+gridPointF]; u = Dot3( distFromL, distFromU, distFromF, q );
	q = gradientTable3d[indexRU+gridPointF]; v = Dot3( distFromR, distFromU, distFromF, q );
	b = LinearInterp( sX, u, v );

	d = LinearInterp( sY, a, b );

	return LinearInterp( sZ, c, d );
}

/////////////////////////////////////////////////////////////////////
// you can call noise component-wise, too.

float sdPerlin2::Noise(float x) { 
	return Noise1d(&x);
}

float sdPerlin2::Noise(float x, float y) { 
	float p[2] = { x, y };
	return Noise2d(p); 
}

float sdPerlin2::Noise(float x, float y, float z) { 
	float p[3] = { x, y, z };
	return Noise3d(p);
}

/////////////////////////////////////////////////////////////////////
// reinitialize with new, random values.

void sdPerlin2::Reseed() {
	srand((unsigned int) (time(NULL) + rand()));
	GenerateLookupTables();
}

/////////////////////////////////////////////////////////////////////
// reinitialize using a user-specified random seed.

void sdPerlin2::Reseed(unsigned int rSeed) {
	srand(rSeed);
	GenerateLookupTables();
}

/////////////////////////////////////////////////////////////////////
// initialize everything during constructor or reseed -- note
// that space was already allocated for the gradientTable
// during the constructor

void sdPerlin2::GenerateLookupTables() {
	unsigned i, j, temp;

	for (i=0; i<NOISE_WRAP_INDEX; i++) {
		// put index into permutationTable[index], we will shuffle later
		permutationTable[i] = i;

		gradientTable1d[i] = RandNoiseFloat();  

		for (j=0; j<2; j++) { gradientTable2d[i][j] = RandNoiseFloat(); }
		Normalize2d(gradientTable2d[i]);

		for (j=0; j<3; j++) { gradientTable3d[i][j] = RandNoiseFloat(); }
		Normalize3d(gradientTable3d[i]);
	}

	// Shuffle permutation table up to NOISE_WRAP_INDEX
	for (i=0; i<NOISE_WRAP_INDEX; i++) {
		j = rand() & NOISE_MOD_MASK;
		temp = permutationTable[i];
		permutationTable[i] = permutationTable[j];
		permutationTable[j] = temp;
	}

	// Add the rest of the table entries in, duplicating 
	// indices and entries so that they can effectively be indexed
	// by unsigned chars.  I think.  Ask Perlin what this is really doing.
	//
	// This is the only part of the algorithm that I don't understand 100%.

	for (i=0; i<NOISE_WRAP_INDEX+2; i++) {
		permutationTable[NOISE_WRAP_INDEX + i] = permutationTable[i];

		gradientTable1d[NOISE_WRAP_INDEX + i] = gradientTable1d[i];

		for (j=0; j<2; j++) {
			gradientTable2d[NOISE_WRAP_INDEX + i][j] = gradientTable2d[i][j]; 
		}

		for (j=0; j<3; j++) {
			gradientTable3d[NOISE_WRAP_INDEX + i][j] = gradientTable3d[i][j]; 
		}
	}

	// And we're done. Set initialized to true
	initialized = 1;
}

////////////////////////////////////

int sdPerlin3::p[ 512 ] = { 
	151,160,137,91,90,15,
	131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
	190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
	88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
	77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
	102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
	135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
	5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
	223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
	129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
	251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
	49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
	138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,

	// and again
	151,160,137,91,90,15,
	131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
	190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
	88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
	77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
	102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
	135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
	5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
	223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
	129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
	251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
	49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
	138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

float sdPerlin3::Noise( float x ) {
	return 0.f;
}

float sdPerlin3::Noise( float x, float y ) {
	return 0.f;
}

float sdPerlin3::Noise( float x, float y, float z ) {
	// Find unit cube that contains point
	int ucX = static_cast< int >( idMath::Floor( x ) ) & 255;
	int ucY = static_cast< int >( idMath::Floor( y ) ) & 255;
	int ucZ = static_cast< int >( idMath::Floor( z ) ) & 255;

	// Find relative x, y, z of point in cube
	x -= idMath::Floor( x );
	y -= idMath::Floor( y );
	z -= idMath::Floor( z );

	// Compute fade curves for each of x, y, z
	float u = Fade( x );
	float v = Fade( y );
	float w = Fade( z );

	// Hash coordinates of the 8 cube corners;
	int a = p[ ucX ] + ucY;
	int aa = p[ a ] + ucZ;
	int ab = p[ a + 1 ] + ucZ;
	int b = p[ ucX + 1 ] + ucY;
	int ba = p[ b ] + ucZ;
	int bb = p[ b + 1 ] + ucZ;

	// And add blended results from 8 corners of cube
	return( Lerp( w, Lerp( v, Lerp( u, Grad( p[ aa ], x, y, z ),
									   Grad( p[ ba ], x - 1, y, z ) ),
							  Lerp( u, Grad( p[ ab ], x, y - 1, z ),
									   Grad( p[ bb ], x - 1, y - 1, z ) ) ),
					 Lerp( v, Lerp( u, Grad( p[ aa + 1 ], x, y, z - 1 ),
									   Grad( p[ ba + 1 ], x - 1, y, z - 1) ),
							  Lerp( u, Grad( p[ ab + 1 ], x, y - 1, z - 1 ),
									   Grad( p[ bb + 1 ], x - 1, y - 1, z - 1 ) ) ) ) );
}
