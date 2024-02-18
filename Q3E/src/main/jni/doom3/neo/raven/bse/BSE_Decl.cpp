#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "BSE.h"

#ifdef _K_DEV
#if !defined(_MSC_VER)
#define LOGW_SKIP(fmt, args...) common->Warning(fmt, ##args)
#else
#define LOGW_SKIP(fmt, ...) common->Warning(fmt, __VA_ARGS__)
#endif

#else

#if !defined(_MSC_VER)
#define LOGW_SKIP(fmt, args...)
#else
#define LOGW_SKIP(fmt, ...)
#endif
#endif

#include "BSE_Parser.cpp"

/*
=================
rvDeclEffect::Size
=================
*/
size_t rvDeclEffect::Size(void) const
{
	return sizeof(rvDeclEffect);
}

/*
===============
rvDeclEffect::Print
===============
*/
void rvDeclEffect::Print(void) const
{
	const rvDeclEffect *list = this;

	common->Printf("%d events\n", list->events.Num());

	for (int i = 0; i < list->events.Num(); i++) {
		switch (list->events[i].type) {
			case FX_LIGHT:
				common->Printf("FX_LIGHT %s\n", list->events[i].data.c_str());
				break;
			case FX_PARTICLE:
				common->Printf("FX_PARTICLE %s\n", list->events[i].data.c_str());
				break;
			case FX_MODEL:
				common->Printf("FX_MODEL %s\n", list->events[i].data.c_str());
				break;
			case FX_SOUND:
				common->Printf("FX_SOUND %s\n", list->events[i].data.c_str());
				break;
			case FX_DECAL:
				common->Printf("FX_DECAL %s\n", list->events[i].data.c_str());
				break;
			case FX_SHAKE:
				common->Printf("FX_SHAKE %s\n", list->events[i].data.c_str());
				break;
			case FX_ATTACHLIGHT:
				common->Printf("FX_ATTACHLIGHT %s\n", list->events[i].data.c_str());
				break;
			case FX_ATTACHENTITY:
				common->Printf("FX_ATTACHENTITY %s\n", list->events[i].data.c_str());
				break;
			case FX_LAUNCH:
				common->Printf("FX_LAUNCH %s\n", list->events[i].data.c_str());
				break;
			case FX_SHOCKWAVE:
				common->Printf("FX_SHOCKWAVE %s\n", list->events[i].data.c_str());
				break;
		}
	}
}

/*
===============
rvDeclEffect::List
===============
*/
void rvDeclEffect::List(void) const
{
	common->Printf("%s, %d stages\n", GetName(), events.Num());
}

/*
================
rvDeclEffect::Parse
================
*/
bool rvDeclEffect::Parse(const char *text, const int textLength, bool noCaching)
{
	idLexer src;
	idToken token;

	src.LoadMemory(text, textLength, GetFileName(), GetLineNum());
	src.SetFlags(DECL_LEXER_FLAGS);

	rvDeclEffectParser parser(this, src);
	parser.Parse();

	if (src.HadError()) {
		src.Warning("FX decl '%s' had a parse error", GetName());
		return false;
	}

	return true;
}

/*
===================
rvDeclEffect::DefaultDefinition
===================
*/
const char *rvDeclEffect::DefaultDefinition(void) const
{
	return
	        "{\n"
	        "}";
}

/*
===================
rvDeclEffect::FreeData
===================
*/
void rvDeclEffect::FreeData(void)
{
	mFlags = 0;
	events.Clear();
	for(int i = 0; i < particles.Num(); i++)
	{
		rvBSEParticle *decl = BSE_SetDeclParticle(particles[i]);
		decl->FreeData();
		delete decl;
	}
	particles.Clear();
}

bool rvDeclEffect::SetDefaultText()
{
	char generated[1024]; // [esp+4h] [ebp-404h]

	idStr::snPrintf(generated, sizeof(generated), "effect %s // IMPLICITLY GENERATED\n" , GetName());
	SetText(generated);
	return false;
}

