// DeclPlayback.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop


rvDeclPlayback::rvDeclPlayback() {

}

rvDeclPlayback::~rvDeclPlayback() {

}

/*
=====================
rvDeclPlayback::ParseSample
=====================
*/
void rvDeclPlayback::ParseSample(idLexer* src, idVec3& pos, idAngles& ang)
{
	idToken token; 

	while (true)
	{
		src->ReadToken(&token);

		if (token == "}")
		{
			break;
		}
		else if (token == "down" || token == "up" || token == "impulse")
		{
			src->ParseInt(); // jmarshall: decompiled code doesn't use this, seems like its just skipped.
			continue;
		}
		else if (token == "rotate")
		{
			ang.pitch = src->ParseFloat();
			src->ExpectTokenString(",");
			ang.yaw = src->ParseFloat();
			src->ExpectTokenString(",");
			ang.roll = src->ParseFloat();
			continue;
		}
		else if (token == "ang")
		{
			ang.pitch = src->ParseFloat();
			src->ExpectTokenString(",");
			ang.yaw = src->ParseFloat();
			ang.roll = 0.0;
			continue;
		}
		else if (token == "pos")
		{
			pos.x = src->ParseFloat();
			src->ExpectTokenString(",");
			pos.y = src->ParseFloat();
			src->ExpectTokenString(",");
			pos.z = src->ParseFloat();
			src->ReadToken(&token);
			continue;
		}
		else
		{
			src->Error("rvDeclPlayback::ParseSample: Invalid or unexpected token %s\n", token.c_str());
			return;
		}
	}
}

/*
=====================
rvDeclPlayback::ParseData
=====================
*/
bool rvDeclPlayback::ParseData(idLexer* src) {
	idToken token;

	float t = 0;

	while (true)
	{
		src->ReadToken(&token);

		if (token == "}")
		{
			break;
		}

		if (token == "{")
		{
			idVec3 pos;
			idAngles ang;
			
			ParseSample(src, pos, ang);

			points.AddValue(t, pos);
			angles.AddValue(t, ang);
			t += (1.0 / frameRate);
			continue;
		}
		else
		{
			src->Error("rvDeclPlayback::ParseData: Invalid or unexpected token %s\n", token.c_str());
			return false;
		}
	}

	duration = t;

	return true;
}

/*
=====================
rvDeclPlayback::ParseButton
=====================
*/
void rvDeclPlayback::ParseButton(idLexer* src, byte& button, rvButtonState& state) {
	idToken token;

	state.time = src->ParseFloat();

	while (true)
	{
		src->ReadToken(&token);

		if (token == "}")
		{
			break;
		}

		if (token == "impulse")
		{
			state.impulse = src->ParseInt();
			continue;
		}
		else if (token == "up")
		{
			button = ~src->ParseInt() & button;
			continue;
		}
		else if (token == "down")
		{
			button = src->ParseInt() | button;
			continue;
		}
	}

	state.state = button;
	state.impulse = 0;
}


/*
=====================
rvDeclPlayback::ParseSequence
=====================
*/
bool rvDeclPlayback::ParseSequence(idLexer* src) {
	src->ExpectTokenString("sequence");
	src->ExpectTokenString("{");

	idToken token;

	while (true)
	{
		src->ReadToken(&token);

		if (token == "}")
		{
			break;
		}

		if (token == "framerate")
		{
			frameRate = src->ParseFloat();
			continue;
		}
		else if (token == "origin")
		{
			origin.x = src->ParseFloat();
			src->ExpectTokenString(",");
			origin.y = src->ParseFloat();
			src->ExpectTokenString(",");
			origin.z = src->ParseFloat();
			continue;
		}
		else if (token == "destination")
		{
			idStr dest;
			src->ParseRestOfLine(dest);
			continue;
		}
		else
		{
			src->Error("rvDeclPlayback::ParseSequence: Invalid or unexpected token %s\n", token.c_str());
			return false;
		}
	}

	return true;
}

/*
=====================
rvDeclPlayback::DefaultDefinition
=====================
*/
const char* rvDeclPlayback::DefaultDefinition() const
{
	return "{ sequence { } data { } }";
}

/*
=====================
rvDeclPlayback::Size
=====================
*/
size_t rvDeclPlayback::Size(void) const {
	return sizeof(rvDeclPlayback);
}
/*
=====================
rvDeclPlayback::Parse
=====================
*/
bool rvDeclPlayback::Parse(const char* text, const int textLength) {
	idLexer src;
	idToken	token, token2;

	src.LoadMemory(text, textLength, GetFileName(), GetLineNum());
	src.SetFlags(DECL_LEXER_FLAGS);
	src.SkipUntilString("{");

	if (ParseSequence(&src))
	{
		while (1) {
			if (!src.ReadToken(&token)) {
				break;
			}

			if (!token.Icmp("}")) {
				origin = points.GetValue(0) + origin;
				bounds.Clear();
				for (int i = 0; i < points.GetNumValues(); i++)
				{
					bounds.AddPoint(points.GetValue(i));
				}
				break;
			}
			else if (token == "data")
			{
				ParseData(&src);
				continue;
			}
			else
			{
				src.Error("Unexpected token %s", token.c_str());
			}
		}
	}

	return true;
}

/*
=====================
rvDeclPlayback::FreeData
=====================
*/
void rvDeclPlayback::FreeData(void) {

}

/*
=====================
rvDeclPlayback::WriteData
=====================
*/
void rvDeclPlayback::WriteData(idFile_Memory& f) {
	// todo implement me
}

/*
=====================
rvDeclPlayback::WriteButtons
=====================
*/
void rvDeclPlayback::WriteButtons(idFile_Memory& f) {

}

/*
=====================
rvDeclPlayback::WriteSequence
=====================
*/
void rvDeclPlayback::WriteSequence(idFile_Memory& f) {

}
