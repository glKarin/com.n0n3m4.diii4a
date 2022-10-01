// DeclMatType.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop



/*
=======================
rvDeclMatType::DefaultDefinition
=======================
*/
const char* rvDeclMatType::DefaultDefinition(void) const {
	return "{ description \"<DEFAULTED>\" rgb 0,0,0 }";
}

/*
=======================
rvDeclMatType::Parse
=======================
*/
bool rvDeclMatType::Parse(const char* text, const int textLength) {
	idLexer src;
	idToken	token, token2;

	src.LoadMemory(text, textLength, GetFileName(), GetLineNum());
	src.SetFlags(DECL_LEXER_FLAGS);
	src.SkipUntilString("{");

	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}
		else if (token == "rgb")
		{
			mTint[0] = src.ParseInt();
			src.ExpectTokenString(",");
			mTint[1] = src.ParseInt();
			src.ExpectTokenString(",");
			mTint[2] = src.ParseInt();
		}
		else if (token == "description")
		{
			src.ReadToken(&token);
			mDescription = token;
			continue;
		}
		else
		{
			src.Error("rvDeclMatType::Parse: Invalid or unexpected token %s\n", token.c_str());
			return false;
		}
	}
	return true;
}

/*
=======================
rvDeclMatType::FreeData
=======================
*/
void rvDeclMatType::FreeData(void) {

}

/*
=======================
rvDeclMatType::Size
=======================
*/
size_t rvDeclMatType::Size(void) const {
	return sizeof(rvDeclMatType);
}
