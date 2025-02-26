//gl suppliment for Quake

#define APIENTRYP APIENTRY *

//contains the extra things that would otherwise be found in glext.h

//typedef void (APIENTRY *qlpMTex2FUNC) (GLenum, GLfloat, GLfloat);
//typedef void (APIENTRY *qlpMTex3FUNC) (GLenum, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY *qlpSelTexFUNC) (GLenum);

extern qlpSelTexFUNC	qglActiveTextureARB;
extern qlpSelTexFUNC	qglClientActiveTextureARB;
//extern qlpMTex3FUNC		qglMultiTexCoord3fARB;
//extern qlpMTex2FUNC		qglMultiTexCoord2fARB;

//This stuff is normally supplied in the <GL/glext.h> header file. I don't actually have one of them, so it's here instead.
#if 0	//change to 1 if you do actually have the file in question - and its up to date.
#include <GL/glext.h>	//would be ideal.
#else

#ifndef GL_TEXTURE_WIDTH
#define GL_TEXTURE_WIDTH                  0x1000
#endif
#ifndef GL_TEXTURE_HEIGHT
#define GL_TEXTURE_HEIGHT                 0x1001
#endif
#ifndef GL_TEXTURE_INTERNAL_FORMAT
#define GL_TEXTURE_INTERNAL_FORMAT		0x1003
#endif

#ifndef GL_DEPTH_COMPONENT
#define GL_DEPTH_COMPONENT                0x1902
#endif

//#ifndef GL_VERSION_1_2
#define GL_CLAMP_TO_EDGE                  0x812F
//#endif


#ifndef GL_MAX_ARRAY_TEXTURE_LAYERS
#define GL_MAX_ARRAY_TEXTURE_LAYERS       0x88FF	/*opengl 3.0*/
#endif


// Added to make morphos and mingw32 crosscompilers to work
/*
./gl/gl_draw.c: In function `GL_Upload32_BGRA':
./gl/gl_draw.c:3251: error: `GL_BGRA_EXT' undeclared (first use in this function)
./gl/gl_draw.c:3251: error: (Each undeclared identifier is reported only once
./gl/gl_draw.c:3251: error: for each function it appears in.)
*/
#ifndef GL_EXT_bgra
#define GL_BGR_EXT							0x80E0	/*core in opengl 1.2*/
#define GL_BGRA_EXT							0x80E1
#endif

#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
#define GL_UNSIGNED_INT_8_8_8_8_REV			0x8367	/*opengl 1.2*/
#endif
#ifndef GL_UNSIGNED_INT_2_10_10_10_REV
#define GL_UNSIGNED_INT_2_10_10_10_REV		0x8368	/*opengl 1.2*/
#endif
#ifndef GL_UNSIGNED_INT_5_9_9_9_REV
#define GL_UNSIGNED_INT_5_9_9_9_REV			0x8C3E	/*opengl 3.0*/
#endif
#ifndef GL_UNSIGNED_SHORT_4_4_4_4_REV
#define GL_UNSIGNED_SHORT_4_4_4_4_REV		0x8365
#endif
#ifndef GL_UNSIGNED_SHORT_1_5_5_5_REV
#define GL_UNSIGNED_SHORT_1_5_5_5_REV		0x8366
#endif
#ifndef GL_UNSIGNED_SHORT_4_4_4_4
#define GL_UNSIGNED_SHORT_4_4_4_4			0x8033
#endif
#ifndef GL_UNSIGNED_SHORT_5_5_5_1
#define GL_UNSIGNED_SHORT_5_5_5_1			0x8034
#endif
#ifndef GL_UNSIGNED_SHORT_5_6_5
#define GL_UNSIGNED_SHORT_5_6_5				0x8363
#endif
#ifndef GL_HALF_FLOAT
#define GL_HALF_FLOAT						0x140B		/*GL_ARB_half_float_pixel*/
#endif
#ifndef GL_HALF_FLOAT_OES
#define GL_HALF_FLOAT_OES					0x8D61		/*GL_OES_texture_half_float*/
#endif
#ifndef GL_UNSIGNED_INT_24_8
#define GL_UNSIGNED_INT_24_8              0x84FA
#endif

#ifndef GL_ARB_multitexture
#define GL_ARB_multitexture 1
#define GL_TEXTURE0_ARB                   0x84C0
#define GL_TEXTURE1_ARB                   0x84C1
#define GL_TEXTURE2_ARB                   0x84C2
#define GL_TEXTURE3_ARB                   0x84C3
#define GL_TEXTURE4_ARB                   0x84C4
#define GL_TEXTURE5_ARB                   0x84C5
#define GL_TEXTURE6_ARB                   0x84C6
#define GL_TEXTURE7_ARB                   0x84C7
#define GL_TEXTURE8_ARB                   0x84C8
#define GL_TEXTURE9_ARB                   0x84C9
#define GL_TEXTURE10_ARB                  0x84CA
#define GL_TEXTURE11_ARB                  0x84CB
#define GL_TEXTURE12_ARB                  0x84CC
#define GL_TEXTURE13_ARB                  0x84CD
#define GL_TEXTURE14_ARB                  0x84CE
#define GL_TEXTURE15_ARB                  0x84CF
#define GL_TEXTURE16_ARB                  0x84D0
#define GL_TEXTURE17_ARB                  0x84D1
#define GL_TEXTURE18_ARB                  0x84D2
#define GL_TEXTURE19_ARB                  0x84D3
#define GL_TEXTURE20_ARB                  0x84D4
#define GL_TEXTURE21_ARB                  0x84D5
#define GL_TEXTURE22_ARB                  0x84D6
#define GL_TEXTURE23_ARB                  0x84D7
#define GL_TEXTURE24_ARB                  0x84D8
#define GL_TEXTURE25_ARB                  0x84D9
#define GL_TEXTURE26_ARB                  0x84DA
#define GL_TEXTURE27_ARB                  0x84DB
#define GL_TEXTURE28_ARB                  0x84DC
#define GL_TEXTURE29_ARB                  0x84DD
#define GL_TEXTURE30_ARB                  0x84DE
#define GL_TEXTURE31_ARB                  0x84DF
#define GL_ACTIVE_TEXTURE_ARB             0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB      0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB          0x84E2
#endif

#ifndef GL_ARB_texture_cube_map
#define GL_ARB_texture_cube_map 1
#define GL_NORMAL_MAP_ARB                 0x8511
#define GL_REFLECTION_MAP_ARB             0x8512
#define GL_TEXTURE_CUBE_MAP_ARB           0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP_ARB   0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB 0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP_ARB     0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB  0x851C
#endif

#ifndef GL_ARB_texture_cube_map_array
#define GL_ARB_texture_cube_map_array 1
#define GL_TEXTURE_CUBE_MAP_ARRAY_ARB     0x9009
#define GL_TEXTURE_BINDING_CUBE_MAP_ARRAY_ARB 0x900A
#define GL_PROXY_TEXTURE_CUBE_MAP_ARRAY_ARB 0x900B
#define GL_SAMPLER_CUBE_MAP_ARRAY_ARB     0x900C
#define GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW_ARB 0x900D
#define GL_INT_SAMPLER_CUBE_MAP_ARRAY_ARB 0x900E
#define GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY_ARB 0x900F
#endif /* GL_ARB_texture_cube_map_array */

#ifndef GL_TEXTURE_2D_ARRAY				//gl 3.0 or GL_EXT_texture_array
#define GL_TEXTURE_2D_ARRAY				0x8C1A
#endif


#ifndef GL_ARB_depth_texture
#define GL_ARB_depth_texture
#define GL_DEPTH_COMPONENT16_ARB          0x81A5
#define GL_DEPTH_COMPONENT24_ARB          0x81A6
#define GL_DEPTH_COMPONENT32_ARB          0x81A7
#define GL_TEXTURE_DEPTH_SIZE_ARB         0x884A
#define GL_DEPTH_TEXTURE_MODE_ARB         0x884B
#endif
#ifndef GL_DEPTH_COMPONENT32F
#define GL_DEPTH_COMPONENT32F	0x8CAC
#endif

//GL_OES_depth_texture adds this because gles otherwise lacks it.
#ifndef GL_UNSIGNED_INT
#define GL_UNSIGNED_INT						0x1405
#endif

#ifndef GL_EXT_packed_depth_stencil
#define GL_DEPTH24_STENCIL8_EXT                           0x88F0
#define GL_DEPTH_STENCIL_EXT                              0x84F9
#define GL_UNSIGNED_INT_24_8_EXT                          0x84FA
#endif

#ifndef GL_ARB_shadow
#define GL_ARB_shadow
#define GL_TEXTURE_COMPARE_MODE_ARB       0x884C
#define GL_TEXTURE_COMPARE_FUNC_ARB       0x884D
#define GL_COMPARE_R_TO_TEXTURE_ARB       0x884E
#endif

#ifndef GL_EXT_texture3D
#define GL_EXT_texture3D 1
#define GL_PACK_SKIP_IMAGES               0x806B
#define GL_PACK_SKIP_IMAGES_EXT           0x806B
#define GL_PACK_IMAGE_HEIGHT              0x806C
#define GL_PACK_IMAGE_HEIGHT_EXT          0x806C
#define GL_UNPACK_SKIP_IMAGES             0x806D
#define GL_UNPACK_SKIP_IMAGES_EXT         0x806D
#define GL_UNPACK_IMAGE_HEIGHT            0x806E
#define GL_UNPACK_IMAGE_HEIGHT_EXT        0x806E
#define GL_TEXTURE_3D                     0x806F
#define GL_TEXTURE_3D_EXT                 0x806F
#define GL_PROXY_TEXTURE_3D               0x8070
#define GL_PROXY_TEXTURE_3D_EXT           0x8070
#define GL_TEXTURE_DEPTH                  0x8071
#define GL_TEXTURE_DEPTH_EXT              0x8071
#define GL_TEXTURE_WRAP_R                 0x8072
#define GL_TEXTURE_WRAP_R_EXT             0x8072
#define GL_MAX_3D_TEXTURE_SIZE            0x8073
#define GL_MAX_3D_TEXTURE_SIZE_EXT        0x8073
#endif








//some of these were needed.
//They were also not in the ones I could find on the web.
//GL_ARB_texture_env_combine
#define  GL_COMBINE_ARB					0x8570
#define  GL_COMBINE_RGB_ARB				0x8571
#define  GL_COMBINE_ALPHA_ARB			0x8572
#define  GL_SOURCE0_RGB_ARB				0x8580
#define  GL_SOURCE1_RGB_ARB				0x8581
#define  GL_SOURCE2_RGB_ARB				0x8582
#define  GL_SOURCE0_ALPHA_ARB			0x8588
#define  GL_SOURCE1_ALPHA_ARB			0x8589
#define  GL_SOURCE2_ALPHA_ARB			0x858A
#define  GL_OPERAND0_RGB_ARB			0x8590
#define  GL_OPERAND1_RGB_ARB			0x8591
#define  GL_OPERAND2_RGB_ARB			0x8592
#define  GL_OPERAND0_ALPHA_ARB			0x8598
#define  GL_OPERAND1_ALPHA_ARB			0x8599
#define  GL_OPERAND2_ALPHA_ARB			0x859A
#define  GL_RGB_SCALE_ARB				0x8573
#define  GL_ADD_SIGNED_ARB				0x8574
#define  GL_INTERPOLATE_ARB				0x8575
#define  GL_SUBTRACT_ARB				0x84E7
#define  GL_CONSTANT_ARB				0x8576
#define  GL_PRIMARY_COLOR_ARB			0x8577
#define  GL_PREVIOUS_ARB				0x8578


#define  GL_DOT3_RGB_ARB   0x86AE
#define  GL_DOT3_RGBA_ARB   0x86AF

//GL_EXT_texture_env_combine
#define  GL_COMBINE_EXT					0x8570
#define  GL_COMBINE_RGB_EXT				0x8571
#define  GL_COMBINE_ALPHA_EXT			0x8572
#define  GL_SOURCE0_RGB_EXT				0x8580
#define  GL_SOURCE1_RGB_EXT				0x8581
#define  GL_SOURCE2_RGB_EXT				0x8582
#define  GL_SOURCE0_ALPHA_EXT			0x8588
#define  GL_SOURCE1_ALPHA_EXT			0x8589
#define  GL_SOURCE2_ALPHA_EXT			0x858A
#define  GL_OPERAND0_RGB_EXT			0x8590
#define  GL_OPERAND1_RGB_EXT			0x8591
#define  GL_OPERAND2_RGB_EXT			0x8592
#define  GL_OPERAND0_ALPHA_EXT			0x8598
#define  GL_OPERAND1_ALPHA_EXT			0x8599
#define  GL_OPERAND2_ALPHA_EXT			0x859A
#define  GL_RGB_SCALE_EXT				0x8573
#define  GL_ADD_SIGNED_EXT				0x8574
#define  GL_INTERPOLATE_EXT				0x8575
#define  GL_CONSTANT_EXT				0x8576
#define  GL_PRIMARY_COLOR_EXT			0x8577
#define  GL_PREVIOUS_EXT				0x8578

//GL_NV_texture_env_combine4
#define  GL_COMBINE4_NV					0x8503
#define  GL_SOURCE3_RGB_NV				0x8583
#define  GL_SOURCE3_ALPHA_NV			0x858B
#define  GL_OPERAND3_RGB_NV				0x8593
#define  GL_OPERAND3_ALPHA_NV			0x859B



/* GL_ARB_texture_compression */
#ifndef GL_ARB_texture_compression
#define GL_ARB_texture_compression 1

#define GL_COMPRESSED_ALPHA_ARB                                 0x84E9
#define GL_COMPRESSED_LUMINANCE_ARB                             0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB                       0x84EB
#define GL_COMPRESSED_INTENSITY_ARB                             0x84EC
#define GL_COMPRESSED_RGB_ARB                                   0x84ED
#define GL_COMPRESSED_RGBA_ARB                                  0x84EE
#define GL_TEXTURE_COMPRESSION_HINT_ARB                         0x84EF
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB                    0x86A0
#define GL_TEXTURE_COMPRESSED_ARB                               0x86A1
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB                   0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS_ARB                       0x86A3

typedef void (APIENTRY *PFNGLCOMPRESSEDTEXIMAGE3DARBPROC)	(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* data);
typedef void (APIENTRY *PFNGLCOMPRESSEDTEXIMAGE2DARBPROC)	(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);
typedef void (APIENTRY *PFNGLCOMPRESSEDTEXIMAGE1DARBPROC)	(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid* data);
typedef void (APIENTRY *PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC)	(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid* data);
typedef void (APIENTRY *PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC)	(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data);
typedef void (APIENTRY *PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC)	(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid* data);
typedef void (APIENTRY *PFNGLGETCOMPRESSEDTEXIMAGEARBPROC)	(GLenum target, GLint lod, const GLvoid* img);

#endif /* GL_ARB_texture_compression */


#ifndef GL_ATI_pn_triangles	//ati truform
#define GL_PN_TRIANGLES_ATI							0x87F0
#define GL_MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI	0x87F1
#define GL_PN_TRIANGLES_POINT_MODE_ATI				0x87F2
#define GL_PN_TRIANGLES_NORMAL_MODE_ATI				0x87F3
#define GL_PN_TRIANGLES_TESSELATION_LEVEL_ATI		0x87F4
#define GL_PN_TRIANGLES_POINT_MODE_LINEAR_ATI		0x87F5
#define GL_PN_TRIANGLES_POINT_MODE_CUBIC_ATI		0x87F6
#define GL_PN_TRIANGLES_NORMAL_MODE_LINEAR_ATI		0x87F7
#define GL_PN_TRIANGLES_NORMAL_MODE_QUADRATIC_ATI	0x87F8

typedef void (APIENTRY *PFNGLPNTRIANGLESIATIPROC)(GLenum pname, GLint param);
typedef void (APIENTRY *PFNGLPNTRIANGLESFATIPROC)(GLenum pname, GLfloat param);
#endif

#ifndef GL_EXT_depth_bounds_test
#define GL_EXT_depth_bounds_test 1
#define GL_DEPTH_BOUNDS_TEST_EXT					0x8890
#endif

#ifndef GL_EXT_stencil_two_side
#define GL_EXT_stencil_two_side 1

#define GL_STENCIL_TEST_TWO_SIDE_EXT				0x8910
#define GL_ACTIVE_STENCIL_FACE_EXT					0x8911

typedef void (APIENTRY * PFNGLACTIVESTENCILFACEEXTPROC) (GLenum face);
#endif

#ifndef GL_EXT_stencil_wrap
#define GL_EXT_stencil_wrap 1
#define GL_INCR_WRAP_EXT							0x8507
#define GL_DECR_WRAP_EXT							0x8508
#endif

#ifndef GL_EXT_texture_filter_anisotropic
#define GL_EXT_texture_filter_anisotropic 1
#define GL_TEXTURE_MAX_ANISOTROPY_EXT				0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT			0x84FF
#endif


#ifndef GL_VERSION_4_1
#define GL_MAX_VERTEX_UNIFORM_VECTORS     0x8DFB
#endif
#ifndef GL_VERSION_2_0
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS  0x8B4A
#endif


#ifndef GL_ARB_vertex_program
#define GL_COLOR_SUM_ARB                  0x8458
#define GL_VERTEX_PROGRAM_ARB             0x8620
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB 0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB   0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB 0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB   0x8625
#define GL_CURRENT_VERTEX_ATTRIB_ARB      0x8626
#define GL_PROGRAM_LENGTH_ARB             0x8627
#define GL_PROGRAM_STRING_ARB             0x8628
#define GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB 0x862E
#define GL_MAX_PROGRAM_MATRICES_ARB       0x862F
#define GL_CURRENT_MATRIX_STACK_DEPTH_ARB 0x8640
#define GL_CURRENT_MATRIX_ARB             0x8641
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB  0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB    0x8643
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB 0x8645
#define GL_PROGRAM_ERROR_POSITION_ARB     0x864B
#define GL_PROGRAM_BINDING_ARB            0x8677
#define GL_MAX_VERTEX_ATTRIBS_ARB         0x8869
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB 0x886A
#define GL_PROGRAM_ERROR_STRING_ARB       0x8874
#define GL_PROGRAM_FORMAT_ASCII_ARB       0x8875
#define GL_PROGRAM_FORMAT_ARB             0x8876
#define GL_PROGRAM_INSTRUCTIONS_ARB       0x88A0
#define GL_MAX_PROGRAM_INSTRUCTIONS_ARB   0x88A1
#define GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB 0x88A2
#define GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB 0x88A3
#define GL_PROGRAM_TEMPORARIES_ARB        0x88A4
#define GL_MAX_PROGRAM_TEMPORARIES_ARB    0x88A5
#define GL_PROGRAM_NATIVE_TEMPORARIES_ARB 0x88A6
#define GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB 0x88A7
#define GL_PROGRAM_PARAMETERS_ARB         0x88A8
#define GL_MAX_PROGRAM_PARAMETERS_ARB     0x88A9
#define GL_PROGRAM_NATIVE_PARAMETERS_ARB  0x88AA
#define GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB 0x88AB
#define GL_PROGRAM_ATTRIBS_ARB            0x88AC
#define GL_MAX_PROGRAM_ATTRIBS_ARB        0x88AD
#define GL_PROGRAM_NATIVE_ATTRIBS_ARB     0x88AE
#define GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB 0x88AF
#define GL_PROGRAM_ADDRESS_REGISTERS_ARB  0x88B0
#define GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB 0x88B1
#define GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB 0x88B2
#define GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB 0x88B3
#define GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB 0x88B4
#define GL_MAX_PROGRAM_ENV_PARAMETERS_ARB 0x88B5
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB 0x88B6
#define GL_TRANSPOSE_CURRENT_MATRIX_ARB   0x88B7
#define GL_MATRIX0_ARB                    0x88C0
#define GL_MATRIX1_ARB                    0x88C1
#define GL_MATRIX2_ARB                    0x88C2
#define GL_MATRIX3_ARB                    0x88C3
#define GL_MATRIX4_ARB                    0x88C4
#define GL_MATRIX5_ARB                    0x88C5
#define GL_MATRIX6_ARB                    0x88C6
#define GL_MATRIX7_ARB                    0x88C7
#define GL_MATRIX8_ARB                    0x88C8
#define GL_MATRIX9_ARB                    0x88C9
#define GL_MATRIX10_ARB                   0x88CA
#define GL_MATRIX11_ARB                   0x88CB
#define GL_MATRIX12_ARB                   0x88CC
#define GL_MATRIX13_ARB                   0x88CD
#define GL_MATRIX14_ARB                   0x88CE
#define GL_MATRIX15_ARB                   0x88CF
#define GL_MATRIX16_ARB                   0x88D0
#define GL_MATRIX17_ARB                   0x88D1
#define GL_MATRIX18_ARB                   0x88D2
#define GL_MATRIX19_ARB                   0x88D3
#define GL_MATRIX20_ARB                   0x88D4
#define GL_MATRIX21_ARB                   0x88D5
#define GL_MATRIX22_ARB                   0x88D6
#define GL_MATRIX23_ARB                   0x88D7
#define GL_MATRIX24_ARB                   0x88D8
#define GL_MATRIX25_ARB                   0x88D9
#define GL_MATRIX26_ARB                   0x88DA
#define GL_MATRIX27_ARB                   0x88DB
#define GL_MATRIX28_ARB                   0x88DC
#define GL_MATRIX29_ARB                   0x88DD
#define GL_MATRIX30_ARB                   0x88DE
#define GL_MATRIX31_ARB                   0x88DF
#endif

#ifndef GL_ARB_fragment_program
#define GL_FRAGMENT_PROGRAM_ARB           0x8804
#define GL_PROGRAM_ALU_INSTRUCTIONS_ARB   0x8805
#define GL_PROGRAM_TEX_INSTRUCTIONS_ARB   0x8806
#define GL_PROGRAM_TEX_INDIRECTIONS_ARB   0x8807
#define GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB 0x8808
#define GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB 0x8809
#define GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB 0x880A
#define GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB 0x880B
#define GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB 0x880C
#define GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB 0x880D
#define GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB 0x880E
#define GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB 0x880F
#define GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB 0x8810
#define GL_MAX_TEXTURE_COORDS_ARB         0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB    0x8872
#endif





#ifndef GL_ARB_vertex_program
#define GL_ARB_vertex_program 1
//typedef void (APIENTRYP PFNGLVERTEXATTRIB1DARBPROC) (GLuint index, GLdouble x);
//typedef void (APIENTRYP PFNGLVERTEXATTRIB1DVARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB1FARBPROC) (GLuint index, GLfloat x);
typedef void (APIENTRYP PFNGLVERTEXATTRIB1FVARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB1SARBPROC) (GLuint index, GLshort x);
typedef void (APIENTRYP PFNGLVERTEXATTRIB1SVARBPROC) (GLuint index, const GLshort *v);
//typedef void (APIENTRYP PFNGLVERTEXATTRIB2DARBPROC) (GLuint index, GLdouble x, GLdouble y);
//typedef void (APIENTRYP PFNGLVERTEXATTRIB2DVARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB2FARBPROC) (GLuint index, GLfloat x, GLfloat y);
typedef void (APIENTRYP PFNGLVERTEXATTRIB2FVARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB2SARBPROC) (GLuint index, GLshort x, GLshort y);
typedef void (APIENTRYP PFNGLVERTEXATTRIB2SVARBPROC) (GLuint index, const GLshort *v);
//typedef void (APIENTRYP PFNGLVERTEXATTRIB3DARBPROC) (GLuint index, GLdouble x, GLdouble y, GLdouble z);
//typedef void (APIENTRYP PFNGLVERTEXATTRIB3DVARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB3FARBPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRYP PFNGLVERTEXATTRIB3FVARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB3SARBPROC) (GLuint index, GLshort x, GLshort y, GLshort z);
typedef void (APIENTRYP PFNGLVERTEXATTRIB3SVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4NBVARBPROC) (GLuint index, const GLbyte *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4NIVARBPROC) (GLuint index, const GLint *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4NSVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4NUBARBPROC) (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4NUBVARBPROC) (GLuint index, const GLubyte *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4NUIVARBPROC) (GLuint index, const GLuint *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4NUSVARBPROC) (GLuint index, const GLushort *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4BVARBPROC) (GLuint index, const GLbyte *v);
//typedef void (APIENTRYP PFNGLVERTEXATTRIB4DARBPROC) (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
//typedef void (APIENTRYP PFNGLVERTEXATTRIB4DVARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4FARBPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4FVARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4IVARBPROC) (GLuint index, const GLint *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4SARBPROC) (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4SVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4UBVARBPROC) (GLuint index, const GLubyte *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4UIVARBPROC) (GLuint index, const GLuint *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4USVARBPROC) (GLuint index, const GLushort *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIBPOINTERARBPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
typedef void (APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYARBPROC) (GLuint index);
typedef void (APIENTRYP PFNGLDISABLEVERTEXATTRIBARRAYARBPROC) (GLuint index);
typedef void (APIENTRYP PFNGLPROGRAMSTRINGARBPROC) (GLenum target, GLenum format, GLsizei len, const GLvoid *string);
typedef void (APIENTRYP PFNGLBINDPROGRAMARBPROC) (GLenum target, GLuint program);
typedef void (APIENTRYP PFNGLDELETEPROGRAMSARBPROC) (GLsizei n, const GLuint *programs);
typedef void (APIENTRYP PFNGLGENPROGRAMSARBPROC) (GLsizei n, GLuint *programs);
//typedef void (APIENTRYP PFNGLPROGRAMENVPARAMETER4DARBPROC) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
//typedef void (APIENTRYP PFNGLPROGRAMENVPARAMETER4DVARBPROC) (GLenum target, GLuint index, const GLdouble *params);
typedef void (APIENTRYP PFNGLPROGRAMENVPARAMETER4FARBPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRYP PFNGLPROGRAMENVPARAMETER4FVARBPROC) (GLenum target, GLuint index, const GLfloat *params);
//typedef void (APIENTRYP PFNGLPROGRAMLOCALPARAMETER4DARBPROC) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
//typedef void (APIENTRYP PFNGLPROGRAMLOCALPARAMETER4DVARBPROC) (GLenum target, GLuint index, const GLdouble *params);
typedef void (APIENTRYP PFNGLPROGRAMLOCALPARAMETER4FARBPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRYP PFNGLPROGRAMLOCALPARAMETER4FVARBPROC) (GLenum target, GLuint index, const GLfloat *params);
//typedef void (APIENTRYP PFNGLGETPROGRAMENVPARAMETERDVARBPROC) (GLenum target, GLuint index, GLdouble *params);
typedef void (APIENTRYP PFNGLGETPROGRAMENVPARAMETERFVARBPROC) (GLenum target, GLuint index, GLfloat *params);
//typedef void (APIENTRYP PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC) (GLenum target, GLuint index, GLdouble *params);
typedef void (APIENTRYP PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC) (GLenum target, GLuint index, GLfloat *params);
typedef void (APIENTRYP PFNGLGETPROGRAMIVARBPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETPROGRAMSTRINGARBPROC) (GLenum target, GLenum pname, GLvoid *string);
//typedef void (APIENTRYP PFNGLGETVERTEXATTRIBDVARBPROC) (GLuint index, GLenum pname, GLdouble *params);
typedef void (APIENTRYP PFNGLGETVERTEXATTRIBFVARBPROC) (GLuint index, GLenum pname, GLfloat *params);
typedef void (APIENTRYP PFNGLGETVERTEXATTRIBIVARBPROC) (GLuint index, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETVERTEXATTRIBPOINTERVARBPROC) (GLuint index, GLenum pname, GLvoid* *pointer);
typedef GLboolean (APIENTRYP PFNGLISPROGRAMARBPROC) (GLuint program);
#endif

#ifndef GL_ARB_fragment_program
#define GL_ARB_fragment_program 1
/* All ARB_fragment_program entry points are shared with ARB_vertex_program. */
#endif




#ifndef GL_ARB_shader_objects
#define GL_ARB_shader_objects 1
#define GL_PROGRAM_OBJECT_ARB			0x8B40
#define GL_OBJECT_TYPE_ARB				0x8B4E
#define GL_OBJECT_SUBTYPE_ARB			0x8B4F
#define GL_OBJECT_DELETE_STATUS_ARB		0x8B80
#define GL_OBJECT_COMPILE_STATUS_ARB	0x8B81
#define GL_OBJECT_LINK_STATUS_ARB		0x8B82
#define GL_OBJECT_VALIDATE_STATUS_ARB	0x8B83
#define GL_OBJECT_INFO_LOG_LENGTH_ARB	0x8B84
#define GL_OBJECT_ATTACHED_OBJECTS_ARB	0x8B85
#define GL_OBJECT_ACTIVE_UNIFORMS_ARB	0x8B86
#define GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB	0x8B87
#define GL_OBJECT_SHADER_SOURCE_LENGTH_ARB		0x8B88
#define GL_SHADER_OBJECT_ARB			0x8B48
#define GL_FLOAT						0x1406
#define GL_FLOAT_VEC2_ARB				0x8B50
#define GL_FLOAT_VEC3_ARB				0x8B51
#define GL_FLOAT_VEC4_ARB				0x8B52
//#define GL_INT							0x1404
#define GL_INT_VEC2_ARB					0x8B53
#define GL_INT_VEC3_ARB					0x8B54
#define GL_INT_VEC4_ARB					0x8B55
#define GL_BOOL_ARB						0x8B56
#define GL_BOOL_VEC2_ARB				0x8B57
#define GL_BOOL_VEC3_ARB				0x8B58
#define GL_BOOL_VEC4_ARB				0x8B59
#define GL_FLOAT_MAT2_ARB				0x8B5A
#define GL_FLOAT_MAT3_ARB				0x8B5B
#define GL_FLOAT_MAT4_ARB				0x8B5C
#define GL_SAMPLER_1D_ARB				0x8B5D
#define GL_SAMPLER_2D_ARB				0x8B5E
#define GL_SAMPLER_3D_ARB				0x8B5F
#define GL_SAMPLER_CUBE_ARB				0x8B60
#define GL_SAMPLER_1D_SHADOW_ARB		0x8B61
#define GL_SAMPLER_2D_SHADOW_ARB		0x8B62
#define GL_SAMPLER_2D_RECT_ARB			0x8B63
#define GL_SAMPLER_2D_RECT_SHADOW_ARB	0x8B64
// dont know if these two should go somewhere better:
#ifdef __APPLE__
typedef void *GLhandleARB; //Royally Fucked.
#else
typedef unsigned int GLhandleARB;
#endif
typedef char         GLcharARB;
typedef void		(APIENTRYP PFNGLDELETEOBJECTARBPROC)		(GLhandleARB obj);
typedef GLhandleARB	(APIENTRYP PFNGLGETHANDLEARBPROC)			(GLenum pname);
typedef void		(APIENTRYP PFNGLDETACHOBJECTARBPROC)		(GLhandleARB containerObj, GLhandleARB attachedObj);
typedef GLhandleARB	(APIENTRYP PFNGLCREATESHADEROBJECTARBPROC)	(GLenum shaderType);
typedef void		(APIENTRYP PFNGLSHADERSOURCEARBPROC)		(GLhandleARB shaderObj, GLsizei count, const GLcharARB* *string, const GLint *length);
typedef void		(APIENTRYP PFNGLCOMPILESHADERARBPROC)		(GLhandleARB shaderObj);
typedef GLhandleARB	(APIENTRYP PFNGLCREATEPROGRAMOBJECTARBPROC)	(void);
typedef void		(APIENTRYP PFNGLATTACHOBJECTARBPROC)		(GLhandleARB containerObj, GLhandleARB obj);
typedef void		(APIENTRYP PFNGLLINKPROGRAMARBPROC)			(GLhandleARB programObj);
typedef void		(APIENTRYP PFNGLUSEPROGRAMOBJECTARBPROC)	(GLhandleARB programObj);
typedef void		(APIENTRYP PFNGLVALIDATEPROGRAMARBPROC)		(GLhandleARB programObj);
typedef void		(APIENTRYP PFNGLUNIFORM1FARBPROC)			(GLint location, GLfloat v0);
typedef void		(APIENTRYP PFNGLUNIFORM2FARBPROC)			(GLint location, GLfloat v0, GLfloat v1);
typedef void		(APIENTRYP PFNGLUNIFORM3FARBPROC)			(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void		(APIENTRYP PFNGLUNIFORM4FARBPROC)			(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void		(APIENTRYP PFNGLUNIFORM1IARBPROC)			(GLint location, GLint v0);
typedef void		(APIENTRYP PFNGLUNIFORM2IARBPROC)			(GLint location, GLint v0, GLint v1);
typedef void		(APIENTRYP PFNGLUNIFORM3IARBPROC)			(GLint location, GLint v0, GLint v1, GLint v2);
typedef void		(APIENTRYP PFNGLUNIFORM4IARBPROC)			(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
typedef void		(APIENTRYP PFNGLUNIFORM1FVARBPROC)			(GLint location, GLsizei count, GLfloat *value);
typedef void		(APIENTRYP PFNGLUNIFORM2FVARBPROC)			(GLint location, GLsizei count, GLfloat *value);
typedef void		(APIENTRYP PFNGLUNIFORM3FVARBPROC)			(GLint location, GLsizei count, GLfloat *value);
typedef void		(APIENTRYP PFNGLUNIFORM4FVARBPROC)			(GLint location, GLsizei count, GLfloat *value);
typedef void		(APIENTRYP PFNGLUNIFORM1IVARBPROC)			(GLint location, GLsizei count, GLint *value);
typedef void		(APIENTRYP PFNGLUNIFORM2IVARBPROC)			(GLint location, GLsizei count, GLint *value);
typedef void		(APIENTRYP PFNGLUNIFORM3IVARBPROC)			(GLint location, GLsizei count, GLint *value);
typedef void		(APIENTRYP PFNGLUNIFORM4IVARBPROC)			(GLint location, GLsizei count, GLint *value);
typedef void		(APIENTRYP PFNGLUNIFORMMATRIX2FVARBPROC)	(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void		(APIENTRYP PFNGLUNIFORMMATRIX3FVARBPROC)	(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void		(APIENTRYP PFNGLUNIFORMMATRIX4FVARBPROC)	(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void        (APIENTRYP PFNGLGETOBJECTPARAMETERFVARBPROC) (GLhandleARB obj, GLenum pname, GLfloat *params);
typedef void        (APIENTRYP PFNGLGETOBJECTPARAMETERIVARBPROC) (GLhandleARB obj, GLenum pname, GLint *params);
typedef void		(APIENTRYP PFNGLGETINFOLOGARBPROC)			(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
typedef void		(APIENTRYP PFNGLGETATTACHEDOBJECTSARB)		(GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj);
typedef GLint		(APIENTRYP PFNGLGETUNIFORMLOCATIONARBPROC)	(GLhandleARB programObj, const GLcharARB *name);
typedef void		(APIENTRYP PFNGLGETACTIVEUNIFORMARBPROC)	(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLsizei *size, GLenum *type, GLcharARB *name);
typedef void		(APIENTRYP PFNGLGETUNIFORMFVARBPROC)		(GLhandleARB programObj, GLint location, GLfloat *parms);
typedef void		(APIENTRYP PFNGLGETUNIFORMIVARBPROC)		(GLhandleARB programObj, GLint location, GLint *parms);
typedef void		(APIENTRYP PFNGLGETSHADERSOURCEARBPROC)		(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source);
#endif // GL_ARB_shader_objects

#ifndef GL_ARB_vertex_shader
#define GL_VERTEX_SHADER_ARB						0x8B31
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB		0x8B4A
#define GL_MAX_VARYING_FLOATS_ARB					0x8B4B
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB		0x8B4C
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB		0x8B4D
#define GL_OBJECT_ACTIVE_ATTRIBUTES_ARB				0x8B89
#define GL_OBJECT_ACTIVE_ATTRIBUTE_MAX_LENGTH_ARB	0x8B8A
#endif

#ifndef GL_GEOMETRY_SHADER_ARB
#define GL_GEOMETRY_SHADER_ARB 0x8DD9
#endif

#ifndef GL_PATCH_VERTICES_ARB //GL_ARB_tessellation_shader lacks _ARB postfix.
#define GL_PATCHES_ARB								0xE
#define GL_PATCH_VERTICES_ARB						0x8E72
#define GL_TESS_EVALUATION_SHADER_ARB				0x8E87
#define GL_TESS_CONTROL_SHADER_ARB					0x8E88
#endif

#ifndef GL_ARB_fragment_shader
#define GL_FRAGMENT_SHADER_ARB						0x8B30
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB		0x8B49
#endif

#define WGL_SAMPLE_BUFFERS_ARB	0x2041
#define WGL_SAMPLES_ARB		0x2042
#define GL_MULTISAMPLE_ARB 0x809D

#ifndef GL_FRAMEBUFFER_SRGB
#define GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB			0x20B2
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB			0x20A9
#define GL_FRAMEBUFFER_SRGB							0x8DB9
#endif
#ifndef GL_FRAMEBUFFER_SRGB_CAPABLE_EXT
#define GL_FRAMEBUFFER_SRGB_CAPABLE_EXT				0x8DBA
#endif


#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT				0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT			0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT			0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT			0x83F3
#endif
#ifndef GL_COMPRESSED_RED_RGTC1
#define	GL_COMPRESSED_RED_RGTC1						0x8DBB
#define	GL_COMPRESSED_SIGNED_RED_RGTC1				0x8DBC
#define	GL_COMPRESSED_RG_RGTC2						0x8DBD
#define	GL_COMPRESSED_SIGNED_RG_RGTC2				0x8DBE
#endif
#ifndef GL_COMPRESSED_RGBA_BPTC_UNORM_ARB
#define GL_COMPRESSED_RGBA_BPTC_UNORM_ARB			0x8E8C
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB		0x8E8D
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB		0x8E8E
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB	0x8E8F
#endif

#ifndef GL_EXT_texture_sRGB
#define GL_EXT_texture_sRGB 1
#define GL_SRGB_EXT                       0x8C40
#define GL_SRGB8_EXT                      0x8C41
#define GL_SRGB_ALPHA_EXT                 0x8C42
#define GL_SRGB8_ALPHA8_EXT               0x8C43
#define GL_SLUMINANCE_ALPHA_EXT           0x8C44
#define GL_SLUMINANCE8_ALPHA8_EXT         0x8C45
#define GL_SLUMINANCE_EXT                 0x8C46
#define GL_SLUMINANCE8_EXT                0x8C47
#define GL_COMPRESSED_SRGB_EXT            0x8C48
#define GL_COMPRESSED_SRGB_ALPHA_EXT      0x8C49
#define GL_COMPRESSED_SLUMINANCE_EXT      0x8C4A
#define GL_COMPRESSED_SLUMINANCE_ALPHA_EXT 0x8C4B
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT  0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
#endif /* GL_EXT_texture_sRGB */

#ifndef GL_EXT_texture_sRGB_R8
#define GL_EXT_texture_sRGB_R8
#define GL_SR8_EXT							0x8FBD
#endif
#ifndef GL_EXT_texture_sRGB_RG8
#define GL_EXT_texture_sRGB_RG8
#define GL_SRG8_EXT							0x8FBE
#endif

#ifndef GL_RGB9_E5
#define GL_RGB9_E5                        0x8C3D	/*opengl 3.0*/
#endif
#ifndef GL_RG
#define GL_RG                             0x8227
#define GL_R8                             0x8229	/*opengl 3.0*/
#define GL_R16                            0x822A	/*opengl 3.0*/
#define GL_RG8                            0x822B	/*opengl 3.0*/
#endif
#ifndef GL_RG8_SNORM
#define GL_R8_SNORM                       0x8F94	/*opengl 3.1*/
#define GL_RG8_SNORM                      0x8F95	/*opengl 3.1*/
#endif

#ifndef GL_R11F_G11F_B10F
#define GL_R11F_G11F_B10F				  0x8C3A
#define GL_UNSIGNED_INT_10F_11F_11F_REV	  0x8C3B
#endif

#ifndef GL_TEXTURE_SWIZZLE_R
#define GL_TEXTURE_SWIZZLE_R              0x8E42
#define GL_TEXTURE_SWIZZLE_G              0x8E43
#define GL_TEXTURE_SWIZZLE_B              0x8E44
#define GL_TEXTURE_SWIZZLE_A              0x8E45
#endif

#ifndef GL_ETC1_RGB8_OES
#define GL_ETC1_RGB8_OES 0x8D64	//4*4 blocks of 8 bytes
#endif
#ifndef GL_COMPRESSED_RGB8_ETC2
#define GL_COMPRESSED_R11_EAC							0x9270
#define GL_COMPRESSED_SIGNED_R11_EAC					0x9271
#define GL_COMPRESSED_RG11_EAC							0x9272
#define GL_COMPRESSED_SIGNED_RG11_EAC					0x9273
#define GL_COMPRESSED_RGB8_ETC2							0x9274	//4*4 blocks of 8 bytes. also accepts valid etc1 images
#define GL_COMPRESSED_SRGB8_ETC2						0x9275
#define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2		0x9276	//
#define GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2	0x9277
#define GL_COMPRESSED_RGBA8_ETC2_EAC					0x9278	//4*4 blocks of 8+8 bytes.
#define GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC				0x9279
#endif
#ifndef GL_COMPRESSED_RGBA_ASTC_4x4_KHR
#define GL_COMPRESSED_RGBA_ASTC_4x4_KHR					0x93B0
#define GL_COMPRESSED_RGBA_ASTC_5x4_KHR					0x93B1
#define GL_COMPRESSED_RGBA_ASTC_5x5_KHR					0x93B2
#define GL_COMPRESSED_RGBA_ASTC_6x5_KHR					0x93B3
#define GL_COMPRESSED_RGBA_ASTC_6x6_KHR					0x93B4
#define GL_COMPRESSED_RGBA_ASTC_8x5_KHR					0x93B5
#define GL_COMPRESSED_RGBA_ASTC_8x6_KHR					0x93B6
#define GL_COMPRESSED_RGBA_ASTC_8x8_KHR					0x93B7
#define GL_COMPRESSED_RGBA_ASTC_10x5_KHR				0x93B8
#define GL_COMPRESSED_RGBA_ASTC_10x6_KHR				0x93B9
#define GL_COMPRESSED_RGBA_ASTC_10x8_KHR				0x93BA
#define GL_COMPRESSED_RGBA_ASTC_10x10_KHR				0x93BB
#define GL_COMPRESSED_RGBA_ASTC_12x10_KHR				0x93BC
#define GL_COMPRESSED_RGBA_ASTC_12x12_KHR				0x93BD
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR			0x93D0
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR			0x93D1
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR			0x93D2
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR			0x93D3
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR			0x93D4
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR			0x93D5
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR			0x93D6
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR			0x93D7
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR		0x93D8
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR		0x93D9
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR		0x93DA
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR		0x93DB
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR		0x93DC
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR		0x93DD
#endif
#ifndef GL_COMPRESSED_RGBA_ASTC_3x3x3_OES
#define GL_COMPRESSED_RGBA_ASTC_3x3x3_OES          0x93C0
#define GL_COMPRESSED_RGBA_ASTC_4x3x3_OES          0x93C1
#define GL_COMPRESSED_RGBA_ASTC_4x4x3_OES          0x93C2
#define GL_COMPRESSED_RGBA_ASTC_4x4x4_OES          0x93C3
#define GL_COMPRESSED_RGBA_ASTC_5x4x4_OES          0x93C4
#define GL_COMPRESSED_RGBA_ASTC_5x5x4_OES          0x93C5
#define GL_COMPRESSED_RGBA_ASTC_5x5x5_OES          0x93C6
#define GL_COMPRESSED_RGBA_ASTC_6x5x5_OES          0x93C7
#define GL_COMPRESSED_RGBA_ASTC_6x6x5_OES          0x93C8
#define GL_COMPRESSED_RGBA_ASTC_6x6x6_OES          0x93C9
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES  0x93E0
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES  0x93E1
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES  0x93E2
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES  0x93E3
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES  0x93E4
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES  0x93E5
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES  0x93E6
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES  0x93E7
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES  0x93E8
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES  0x93E9
#endif

#ifndef GL_ARB_pixel_buffer_object
#define GL_PIXEL_PACK_BUFFER_ARB                        0x88EB
#define GL_PIXEL_UNPACK_BUFFER_ARB                      0x88EC
#endif

#ifndef GL_ARB_vertex_buffer_object
#define GL_BUFFER_SIZE_ARB                0x8764
#define GL_BUFFER_USAGE_ARB               0x8765
#define GL_ARRAY_BUFFER_ARB               0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB       0x8893
#define GL_ARRAY_BUFFER_BINDING_ARB       0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB 0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB 0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB 0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB 0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB 0x8899
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB 0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB 0x889B
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB 0x889C
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB 0x889D
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB 0x889E
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB 0x889F
#define GL_READ_ONLY_ARB                  0x88B8
#define GL_WRITE_ONLY_ARB                 0x88B9
#define GL_READ_WRITE_ARB                 0x88BA
#define GL_BUFFER_ACCESS_ARB              0x88BB
#define GL_BUFFER_MAPPED_ARB              0x88BC
#define GL_BUFFER_MAP_POINTER_ARB         0x88BD
#define GL_STREAM_DRAW_ARB                0x88E0
#define GL_STREAM_READ_ARB                0x88E1
#define GL_STREAM_COPY_ARB                0x88E2
#define GL_STATIC_DRAW_ARB                0x88E4
#define GL_STATIC_READ_ARB                0x88E5
#define GL_STATIC_COPY_ARB                0x88E6
#define GL_DYNAMIC_DRAW_ARB               0x88E8
#define GL_DYNAMIC_READ_ARB               0x88E9
#define GL_DYNAMIC_COPY_ARB               0x88EA
#endif




#ifndef GL_SGIS_generate_mipmap
#define GL_SGIS_generate_mipmap 1

#define GL_GENERATE_MIPMAP_SGIS           0x8191
#define GL_GENERATE_MIPMAP_HINT_SGIS      0x8192
#endif


#ifndef GL_EXT_compiled_vertex_array
#define GL_ARRAY_ELEMENT_LOCK_FIRST_EXT   0x81A8
#define GL_ARRAY_ELEMENT_LOCK_COUNT_EXT   0x81A9

#define GL_EXT_compiled_vertex_array 1
#ifdef GL_GLEXT_PROTOTYPES
extern void APIENTRY glLockArraysEXT (GLint, GLsizei);
extern void APIENTRY glUnlockArraysEXT (void);
#endif /* GL_GLEXT_PROTOTYPES */
typedef void (APIENTRY * PFNGLLOCKARRAYSEXTPROC) (GLint first, GLsizei count);
typedef void (APIENTRY * PFNGLUNLOCKARRAYSEXTPROC) (void);
#endif


#ifndef GL_EXT_texture_compression_astc_decode_mode
#define GL_TEXTURE_ASTC_DECODE_PRECISION_EXT	0x8F69
#endif

#ifndef GL_EXT_framebuffer_object
#define GL_INVALID_FRAMEBUFFER_OPERATION_EXT 0x0506
#define GL_MAX_RENDERBUFFER_SIZE_EXT      0x84E8
#define GL_FRAMEBUFFER_BINDING_EXT        0x8CA6
#define GL_RENDERBUFFER_BINDING_EXT       0x8CA7
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT 0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT 0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT 0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT 0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT 0x8CD4
#define GL_FRAMEBUFFER_COMPLETE_EXT       0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT 0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT 0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT 0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT 0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT    0x8CDD
#define GL_MAX_COLOR_ATTACHMENTS_EXT      0x8CDF
#define GL_COLOR_ATTACHMENT0_EXT          0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT          0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT          0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT          0x8CE3
#define GL_COLOR_ATTACHMENT4_EXT          0x8CE4
#define GL_COLOR_ATTACHMENT5_EXT          0x8CE5
#define GL_COLOR_ATTACHMENT6_EXT          0x8CE6
#define GL_COLOR_ATTACHMENT7_EXT          0x8CE7
#define GL_COLOR_ATTACHMENT8_EXT          0x8CE8
#define GL_COLOR_ATTACHMENT9_EXT          0x8CE9
#define GL_COLOR_ATTACHMENT10_EXT         0x8CEA
#define GL_COLOR_ATTACHMENT11_EXT         0x8CEB
#define GL_COLOR_ATTACHMENT12_EXT         0x8CEC
#define GL_COLOR_ATTACHMENT13_EXT         0x8CED
#define GL_COLOR_ATTACHMENT14_EXT         0x8CEE
#define GL_COLOR_ATTACHMENT15_EXT         0x8CEF
#define GL_DEPTH_ATTACHMENT_EXT           0x8D00
#define GL_STENCIL_ATTACHMENT_EXT         0x8D20
#define GL_FRAMEBUFFER_EXT                0x8D40
#define GL_RENDERBUFFER_EXT               0x8D41
#define GL_RENDERBUFFER_WIDTH_EXT         0x8D42
#define GL_RENDERBUFFER_HEIGHT_EXT        0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT 0x8D44
#define GL_STENCIL_INDEX1_EXT             0x8D46
#define GL_STENCIL_INDEX4_EXT             0x8D47
#define GL_STENCIL_INDEX8_EXT             0x8D48
#define GL_STENCIL_INDEX16_EXT            0x8D49
#define GL_RENDERBUFFER_RED_SIZE_EXT      0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE_EXT    0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE_EXT     0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE_EXT    0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE_EXT    0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE_EXT  0x8D55
#endif

#ifndef GL_ARB_framebuffer_object
#define GL_DRAW_FRAMEBUFFER_ARB           0x8CA9
#define GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE 0x8217 
#define GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING_ARB 0x8210
#endif

#ifndef GL_VERSION_3_0
#define GL_MAJOR_VERSION                  0x821B
#define GL_MINOR_VERSION                  0x821C
#define GL_NUM_EXTENSIONS                 0x821D
#endif

//GL_ARB_robustness, core in 4.5
#ifndef GL_GUILTY_CONTEXT_RESET
//#define GL_NO_ERROR						0x0000
#define GL_GUILTY_CONTEXT_RESET				0x8253
#define GL_INNOCENT_CONTEXT_RESET			0x8254
#define GL_UNKNOWN_CONTEXT_RESET			0x8255
#endif

#ifndef GL_DEPTH_CLAMP_ARB
#define GL_DEPTH_CLAMP_ARB			      0x864F
#endif

#ifndef GL_TEXTURE_BASE_LEVEL	//GL_SGIS_texture_lod (core gl 1.2)
#define GL_TEXTURE_BASE_LEVEL 0x813c 
#define GL_TEXTURE_MAX_LEVEL 0x813d 
#endif

#ifndef GL_TEXTURE_LOD_BIAS
#define GL_TEXTURE_LOD_BIAS				0x8501	//gl1.4
#endif

#ifndef GL_RGBA16
#define GL_RGBA16						0x805B	//gl1.1, but not in gles.
#endif
#ifndef GL_RGBA16F
#define GL_RGBA16F						0x881A
#define GL_RGBA32F						0x8814
#endif
#ifndef GL_RGB32F
#define GL_RGB32F						0x8815
#endif
#ifndef GL_R16F
#define GL_R16F							0x822D
#define GL_R32F							0x822E
#endif

#ifndef GL_RED
//gles2 does not support swizzles, but gles3 does
#define GL_RED								0x1903
#define GL_GREEN							0x1904
#define GL_BLUE								0x1905
#endif
#ifndef GL_RGBA8
//gles2 does not support sized formats, but gl1.1 and gles3 do.
#define GL_RGBA8							0x8058
#define GL_RGB8								0x8051
#define GL_RGB10_A2							0x8059
#define GL_RGB5								0x8050	//note: not in gles3. a poor-man's substitute for rgb565
#define GL_RGBA4							0x8056
#define GL_RGB5_A1							0x8057
#endif
#ifndef GL_LUMINANCE8
#define GL_LUMINANCE8						0x8040	//not in gles2, nor gl3core (use gl_red+swizzles for gles3)
#define GL_LUMINANCE8_ALPHA8				0x8045	//not in gles2, nor gl3core (use gl_red+swizzles for gles3)
#endif

#ifndef GL_LINE
#define GL_LINE								0x1B01
#endif
#ifndef GL_POLYGON_OFFSET_LINE
#define GL_POLYGON_OFFSET_LINE				0x2A02
#endif

#ifndef GL_SAMPLES_PASSED_ARB
#define GL_SAMPLES_PASSED_ARB                             0x8914
//#define GL_QUERY_COUNTER_BITS_ARB                         0x8864
//#define GL_CURRENT_QUERY_ARB                              0x8865
#define GL_QUERY_RESULT_ARB                               0x8866
#define GL_QUERY_RESULT_AVAILABLE_ARB                     0x8867
#endif


#endif
