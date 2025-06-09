typedef union {
  INDEX i;
  float f;
  const char *str;
  float v2[2];
  float v3[3];
  int   i3[3];
  float f12[12];
} YYSTYPE;
#define	c_float	258
#define	c_int	259
#define	c_string	260
#define	k_SE_MESH	261
#define	k_SE_SKELETON	262
#define	k_PARENT	263
#define	k_BONES	264
#define	k_VERTICES	265
#define	k_NORMALS	266
#define	k_UVMAPS	267
#define	k_NAME	268
#define	k_TEXCOORDS	269
#define	k_SURFACES	270
#define	k_TRIANGLE_SET	271
#define	k_WEIGHTS	272
#define	k_WEIGHT_SET	273
#define	k_MORPHS	274
#define	k_RELATIVE	275
#define	k_TRUE	276
#define	k_FALSE	277
#define	k_MORPH_SET	278
#define	k_SE_MESH_END	279
#define	k_SE_SKELETON_END	280
#define	k_SE_ANIM	281
#define	k_SEC_PER_FRAME	282
#define	k_FRAMES	283
#define	k_BONEENVELOPES	284
#define	k_MORPHENVELOPES	285
#define	k_DEFAULT_POSE	286
#define	k_SE_ANIM_END	287
#define	k_ANIM_SET_LIST	288
#define	k_ANIM_ID	289
#define	k_MAX_DISTANCE	290
#define	k_MESHLODLIST	291
#define	k_SKELETONLODLIST	292
#define	k_TRESHOLD	293
#define	k_COMPRESION	294
#define	k_LENGTH	295
#define	k_ANIMSPEED	296
#define	k_SHADER_PARAMS	297
#define	k_SHADER_PARAMS_END	298
#define	k_SHADER_SURFACES	299
#define	k_SHADER_SURFACE	300
#define	k_SHADER_NAME	301
#define	k_SHADER_TEXTURES	302
#define	k_SHADER_UVMAPS	303
#define	k_SHADER_COLORS	304
#define	k_SHADER_FLOATS	305
#define	k_SHADER_FLAGS	306
#define	k_FULL_FACE_FORWARD	307
#define	k_HALF_FACE_FORWARD	308


extern YYSTYPE yylval;
