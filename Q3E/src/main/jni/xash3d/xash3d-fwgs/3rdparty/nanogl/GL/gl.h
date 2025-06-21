#ifndef __GL__H__
#define __GL__H__

#ifdef NANOGL_MANGLE_PREPEND
#define GL_MANGLE( x ) p ## x
#else
#define GL_MANGLE( x ) x
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef double GLdouble;
typedef float GLclampf;
typedef double GLclampd;
typedef void GLvoid;
typedef int GLfixed;
typedef int GLclampx;

/* Boolean values */
#define GL_FALSE 0x0
#define GL_TRUE 0x1

/* Data types */
#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_2_BYTES 0x1407
#define GL_3_BYTES 0x1408
#define GL_4_BYTES 0x1409
#define GL_DOUBLE 0x140A

/* StringName */
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_EXTENSIONS 0x1F03

/* TextureEnvMode */
#define GL_MODULATE 0x2100
#define GL_DECAL 0x2101
/*      GL_BLEND */
#define GL_ADD 0x0104
/*      GL_REPLACE */

/* Primitives */
#define GL_POINTS 0x0000
#define GL_LINES 0x0001
#define GL_LINE_LOOP 0x0002
#define GL_LINE_STRIP 0x0003
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN 0x0006
#define GL_QUADS 0x0007
#define GL_QUAD_STRIP 0x0008
#define GL_POLYGON 0x0009

/* EnableCap */
#define GL_FOG 0x0B60
#define GL_LIGHTING 0x0B50
#define GL_TEXTURE_2D 0x0DE1
#define GL_CULL_FACE 0x0B44
#define GL_ALPHA_TEST 0x0BC0
#define GL_BLEND 0x0BE2
#define GL_COLOR_LOGIC_OP 0x0BF2
#define GL_DITHER 0x0BD0
#define GL_STENCIL_TEST 0x0B90
#define GL_DEPTH_TEST 0x0B71
/*      GL_LIGHT0 */
/*      GL_LIGHT1 */
/*      GL_LIGHT2 */
/*      GL_LIGHT3 */
/*      GL_LIGHT4 */
/*      GL_LIGHT5 */
/*      GL_LIGHT6 */
/*      GL_LIGHT7 */
#define GL_POINT_SMOOTH 0x0B10
#define GL_LINE_SMOOTH 0x0B20
#define GL_SCISSOR_TEST 0x0C11
#define GL_COLOR_MATERIAL 0x0B57
#define GL_NORMALIZE 0x0BA1
#define GL_RESCALE_NORMAL 0x803A
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_VERTEX_ARRAY 0x8074
#define GL_NORMAL_ARRAY 0x8075
#define GL_COLOR_ARRAY 0x8076
#define GL_TEXTURE_COORD_ARRAY 0x8078
#define GL_MULTISAMPLE 0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE 0x809E
#define GL_SAMPLE_ALPHA_TO_ONE 0x809F
#define GL_SAMPLE_COVERAGE 0x80A0

/* Texture mapping */
#define GL_TEXTURE_ENV 0x2300
#define GL_TEXTURE_ENV_MODE 0x2200
#define GL_TEXTURE_1D 0x0DE0
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_ENV_COLOR 0x2201
#define GL_TEXTURE_GEN_S 0x0C60
#define GL_TEXTURE_GEN_T 0x0C61
#define GL_TEXTURE_GEN_MODE 0x2500
#define GL_TEXTURE_BORDER_COLOR 0x1004
#define GL_TEXTURE_WIDTH 0x1000
#define GL_TEXTURE_HEIGHT 0x1001
#define GL_TEXTURE_BORDER 0x1005
#define GL_TEXTURE_COMPONENTS 0x1003
#define GL_TEXTURE_RED_SIZE 0x805C
#define GL_TEXTURE_GREEN_SIZE 0x805D
#define GL_TEXTURE_BLUE_SIZE 0x805E
#define GL_TEXTURE_ALPHA_SIZE 0x805F
#define GL_TEXTURE_LUMINANCE_SIZE 0x8060
#define GL_TEXTURE_INTENSITY_SIZE 0x8061
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_OBJECT_LINEAR 0x2401
#define GL_OBJECT_PLANE 0x2501
#define GL_EYE_LINEAR 0x2400
#define GL_EYE_PLANE 0x2502
#define GL_SPHERE_MAP 0x2402
#define GL_DECAL 0x2101
#define GL_MODULATE 0x2100
#define GL_NEAREST 0x2600
#define GL_REPEAT 0x2901
#define GL_CLAMP 0x2900
#define GL_S 0x2000
#define GL_T 0x2001
#define GL_R 0x2002
#define GL_Q 0x2003
#define GL_TEXTURE_GEN_R 0x0C62
#define GL_TEXTURE_GEN_Q 0x0C63
#define GL_CLAMP_TO_EDGE 0x812F

/* Matrix Mode */
#define GL_MATRIX_MODE 0x0BA0
#define GL_MODELVIEW 0x1700
#define GL_PROJECTION 0x1701
#define GL_TEXTURE 0x1702

/* Buffers, Pixel Drawing/Reading */
#define GL_NONE 0x0
#define GL_LEFT 0x0406
#define GL_RIGHT 0x0407
/*GL_FRONT					0x0404 */
/*GL_BACK					0x0405 */
/*GL_FRONT_AND_BACK				0x0408 */
#define GL_FRONT_LEFT 0x0400
#define GL_FRONT_RIGHT 0x0401
#define GL_BACK_LEFT 0x0402
#define GL_BACK_RIGHT 0x0403
#define GL_AUX0 0x0409
#define GL_AUX1 0x040A
#define GL_AUX2 0x040B
#define GL_AUX3 0x040C
#define GL_COLOR_INDEX 0x1900
#define GL_RED 0x1903
#define GL_GREEN 0x1904
#define GL_BLUE 0x1905
#define GL_ALPHA 0x1906
#define GL_LUMINANCE 0x1909
#define GL_LUMINANCE_ALPHA 0x190A
#define GL_ALPHA_BITS 0x0D55
#define GL_RED_BITS 0x0D52
#define GL_GREEN_BITS 0x0D53
#define GL_BLUE_BITS 0x0D54
#define GL_INDEX_BITS 0x0D51
#define GL_SUBPIXEL_BITS 0x0D50
#define GL_AUX_BUFFERS 0x0C00
#define GL_READ_BUFFER 0x0C02
#define GL_DRAW_BUFFER 0x0C01
#define GL_DOUBLEBUFFER 0x0C32
#define GL_STEREO 0x0C33
#define GL_BITMAP 0x1A00
#define GL_COLOR 0x1800
#define GL_DEPTH 0x1801
#define GL_STENCIL 0x1802
#define GL_DITHER 0x0BD0
#define GL_RGB 0x1907
#define GL_RGBA 0x1908

/* Fog */
#define GL_FOG 0x0B60
#define GL_FOG_MODE 0x0B65
#define GL_FOG_DENSITY 0x0B62
#define GL_FOG_COLOR 0x0B66
#define GL_FOG_INDEX 0x0B61
#define GL_FOG_START 0x0B63
#define GL_FOG_END 0x0B64
#define GL_LINEAR 0x2601
#define GL_EXP 0x0800
#define GL_EXP2 0x0801

/* Polygons */
#define GL_POINT 0x1B00
#define GL_LINE 0x1B01
#define GL_FILL 0x1B02
#define GL_CW 0x0900
#define GL_CCW 0x0901
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_POLYGON_MODE 0x0B40
#define GL_POLYGON_SMOOTH 0x0B41
#define GL_POLYGON_STIPPLE 0x0B42
#define GL_EDGE_FLAG 0x0B43
#define GL_CULL_FACE 0x0B44
#define GL_CULL_FACE_MODE 0x0B45
#define GL_FRONT_FACE 0x0B46
#define GL_POLYGON_OFFSET_FACTOR 0x8038
#define GL_POLYGON_OFFSET_UNITS 0x2A00
#define GL_POLYGON_OFFSET_POINT 0x2A01
#define GL_POLYGON_OFFSET_LINE 0x2A02
#define GL_POLYGON_OFFSET_FILL 0x8037

/* Lighting */
#define GL_LIGHTING 0x0B50
#define GL_LIGHT0 0x4000
#define GL_LIGHT1 0x4001
#define GL_LIGHT2 0x4002
#define GL_LIGHT3 0x4003
#define GL_LIGHT4 0x4004
#define GL_LIGHT5 0x4005
#define GL_LIGHT6 0x4006
#define GL_LIGHT7 0x4007
#define GL_SPOT_EXPONENT 0x1205
#define GL_SPOT_CUTOFF 0x1206
#define GL_CONSTANT_ATTENUATION 0x1207
#define GL_LINEAR_ATTENUATION 0x1208
#define GL_QUADRATIC_ATTENUATION 0x1209
#define GL_AMBIENT 0x1200
#define GL_DIFFUSE 0x1201
#define GL_SPECULAR 0x1202
#define GL_SHININESS 0x1601
#define GL_EMISSION 0x1600
#define GL_POSITION 0x1203
#define GL_SPOT_DIRECTION 0x1204
#define GL_AMBIENT_AND_DIFFUSE 0x1602
#define GL_COLOR_INDEXES 0x1603
#define GL_LIGHT_MODEL_TWO_SIDE 0x0B52
#define GL_LIGHT_MODEL_LOCAL_VIEWER 0x0B51
#define GL_LIGHT_MODEL_AMBIENT 0x0B53
#define GL_FRONT_AND_BACK 0x0408
#define GL_SHADE_MODEL 0x0B54
#define GL_FLAT 0x1D00
#define GL_SMOOTH 0x1D01
#define GL_COLOR_MATERIAL 0x0B57
#define GL_COLOR_MATERIAL_FACE 0x0B55
#define GL_COLOR_MATERIAL_PARAMETER 0x0B56
#define GL_NORMALIZE 0x0BA1

/* Blending */
#define GL_BLEND 0x0BE2
#define GL_BLEND_SRC 0x0BE1
#define GL_BLEND_DST 0x0BE0
#define GL_ZERO 0x0
#define GL_ONE 0x1
#define GL_SRC_COLOR 0x0300
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_DST_ALPHA 0x0304
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#define GL_DST_COLOR 0x0306
#define GL_ONE_MINUS_DST_COLOR 0x0307
#define GL_SRC_ALPHA_SATURATE 0x0308

/* ClipPlaneName */
#define GL_CLIP_PLANE0 0x3000
#define GL_CLIP_PLANE1 0x3001
#define GL_CLIP_PLANE2 0x3002
#define GL_CLIP_PLANE3 0x3003
#define GL_CLIP_PLANE4 0x3004
#define GL_CLIP_PLANE5 0x3005

/* OpenGL 1.1 */
#define GL_PROXY_TEXTURE_1D 0x8063
#define GL_PROXY_TEXTURE_2D 0x8064
#define GL_TEXTURE_PRIORITY 0x8066
#define GL_TEXTURE_RESIDENT 0x8067
#define GL_TEXTURE_BINDING_1D 0x8068
#define GL_TEXTURE_BINDING_2D 0x8069
#define GL_TEXTURE_INTERNAL_FORMAT 0x1003
#define GL_ALPHA4 0x803B
#define GL_ALPHA8 0x803C
#define GL_ALPHA12 0x803D
#define GL_ALPHA16 0x803E
#define GL_LUMINANCE4 0x803F
#define GL_LUMINANCE8 0x8040
#define GL_LUMINANCE12 0x8041
#define GL_LUMINANCE16 0x8042
#define GL_LUMINANCE4_ALPHA4 0x8043
#define GL_LUMINANCE6_ALPHA2 0x8044
#define GL_LUMINANCE8_ALPHA8 0x8045
#define GL_LUMINANCE12_ALPHA4 0x8046
#define GL_LUMINANCE12_ALPHA12 0x8047
#define GL_LUMINANCE16_ALPHA16 0x8048
#define GL_INTENSITY 0x8049
#define GL_INTENSITY4 0x804A
#define GL_INTENSITY8 0x804B
#define GL_INTENSITY12 0x804C
#define GL_INTENSITY16 0x804D
#define GL_R3_G3_B2 0x2A10
#define GL_RGB4 0x804F
#define GL_RGB5 0x8050
#define GL_RGB8 0x8051
#define GL_RGB10 0x8052
#define GL_RGB12 0x8053
#define GL_RGB16 0x8054
#define GL_RGBA2 0x8055
#define GL_RGBA4 0x8056
#define GL_RGB5_A1 0x8057
#define GL_RGBA8 0x8058
#define GL_RGB10_A2 0x8059
#define GL_RGBA12 0x805A
#define GL_RGBA16 0x805B
#define GL_UNSIGNED_SHORT_4_4_4_4 0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1 0x8034
#define GL_UNSIGNED_SHORT_5_6_5 0x8363

#define GL_CLIENT_PIXEL_STORE_BIT 0x00000001
#define GL_CLIENT_VERTEX_ARRAY_BIT 0x00000002
#define GL_ALL_CLIENT_ATTRIB_BITS 0xFFFFFFFF
#define GL_CLIENT_ALL_ATTRIB_BITS 0xFFFFFFFF

/* Stencil */
#define GL_STENCIL_TEST 0x0B90
#define GL_STENCIL_WRITEMASK 0x0B98
#define GL_STENCIL_BITS 0x0D57
#define GL_STENCIL_FUNC 0x0B92
#define GL_STENCIL_VALUE_MASK 0x0B93
#define GL_STENCIL_REF 0x0B97
#define GL_STENCIL_FAIL 0x0B94
#define GL_STENCIL_PASS_DEPTH_PASS 0x0B96
#define GL_STENCIL_PASS_DEPTH_FAIL 0x0B95
#define GL_STENCIL_CLEAR_VALUE 0x0B91
#define GL_STENCIL_INDEX 0x1901
#define GL_KEEP 0x1E00
#define GL_REPLACE 0x1E01
#define GL_INCR 0x1E02
#define GL_DECR 0x1E03

/* Hints */
#define GL_FOG_HINT 0x0C54
#define GL_LINE_SMOOTH_HINT 0x0C52
#define GL_PERSPECTIVE_CORRECTION_HINT 0x0C50
#define GL_POINT_SMOOTH_HINT 0x0C51
#define GL_POLYGON_SMOOTH_HINT 0x0C53
#define GL_DONT_CARE 0x1100
#define GL_FASTEST 0x1101
#define GL_NICEST 0x1102

/* Gets */
#define GL_ATTRIB_STACK_DEPTH 0x0BB0
#define GL_CLIENT_ATTRIB_STACK_DEPTH 0x0BB1
#define GL_COLOR_CLEAR_VALUE 0x0C22
#define GL_COLOR_WRITEMASK 0x0C23
#define GL_CURRENT_INDEX 0x0B01
#define GL_CURRENT_COLOR 0x0B00
#define GL_CURRENT_NORMAL 0x0B02
#define GL_CURRENT_RASTER_COLOR 0x0B04
#define GL_CURRENT_RASTER_DISTANCE 0x0B09
#define GL_CURRENT_RASTER_INDEX 0x0B05
#define GL_CURRENT_RASTER_POSITION 0x0B07
#define GL_CURRENT_RASTER_TEXTURE_COORDS 0x0B06
#define GL_CURRENT_RASTER_POSITION_VALID 0x0B08
#define GL_CURRENT_TEXTURE_COORDS 0x0B03
#define GL_INDEX_CLEAR_VALUE 0x0C20
#define GL_INDEX_MODE 0x0C30
#define GL_INDEX_WRITEMASK 0x0C21
#define GL_MODELVIEW_MATRIX 0x0BA6
#define GL_MODELVIEW_STACK_DEPTH 0x0BA3
#define GL_NAME_STACK_DEPTH 0x0D70
#define GL_PROJECTION_MATRIX 0x0BA7
#define GL_PROJECTION_STACK_DEPTH 0x0BA4
#define GL_RENDER_MODE 0x0C40
#define GL_RGBA_MODE 0x0C31
#define GL_TEXTURE_MATRIX 0x0BA8
#define GL_TEXTURE_STACK_DEPTH 0x0BA5
#define GL_VIEWPORT 0x0BA2

/* glPush/PopAttrib bits */
#define GL_CURRENT_BIT 0x00000001
#define GL_POINT_BIT 0x00000002
#define GL_LINE_BIT 0x00000004
#define GL_POLYGON_BIT 0x00000008
#define GL_POLYGON_STIPPLE_BIT 0x00000010
#define GL_PIXEL_MODE_BIT 0x00000020
#define GL_LIGHTING_BIT 0x00000040
#define GL_FOG_BIT 0x00000080
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_ACCUM_BUFFER_BIT 0x00000200
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_VIEWPORT_BIT 0x00000800
#define GL_TRANSFORM_BIT 0x00001000
#define GL_ENABLE_BIT 0x00002000
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_HINT_BIT 0x00008000
#define GL_EVAL_BIT 0x00010000
#define GL_LIST_BIT 0x00020000
#define GL_TEXTURE_BIT 0x00040000
#define GL_SCISSOR_BIT 0x00080000
#define GL_ALL_ATTRIB_BITS 0x000FFFFF

/* Depth buffer */
#define GL_NEVER 0x0200
#define GL_LESS 0x0201
#define GL_EQUAL 0x0202
#define GL_LEQUAL 0x0203
#define GL_GREATER 0x0204
#define GL_NOTEQUAL 0x0205
#define GL_GEQUAL 0x0206
#define GL_ALWAYS 0x0207
#define GL_DEPTH_TEST 0x0B71
#define GL_DEPTH_BITS 0x0D56
#define GL_DEPTH_CLEAR_VALUE 0x0B73
#define GL_DEPTH_FUNC 0x0B74
#define GL_DEPTH_RANGE 0x0B70
#define GL_DEPTH_WRITEMASK 0x0B72
#define GL_DEPTH_COMPONENT 0x1902

/* TextureUnit */
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2
#define GL_TEXTURE3 0x84C3
#define GL_TEXTURE4 0x84C4
#define GL_TEXTURE5 0x84C5
#define GL_TEXTURE6 0x84C6
#define GL_TEXTURE7 0x84C7
#define GL_TEXTURE8 0x84C8
#define GL_TEXTURE9 0x84C9
#define GL_TEXTURE10 0x84CA
#define GL_TEXTURE11 0x84CB
#define GL_TEXTURE12 0x84CC
#define GL_TEXTURE13 0x84CD
#define GL_TEXTURE14 0x84CE
#define GL_TEXTURE15 0x84CF
#define GL_TEXTURE16 0x84D0
#define GL_TEXTURE17 0x84D1
#define GL_TEXTURE18 0x84D2
#define GL_TEXTURE19 0x84D3
#define GL_TEXTURE20 0x84D4
#define GL_TEXTURE21 0x84D5
#define GL_TEXTURE22 0x84D6
#define GL_TEXTURE23 0x84D7
#define GL_TEXTURE24 0x84D8
#define GL_TEXTURE25 0x84D9
#define GL_TEXTURE26 0x84DA
#define GL_TEXTURE27 0x84DB
#define GL_TEXTURE28 0x84DC
#define GL_TEXTURE29 0x84DD
#define GL_TEXTURE30 0x84DE
#define GL_TEXTURE31 0x84DF
#define GL_ACTIVE_TEXTURE 0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE 0x84E1

/* GetPName */
#define GL_CURRENT_COLOR 0x0B00
#define GL_CURRENT_NORMAL 0x0B02
#define GL_CURRENT_TEXTURE_COORDS 0x0B03
#define GL_POINT_SIZE 0x0B11
#define GL_POINT_SIZE_MIN 0x8126
#define GL_POINT_SIZE_MAX 0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE 0x8128
#define GL_POINT_DISTANCE_ATTENUATION 0x8129
#define GL_SMOOTH_POINT_SIZE_RANGE 0x0B12
#define GL_LINE_WIDTH 0x0B21
#define GL_SMOOTH_LINE_WIDTH_RANGE 0x0B22
#define GL_ALIASED_POINT_SIZE_RANGE 0x846D
#define GL_ALIASED_LINE_WIDTH_RANGE 0x846E
#define GL_CULL_FACE_MODE 0x0B45
#define GL_FRONT_FACE 0x0B46
#define GL_SHADE_MODEL 0x0B54
#define GL_DEPTH_RANGE 0x0B70
#define GL_DEPTH_WRITEMASK 0x0B72
#define GL_DEPTH_CLEAR_VALUE 0x0B73
#define GL_DEPTH_FUNC 0x0B74
#define GL_STENCIL_CLEAR_VALUE 0x0B91
#define GL_STENCIL_FUNC 0x0B92
#define GL_STENCIL_VALUE_MASK 0x0B93
#define GL_STENCIL_FAIL 0x0B94
#define GL_STENCIL_PASS_DEPTH_FAIL 0x0B95
#define GL_STENCIL_PASS_DEPTH_PASS 0x0B96
#define GL_STENCIL_REF 0x0B97
#define GL_STENCIL_WRITEMASK 0x0B98
#define GL_MATRIX_MODE 0x0BA0
#define GL_VIEWPORT 0x0BA2
#define GL_MODELVIEW_STACK_DEPTH 0x0BA3
#define GL_PROJECTION_STACK_DEPTH 0x0BA4
#define GL_TEXTURE_STACK_DEPTH 0x0BA5
#define GL_MODELVIEW_MATRIX 0x0BA6
#define GL_PROJECTION_MATRIX 0x0BA7
#define GL_TEXTURE_MATRIX 0x0BA8
#define GL_ALPHA_TEST_FUNC 0x0BC1
#define GL_ALPHA_TEST_REF 0x0BC2
#define GL_BLEND_DST 0x0BE0
#define GL_BLEND_SRC 0x0BE1
#define GL_LOGIC_OP_MODE 0x0BF0
#define GL_SCISSOR_BOX 0x0C10
#define GL_SCISSOR_TEST 0x0C11
#define GL_COLOR_CLEAR_VALUE 0x0C22
#define GL_COLOR_WRITEMASK 0x0C23
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_MAX_LIGHTS 0x0D31
#define GL_MAX_CLIP_PLANES 0x0D32
#define GL_MAX_TEXTURE_SIZE 0x0D33
#define GL_MAX_MODELVIEW_STACK_DEPTH 0x0D36
#define GL_MAX_PROJECTION_STACK_DEPTH 0x0D38
#define GL_MAX_TEXTURE_STACK_DEPTH 0x0D39
#define GL_MAX_VIEWPORT_DIMS 0x0D3A
#define GL_MAX_ELEMENTS_VERTICES 0x80E8
#define GL_MAX_ELEMENTS_INDICES 0x80E9
#define GL_MAX_TEXTURE_UNITS 0x84E2
#define GL_SUBPIXEL_BITS 0x0D50
#define GL_RED_BITS 0x0D52
#define GL_GREEN_BITS 0x0D53
#define GL_BLUE_BITS 0x0D54
#define GL_ALPHA_BITS 0x0D55
#define GL_DEPTH_BITS 0x0D56
#define GL_STENCIL_BITS 0x0D57
#define GL_POLYGON_OFFSET_UNITS 0x2A00
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_POLYGON_OFFSET_FACTOR 0x8038
#define GL_TEXTURE_BINDING_2D 0x8069
#define GL_VERTEX_ARRAY_SIZE 0x807A
#define GL_VERTEX_ARRAY_TYPE 0x807B
#define GL_VERTEX_ARRAY_STRIDE 0x807C
#define GL_NORMAL_ARRAY_TYPE 0x807E
#define GL_NORMAL_ARRAY_STRIDE 0x807F
#define GL_COLOR_ARRAY_SIZE 0x8081
#define GL_COLOR_ARRAY_TYPE 0x8082
#define GL_COLOR_ARRAY_STRIDE 0x8083
#define GL_TEXTURE_COORD_ARRAY_SIZE 0x8088
#define GL_TEXTURE_COORD_ARRAY_TYPE 0x8089
#define GL_TEXTURE_COORD_ARRAY_STRIDE 0x808A
#define GL_VERTEX_ARRAY_POINTER 0x808E
#define GL_NORMAL_ARRAY_POINTER 0x808F
#define GL_COLOR_ARRAY_POINTER 0x8090
#define GL_TEXTURE_COORD_ARRAY_POINTER 0x8092
#define GL_SAMPLE_BUFFERS 0x80A8
#define GL_SAMPLES 0x80A9
#define GL_SAMPLE_COVERAGE_VALUE 0x80AA
#define GL_SAMPLE_COVERAGE_INVERT 0x80AB

/* ErrorCode */
#define GL_NO_ERROR 0
#define GL_INVALID_ENUM 0x0500
#define GL_INVALID_VALUE 0x0501
#define GL_INVALID_OPERATION 0x0502
#define GL_STACK_OVERFLOW 0x0503
#define GL_STACK_UNDERFLOW 0x0504
#define GL_OUT_OF_MEMORY 0x0505

// Vladimir
// #define glVertex2i( x, y ) glVertex3f( x, y, 0.0 )
// #define glTexCoord2d glTexCoord2f
// #define glVertex3d glVertex3f
//

void GL_MANGLE( glBegin )( GLenum mode );
void GL_MANGLE( glEnd )( void );
void GL_MANGLE( glEnable )( GLenum cap );
void GL_MANGLE(glDisable)( GLenum cap );
void GL_MANGLE(glVertex2f)( GLfloat x, GLfloat y );
void GL_MANGLE(glColor3f)( GLfloat red, GLfloat green, GLfloat blue );
void GL_MANGLE(glTexCoord2f)( GLfloat s, GLfloat t );
void GL_MANGLE(glViewport)( GLint x, GLint y, GLsizei width, GLsizei height );
void GL_MANGLE(glLoadIdentity)( void );
void GL_MANGLE(glColor4f)( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
void GL_MANGLE(glOrtho)( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
void GL_MANGLE(glMatrixMode)( GLenum mode );
void GL_MANGLE(glTexParameterf)( GLenum target, GLenum pname, GLfloat param );
void GL_MANGLE(glTexImage2D)( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels );
void GL_MANGLE(glDrawBuffer)( GLenum mode );
void GL_MANGLE(glTranslatef)( GLfloat x, GLfloat y, GLfloat z );
void GL_MANGLE(glRotatef)( GLfloat angle, GLfloat x, GLfloat y, GLfloat z );
void GL_MANGLE(glScalef)( GLfloat x, GLfloat y, GLfloat z );
void GL_MANGLE(glDepthRange)(GLclampd zNear, GLclampd zFar );
void GL_MANGLE(glDepthFunc)( GLenum func );
void GL_MANGLE(glFinish)( void );
void GL_MANGLE(glGetFloatv)( GLenum pname, GLfloat *params );
void GL_MANGLE(glGetIntegerv)( GLenum pname, GLint *params );
void GL_MANGLE(glCullFace)( GLenum mode );
void GL_MANGLE(glFrustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
void GL_MANGLE(glClear)( GLbitfield mask );
void GL_MANGLE(glVertex3f)( GLfloat x, GLfloat y, GLfloat z );
void GL_MANGLE(glColor4fv)( const GLfloat *v );
void GL_MANGLE(glHint)( GLenum target, GLenum mode );
void GL_MANGLE(glBlendFunc)( GLenum sfactor, GLenum dfactor );
void GL_MANGLE(glPopMatrix)( void );
void GL_MANGLE(glShadeModel)( GLenum mode );
void GL_MANGLE(glPushMatrix)( void );
void GL_MANGLE(glTexEnvf)( GLenum target, GLenum pname, GLfloat param );
void GL_MANGLE(glVertex3fv)( const GLfloat *v );
void GL_MANGLE(glDepthMask)( GLboolean flag );
void GL_MANGLE(glBindTexture)( GLenum target, GLuint texture );
const GLubyte * GL_MANGLE(glGetString)( GLenum name );
void GL_MANGLE(glAlphaFunc)( GLenum func, GLclampf ref );
void GL_MANGLE(glFlush)( void );
void GL_MANGLE(glReadPixels)( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels );
void GL_MANGLE(glReadBuffer)( GLenum mode );
void GL_MANGLE(glLoadMatrixf)( const GLfloat *m );
void GL_MANGLE(glTexSubImage2D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels );
void GL_MANGLE(glClearColor)( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha );
GLenum GL_MANGLE(glGetError)( void );
void GL_MANGLE(glActiveTexture)( GLenum texture );
void GL_MANGLE(glClientActiveTexture)( GLenum texture );
void GL_MANGLE(glActiveTextureARB)( GLenum texture );
void GL_MANGLE(glClientActiveTextureARB)( GLenum texture );
void GL_MANGLE(glColor3ubv)( const GLubyte *v );
void GL_MANGLE(glPolygonMode)( GLenum face, GLenum mode );

void GL_MANGLE(glArrayElement)( GLint i );
void GL_MANGLE(glLineWidth)( GLfloat width );
void GL_MANGLE(glCallList)( GLuint list );
void GL_MANGLE(glTexCoord2fv)( const GLfloat *v );
void GL_MANGLE(glColorMask)( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha );
void GL_MANGLE(glStencilFunc)( GLenum func, GLint ref, GLuint mask );
void GL_MANGLE(glStencilOp)( GLenum fail, GLenum zfail, GLenum zpass );
void GL_MANGLE(glColor4ubv)( const GLubyte *v );

void GL_MANGLE(glDrawElements)( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices );
void GL_MANGLE(glEnableClientState)( GLenum array );
void GL_MANGLE(glDisableClientState)( GLenum array );
void GL_MANGLE(glVertexPointer)( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer );
void GL_MANGLE(glTexCoordPointer)( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer );
void GL_MANGLE(glColorPointer)( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer );
void GL_MANGLE(glPolygonOffset)( GLfloat factor, GLfloat units );
void GL_MANGLE(glClearDepth)( GLclampf depth );
void GL_MANGLE(glDeleteTextures)( GLsizei n, const GLuint *textures );
void GL_MANGLE(glTexParameterfv)( GLenum target, GLenum pname, const GLfloat *params );
void GL_MANGLE(glStencilMask)( GLuint mask );
void GL_MANGLE(glClearStencil)( GLint s );
void GL_MANGLE(glScissor)( GLint x, GLint y, GLsizei width, GLsizei height );
void GL_MANGLE(glClipPlane)( GLenum plane, const GLdouble *equation );
void GL_MANGLE(glColor3fv)( const GLfloat *v );
void GL_MANGLE(glPointSize)( GLfloat size );

// Vladimir
void GL_MANGLE(glDrawArrays)( GLenum mode, int first, int count );
void GL_MANGLE(glMultMatrixf)( const GLfloat *m );
void GL_MANGLE(glPixelStorei)( GLenum pname, GLint param );
void GL_MANGLE(glFogi)( GLenum pname, GLint param );
void GL_MANGLE(glFogf)( GLenum pname, GLfloat param );
void GL_MANGLE(glFogfv)( GLenum pname, const GLfloat *params );
void GL_MANGLE(glGetTexParameteriv)( GLenum target, GLenum pname, GLint *params );

void GL_MANGLE(glTexParameteri)( GLenum target, GLenum pname, GLint param );
void GL_MANGLE(glTexParameterf)( GLenum target, GLenum pname, GLfloat param );
void GL_MANGLE(glTexParameterx)( GLenum target, GLenum pname, GLfixed param );
void GL_MANGLE(glGenTextures)( GLsizei n, GLuint *textures );
void GL_MANGLE(glFrontFace)( GLenum mode );

// Rikku2000: Light
void GL_MANGLE(glLightf)( GLenum light, GLenum pname, GLfloat param );
void GL_MANGLE(glLightfv)( GLenum light, GLenum pname, const GLfloat *params );
void GL_MANGLE(glLightModelf)( GLenum pname, GLfloat param );
void GL_MANGLE(glLightModelfv)( GLenum pname, const GLfloat *params );
void GL_MANGLE(glMaterialf)( GLenum face, GLenum pname, GLfloat param );
void GL_MANGLE(glMaterialfv)( GLenum face, GLenum pname, const GLfloat *params );
void GL_MANGLE(glColorMaterial)( GLenum face, GLenum mode );

//nicknekit: for xash3d

void GL_MANGLE(glColor3ub)( GLubyte red, GLubyte green, GLubyte blue );
void GL_MANGLE(glNormal3fv)( const GLfloat *v );
void GL_MANGLE(glCopyTexImage2D)( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border );
void GL_MANGLE(glTexImage1D)( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels );
void GL_MANGLE(glTexImage3D)( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels );
void GL_MANGLE(glTexSubImage1D)( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels );
void GL_MANGLE(glTexSubImage3D)( GLenum target, GLint level,
                      GLint xoffset, GLint yoffset,
                      GLint zoffset, GLsizei width,
                      GLsizei height, GLsizei depth,
                      GLenum format,
                      GLenum type, const GLvoid *pixels );
GLboolean GL_MANGLE(glIsTexture)( GLuint texture );
void GL_MANGLE(glTexGeni)( GLenum coord, GLenum pname, GLint param );
void GL_MANGLE(glTexGenfv)( GLenum coord, GLenum pname, const GLfloat *params );
void GL_MANGLE(glColor4ub)( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );

// for XashXT

void GL_MANGLE(glCopyTexSubImage2D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );

void GL_MANGLE(glTexEnvi)( GLenum target, GLenum pname, GLint param );
void GL_MANGLE(glTexEnvfv)( GLenum target, GLenum pname, const GLfloat *param );

void GL_MANGLE(glBindFramebuffer)( GLenum target, GLuint framebuffer );
void GL_MANGLE(glDeleteFramebuffers)( GLsizei n, const GLuint *framebuffers );
void GL_MANGLE(glGenFramebuffers)( GLsizei n, GLuint *framebuffers );
GLenum GL_MANGLE(glCheckFramebufferStatus)( GLenum target );

//GLboolean GL_MANGLE(glIsRenderbuffer)(GLuint renderbuffer);
void GL_MANGLE(glBindRenderbuffer)( GLenum target, GLuint renderbuffer );
void GL_MANGLE(glDeleteRenderbuffers)( GLsizei n, const GLuint *renderbuffers );
void GL_MANGLE(glGenRenderbuffers)( GLsizei n, GLuint *renderbuffers );

void GL_MANGLE(glRenderbufferStorage)( GLenum target, GLenum internalformat, GLsizei width, GLsizei height );

void GL_MANGLE(glFramebufferTexture2D)( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level );

void GL_MANGLE(glFramebufferRenderbuffer)( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer );

void GL_MANGLE(glNormalPointer)( GLenum type, GLsizei stride, const void *ptr );

void GL_MANGLE(glMultiTexCoord3f)( GLenum, GLfloat, GLfloat, GLfloat );
void GL_MANGLE(glMultiTexCoord3fARB)( GLenum, GLfloat, GLfloat, GLfloat );

void GL_MANGLE(glMultiTexCoord2f)( GLenum, GLfloat, GLfloat );
void GL_MANGLE(glMultiTexCoord2fARB)( GLenum, GLfloat, GLfloat );


void GL_MANGLE(glDrawArrays)( GLenum mode, GLint first, GLsizei count );


void GL_MANGLE(glBindBufferARB)( GLuint target, GLuint index );

void GL_MANGLE(glGenBuffersARB)( GLuint count, GLuint *indexes );

void GL_MANGLE(glDeleteBuffersARB)( GLuint count, GLuint *indexes );

void GL_MANGLE(glBufferDataARB)( GLuint target, GLuint size, void *buffer, GLuint type );

void GL_MANGLE(glBufferSubDataARB)( GLuint target, GLsizei offset, GLsizei size, void *buffer );

GLboolean GL_MANGLE(glIsEnabled)( GLenum cap );

typedef void ( *GL_DEBUG_PROC_ARB )( unsigned int source, unsigned int type, unsigned int id, unsigned int severity, int length, const char* message, void* userParam );
void GL_MANGLE(glDebugMessageControlARB)( GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled );
void GL_MANGLE(glDebugMessageInsertARB)( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* buf );
void GL_MANGLE(glDebugMessageCallbackARB)( GL_DEBUG_PROC_ARB callback, void* userParam );
GLuint GL_MANGLE(glGetDebugMessageLogARB)( GLuint count, GLsizei bufsize, GLenum* sources, GLenum* types, GLuint* ids, GLuint* severities, GLsizei* lengths, char* messageLog );


#ifdef __cplusplus
}
#endif

#endif
