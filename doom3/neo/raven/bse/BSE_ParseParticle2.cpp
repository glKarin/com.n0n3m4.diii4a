// BSE_ParseParticle2.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop



#include "BSE_Envelope.h"
#include "BSE_Particle.h"
#include "BSE.h"
#include "BSE_SpawnDomains.h"

//k
typedef int16_t __int16;
typedef int8_t __int8;
#define LOWORD(x) ((x) & 0xFFFF) // ((WORD)(x))

rvTrailInfo rvParticleTemplate::sTrailInfo;
rvElectricityInfo rvParticleTemplate::sElectricityInfo;
rvEnvParms rvParticleTemplate::sDefaultEnvelope;
rvEnvParms rvParticleTemplate::sEmptyEnvelope;
rvParticleParms rvParticleTemplate::sSPF_ONE_1;
rvParticleParms rvParticleTemplate::sSPF_ONE_2;
rvParticleParms rvParticleTemplate::sSPF_ONE_3;
rvParticleParms rvParticleTemplate::sSPF_NONE_0;
rvParticleParms rvParticleTemplate::sSPF_NONE_1;
rvParticleParms rvParticleTemplate::sSPF_NONE_3;
bool rvParticleTemplate::sInited = false;

float rvSegmentTemplate::mSegmentBaseCosts[SEG_COUNT];

void rvParticleTemplate::AllocTrail()
{
	if (mTrailInfo != NULL) {
		mTrailInfo = new rvTrailInfo();
	}	
}

bool rvParticleTemplate::ParseBlendParms(rvDeclEffect* effect, idParser* src)
{
	rvParticleTemplate* v3; // edi
	char result; // al
	idLexer* v5; // eax
	int v6; // edi
	idBitMsg** v7; // esi
	int v8; // eax
	idToken token; // [esp+0h] [ebp-60h]
	__int16 v10; // [esp+50h] [ebp-10h]
	int v11; // [esp+5Ch] [ebp-4h]

	v3 = this;
	v10 = 0;
	v11 = 0;
	if (src->ReadToken(&token))
	{
		if (idStr::Icmp(token, "add"))
		{
			src->Error("Invalid blend type");
			return false;
		}
		else
		{
			v3->mFlags |= 0x8000u;
		}
		v11 = -1;
		//idStr::FreeData((idStr*)&token.data);
		result = 1;
	}
	else
	{
		v11 = -1;
	//	idStr::FreeData((idStr*)&token.data);
		result = 0;
	}
	return result;
}

bool rvParticleTemplate::ParseImpact(rvDeclEffect* effect, idParser* src)
{
	int v3; // ebp
	rvParticleTemplate* v4; // esi
	int v6; // edi
	//sdDeclTypeHolder* v7; // eax
	idLexer* v8; // eax
	idBitMsg** v9; // edi
	int v10; // ST0C_4
	idLexer* v11; // eax
	idBitMsg** v12; // edi
	int v13; // ST0C_4
	idToken token; // [esp+4h] [ebp-60h]
	__int16 v15; // [esp+54h] [ebp-10h]
	int v16; // [esp+60h] [ebp-4h]

	v3 = 0;
	v4 = this;
	v15 = 0;
	v16 = 0;
	if (!src->ExpectTokenString("{"))
	{
		v16 = -1;
		return false;
	}
	v4->mFlags |= 0x200u;
	if (!src->ReadToken(&token))
	{
	LABEL_30:
		v16 = -1;	
		return 0;
	}
	while (idStr::Cmp(token, "}"))
	{
		if (idStr::Icmp(token, "effect"))
		{
			if (!idStr::Icmp(token, "remove"))
			{
				if (src->ParseInt() != 0)
					v4->mFlags |= 0x400u;
				else
					v4->mFlags &= 0xFFFFFBFF;
				goto LABEL_29;
			}
			if (idStr::Icmp(token, "bounce"))
			{
				if (idStr::Icmp(token, "physicsDistance"))
				{
					//v11 = src->scriptstack;
					//if (v11)
					//	v3 = v11->line;
					//if (v11)
					//	v12 = (idBitMsg**)v11->filename.data;
					//else
					//	v12 = &s2;
					//v13 = (*(int (**)(void))effect->base->vfptr->gap4)();
					//(*(void(__cdecl**)(netadrtype_t, const char*, int, int, idBitMsg**, int))(*(_DWORD*)common.type + 68))(
					//	common.type,
					//	"^4BSE:^1 Invalid impact parameter '%s' in '%s' (file: %s, line: %d)",
					//	token.alloced,
					//	v13,
					//	v12,
					//	v3);
					src->Error("Invalid impact parameter");
					return false;
				}
				v4->mPhysicsDistance = src->ParseFloat();
			}
			else
			{
				v4->mBounce = src->ParseFloat();
			}
		}
		else
		{
			//idParser::ReadToken(src, (idToken*)((char*)&token + 4));
			src->ReadToken(&token);
			if (v4->mNumImpactEffects >= 4)
			{
				src->Error("too many impact effects");
				goto LABEL_29;
			}
			//v7 = sdSingleton<sdDeclTypeHolder>::GetInstance();
			//v4->mImpactEffects[v4->mNumImpactEffects++] = (rvDeclEffect*)((int(__stdcall*)(int, int, signed int))declManager->vfptr->FindType)(
			//	v7->declEffectsType.declTypeHandle,
			//	v6,
			//	1);

			v4->mImpactEffects[v4->mNumImpactEffects++] = declManager->FindEffect(token);
		}
	LABEL_29:
		if (!src->ReadToken(&token))
			goto LABEL_30;
	}
	v16 = -1;
	return true;
}

bool rvParticleTemplate::ParseTimeout(rvDeclEffect* effect, idParser* src)
{
	rvParticleTemplate* v3; // edi
	idParser* v4; // ebp
	char result; // al
	int v6; // esi
	//sdDeclTypeHolder* v7; // eax
	idLexer* v8; // eax
	int v9; // ebp
	idBitMsg** v10; // esi
	int v11; // ST0C_4
	idLexer* v12; // eax
	int v13; // ebp
	idBitMsg** v14; // esi
	int v15; // ST0C_4
	idToken token; // [esp+4h] [ebp-60h]
	__int16 v17; // [esp+54h] [ebp-10h]
	int v18; // [esp+60h] [ebp-4h]

	v3 = this;
	v17 = 0;
	v4 = src;
	v18 = 0;
	if (src->ExpectTokenString("{"))
	{
		if (src->ReadToken(&token))
		{
			while (idStr::Cmp(token, "}"))
			{
				if (idStr::Icmp(token, "effect"))
				{
					//v12 = v4->scriptstack;
					//if (v12)
					//	v13 = v12->line;
					//else
					//	v13 = 0;
					//if (v12)
					//	v14 = (idBitMsg**)v12->filename.data;
					//else
					//	v14 = &s2;
					//v15 = (*(int (**)(void))effect->base->vfptr->gap4)();
					//(*(void(__cdecl**)(netadrtype_t, const char*, int, int, idBitMsg**, int))(*(_DWORD*)common.type + 68))(
					//	common.type,
					//	"^4BSE:^1 Invalid timeout parameter '%s' in '%s' (file: %s, line: %d)",
					//	token.alloced,
					//	v15,
					//	v14,
					//	v13);
					src->Error("Invalid timeout parameter");
				}
				else
				{
					src->ReadToken(&token);
					if (v3->mNumTimeoutEffects >= 4)
					{
						//v8 = v4->scriptstack;
						//if (v8)
						//	v9 = v8->line;
						//else
						//	v9 = 0;
						//if (v8)
						//	v10 = (idBitMsg**)v8->filename.data;
						//else
						//	v10 = &s2;
						//v11 = (*(int (**)(void))effect->base->vfptr->gap4)();
						//(*(void(__cdecl**)(netadrtype_t, const char*, int, int, idBitMsg**, int))(*(_DWORD*)common.type + 68))(
						//	common.type,
						//	"^4BSE:^1 Too many timeout effects '%s' in '%s' (file: %s, line: %d)",
						//	token.alloced,
						//	v11,
						//	v10,
						//	v9);
						src->Error("Too many timeout effects");
					}
					else
					{
						//v6 = token.alloced;
						//v7 = sdSingleton<sdDeclTypeHolder>::GetInstance();
						//v3->mTimeoutEffects[v3->mNumTimeoutEffects++] = (rvDeclEffect*)((int(__stdcall*)(int, int, signed int))declManager->vfptr->FindType)(
						//	v7->declEffectsType.declTypeHandle,
						//	v6,
						//	1);

						v3->mTimeoutEffects[v3->mNumTimeoutEffects++] = declManager->FindEffect(token);
					}
				}
				if (!src->ReadToken(&token))
					goto LABEL_25;
				v4 = src;
			}
			v18 = -1;
			//idStr::FreeData((idStr*)&token.data);
			result = 1;
		}
		else
		{
		LABEL_25:
			v18 = -1;
			//idStr::FreeData((idStr*)&token.data);
			result = 0;
		}
	}
	else
	{
		v18 = -1;
	//	idStr::FreeData((idStr*)&token.data);
		result = 0;
	}
	return result;
}

rvEnvParms* rvParticleTemplate::ParseMotionParms(idParser* src, int count, rvEnvParms* def)
{
	rvEnvParms* result; // eax
	//sdDetails::sdPoolAlloc<rvEnvParms, 128>* v4; // ecx
	//rvEnvParms* v5; // eax
	rvEnvParms* v6; // esi
	int v7; // ebp
	//sdDeclTypeHolder* v8; // eax
	idLexer* v9; // eax
	int v10; // edx
	idBitMsg** v11; // ecx
	//sdDetails::sdPoolAlloc<rvEnvParms, 128>* v12; // eax
	//sdDetails::sdPoolAlloc<rvEnvParms, 128>* v13; // eax
	idToken token; // [esp+4h] [ebp-60h]
	__int16 v15; // [esp+54h] [ebp-10h]
	int v16; // [esp+60h] [ebp-4h]

	v15 = 0;
	v16 = 0;
	if (src->ExpectTokenString("{"))
	{
		//v4 = sdPoolAllocator<rvEnvParms, &char const* const sdPoolAllocator_rvEnvParms, 128, sdLockingPolicy_None>::GetMemoryManager()->allocator;
		//if (v4 && (v5 = sdDetails::sdPoolAlloc<rvEnvParms, 128>::Alloc(v4)) != 0)
		//{
		//	v5->mStatic = 0;
		//	v5->mFastLookUp = 0;
		//	v6 = v5;
		//}
		//else
		//{
		//	v6 = 0;
		//}
		v6 = new rvEnvParms(); // jmarshall: mem leak
		v6->Init();
		if (src->ReadToken(&token))
		{
			while (idStr::Cmp(token, "}"))
			{
				if (idStr::Icmp(token, "envelope"))
				{
					if (idStr::Icmp(token, "rate"))
					{
						if (idStr::Icmp(token, "count"))
						{
							if (idStr::Icmp(token, "offset"))
							{
								//v9 = src->scriptstack;
								//if (v9)
								//	v10 = v9->line;
								//else
								//	v10 = 0;
								//if (v9)
								//	v11 = (idBitMsg**)v9->filename.data;
								//else
								//	v11 = &s2;
								//(*(void (**)(netadrtype_t, const char*, ...))(*(_DWORD*)common.type + 68))(
								//	common.type,
								//	"^4BSE:^1 Invalid motion parameter '%s' (file: %s, line: %d)",
								//	token.alloced,
								//	v11,
								//	v10);

								src->Error("Invalid motion parameter");
								src->SkipBracedSection(1);
							}
							else
							{
							//	rvParticleTemplate::GetVector(src, count, v6->mEnvOffset);

								src->Parse1DMatrix(count, v6->mEnvOffset.ToFloatPtr(), true);
							}
						}
						else
						{
							//rvParticleTemplate::GetVector(src, count, v6->mRate);
							src->Parse1DMatrix(count, v6->mRate.ToFloatPtr(), true);
							v6->mIsCount = 1;
						}
					}
					else
					{
						//rvParticleTemplate::GetVector(src, count, v6->mRate);
						src->Parse1DMatrix(count, v6->mRate.ToFloatPtr(), true);
						v6->mIsCount = 0;
					}
				}
				else
				{
					//idParser::ReadToken(src, (idToken*)((char*)&token + 4));
					//v7 = token.alloced;
					//v8 = sdSingleton<sdDeclTypeHolder>::GetInstance();
					//v6->mTable = (idDeclTable*)((int(__stdcall*)(int, int, signed int))declManager->vfptr->FindType)(
					//	v8->declTableType.declTypeHandle,
					//	v7,
					//	1);

					src->ReadToken(&token);
					v6->mTable = declManager->FindTable(token);
				}
				if (!src->ReadToken(&token))
					goto LABEL_25;
			}
			v6->Finalize();
			if (v6->Compare(*def))
			{
				if (v6)
				{
// jmarshall - unknown memory
					//v13 = sdPoolAllocator<rvEnvParms, &char const* const sdPoolAllocator_rvEnvParms, 128, sdLockingPolicy_None>::GetMemoryManager()->allocator;
					//if (v13)
					//{
					//	LODWORD(v6[-1].mRate.z) = v13->free;
					//	--v13->active;
					//	++v13->numFree;
					//	v13->free = (sdDetails::sdPoolAlloc<rvEnvParms, 128>::element_t*) & v6[-1].mRate.z;
					//}
// jmarshall - unknown memory
				}
				v16 = -1;
				//idStr::FreeData((idStr*)&token.data);
				result = def;
			}
			else
			{
				v16 = -1;
				//idStr::FreeData((idStr*)&token.data);
				result = v6;
			}
		}
		else
		{
		LABEL_25:
			if (v6)
			{
// jmarshall - unknown memory
				//v12 = sdPoolAllocator<rvEnvParms, &char const* const sdPoolAllocator_rvEnvParms, 128, sdLockingPolicy_None>::GetMemoryManager()->allocator;
				//if (v12)
				//{
				//	LODWORD(v6[-1].mRate.z) = v12->free;
				//	--v12->active;
				//	++v12->numFree;
				//	v12->free = (sdDetails::sdPoolAlloc<rvEnvParms, 128>::element_t*) & v6[-1].mRate.z;
				//}
// jmarshall - unknown memory
			}
			v16 = -1;
		//	idStr::FreeData((idStr*)&token.data);
			result = def;
		}
	}
	else
	{
		v16 = -1;
		//idStr::FreeData((idStr*)&token.data);
		result = def;
	}
	return result;
}


bool rvParticleTemplate::ParseMotionDomains(rvDeclEffect* effect, idParser* src)
{
	rvParticleTemplate* v3; // esi
	idParser* v4; // edi
	char result; // al
	idLexer* v6; // eax
	idBitMsg** v7; // ebp
	int v8; // eax
	idToken token; // [esp+0h] [ebp-60h]
	__int16 v10; // [esp+50h] [ebp-10h]
	int v11; // [esp+5Ch] [ebp-4h]
	idParser* srca; // [esp+68h] [ebp+8h]

	v3 = this;
	v10 = 0;
	v4 = src;
	v11 = 0;
	if (src->ExpectTokenString("{"))
	{
		if (src->ReadToken(&token))
		{
			while (token.Cmp("}"))
			{
				if (token.Icmp("tint"))
				{
					if (token.Icmp("fade"))
					{
						if (token.Icmp("size"))
						{
							if (token.Icmp("rotate"))
							{
								if (token.Icmp("angle"))
								{
									if (token.Icmp("offset"))
									{
										if (token.Icmp("length"))
										{
											//v6 = v4->scriptstack;
											//if (v6)
											//	srca = (idParser*)v6->line;
											//else
											//	srca = 0;
											//if (v6)
											//	v7 = (idBitMsg**)v6->filename.data;
											//else
											//	v7 = &s2;
											//v8 = (*(int (**)(void))effect->base->vfptr->gap4)();
											//(*(void (**)(netadrtype_t, const char*, ...))(*(_DWORD*)common.type + 68))(
											//	common.type,
											//	"^4BSE:^1 Invalid motion domain '%s' in %s (file: %s, line: %d)",
											//	token.alloced,
											//	v8,
											//	v7,
											//	srca);

											src->Error("Invalid motion domain");
											src->SkipBracedSection(1);
										}
										else
										{
											v3->mpLengthEnvelope = rvParticleTemplate::ParseMotionParms(
												v4,
												3,
												&rvParticleTemplate::sDefaultEnvelope);
										}
									}
									else
									{
										v3->mpOffsetEnvelope = rvParticleTemplate::ParseMotionParms(
											v4,
											3,
											&rvParticleTemplate::sDefaultEnvelope);
									}
								}
								else
								{
									v3->mpAngleEnvelope = rvParticleTemplate::ParseMotionParms(
										v4,
										3,
										&rvParticleTemplate::sDefaultEnvelope);
								}
							}
							else
							{
								v3->mpRotateEnvelope = rvParticleTemplate::ParseMotionParms(
									v4,
									(unsigned __int8)v3->mNumRotateParms,
									&rvParticleTemplate::sDefaultEnvelope);
							}
						}
						else
						{
							v3->mpSizeEnvelope = rvParticleTemplate::ParseMotionParms(
								v4,
								(unsigned __int8)v3->mNumSizeParms,
								&rvParticleTemplate::sDefaultEnvelope);
						}
					}
					else
					{
						v3->mpFadeEnvelope = rvParticleTemplate::ParseMotionParms(v4, 1, &rvParticleTemplate::sDefaultEnvelope);
					}
				}
				else
				{
					v3->mpTintEnvelope = rvParticleTemplate::ParseMotionParms(v4, 3, &rvParticleTemplate::sDefaultEnvelope);
				}
				if (!src->ReadToken(&token))
					goto LABEL_27;
			}
			v11 = -1;
			//idStr::FreeData((idStr*)&token.data);
			result = 1;
		}
		else
		{
		LABEL_27:
			v11 = -1;
			//idStr::FreeData((idStr*)&token.data);
			result = 0;
		}
	}
	else
	{
		v11 = -1;
		//idStr::FreeData((idStr*)&token.data);
		result = 0;
	}
	return result;
}

void rvParticleTemplate::FixupParms(rvParticleParms* parms)
{
	rvParticleParms* v1; // esi
	char v2; // cl
	int v3; // edx
	unsigned int v4; // eax
	float* v5; // ebx
	float* v6; // ebp
	int v7; // ecx
	float* v8; // esi
	float* v9; // edi
	float* v10; // esi
	bool maxs_3; // [esp+7h] [ebp-5h]

#if 0// jmarshall fix me
	v1 = parms;
	v2 = parms->mSpawnType;
	v3 = parms->mSpawnType & 3;
	v4 = 4 * ((unsigned int)(unsigned __int8)parms->mSpawnType >> 2);
	if (!v4)
		return;
	if (v4 == 4)
		return;
	maxs_3 = v4 != 8;
	if (v2 == 43 || v2 == 47)
		return;
	v5 = &parms->mMaxs.x;
	v6 = &parms->mMins.x;
	if ((v3 < 2 || *v6 == parms->mMins.y)
		&& (v3 < 3 || *v6 == parms->mMins.z)
		&& (!maxs_3 || *v6 == *v5 && (v3 < 2 || *v6 == parms->mMaxs.y) && (v3 < 3 || *v6 == parms->mMaxs.z)))
	{
		if (0.0 == *v6)
		{
			parms->mSpawnType = v3;
		}
		else if (1.0 == *v6)
		{
			parms->mSpawnType = v3 + 4;
		}
		else
		{
			parms->mSpawnType = v3 + 8;
		}
		goto LABEL_42;
	}
	if (maxs_3)
	{
		v7 = 0;
		if (v3 < 4)
		{
		LABEL_24:
			if (v7 >= v3)
			{
			LABEL_35:
				v1->mSpawnType = v3 + 8;
				goto LABEL_42;
			}
			v10 = &v5[v7];
			while (*v10 == *(float*)((char*)v10 + (char*)v6 - (char*)v5))
			{
				++v7;
				++v10;
				if (v7 >= v3)
				{
					v1 = parms;
					parms->mSpawnType = v3 + 8;
					goto LABEL_42;
				}
			}
		}
		else
		{
			v8 = &parms->mMaxs.y;
			v9 = &parms->mMaxs.x;
			while (*(v8 - 1) == *(v9 - 3))
			{
				if (*v8 != *(v8 - 3))
				{
					++v7;
					break;
				}
				if (v8[1] != *(v9 - 1))
				{
					v7 += 2;
					break;
				}
				if (v8[2] != *v9)
				{
					v7 += 3;
					break;
				}
				v7 += 4;
				v8 += 4;
				v9 += 4;
				if (v7 >= v3 - 3)
				{
					v5 = &parms->mMaxs.x;
					v1 = parms;
					goto LABEL_24;
				}
			}
			v5 = &parms->mMaxs.x;
		}
		if (v7 < v3)
		{
			v1 = parms;
			goto LABEL_42;
		}
		v1 = parms;
		goto LABEL_35;
	}
LABEL_42:
	if (v1->mSpawnType >= 8u)
	{
		if (v3 == 1)
		{
			parms->mMins.y = 0.0;
			v5[1] = 0.0;
			parms->mMins.z = 0.0;
			v5[2] = 0.0;
		}
		else if (v3 == 2)
		{
			parms->mMins.z = 0.0;
			v5[2] = 0.0;
		}
	}
	else
	{
		parms->mMins.z = 0.0;
		parms->mMins.y = 0.0;
		*v6 = 0.0;
	}
	if (v1->mSpawnType <= 0xBu)
	{
		*v5 = *v6;
		v5[1] = parms->mMins.y;
		v5[2] = parms->mMins.z;
	}
	if (v1->mFlags & 2)
	{
		if (v1->mSpawnType <= 0xCu)
			v1->mSpawnType = v3 + 12;
	}
#endif
}

bool rvParticleTemplate::CheckCommonParms(idParser* src, rvParticleParms& parms)
{
	bool result; // al
	idToken token; // [esp+0h] [ebp-60h]
	__int16 v4; // [esp+50h] [ebp-10h]
	int v5; // [esp+5Ch] [ebp-4h]

	if (src->ReadToken(&token))
	{
		while (idStr::Cmp((const char*)token, "}"))
		{
			if (idStr::Icmp((const char*)token, "surface"))
			{
				if (idStr::Icmp((const char*)token, "useEndOrigin"))
				{
					if (idStr::Icmp((const char*)token, "cone"))
					{
						if (idStr::Icmp((const char*)token, "relative"))
						{
							if (idStr::Icmp((const char*)token, "linearSpacing"))
							{
								if (idStr::Icmp((const char*)token, "attenuate"))
								{
									if (!idStr::Icmp((const char*)token, "inverseAttenuate"))
										parms.mFlags |= 0x40u;
								}
								else
								{
									parms.mFlags |= 0x20u;
								}
							}
							else
							{
								parms.mFlags |= 0x10u;
							}
						}
						else
						{
							parms.mFlags |= 8u;
						}
					}
					else
					{
						parms.mFlags |= 4u;
					}
				}
				else
				{
					parms.mFlags |= 2u;
				}
			}
			else
			{
				parms.mFlags |= 1u;
			}
			if (!src->ReadToken(&token))
				goto LABEL_18;
		}
		v5 = -1;
	//	idStr::FreeData((idStr*)&token.data);
		result = 1;
	}
	else
	{
	LABEL_18:
		v5 = -1;
	//	idStr::FreeData((idStr*)&token.data);
		result = 0;
	}
	return result;
}

rvParticleParms* rvParticleTemplate::ParseSpawnParms(rvDeclEffect* effect, idParser* src, int count, rvParticleParms* def) {
	idToken token;
	rvParticleParms* v7; 
	rvParticleParms* v8; 

	if (!src->ExpectTokenString("{")) {
		return def;
	}

	src->ReadToken(&token);

	if (token == "}") {
		return def;
	}

	// jmarshall: mem leak potential
	v7 = new rvParticleParms();
	v8 = v7;

	if (token == "box") {
		v8->mSpawnType = count + 16;
		src->Parse1DMatrix(count, v8->mMins.ToFloatPtr(), true);
		src->ExpectTokenString(",");
		src->Parse1DMatrix(count, v8->mMaxs.ToFloatPtr(), true);

		if (!rvParticleTemplate::CheckCommonParms(src, *v8))
		{
			src->Error("Invalid box parameter!");
		}

		if (v8->mFlags & 1)
			v8->mSpawnType = count + 20;
		FixupParms(v8);
		return v8;
	}
	else if (token == "sphere") {
		v8->mSpawnType = count + 24;

		src->Parse1DMatrix(count, v8->mMins.ToFloatPtr(), true);
		src->ExpectTokenString(",");
		src->Parse1DMatrix(count, v8->mMaxs.ToFloatPtr(), true);

		if (!rvParticleTemplate::CheckCommonParms(src, *v8))
		{
			src->Error("Invalid sphere parameter!");
		}

		if (v8->mFlags & 1)
			v8->mSpawnType = count + 28;
		FixupParms(v8);

		return v8;
	}
	else if (token == "cylinder") {
		v8->mSpawnType = count + 32;

		src->Parse1DMatrix(count, v8->mMins.ToFloatPtr(), true);
		src->ExpectTokenString(",");
		src->Parse1DMatrix(count, v8->mMaxs.ToFloatPtr(), true);

		if (!rvParticleTemplate::CheckCommonParms(src, *v8))
		{
			src->Error("Invalid cylinder parameter!");
		}

		if (v8->mFlags & 1)
			v8->mSpawnType = count + 36;
		FixupParms(v8);

		return v8;
	}
	else if (token == "model") {
		v8->mSpawnType = count + 44;
		src->ReadToken(&token);

		idRenderModel* model = renderModelManager->FindModel(token);
		if (model == NULL) {
			src->Error("Failed to load model %s\n", token.c_str());
		}

		v8->mModelInfo = new sdModelInfo();
		v8->mModelInfo->model = model;

		src->ExpectTokenString(",");
		src->Parse1DMatrix(count, v8->mMins.ToFloatPtr(), true);
		src->ExpectTokenString(",");
		src->Parse1DMatrix(count, v8->mMaxs.ToFloatPtr(), true);

		if (!rvParticleTemplate::CheckCommonParms(src, *v8))
		{
			src->Error("Invalid model parameter!");
		}
		FixupParms(v8);
		return v8;
	}
	else if (token == "spiral") {
		v8->mSpawnType = count + 40;

		src->Parse1DMatrix(count, v8->mMins.ToFloatPtr(), true);
		src->ExpectTokenString(",");
		src->Parse1DMatrix(count, v8->mMaxs.ToFloatPtr(), true);

		v8->mRange = src->ParseFloat();

		if (!rvParticleTemplate::CheckCommonParms(src, *v8))
		{
			src->Error("Invalid spiral parameter!");
		}
		FixupParms(v8);
		return v8;
	}
	else if (token == "line") {
		v8->mSpawnType = count + 12;

		src->Parse1DMatrix(count, v8->mMins.ToFloatPtr(), true);
		src->ExpectTokenString(",");
		src->Parse1DMatrix(count, v8->mMaxs.ToFloatPtr(), true);

		if (!rvParticleTemplate::CheckCommonParms(src, *v8))
		{
			src->Error("Invalid line parameter!");
		}
		FixupParms(v8);
		return v8;
	}
	else if (token == "point") {
		v8->mSpawnType = count + 8;

		src->Parse1DMatrix(count, v8->mMins.ToFloatPtr(), true);

		if (!rvParticleTemplate::CheckCommonParms(src, *v8))
		{
			src->Error("Invalid point parameter!");
		}
		FixupParms(v8);
		return v8;
	}

	return v8;
}

bool rvParticleTemplate::ParseSpawnDomains(rvDeclEffect* effect, idParser* src) {
	idToken token;

	src->ExpectTokenString("{");
	while (true) {
		src->ReadToken(&token);

		if (token == "}")
			break;

		if (token == "windStrength") {
			mpSpawnWindStrength = ParseSpawnParms(effect, src, 1, &rvParticleTemplate::sSPF_NONE_1);
		}
		else if (token == "length") {
			mpSpawnLength = ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_NONE_3);
		}
		else if (token == "offset") {
			mpSpawnOffset = ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_NONE_3);
		}
		else if (token == "angle") {
			mpSpawnAngle = ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_NONE_3);
		}
		else if (token == "rotate") {
			mpSpawnRotate = ParseSpawnParms(effect, src, mNumRotateParms, &rvParticleTemplate::sSPF_NONE_3);
		}
		else if (token == "size") {
			mpSpawnSize = ParseSpawnParms(effect, src, mNumSizeParms, &rvParticleTemplate::sSPF_ONE_3);
		}
		else if (token == "fade") {
			mpSpawnFade = ParseSpawnParms(effect, src, 1, &rvParticleTemplate::sSPF_ONE_1);
		}
		else if (token == "tint") {
			mpSpawnTint = ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_ONE_3);
		}
		else if (token == "friction") {
			mpSpawnFriction = ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_NONE_3);
		}
		else if (token == "acceleration") {
			mpSpawnAcceleration = ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_NONE_3);
		}
		else if (token == "velocity") {
			mpSpawnVelocity = ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_NONE_3);
		}
		else if (token == "direction") {
			mpSpawnDirection = rvParticleTemplate::ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_NONE_3);
			mFlags |= 0x4000u;
		}
		else if (token == "position") {
			mpSpawnPosition = rvParticleTemplate::ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_NONE_3);
		}
		else {
			src->Error("Invalid spawn type %s\n", token.c_str());
		}
	}

	return true;
}

bool rvParticleTemplate::ParseDeathDomains(rvDeclEffect* effect, idParser* src) {
	idToken token;

	src->ExpectTokenString("{");
	while (true) {
		src->ReadToken(&token);

		if (token == "}")
			break;

		if (token == "length") {
			rvParticleParms* v13 = ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_NONE_3);
			bool v7 = mpLengthEnvelope == &rvParticleTemplate::sEmptyEnvelope;
			mpDeathLength = v13;
			if (v7) {
				mpLengthEnvelope = &rvParticleTemplate::sDefaultEnvelope;
			}
		}
		else if (token == "offset") {
			rvParticleParms* v13 = ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_NONE_3);
			bool v7 = mpOffsetEnvelope == &rvParticleTemplate::sEmptyEnvelope;
			mpDeathOffset = v13;
			if (v7) {
				mpOffsetEnvelope = &rvParticleTemplate::sDefaultEnvelope;
			}
		}
		else if (token == "angle") {
			rvParticleParms* v13 = ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_NONE_3);
			bool v7 = mpAngleEnvelope == &rvParticleTemplate::sEmptyEnvelope;
			mpDeathAngle = v13;
			if (v7) {
				mpAngleEnvelope = &rvParticleTemplate::sDefaultEnvelope;
			}
		}
		else if (token == "rotate") {
			rvParticleParms* v10 = ParseSpawnParms(effect, src, mNumRotateParms, &rvParticleTemplate::sSPF_NONE_3);
			mpDeathRotate = v10;
			if (mpRotateEnvelope == &rvParticleTemplate::sEmptyEnvelope) {
				mpRotateEnvelope = &rvParticleTemplate::sDefaultEnvelope;
			}
		}
		else if (token == "size") {
			rvParticleParms* v9 = ParseSpawnParms(effect, src, mNumSizeParms, &rvParticleTemplate::sSPF_ONE_3);
			mpDeathSize = v9;
			if (mpSizeEnvelope == &rvParticleTemplate::sEmptyEnvelope) {
				mpSizeEnvelope = &rvParticleTemplate::sDefaultEnvelope;
			}
		}
		else if (token == "fade") {
			rvParticleParms* v8 = ParseSpawnParms(effect, src, 1, &rvParticleTemplate::sSPF_NONE_1);
			mpDeathFade = v8;
			if (mpFadeEnvelope == &rvParticleTemplate::sEmptyEnvelope) {
				mpFadeEnvelope = &rvParticleTemplate::sDefaultEnvelope;
			}
		}
		else if (token == "tint") {
			rvParticleParms* v6 = ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_NONE_3);
			mpDeathTint = v6;
			if (mpTintEnvelope == &rvParticleTemplate::sEmptyEnvelope) {
				mpTintEnvelope = &rvParticleTemplate::sDefaultEnvelope;
			}
		}
		else {
			src->Error("Invalid end type %s\n", token.c_str());
		}
	}

	return true;
}

bool rvParticleTemplate::Parse(rvDeclEffect* effect, idParser* src) {
	idToken token;

	src->ExpectTokenString("{");	
	while (true) {
		src->ReadToken(&token);

		if (token == "}")
			break;

		if (token == "windDeviationAngle") {
			mWindDeviationAngle = src->ParseFloat();
		}
		else if (token == "timeout") {
			ParseTimeout(effect, src);
		}
		else if (token == "impact") {
			ParseImpact(effect, src);
		}
		else if (token == "model") {
			src->ReadToken(&token);
			mModel = renderModelManager->FindModel(token);

			if (mModel == NULL) {
				mModel = renderModelManager->FindModel("_default");

				src->Warning("No surfaces defined in model %s", token.c_str());
			}
		}
		else if (token == "numFrames") {
			mNumFrames = src->ParseInt();
		}
		else if (token == "fadeIn") {
			// TODO  v3->mFlags |= (unsigned int)&vwin8192[2696]; <-- garbage.
		}
		else if (token == "useLightningAxis") {
			mFlags |= 0x400000u;
		}
		else if (token == "specular") {
			mFlags |= 0x40000u;
		}
		else if (token == "shadows") {
			mFlags |= 0x20000u;
		}
		else if (token == "blend") {
			ParseBlendParms(effect, src);
		}
		else if (token == "entityDef") {
			src->ReadToken(&token);
			mEntityDefName = token;
		}
		else if (token == "material") {
			src->ReadToken(&token);
			mMaterial = declManager->FindMaterial(token);
		}
		else if (token == "trailScale") {
			AllocTrail();
			mTrailInfo->mTrailScale = src->ParseFloat();
		}
		else if (token == "trailCount") {
			AllocTrail();
			mTrailInfo->mTrailCount.x = src->ParseFloat();
			src->ExpectTokenString(",");
			mTrailInfo->mTrailCount.y = src->ParseFloat();
		}
		else if (token == "trailRepeat") {
			AllocTrail();
			mTrailRepeat = src->ParseInt();
		}
		else if (token == "trailTime") {
			AllocTrail();
			mTrailInfo->mTrailTime.x = src->ParseFloat();
			src->ExpectTokenString(",");
			mTrailInfo->mTrailTime.y = src->ParseFloat();
		}
		else if (token == "trailMaterial") {
			AllocTrail();
			src->ReadToken(&token);
			mTrailInfo->mTrailMaterial = declManager->FindMaterial(token);
		}
		else if (token == "trailType") {
			src->ReadToken(&token);

			if (token == "burn") {
				mTrailInfo->mTrailType = 1;
			}
			else if (token == "motion") {
				mTrailInfo->mTrailType = 2;
			}
			else {
				mTrailInfo->mTrailType = 3;
				mTrailInfo->mTrailTypeName = token;
			}
		}
		else if (token == "gravity") {
			mGravity.x = src->ParseFloat();
			src->ExpectTokenString(",");
			mGravity.y = src->ParseFloat();
		}
		else if (token == "duration") {
			float srcb = src->ParseFloat();
			float v8 = 0.0020000001;
			if (srcb >= 0.0020000001)
			{
				v8 = srcb;
				if (srcb > 300.0)
					v8 = 300.0;
			}
			float srcg = v8;
			mDuration.x = srcg;
			src->ExpectTokenString(",");

			float srcc = src->ParseFloat();
			float v9 = 0.0020000001;
			if (srcc < 0.0020000001 || (v9 = srcc, srcc <= 300.0))
			{
				float srch = v9;
				mDuration.y = srch;
			}
			else
			{
				mDuration.y = 300.0;
			}
		}
		else if (token == "parentvelocity") {
			mFlags |= 0x2000000u;
		}
		else if (token == "tiling") {
			mFlags |= 0x100000u;
			float srca = src->ParseFloat();
			float v7 = 0.0020000001;
			if (srca < 0.0020000001 || (v7 = srca, srca <= 1024.0))
			{
				float srcf = v7;
				mTiling = srcf;
			}
			else
			{
				mTiling = 1024.0;
			}
		}
		else if (token == "persist") {
			mFlags |= 0x200000u;
		}
		else if (token == "generatedLine") {
			mFlags |= 0x10000u;
		}
		else if (token == "flipNormal") {
			mFlags |= 0x2000u;
		}
		else if (token == "lineHit") {
			mFlags |= 0x4000000u;
		}
		else if (token == "generatedOriginNormal") {
			mFlags |= 0x1000u;
		}
		else if (token == "generatedNormal") {
			mFlags |= 0x800u;
		}
		else if (token == "motion") {
			ParseMotionDomains(effect, src);
		}
		else if (token == "end") {
			ParseDeathDomains(effect, src);
		}
		else if (token == "start") {
			ParseSpawnDomains(effect, src);
		}
		else {
			src->Error("Invalid particle keyword %s\n", token.c_str());
		}
	}

	Finish();

	return true;
}

void rvParticleTemplate::Duplicate(rvParticleTemplate const& copy) {

}

void  rvParticleTemplate::Finish()
{
	double v2; // st7
	rvParticleTemplate* v3; // esi
	rvTrailInfo* v4; // eax
	float* v5; // eax
	const modelSurface_t* v6; // eax
	const modelSurface_t* v7; // ebp
	idTraceModel* v8; // eax
	idTraceModel* v9; // edi
	idBounds* v10; // ebp
	rvTrailInfo* v11; // ecx
	rvElectricityInfo* v12; // eax
	float v13; // ST10_4
	float v14; // ST10_4
	rvTrailInfo* v15; // ecx
	float v16; // ST10_4
	rvTrailInfo* v17; // ecx
	double v18; // st6
	float* v19; // ecx
	float v20; // ST10_4
	float* v21; // eax
	float v22; // ST14_4
	float v23; // ST18_4
	float v24; // ST1C_4
	float v25; // ST20_4
	float v26; // ST24_4
	float v27; // ST28_4
	signed int retaddr; // [esp+2Ch] [ebp+0h]

	v2 = 0.0;
	v3 = this;
	v3->mFlags |= 0x100u;
	v4 = this->mTrailInfo;
	if ((!v4->mTrailType || v4->mTrailType == 3) && !v4->mStatic)
	{
		v4->mTrailTime.y = 0.0;
		v4->mTrailTime.x = 0.0;
		v5 = &this->mTrailInfo->mTrailCount.x;
		v5[1] = 0.0;
		*v5 = 0.0;
	}
	switch (this->mType)
	{
	case 1:
	case 2:
		v11 = this->mTrailInfo;
		v3->mVertexCount = 4;
		v3->mIndexCount = 6;
		if (0.0 != v11->mTrailCount.y && v11->mTrailType == 1)
		{
			v3->mVertexCount *= (unsigned __int16)v3->GetMaxTrailCount();
			v2 = 0.0;
			v3->mIndexCount *= (unsigned __int16)v3->GetMaxTrailCount();
		}
		break;
	case 4:
	case 6:
	case 8:
	case 9:
		this->mVertexCount = 4;
		this->mIndexCount = 6;
		break;
	case 5:
		v6 = this->mModel->Surface(0);
		v7 = v6;
		if (v6)
		{
			v3->mVertexCount = v6->geometry->numVerts;// *(_WORD*)(*(_DWORD*)(v6 + 8) + 48);
			v3->mIndexCount = v6->geometry->numIndexes; // *(_WORD*)(*(_DWORD*)(v6 + 8) + 56);
		}
		v3->mMaterial = *(idMaterial**)(v6 + 4);
		v3->PurgeTraceModel();
		v8 = (idTraceModel*)operator new(0xB4Cu);
		v9 = v8;
		retaddr = 0;
		if (v8)
		{
			v10 = *(idBounds**)(v7 + 8);
			v8->InitBox();
			v9->SetupBox(*v10);
		}
		retaddr = -1;
		v2 = 0.0;
		v3->mTraceModelIndex = bse->AddTraceModel(v8);
		break;
	case 7:
		v12 = this->mElecInfo;
		this->mVertexCount = 20 * (LOWORD(v12->mNumForks) + 1);
		this->mIndexCount = 60 * (LOWORD(v12->mNumForks) + 1);
		break;
	case 0xA:
		this->mVertexCount = 0;
		this->mIndexCount = 0;
		break;
	default:
		break;
	}
	if (v3->mDuration.y <= (double)v3->mDuration.x)
	{
		v13 = v3->mDuration.x;
		v3->mDuration.x = v3->mDuration.y;
		v3->mDuration.y = v13;
	}
	if (v3->mGravity.y <= (double)v3->mGravity.x)
	{
		v14 = v3->mGravity.x;
		v3->mGravity.x = v3->mGravity.y;
		v3->mGravity.y = v14;
	}
	v15 = v3->mTrailInfo;
	if (!v15->mStatic)
	{
		if (v15->mTrailTime.y <= (double)v15->mTrailTime.x)
		{
			v16 = v15->mTrailTime.x;
			v15->mTrailTime.x = v15->mTrailTime.y;
			v15->mTrailTime.y = v16;
		}
		v17 = v3->mTrailInfo;
		v18 = v17->mTrailCount.x;
		v19 = &v17->mTrailCount.x;
		if (v19[1] <= v18)
		{
			v20 = *v19;
			*v19 = v19[1];
			v19[1] = v20;
		}
	}
	v3->mCentre.z = v2;
	v3->mCentre.y = v2;
	v3->mCentre.x = v2;
	if (!(((unsigned int)v3->mFlags >> 12) & 1))
	{
		v21 = (float*)&v3->mpSpawnPosition->mSpawnType;
		switch (*(unsigned __int8*)v21)
		{
		case 0xBu:
			v3->mCentre.x = v21[3];
			v3->mCentre.y = v21[4];
			v3->mCentre.z = v21[5];
			break;
		case 0xFu:
		case 0x13u:
		case 0x17u:
		case 0x1Bu:
		case 0x1Fu:
		case 0x23u:
		case 0x27u:
		case 0x2Bu:
		case 0x2Fu:
			v22 = v21[6] + v21[3];
			v23 = v21[7] + v21[4];
			v24 = v21[8] + v21[5];
			v25 = v22 * 0.5;
			v26 = v23 * 0.5;
			v27 = 0.5 * v24;
			v3->mCentre.x = v25;
			v3->mCentre.y = v26;
			v3->mCentre.z = v27;
			break;
		default:
			return;
		}
	}
}

void rvParticleTemplate::InitStatic()
{
	//sdDeclTypeHolder* v0; // eax
	int v1; // eax
	//sdDeclTypeHolder* v2; // eax

	if (!rvParticleTemplate::sInited)
	{
		rvParticleTemplate::sInited = 1;
		rvParticleTemplate::sTrailInfo.mTrailType = 0;
		//unk_7E672A = 1;
		//v0 = sdSingleton<sdDeclTypeHolder>::GetInstance();
		//v1 = ((int(__stdcall*)(int, const char*, signed int))declManager->vfptr->FindType)(
		//	v0->declMaterialType.declTypeHandle,
		//	"_default",
		//	1);
		//unk_7E6754 = 0.0;
		//unk_7E674C = v1;
		//unk_7E6750 = 0.0;
		rvParticleTemplate::sElectricityInfo.mNumForks = 0;
		//unk_7E675C = 0.0;
		rvParticleTemplate::sElectricityInfo.mStatic = 1;
		//unk_7E6758 = 0.0;
		//unk_7E6760 = 0.5;
		rvParticleTemplate::sElectricityInfo.mForkSizeMins.x = -20.0;
		rvParticleTemplate::sElectricityInfo.mForkSizeMins.y = -20.0;
		rvParticleTemplate::sElectricityInfo.mForkSizeMins.z = -20.0;
		rvParticleTemplate::sElectricityInfo.mForkSizeMaxs.x = 20.0;
		rvParticleTemplate::sElectricityInfo.mForkSizeMaxs.y = 20.0;
		rvParticleTemplate::sElectricityInfo.mForkSizeMaxs.z = 20.0;
		rvParticleTemplate::sElectricityInfo.mJitterSize.x = 3.0;
		rvParticleTemplate::sElectricityInfo.mJitterSize.y = 7.0;
		rvParticleTemplate::sElectricityInfo.mJitterSize.z = 7.0;
		rvParticleTemplate::sElectricityInfo.mJitterRate = 0.0;
		//v2 = sdSingleton<sdDeclTypeHolder>::GetInstance();
		rvParticleTemplate::sElectricityInfo.mJitterTable = declManager->FindTable("halfsintable", false);
		rvParticleTemplate::sDefaultEnvelope.Init(); //rvEnvParms::Init(&rvParticleTemplate::sDefaultEnvelope);		
		rvParticleTemplate::sDefaultEnvelope.SetDefaultType(); // rvEnvParms::SetDefaultType(&rvParticleTemplate::sDefaultEnvelope);
		rvParticleTemplate::sDefaultEnvelope.mStatic = 1;
		rvParticleTemplate::sEmptyEnvelope.Init(); // rvEnvParms::Init(&rvParticleTemplate::sEmptyEnvelope);
		rvParticleTemplate::sSPF_ONE_1.mRange = 0.0;
		rvParticleTemplate::sEmptyEnvelope.mStatic = 1;
		rvParticleTemplate::sSPF_ONE_1.mMins.z = 0.0;
		rvParticleTemplate::sSPF_ONE_1.mSpawnType = 5;
		rvParticleTemplate::sSPF_ONE_1.mMins.y = 0.0;
		rvParticleTemplate::sSPF_ONE_1.mFlags = 0;
		rvParticleTemplate::sSPF_ONE_1.mMins.x = 0.0;
		rvParticleTemplate::sSPF_ONE_1.mModelInfo = 0;
		rvParticleTemplate::sSPF_ONE_1.mMaxs.z = 0.0;
		rvParticleTemplate::sSPF_ONE_1.mStatic = 1;
		rvParticleTemplate::sSPF_ONE_1.mMaxs.y = 0.0;
		rvParticleTemplate::sSPF_ONE_2.mSpawnType = 6;
		rvParticleTemplate::sSPF_ONE_1.mMaxs.x = 0.0;
		rvParticleTemplate::sSPF_ONE_2.mFlags = 0;
		rvParticleTemplate::sSPF_ONE_2.mRange = 0.0;
		rvParticleTemplate::sSPF_ONE_2.mModelInfo = 0;
		rvParticleTemplate::sSPF_ONE_2.mMins.z = 0.0;
		rvParticleTemplate::sSPF_ONE_2.mStatic = 1;
		rvParticleTemplate::sSPF_ONE_2.mMins.y = 0.0;
		rvParticleTemplate::sSPF_ONE_3.mSpawnType = 7;
		rvParticleTemplate::sSPF_ONE_2.mMins.x = 0.0;
		rvParticleTemplate::sSPF_ONE_3.mFlags = 0;
		rvParticleTemplate::sSPF_ONE_2.mMaxs.z = 0.0;
		rvParticleTemplate::sSPF_ONE_2.mMaxs.y = 0.0;
		rvParticleTemplate::sSPF_ONE_2.mMaxs.x = 0.0;
		rvParticleTemplate::sSPF_ONE_3.mRange = 0.0;
		rvParticleTemplate::sSPF_ONE_3.mMins.z = 0.0;
		rvParticleTemplate::sSPF_ONE_3.mModelInfo = 0;
		rvParticleTemplate::sSPF_ONE_3.mMins.y = 0.0;
		rvParticleTemplate::sSPF_ONE_3.mStatic = 1;
		rvParticleTemplate::sSPF_ONE_3.mMins.x = 0.0;
		rvParticleTemplate::sSPF_NONE_0.mSpawnType = 0;
		rvParticleTemplate::sSPF_ONE_3.mMaxs.z = 0.0;
		rvParticleTemplate::sSPF_NONE_0.mFlags = 0;
		rvParticleTemplate::sSPF_ONE_3.mMaxs.y = 0.0;
		rvParticleTemplate::sSPF_NONE_0.mModelInfo = 0;
		rvParticleTemplate::sSPF_ONE_3.mMaxs.x = 0.0;
		rvParticleTemplate::sSPF_NONE_0.mStatic = 1;
		rvParticleTemplate::sSPF_NONE_0.mRange = 0.0;
		rvParticleTemplate::sSPF_NONE_1.mSpawnType = 1;
		rvParticleTemplate::sSPF_NONE_0.mMins.z = 0.0;
		rvParticleTemplate::sSPF_NONE_1.mFlags = 0;
		rvParticleTemplate::sSPF_NONE_0.mMins.y = 0.0;
		rvParticleTemplate::sSPF_NONE_1.mModelInfo = 0;
		rvParticleTemplate::sSPF_NONE_0.mMins.x = 0.0;
		rvParticleTemplate::sSPF_NONE_1.mStatic = 1;
		rvParticleTemplate::sSPF_NONE_0.mMaxs.z = 0.0;
		rvParticleTemplate::sSPF_NONE_3.mSpawnType = 3;
		rvParticleTemplate::sSPF_NONE_0.mMaxs.y = 0.0;
		rvParticleTemplate::sSPF_NONE_3.mFlags = 0;
		rvParticleTemplate::sSPF_NONE_0.mMaxs.x = 0.0;
		rvParticleTemplate::sSPF_NONE_3.mModelInfo = 0;
		rvParticleTemplate::sSPF_NONE_1.mRange = 0.0;
		rvParticleTemplate::sSPF_NONE_3.mStatic = 1;
		rvParticleTemplate::sSPF_NONE_1.mMins.z = 0.0;
		rvParticleTemplate::sSPF_NONE_1.mMins.y = 0.0;
		rvParticleTemplate::sSPF_NONE_1.mMins.x = 0.0;
		rvParticleTemplate::sSPF_NONE_1.mMaxs.z = 0.0;
		rvParticleTemplate::sSPF_NONE_1.mMaxs.y = 0.0;
		rvParticleTemplate::sSPF_NONE_1.mMaxs.x = 0.0;
		rvParticleTemplate::sSPF_NONE_3.mRange = 0.0;
		rvParticleTemplate::sSPF_NONE_3.mMins.z = 0.0;
		rvParticleTemplate::sSPF_NONE_3.mMins.y = 0.0;
		rvParticleTemplate::sSPF_NONE_3.mMins.x = 0.0;
		rvParticleTemplate::sSPF_NONE_3.mMaxs.z = 0.0;
		rvParticleTemplate::sSPF_NONE_3.mMaxs.y = 0.0;
		rvParticleTemplate::sSPF_NONE_3.mMaxs.x = 0.0;
	}
}

void rvParticleTemplate::Init(void)
{
	rvParticleTemplate* v1; // esi
	//sdDeclTypeHolder* v2; // eax
	rvDeclEffect** v3; // eax
	signed int v4; // ecx

	v1 = this;
	rvParticleTemplate::InitStatic();
	v1->mFlags = 0;
	v1->mType = 0;
	//v2 = sdSingleton<sdDeclTypeHolder>::GetInstance();
	v1->SetMaterial((idMaterial *)declManager->FindMaterial("_default"));
	v1->mModel = renderModelManager->FindModel("_default");
	//v1->mMaterial = (idMaterial*)((int(__stdcall*)(int, const char*, signed int))declManager->vfptr->FindType)(
	//	v2->declMaterialType.declTypeHandle,
	//	"_default",
	//	1);
	//v1->mModel = (idRenderModel*)((int(__stdcall*)(const char*))renderModelManager->vfptr->FindModel)("_default");
	v1->mTraceModelIndex = -1;
	v1->mGravity.y = 0.0;
	v1->mGravity.x = 0.0;
	v1->mTiling = 8.0;
	v1->mPhysicsDistance = 0.0;
	v1->mBounce = 0.0;
	v1->mDuration.x = 0.0020000001;
	v1->mDuration.y = 0.0020000001;
	v1->mCentre.z = 0.0;
	v1->mCentre.y = 0.0;
	v1->mCentre.x = 0.0;
	v1->mFlags |= 0x4000000u;
	v1->mpSpawnPosition = &rvParticleTemplate::sSPF_NONE_3;
	v1->mWindDeviationAngle = 0.0;
	v1->mpSpawnDirection = &rvParticleTemplate::sSPF_NONE_3;
	v1->mpSpawnVelocity = &rvParticleTemplate::sSPF_NONE_3;
	v1->mpSpawnAcceleration = &rvParticleTemplate::sSPF_NONE_3;
	v1->mpSpawnFriction = &rvParticleTemplate::sSPF_NONE_3;
	v1->mpSpawnRotate = &rvParticleTemplate::sSPF_NONE_3;
	v1->mpSpawnAngle = &rvParticleTemplate::sSPF_NONE_3;
	v1->mpSpawnOffset = &rvParticleTemplate::sSPF_NONE_3;
	v1->mpSpawnLength = &rvParticleTemplate::sSPF_NONE_3;
	v1->mpDeathTint = &rvParticleTemplate::sSPF_NONE_3;
	v1->mpDeathRotate = &rvParticleTemplate::sSPF_NONE_3;
	v1->mpDeathAngle = &rvParticleTemplate::sSPF_NONE_3;
	v1->mpDeathOffset = &rvParticleTemplate::sSPF_NONE_3;
	v1->mpDeathLength = &rvParticleTemplate::sSPF_NONE_3;
	v1->mNumSizeParms = 2;
	v1->mNumRotateParms = 1;
	v1->mVertexCount = 4;
	v1->mIndexCount = 6;
	v1->mTrailInfo = &rvParticleTemplate::sTrailInfo;
	v1->mElecInfo = &rvParticleTemplate::sElectricityInfo;
	v1->mpSpawnTint = &rvParticleTemplate::sSPF_ONE_3;
	v1->mpSpawnFade = &rvParticleTemplate::sSPF_ONE_1;
	v1->mpSpawnSize = &rvParticleTemplate::sSPF_ONE_3;
	v1->mpSpawnWindStrength = &rvParticleTemplate::sSPF_NONE_1;
	v1->mpTintEnvelope = &rvParticleTemplate::sEmptyEnvelope;
	v1->mpFadeEnvelope = &rvParticleTemplate::sEmptyEnvelope;
	v1->mpSizeEnvelope = &rvParticleTemplate::sEmptyEnvelope;
	v1->mpRotateEnvelope = &rvParticleTemplate::sEmptyEnvelope;
	v1->mpAngleEnvelope = &rvParticleTemplate::sEmptyEnvelope;
	v1->mpOffsetEnvelope = &rvParticleTemplate::sEmptyEnvelope;
	v1->mpLengthEnvelope = &rvParticleTemplate::sEmptyEnvelope;
	v1->mpDeathFade = &rvParticleTemplate::sSPF_NONE_1;
	v1->mpDeathSize = &rvParticleTemplate::sSPF_ONE_3;
	v1->mTrailRepeat = 1;
	v1->mNumFrames = 0;
	v1->mNumImpactEffects = 0;
	v1->mNumTimeoutEffects = 0;
	// jmarshall: no idea
	//v3 = &v1->mTimeoutEffects[0];
	// jmarshall end
	v4 = 4;

	// jmarshall: no idea
	//do
	//{
	//	*(v3 - 5) = 0;
	//	*v3 = 0;
	//	++v3;
	//	--v4;
	//} while (v4);
	// jmarshall end
	v1->mFlags |= 0x8000000u;
}

void rvParticleTemplate::SetParameterCounts()
{
	rvParticleParms* v1; // eax
	rvParticleParms* v2; // edx

	switch (this->mType)
	{
	case 1:
		this->mNumSizeParms = 2;
		this->mNumRotateParms = 1;
		v1 = &rvParticleTemplate::sSPF_ONE_2;
		v2 = &rvParticleTemplate::sSPF_NONE_1;
		goto LABEL_11;
	case 2:
	case 7:
	case 8:
	case 9:
		this->mNumSizeParms = 1;
		this->mNumRotateParms = 0;
		v1 = &rvParticleTemplate::sSPF_ONE_1;
		v2 = &rvParticleTemplate::sSPF_NONE_0;
		goto LABEL_11;
	case 3:
		this->mNumSizeParms = 2;
		this->mNumRotateParms = 3;
		v1 = &rvParticleTemplate::sSPF_ONE_2;
		v2 = &rvParticleTemplate::sSPF_NONE_3;
		goto LABEL_11;
	case 4:
		this->mNumSizeParms = 3;
		this->mNumRotateParms = 1;
		v2 = &rvParticleTemplate::sSPF_NONE_1;
		goto LABEL_10;
	case 5:
		this->mNumSizeParms = 3;
		this->mNumRotateParms = 3;
		goto LABEL_9;
	case 6:
		this->mNumSizeParms = 3;
		this->mNumRotateParms = 0;
		v2 = &rvParticleTemplate::sSPF_NONE_0;
		goto LABEL_10;
	case 0xA:
		this->mNumSizeParms = 0;
		this->mNumRotateParms = 3;
	LABEL_9:
		v2 = &rvParticleTemplate::sSPF_NONE_3;
	LABEL_10:
		v1 = &rvParticleTemplate::sSPF_ONE_3;
	LABEL_11:
		this->mpSpawnSize = v1;
		this->mpSpawnRotate = v2;
		this->mpDeathSize = v1;
		this->mpDeathRotate = v2;
		break;
	default:
		return;
	}
}

void rvParticleTemplate::PurgeTraceModel(void) {
	rvParticleTemplate* v1; // esi
	int v2; // eax

	v1 = this;
	v2 = this->mTraceModelIndex;
	if (v2 != -1)
	{
		//bse->FreeTraceModel(v2); // jmarshall: todo
		v1->mTraceModelIndex = -1;
	}
}

void rvParticleTemplate::Purge() {
	// TODO
}

float rvParticleTemplate::CostTrail(float cost) const
{
	rvTrailInfo* v2; // eax
	double result; // st7
	float costa; // [esp+4h] [ebp+4h]
	float costb; // [esp+4h] [ebp+4h]

	v2 = this->mTrailInfo;
	switch (v2->mTrailType)
	{
	case 1:
		costa = v2->mTrailCount.y * (cost + cost);
		result = costa;
		break;
	case 2:
		costb = v2->mTrailCount.y * (cost * 1.5) + 20.0;
		result = costb;
		break;
	default:
		result = cost;
		break;
	}
	return result;
}

bool rvParticleTemplate::UsesEndOrigin(void) {
	bool result; // al

	if (this->mpSpawnPosition->mFlags & 2 || this->mpSpawnLength->mFlags & 2)
		result = 1;
	else
		result = ((unsigned int)this->mFlags >> 22) & 1;
	return result;
}
