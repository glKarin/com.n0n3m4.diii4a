
#if 0
#define SAMPLE_POISSON_DISK \
        "#define SAMPLES 9\n" \
        "    vec2 sampleOffsetTable[SAMPLES];\n" \
        "    sampleOffsetTable[0] = vec2( 0.0, 0.0);\n" \
        "    sampleOffsetTable[1] = vec2( 1.0, 1.0); sampleOffsetTable[2] = vec2( 1.0, -1.0); sampleOffsetTable[3] = vec2(-1.0, -1.0); sampleOffsetTable[4] = vec2(-1.0, 1.0);\n" \
        "    sampleOffsetTable[5] = vec2( 1.0, 0.0); sampleOffsetTable[6] = vec2( -1.0, 0.0); sampleOffsetTable[7] = vec2(0.0, -1.0); sampleOffsetTable[8] = vec2(0.0, 1.0);\n"
#endif

#define SHADOW_MAPPING_SAMPLE_POISSON_DISK \
        "#define SAMPLES 12\n" \
        "#define SAMPLE_MULTIPLICATOR (1.0 / 12.0)\n" \
        "vec2 sampleOffsetTable[SAMPLES];\n" \
		"sampleOffsetTable[0] = vec2( 0.6111618, 0.1050905 );\n" \
		"sampleOffsetTable[1] = vec2( 0.1088336, 0.1127091 );\n" \
		"sampleOffsetTable[2] = vec2( 0.3030421, -0.6292974 );\n" \
		"sampleOffsetTable[3] = vec2( 0.4090526, 0.6716492 );\n" \
		"sampleOffsetTable[4] = vec2( -0.1608387, -0.3867823 );\n" \
		"sampleOffsetTable[5] = vec2( 0.7685862, -0.6118501 );\n" \
		"sampleOffsetTable[6] = vec2( -0.1935026, -0.856501 );\n" \
		"sampleOffsetTable[7] = vec2( -0.4028573, 0.07754025 );\n" \
		"sampleOffsetTable[8] = vec2( -0.6411021, -0.4748057 );\n" \
		"sampleOffsetTable[9] = vec2( -0.1314865, 0.8404058 );\n" \
		"sampleOffsetTable[10] = vec2( -0.7005203, 0.4596822 );\n" \
		"sampleOffsetTable[11] = vec2( -0.9713828, -0.06329931 );\n" \
		"// sampleOffsetTable[12] = vec2( 0.0, 0.0 );\n"
#define STENCIL_SHADOW_SAMPLE_POISSON_DISK \
        "#define SAMPLES 16\n" \
        "#define SAMPLE_MULTIPLICATOR (1.0 / 16.0)\n" \
        "vec2 sampleOffsetTable[SAMPLES];\n" \
		"sampleOffsetTable[0] = vec2( -0.94201624, -0.39906216 );\n" \
		"sampleOffsetTable[1] = vec2( 0.94558609, -0.76890725 );\n" \
		"sampleOffsetTable[2] = vec2( -0.094184101, -0.92938870 );\n" \
		"sampleOffsetTable[3] = vec2( 0.34495938, 0.29387760 );\n" \
		"sampleOffsetTable[4] = vec2( -0.91588581, 0.45771432 );\n" \
		"sampleOffsetTable[5] = vec2( -0.81544232, -0.87912464 );\n" \
		"sampleOffsetTable[6] = vec2( -0.38277543, 0.27676845 );\n" \
		"sampleOffsetTable[7] = vec2( 0.97484398, 0.75648379 );\n" \
		"sampleOffsetTable[8] = vec2( 0.44323325, -0.97511554 );\n" \
		"sampleOffsetTable[9] = vec2( 0.53742981, -0.47373420 );\n" \
		"sampleOffsetTable[10] = vec2( -0.26496911, -0.41893023 );\n" \
		"sampleOffsetTable[11] = vec2( 0.79197514, 0.19090188 );\n" \
		"sampleOffsetTable[12] = vec2( -0.24188840, 0.99706507 );\n" \
		"sampleOffsetTable[13] = vec2( -0.81409955, 0.91437590 );\n" \
		"sampleOffsetTable[14] = vec2( 0.19984126, 0.78641367 );\n" \
		"sampleOffsetTable[15] = vec2( 0.14383161, -0.14100790 );\n" \
		"// sampleOffsetTable[16] = vec2( 0.0, 0.0 );\n"
#if 1
#define SHADOW_MAPPING_SAMPLE_CUBE_POISSON_DISK \
"    #define SAMPLES 20\n" \
"    #define SAMPLE_MULTIPLICATOR (1.0 / 20.0)\n" \
"    vec3 sampleOffsetTable[SAMPLES];\n" \
"    sampleOffsetTable[0] = vec3(1.0, 1.0, 1.0); sampleOffsetTable[1] = vec3(1.0, -1.0, 1.0); sampleOffsetTable[2] = vec3(-1.0, -1.0, 1.0); sampleOffsetTable[3] = vec3(-1.0, 1.0, 1.0);\n" \
"    sampleOffsetTable[4] = vec3(1.0, 1.0, -1.0); sampleOffsetTable[5] = vec3(1.0, -1.0, -1.0); sampleOffsetTable[6] = vec3(-1.0, -1.0, -1.0); sampleOffsetTable[7] = vec3(-1.0, 1.0, -1.0);\n" \
"    sampleOffsetTable[8] = vec3(1.0, 1.0, 0.0); sampleOffsetTable[9] = vec3(1.0, -1.0, 0.0); sampleOffsetTable[10] = vec3(-1.0, -1.0, 0.0); sampleOffsetTable[11] = vec3(-1.0, 1.0, 0.0);\n" \
"    sampleOffsetTable[12] = vec3(1.0, 0.0, 1.0); sampleOffsetTable[13] = vec3(-1.0, 0.0, 1.0); sampleOffsetTable[14] = vec3(1.0, 0.0, -1.0); sampleOffsetTable[15] = vec3(-1.0, 0.0, -1.0);\n" \
"    sampleOffsetTable[16] = vec3(0.0, 1.0, 1.0); sampleOffsetTable[17] = vec3(0.0, -1.0, 1.0); sampleOffsetTable[18] = vec3(0.0, -1.0, -1.0); sampleOffsetTable[19] = vec3(0.0, 1.0, -1.0);\n" \
"    // sampleOffsetTable[20] = vec3(0.0, 0.0, 0.0);\n"
#else
#define SHADOW_MAPPING_SAMPLE_CUBE_POISSON_DISK \
        "    #define SAMPLES 8\n" \
        "    #define SAMPLE_MULTIPLICATOR (1.0 / 8.0)\n" \
        "    vec3 sampleOffsetTable[SAMPLES];\n" \
        "    sampleOffsetTable[0] = vec3(1.0, 1.0, 1.0); sampleOffsetTable[1] = vec3(1.0, 1.0, -1.0); sampleOffsetTable[2] = vec3(1.0, -1.0, -1.0); sampleOffsetTable[3] = vec3(1.0, -1.0, 1.0);\n" \
        "    sampleOffsetTable[4] = vec3(-1.0, 1.0, 1.0); sampleOffsetTable[5] = vec3(-1.0, -1.0, 1.0); sampleOffsetTable[6] = vec3(-1.0, 1.0, -1.0); sampleOffsetTable[7] = vec3(-1.0, -1.0, -1.0);\n" \
        "    // sampleOffsetTable[8] = vec3(0.0, 0.0, 0.0);\n"
#endif

#define PACK_FLOAT_FUNC() \
"vec4 pack (highp float depth)\n" \
"{\n" \
"    const highp vec4 bitSh = vec4(256.0 * 256.0 * 256.0, 256.0 * 256.0, 256.0, 1.0);\n" \
"    const highp vec4 bitMsk = vec4(0.0, 1.0 / 256.0, 1.0 / 256.0, 1.0 / 256.0);\n" \
"    highp vec4 comp = fract(depth * bitSh);\n" \
"    comp -= comp.xxyz * bitMsk;\n" \
"    return depth < 1.0 ? comp : vec4(1.0, 1.0, 1.0, 1.0);\n" \
"}\n"
#define UNPACK_FLOAT_FUNC() \
"highp float unpack (vec4 colour)\n" \
"{\n" \
"    const highp vec4 bitShifts = vec4(1.0 / (256.0 * 256.0 * 256.0), 1.0 / (256.0 * 256.0), 1.0 / 256.0, 1.0);\n" \
"    highp float dotc = dot(colour , bitShifts);\n" \
"    return /*all(lessThan(colour, vec4(1.0, 1.0, 1.0, 1.0)))*/ colour.r < 1.0 ? dotc : 1.0;\n" \
"}\n"

#define VECTOR_TO_DEPTH_FUNC(far, near) \
"float VectorToDepthValue(vec3 Vec)\n" \
"{\n" \
"    vec3 AbsVec = abs(Vec);\n" \
"    float LocalZcomp = max(AbsVec.x, max(AbsVec.y, AbsVec.z));\n" \
"    float NormZComp = (" #far " + " #near ") / (" #far " - " #near ") - (2.0 * " #far " * " #near ") / (" #far " - " #near ") / LocalZcomp;\n" \
"    return (NormZComp + 1.0) * 0.5;\n" \
"}\n"

