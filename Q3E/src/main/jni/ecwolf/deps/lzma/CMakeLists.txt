cmake_minimum_required( VERSION 3.5.0 )

if(LIBRETRO AND NOT LIBRETRO_STATIC)
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if(POLICY CMP0054)
	cmake_policy(SET CMP0054 NEW)
endif()

#make_release_only()

add_library(${ECWOLF_PREFIX}lzma STATIC
	C/7zArcIn.c
	C/7zBuf.c
	C/7zCrc.c
	C/7zCrcOpt.c
	C/7zDec.c
	C/7zStream.c
	C/Bcj2.c
	C/Bra.c
	C/Bra86.c
	C/BraIA64.c
	C/CpuArch.c
	C/Delta.c
	C/LzFind.c
	C/Lzma2Dec.c
	C/LzmaDec.c
	C/LzmaEnc.c
	C/Ppmd7.c
	C/Ppmd7Dec.c
)

target_include_directories(${ECWOLF_PREFIX}lzma PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/C")

if(LIBRETRO AND LIBRETRO_STATIC)
	target_compile_definitions(${ECWOLF_PREFIX}lzma PRIVATE -DSeqInStream_Read2=ecwolf_SeqInStream_Read2 -DSeqInStream_Read=ecwolf_SeqInStream_Read -DSeqInStream_ReadByte=ecwolf_SeqInStream_ReadByte -DLookInStream_SeekTo=ecwolf_LookInStream_SeekTo -DLookInStream_LookRead=ecwolf_LookInStream_LookRead -DLookInStream_Read2=ecwolf_LookInStream_Read2 -DLookInStream_Read=ecwolf_LookInStream_Read -DSecToLook_CreateVTable=ecwolf_SecToLook_CreateVTable -DSecToRead_CreateVTable=ecwolf_SecToRead_CreateVTable -DCrcGenerateTable=ecwolf_CrcGenerateTable -DCrcUpdateT1=ecwolf_CrcUpdateT1 -Dg_CrcUpdate=ecwolf_g_CrcUpdate -Dg_CrcTable=ecwolf_g_CrcTable -DCrcUpdate=ecwolf_CrcUpdate -Dk7zSignature=ecwolf_k7zSignature -DSzArEx_GetFileNameUtf16=ecwolf_SzArEx_GetFileNameUtf16 -DSzAr_DecodeFolder=ecwolf_SzAr_DecodeFolder -DSzArEx_Extract=ecwolf_SzArEx_Extract -DBuf_Free=ecwolf_Buf_Free -DBuf_Init=ecwolf_Buf_Init -DBuf_Create=ecwolf_Buf_Create -DCrcCalc=ecwolf_CrcCalc -DSzArEx_Open=ecwolf_SzArEx_Open -DSzAr_GetFolderUnpackSize=ecwolf_SzAr_GetFolderUnpackSize -DSzGetNextFolderItem=ecwolf_SzGetNextFolderItem -DSzArEx_Free=ecwolf_SzArEx_Free -DSzArEx_Init=ecwolf_SzArEx_Init -DSzBitUi64s_Free=ecwolf_SzBitUi64s_Free -DSzBitUi32s_Free=ecwolf_SzBitUi32s_Free -Dg_CrcUpdateT4=ecwolf_g_CrcUpdateT4 -Dg_CrcUpdateT8=ecwolf_g_CrcUpdateT8 -DCrcUpdateT4=ecwolf_CrcUpdateT4 -DCrcUpdateT8=ecwolf_CrcUpdateT8 -DLzmaDec_InitDicAndState=ecwolf_LzmaDec_InitDicAndState -DLzmaDec_Init=ecwolf_LzmaDec_Init -DLzmaDec_DecodeToDic=ecwolf_LzmaDec_DecodeToDic -DLzmaDec_DecodeToBuf=ecwolf_LzmaDec_DecodeToBuf -DLzmaDec_FreeProbs=ecwolf_LzmaDec_FreeProbs -DLzmaDec_Free=ecwolf_LzmaDec_Free -DLzmaProps_Decode=ecwolf_LzmaProps_Decode -DLzmaDec_AllocateProbs=ecwolf_LzmaDec_AllocateProbs -DLzmaDec_Allocate=ecwolf_LzmaDec_Allocate -DLzmaDecode=ecwolf_LzmaDecode -DLzma2Dec_AllocateProbs=ecwolf_Lzma2Dec_AllocateProbs -DLzma2Dec_Init=ecwolf_Lzma2Dec_Init -DLzma2Dec_DecodeToDic=ecwolf_Lzma2Dec_DecodeToDic -DBcj2Dec_Init=ecwolf_Bcj2Dec_Init -DBcj2Dec_Decode=ecwolf_Bcj2Dec_Decode -DDelta_Init=ecwolf_Delta_Init -DDelta_Encode=ecwolf_Delta_Encode -DDelta_Decode=ecwolf_Delta_Decode -DLzma2Dec_Allocate=ecwolf_Lzma2Dec_Allocate -DLzma2Dec_DecodeToBuf=ecwolf_Lzma2Dec_DecodeToBuf -DLzma2Decode=ecwolf_Lzma2Decode -DIA64_Convert=ecwolf_IA64_Convert -DSPARC_Convert=ecwolf_SPARC_Convert -Dx86_Convert=ecwolf_x86_Convert -DPPC_Convert=ecwolf_PPC_Convert -DARMT_Convert=ecwolf_ARMT_Convert -DARM_Convert=ecwolf_ARM_Convert)
endif()

if( "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" )
	target_compile_options(${ECWOLF_PREFIX}lzma PRIVATE -Wall -Wextra -fomit-frame-pointer)
endif()

target_compile_definitions(${ECWOLF_PREFIX}lzma PRIVATE -D_7ZIP_PPMD_SUPPPORT)

if( WIN32 )
	target_sources(${ECWOLF_PREFIX}lzma PRIVATE C/LzFindMt.c C/Threads.c)
else()
	target_compile_definitions(${ECWOLF_PREFIX}lzma PRIVATE -D_7ZIP_ST)
endif()
