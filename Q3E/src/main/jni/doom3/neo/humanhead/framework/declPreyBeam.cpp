#include "../../idlib/precompiled.h"
#pragma hdrstop

static const char *Beam_CMD_Type_Str[] = {
	"SplineLinearToTarget",
	"SplineArcToTarget",
	"SplineAdd",
	"SplineAddSin",
	"SplineAddSinTime",
	"SplineAddSinTimeScaled",
	"ConvertSplineToNodes",
	"NodeLinearToTarget",
	"NodeElectric",
	0,
};

static void parse_vec3(idLexer &src, idVec3 &vec)
{
	src.ExpectTokenString("(");
	vec.x = src.ParseFloat();
	src.ExpectTokenString(",");
	vec.y = src.ParseFloat();
	src.ExpectTokenString(",");
	vec.z = src.ParseFloat();
	src.ExpectTokenString(")");
}

static void parse_cmd(idLexer &src, beamCmd_t &cmd)
{
	switch(cmd.type)
	{
		case BEAMCMD_SplineLinearToTarget:
			break;
		case BEAMCMD_SplineArcToTarget:
			break;
		case BEAMCMD_SplineAdd:
			cmd.index = src.ParseInt();
			parse_vec3(src, cmd.offset);
			break;
		case BEAMCMD_SplineAddSin:
		case BEAMCMD_SplineAddSinTim:
		case BEAMCMD_SplineAddSinTimeScaled:
			cmd.index = src.ParseInt();
			parse_vec3(src, cmd.offset);
			parse_vec3(src, cmd.phase);
			break;
		case BEAMCMD_ConvertSplineToNodes:
			break;
		case BEAMCMD_NodeLinearToTarget:
			break;
		case BEAMCMD_NodeElectric:
			parse_vec3(src, cmd.offset);
			break;
	}
}

static bool parse_beam(idLexer &src, hhDeclBeam *self, int i)
{
	idToken token;

	if(i >= MAX_BEAMS)
	{
		common->Error("[Harmattan]: beam decl '%s' over(%d)", self->GetName(), MAX_BEAMS);
		return false;
	}

	idList<beamCmd_t> &cmdList = self->cmds[i];
	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!idStr::Icmp(token, "thickness"))
		{
			self->thickness[i] = src.ParseInt();
			continue;
		}
		else if (!idStr::Icmp(token, "taperEnds"))
		{
			self->bTaperEndPoints[i] = src.ParseBool();
			continue;
		}
		else if (!idStr::Icmp(token, "flat"))
		{
			self->bFlat[i] = src.ParseBool();
			continue;
		}
		else if (!idStr::Icmp(token, "quadEndSize"))
		{
			self->quadSize[i][1] = src.ParseFloat();
			continue;
		}
		else if (!idStr::Icmp(token, "quadoriginSize"))
		{
			self->quadSize[i][0] = src.ParseFloat();
			continue;
		}
		else if (!idStr::Icmp(token, "quadEndShader"))
		{
			src.ReadToken(&token);
			self->quadShader[i][1] = declManager->FindMaterial(token);
			continue;
		}
		else if (!idStr::Icmp(token, "quadoriginShader"))
		{
			src.ReadToken(&token);
			self->quadShader[i][0] = declManager->FindMaterial(token);
			continue;
		}
		else
		{
			int m;
			for(m = 0; Beam_CMD_Type_Str[m]; m++)
			{
				if (!idStr::Icmp(token, Beam_CMD_Type_Str[m]))
				{
					beamCmd_t cmd;
					cmd.type = (beamCmdTypes_t)m;
					parse_cmd(src, cmd);
					cmdList.Append(cmd);
					break;
				}
			}
			if(!Beam_CMD_Type_Str[m])
			{
				src.Warning("Invalid or unexpected token %s in decl '%s'\n", token.c_str(), self->GetName());
				src.SkipRestOfLine();
			}
		}
	}
	return true;
}

hhDeclBeam::hhDeclBeam()
{
	numNodes = 0;
	numBeams = 0;
	memset(thickness, 0, sizeof(thickness));
	memset(bFlat, 0, sizeof(bFlat));
	memset(bTaperEndPoints, 0, sizeof(bTaperEndPoints));
	memset(shader, 0, sizeof(shader));
	memset(quadShader, 0, sizeof(quadShader));
	memset(quadSize, 0, sizeof(quadSize));
	memset(cmds, 0, sizeof(cmds));
}

const char * hhDeclBeam::DefaultDefinition() const
{
	return "beam default.beam{}";
}

bool hhDeclBeam::Parse( const char *text, const int textLength )
{
	idLexer src;
	idToken	token;
	int i = 0;

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

		if (token == "numNodes")
		{
			numNodes = src.ParseInt();
			continue;
		}
		else if (token == "numBeams")
		{
			numBeams = src.ParseInt();
			continue;
		}
		else
		{
			src.ExpectTokenString("{");
			if(!parse_beam(src, this, i))
			{
				src.Warning("Invalid or unexpected token '%s' in decl '%s'\n", token.c_str(), GetName());
				return false;
			}
			shader[i] = declManager->FindMaterial(token);
			i++;
		}
	}
	if(numNodes > MAX_BEAM_NODES) //k: exists more than 32 beam
		numNodes = MAX_BEAM_NODES;
	return true;
}

void hhDeclBeam::FreeData()
{
}
