// DeclPlayerModel.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop


rvDeclPlayerModel::rvDeclPlayerModel() {

}

/*
===================
rvDeclPlayerModel::Size
===================
*/
size_t rvDeclPlayerModel::Size(void) const {
	return sizeof(rvDeclPlayerModel);
}

/*
===================
rvDeclPlayerModel::DefaultDefinition
===================
*/
const char* rvDeclPlayerModel::DefaultDefinition() const {
	return "{\n\t\"model\"\t\"model_player_marine\"\n\t\"def_head\"\t\"char_marinehead_kane2_client\"\n}";
}

/*
===================
rvDeclPlayerModel::FreeData
===================
*/
bool rvDeclPlayerModel::Parse(const char* text, const int textLength) {
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
		else if (token == "head_offset")
		{
			src.Parse1DMatrix(3, headOffset.ToFloatPtr());
			continue;
		}
		else if (token == "description")
		{
			src.ReadToken(&token);
			description = token;
			continue;
		}
		else if (token == "team")
		{
			src.ReadToken(&token);
			team = token;
			continue;
		}
		else if (token == "def_head_ui")
		{
			src.ReadToken(&token);
			head = token;
			continue;
		}
		else if (token == "def_head")
		{
			src.ReadToken(&token);
			uiHead = token;
			continue;
		}
		else if (token == "model")
		{
			src.ReadToken(&token);
			model = token;
			continue;
		}
		else if (token == "skin")
		{
			src.ReadToken(&token);
			skin = token;
			continue;
		}
		else
		{
			src.Error("Invalid or unexpected token '%s'\n", token.c_str());
			return false;
		}
	}

	if (model.Length() <= 0)
	{
		src.Error("playerModel decl '%s' without model declaration", token.c_str());
		return false;
	}



	return true;
}

/*
===================
rvDeclPlayerModel::FreeData
===================
*/
void rvDeclPlayerModel::FreeData(void) {

}

/*
===================
rvDeclPlayerModel::DefaultDefinition
===================
*/
void rvDeclPlayerModel::Print(void) {

}
