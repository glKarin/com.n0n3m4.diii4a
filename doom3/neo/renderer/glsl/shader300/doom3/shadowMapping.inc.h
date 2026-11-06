
#if 0
#define ES3_SAMPLE_POISSON_DISK \
        "#define SAMPLES 9\n" \
        "vec2 sampleOffsetTable[SAMPLES] = vec2[SAMPLES](\n" \
        "    vec2(0.0, 0.0),\n" \
        "    vec2(1.0, 1.0), vec2(1.0, -1.0), vec2(-1.0, -1.0), vec2(-1.0, 1.0),\n" \
        "    vec2(1.0, 0.0), vec2(-1.0, 0.0), vec2(0.0, -1.0), vec2(0.0, 1.0)\n" \
        ");\n"
#endif

#define ES3_SHADOW_MAPPING_SAMPLE_POISSON_DISK \
        "#define SAMPLES 12\n" \
        "#define SAMPLE_MULTIPLICATOR (1.0 / 12.0)\n" \
        "const vec2 sampleOffsetTable[SAMPLES] = vec2[SAMPLES](\n"\
        "    vec2( 0.6111618, 0.1050905 ), \n" \
        "    vec2( 0.1088336, 0.1127091 ), \n" \
        "    vec2( 0.3030421, -0.6292974 ), \n" \
        "    vec2( 0.4090526, 0.6716492 ), \n" \
        "    vec2( -0.1608387, -0.3867823 ), \n" \
        "    vec2( 0.7685862, -0.6118501 ), \n" \
        "    vec2( -0.1935026, -0.856501 ), \n" \
        "    vec2( -0.4028573, 0.07754025 ), \n" \
        "    vec2( -0.6411021, -0.4748057 ), \n" \
        "    vec2( -0.1314865, 0.8404058 ), \n" \
        "    vec2( -0.7005203, 0.4596822 ), \n" \
        "    vec2( -0.9713828, -0.06329931 ) \n" \
        "    // , vec2( 0.0, 0.0 )\n" \
        ");\n"

