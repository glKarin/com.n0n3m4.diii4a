// copy from framework/DeclParticle

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "BSE.h"
#include "../../renderer/Model.h"
struct ParticleParmDesc {
	const char *name;
	int count;
	const char *desc;
};

const ParticleParmDesc ParticleDistributionDesc[] = {
	{ "rect", 3, "" },
	{ "cylinder", 4, "" },
	{ "sphere", 3, "" }
};

const ParticleParmDesc ParticleDirectionDesc[] = {
	{ "cone", 1, "" },
	{ "outward", 1, "" },
};

const ParticleParmDesc ParticleOrientationDesc[] = {
	{ "view", 0, "" },
	{ "aimed", 2, "" },
	{ "x", 0, "" },
	{ "y", 0, "" },
	{ "z", 0, "" }
};

const ParticleParmDesc ParticleCustomDesc[] = {
	{ "standard", 0, "Standard" },
	{ "helix", 5, "sizeX Y Z radialSpeed axialSpeed" },
	{ "flies", 3, "radialSpeed axialSpeed size" },
	{ "orbit", 2, "radius speed"},
	{ "drip", 2, "something something" }
};

const int CustomParticleCount = sizeof(ParticleCustomDesc) / sizeof(const ParticleParmDesc);

/*
=================
rvBSEParticle::Size
=================
*/
size_t rvBSEParticle::Size(void) const
{
	return sizeof(rvBSEParticle);
}

/*
=====================
rvBSEParticle::GetStageBounds
=====================
*/
void rvBSEParticle::GetStageBounds(rvBSEParticleStage *stage)
{

	stage->bounds.Clear();

	// this isn't absolutely guaranteed, but it should be close

	rvBSE_particleGen_t g;

	renderEntity_t	renderEntity;
	memset(&renderEntity, 0, sizeof(renderEntity));
	renderEntity.axis = mat3_identity;

	renderView_t	renderView;
	memset(&renderView, 0, sizeof(renderView));
	renderView.viewaxis = mat3_identity;

	g.renderEnt = &renderEntity;
	g.renderView = &renderView;
	g.origin.Zero();
	g.axis = mat3_identity;

	idRandom	steppingRandom;
	steppingRandom.SetSeed(0);

	// just step through a lot of possible particles as a representative sampling
	for (int i = 0 ; i < 1000 ; i++) {
		g.random = g.originalRandom = steppingRandom;

		int	maxMsec = stage->particleLife * 1000;

		for (int inCycleTime = 0 ; inCycleTime < maxMsec ; inCycleTime += 16) {

			// make sure we get the very last tic, which may make up an extreme edge
			if (inCycleTime + 16 > maxMsec) {
				inCycleTime = maxMsec - 1;
			}

			g.frac = (float)inCycleTime / (stage->particleLife * 1000);
			g.age = inCycleTime * 0.001f;

			// if the particle doesn't get drawn because it is faded out or beyond a kill region,
			// don't increment the verts

			idVec3	origin;
			stage->ParticleOrigin(&g, origin);
			stage->bounds.AddPoint(origin);
		}
	}

	// find the max size
	float	maxSize = 0;

	for (float f = 0; f <= 1.0f; f += 1.0f / 64) {
		float size = stage->size.Eval(f, steppingRandom);
		float aspect = stage->aspect.Eval(f, steppingRandom);

		if (aspect > 1) {
			size *= aspect;
		}

		if (size > maxSize) {
			maxSize = size;
		}
	}

	maxSize += 8;	// just for good measure
	// users can specify a per-stage bounds expansion to handle odd cases
	stage->bounds.ExpandSelf(maxSize + stage->boundsExpansion);
}

/*
================
rvBSEParticle::ParseParms

Parses a variable length list of parms on one line
================
*/
void rvBSEParticle::ParseParms(idLexer &src, float *parms, int maxParms)
{
	idToken token;
	bool neg = false;

	memset(parms, 0, maxParms * sizeof(*parms));
	int	count = 0;

	while (1) {
		if (!src.ReadTokenOnLine(&token)) {
			return;
		}

		if(!idStr::Cmp(token, "-"))
		{
			neg = !neg;
			continue;
		}
		if (count == maxParms) {
			src.Error("too many parms on line");
			return;
		}

		token.StripQuotes();
		parms[count] = atof(token);
		if(neg)
			parms[count] = -parms[count];
		neg = false;
		count++;
	}
}

/*
================
rvBSEParticle::ParseParametric
================
*/
void rvBSEParticle::ParseParametric(idLexer &src, rvBSEParticleParm *parm)
{
	idToken token;

	parm->table = NULL;
	parm->from = parm->to = 0.0f;

	if (!src.ReadToken(&token)) {
		src.Error("not enough parameters");
		return;
	}

	if (token.IsNumeric()) {
		// can have a to + 2nd parm
		parm->from = parm->to = atof(token);

		if (src.ReadToken(&token)) {
			if (!token.Icmp("to")) {
				if (!src.ReadToken(&token)) {
					src.Error("missing second parameter");
					return;
				}

				parm->to = atof(token);
			} else {
				src.UnreadToken(&token);
			}
		}
	} else {
		// table
		parm->table = static_cast<const idDeclTable *>(declManager->FindType(DECL_TABLE, token, false));
	}

}

/*
================
rvBSEParticle::ParseParticleStage
================
*/
rvBSEParticleStage *rvBSEParticle::ParseParticleStage(idLexer &src)
{
	idToken token;

	rvBSEParticleStage *stage = new rvBSEParticleStage;
	stage->Default();

	while (1) {
		if (src.HadError()) {
			break;
		}

		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Icmp("material")) {
			src.ReadToken(&token);
			stage->material = declManager->FindMaterial(token.c_str());
			continue;
		}

		if (!token.Icmp("count")) {
			stage->totalParticles = src.ParseInt();
			continue;
		}

		if (!token.Icmp("time")) {
			stage->particleLife = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("cycles")) {
			stage->cycles = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("timeOffset")) {
			stage->timeOffset = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("deadTime")) {
			stage->deadTime = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("randomDistribution")) {
			stage->randomDistribution = src.ParseBool();
			continue;
		}

		if (!token.Icmp("bunching")) {
			stage->spawnBunching = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("distribution")) {
			src.ReadToken(&token);

			if (!token.Icmp("rect")) {
				stage->distributionType = PDIST_RECT;
			} else if (!token.Icmp("cylinder")) {
				stage->distributionType = PDIST_CYLINDER;
			} else if (!token.Icmp("sphere")) {
				stage->distributionType = PDIST_SPHERE;
			} else {
				src.Error("bad distribution type: %s\n", token.c_str());
			}

			ParseParms(src, stage->distributionParms, sizeof(stage->distributionParms) / sizeof(stage->distributionParms[0]));
			continue;
		}

		if (!token.Icmp("direction")) {
			src.ReadToken(&token);

			if (!token.Icmp("cone")) {
				stage->directionType = PDIR_CONE;
			} else if (!token.Icmp("outward")) {
				stage->directionType = PDIR_OUTWARD;
			} else {
				src.Error("bad direction type: %s\n", token.c_str());
			}

			ParseParms(src, stage->directionParms, sizeof(stage->directionParms) / sizeof(stage->directionParms[0]));
			continue;
		}

		if (!token.Icmp("orientation")) {
			src.ReadToken(&token);

			if (!token.Icmp("view")) {
				stage->orientation = POR_VIEW;
			} else if (!token.Icmp("aimed")) {
				stage->orientation = POR_AIMED;
			} else if (!token.Icmp("x")) {
				stage->orientation = POR_X;
			} else if (!token.Icmp("y")) {
				stage->orientation = POR_Y;
			} else if (!token.Icmp("z")) {
				stage->orientation = POR_Z;
			} else {
				src.Error("bad orientation type: %s\n", token.c_str());
			}

			ParseParms(src, stage->orientationParms, sizeof(stage->orientationParms) / sizeof(stage->orientationParms[0]));
			continue;
		}

		if (!token.Icmp("customPath")) {
			src.ReadToken(&token);

			if (!token.Icmp("standard")) {
				stage->customPathType = PPATH_STANDARD;
			} else if (!token.Icmp("helix")) {
				stage->customPathType = PPATH_HELIX;
			} else if (!token.Icmp("flies")) {
				stage->customPathType = PPATH_FLIES;
			} else if (!token.Icmp("spherical")) {
				stage->customPathType = PPATH_ORBIT;
			} else {
				src.Error("bad path type: %s\n", token.c_str());
			}

			ParseParms(src, stage->customPathParms, sizeof(stage->customPathParms) / sizeof(stage->customPathParms[0]));
			continue;
		}

		if (!token.Icmp("speed")) {
			ParseParametric(src, &stage->speed);
			continue;
		}

		if (!token.Icmp("rotation")) {
			ParseParametric(src, &stage->rotationSpeed);
			continue;
		}

		if (!token.Icmp("angle")) {
			stage->initialAngle = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("entityColor")) {
			stage->entityColor = src.ParseBool();
			continue;
		}

		if (!token.Icmp("size")) {
			ParseParametric(src, &stage->size);
			continue;
		}

		if (!token.Icmp("aspect")) {
			ParseParametric(src, &stage->aspect);
			continue;
		}

		if (!token.Icmp("fadeIn")) {
			stage->fadeInFraction = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("fadeOut")) {
			stage->fadeOutFraction = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("fadeIndex")) {
			stage->fadeIndexFraction = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("color")) {
			stage->color[0] = src.ParseFloat();
			stage->color[1] = src.ParseFloat();
			stage->color[2] = src.ParseFloat();
			stage->color[3] = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("fadeColor")) {
			stage->fadeColor[0] = src.ParseFloat();
			stage->fadeColor[1] = src.ParseFloat();
			stage->fadeColor[2] = src.ParseFloat();
			stage->fadeColor[3] = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("offset")) {
			stage->offset[0] = src.ParseFloat();
			stage->offset[1] = src.ParseFloat();
			stage->offset[2] = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("animationFrames")) {
			stage->animationFrames = src.ParseInt();
			continue;
		}

		if (!token.Icmp("animationRate")) {
			stage->animationRate = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("boundsExpansion")) {
			stage->boundsExpansion = src.ParseFloat();
			continue;
		}

		if (!token.Icmp("gravity")) {
			src.ReadToken(&token);

			if (!token.Icmp("world")) {
				stage->worldGravity = true;
			} else {
				src.UnreadToken(&token);
			}

			stage->gravity = src.ParseFloat();
			continue;
		}

		src.Error("unknown token %s\n", token.c_str());
	}

	// derive values
	stage->cycleMsec = (stage->particleLife + stage->deadTime) * 1000;

	return stage;
}

/*
================
rvBSEParticle::Parse
================
*/
bool rvBSEParticle::Parse(const char *text, const int textLength, bool noCaching)
{
	idLexer src;
	idToken	token;

	src.LoadMemory(text, textLength, GetFileName(), GetLineNum());
	src.SetFlags(DECL_LEXER_FLAGS);
	src.SkipUntilString("{");

	depthHack = 0.0f;

	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Icmp("{")) {
			rvBSEParticleStage *stage = ParseParticleStage(src);

			if (!stage) {
				src.Warning("Particle stage parse failed");
				MakeDefault();
				return false;
			}

			stages.Append(stage);
			continue;
		}

		if (!token.Icmp("depthHack")) {
			depthHack = src.ParseFloat();
			continue;
		}


		src.Warning("bad token %s", token.c_str());
		MakeDefault();
		return false;
	}

	//
	// calculate the bounds
	//
	bounds.Clear();

	for (int i = 0; i < stages.Num(); i++) {
		GetStageBounds(stages[i]);
		bounds.AddBounds(stages[i]->bounds);
	}

	if (bounds.GetVolume() <= 0.1f) {
		bounds = idBounds(vec3_origin).Expand(8.0f);
	}

	return true;
}

/*
================
rvBSEParticle::FreeData
================
*/
void rvBSEParticle::FreeData(void)
{
	stages.DeleteContents(true);
}

/*
================
rvBSEParticle::DefaultDefinition
================
*/
const char *rvBSEParticle::DefaultDefinition(void) const
{
	return
	        "{\n"
	        "\t"	"{\n"
	        "\t\t"		"material\t_default\n"
	        "\t\t"		"count\t20\n"
	        "\t\t"		"time\t\t1.0\n"
	        "\t"	"}\n"
	        "}";
}

/*
================
rvBSEParticle::WriteParticleParm
================
*/
void rvBSEParticle::WriteParticleParm(idFile *f, rvBSEParticleParm *parm, const char *name)
{

	f->WriteFloatString("\t\t%s\t\t\t\t ", name);

	if (parm->table) {
		f->WriteFloatString("%s\n", parm->table->GetName());
	} else {
		f->WriteFloatString("\"%.3f\" ", parm->from);

		if (parm->from == parm->to) {
			f->WriteFloatString("\n");
		} else {
			f->WriteFloatString(" to \"%.3f\"\n", parm->to);
		}
	}
}

/*
================
rvBSEParticle::WriteStage
================
*/
void rvBSEParticle::WriteStage(idFile *f, rvBSEParticleStage *stage)
{

	int i;

	f->WriteFloatString("\t{\n");
	f->WriteFloatString("\t\tcount\t\t\t\t%i\n", stage->totalParticles);
	f->WriteFloatString("\t\tmaterial\t\t\t%s\n", stage->material->GetName());

	if (stage->animationFrames) {
		f->WriteFloatString("\t\tanimationFrames \t%i\n", stage->animationFrames);
	}

	if (stage->animationRate) {
		f->WriteFloatString("\t\tanimationRate \t\t%.3f\n", stage->animationRate);
	}

	f->WriteFloatString("\t\ttime\t\t\t\t%.3f\n", stage->particleLife);
	f->WriteFloatString("\t\tcycles\t\t\t\t%.3f\n", stage->cycles);

	if (stage->timeOffset) {
		f->WriteFloatString("\t\ttimeOffset\t\t\t%.3f\n", stage->timeOffset);
	}

	if (stage->deadTime) {
		f->WriteFloatString("\t\tdeadTime\t\t\t%.3f\n", stage->deadTime);
	}

	f->WriteFloatString("\t\tbunching\t\t\t%.3f\n", stage->spawnBunching);

	f->WriteFloatString("\t\tdistribution\t\t%s ", ParticleDistributionDesc[stage->distributionType].name);

	for (i = 0; i < ParticleDistributionDesc[stage->distributionType].count; i++) {
		f->WriteFloatString("%.3f ", stage->distributionParms[i]);
	}

	f->WriteFloatString("\n");

	f->WriteFloatString("\t\tdirection\t\t\t%s ", ParticleDirectionDesc[stage->directionType].name);

	for (i = 0; i < ParticleDirectionDesc[stage->directionType].count; i++) {
		f->WriteFloatString("\"%.3f\" ", stage->directionParms[i]);
	}

	f->WriteFloatString("\n");

	f->WriteFloatString("\t\torientation\t\t\t%s ", ParticleOrientationDesc[stage->orientation].name);

	for (i = 0; i < ParticleOrientationDesc[stage->orientation].count; i++) {
		f->WriteFloatString("%.3f ", stage->orientationParms[i]);
	}

	f->WriteFloatString("\n");

	if (stage->customPathType != PPATH_STANDARD) {
		f->WriteFloatString("\t\tcustomPath %s ", ParticleCustomDesc[stage->customPathType].name);

		for (i = 0; i < ParticleCustomDesc[stage->customPathType].count; i++) {
			f->WriteFloatString("%.3f ", stage->customPathParms[i]);
		}

		f->WriteFloatString("\n");
	}

	if (stage->entityColor) {
		f->WriteFloatString("\t\tentityColor\t\t\t1\n");
	}

	WriteParticleParm(f, &stage->speed, "speed");
	WriteParticleParm(f, &stage->size, "size");
	WriteParticleParm(f, &stage->aspect, "aspect");

	if (stage->rotationSpeed.from) {
		WriteParticleParm(f, &stage->rotationSpeed, "rotation");
	}

	if (stage->initialAngle) {
		f->WriteFloatString("\t\tangle\t\t\t\t%.3f\n", stage->initialAngle);
	}

	f->WriteFloatString("\t\trandomDistribution\t\t\t\t%i\n", static_cast<int>(stage->randomDistribution));
	f->WriteFloatString("\t\tboundsExpansion\t\t\t\t%.3f\n", stage->boundsExpansion);


	f->WriteFloatString("\t\tfadeIn\t\t\t\t%.3f\n", stage->fadeInFraction);
	f->WriteFloatString("\t\tfadeOut\t\t\t\t%.3f\n", stage->fadeOutFraction);
	f->WriteFloatString("\t\tfadeIndex\t\t\t\t%.3f\n", stage->fadeIndexFraction);

	f->WriteFloatString("\t\tcolor \t\t\t\t%.3f %.3f %.3f %.3f\n", stage->color.x, stage->color.y, stage->color.z, stage->color.w);
	f->WriteFloatString("\t\tfadeColor \t\t\t%.3f %.3f %.3f %.3f\n", stage->fadeColor.x, stage->fadeColor.y, stage->fadeColor.z, stage->fadeColor.w);

	f->WriteFloatString("\t\toffset \t\t\t\t%.3f %.3f %.3f\n", stage->offset.x, stage->offset.y, stage->offset.z);
	f->WriteFloatString("\t\tgravity \t\t\t");

	if (stage->worldGravity) {
		f->WriteFloatString("world ");
	}

	f->WriteFloatString("%.3f\n", stage->gravity);
	f->WriteFloatString("\t}\n");
}

/*
================
rvBSEParticle::RebuildTextSource
================
*/
bool rvBSEParticle::RebuildTextSource(void)
{
	idFile_Memory f;

	f.WriteFloatString("\n\n/*\n"
	                   "\tGenerated by the Particle Editor.\n"
	                   "\tTo use the particle editor, launch the game and type 'editParticles' on the console.\n"
	                   "*/\n");

	f.WriteFloatString("particle %s {\n", GetName());

	if (depthHack) {
		f.WriteFloatString("\tdepthHack\t%f\n", depthHack);
	}

	for (int i = 0; i < stages.Num(); i++) {
		WriteStage(&f, stages[i]);
	}

	f.WriteFloatString("}");

	SetText(f.GetDataPtr());

	return true;
}

/*
================
rvBSEParticle::Save
================
*/
bool rvBSEParticle::Save(const char *fileName)
{
	RebuildTextSource();

	if (fileName) {
		declManager->CreateNewDecl(DECL_PARTICLE, GetName(), fileName);
	}

	ReplaceSourceFileText();
	return true;
}

/*
====================================================================================

rvBSEParticleParm

====================================================================================
*/

float rvBSEParticleParm::Eval(float frac, idRandom &rand) const
{
	if (table) {
		return table->TableLookup(frac);
	}

	return from + frac * (to - from);
}

float rvBSEParticleParm::Integrate(float frac, idRandom &rand) const
{
	if (table) {
		common->Printf("rvBSEParticleParm::Integrate: can't integrate tables\n");
		return 0;
	}

	return (from + frac * (to - from) * 0.5f) * frac;
}

/*
====================================================================================

rvBSEParticleStage

====================================================================================
*/

/*
================
rvBSEParticleStage::rvBSEParticleStage
================
*/
rvBSEParticleStage::rvBSEParticleStage(void)
{
	material = NULL;
	totalParticles = 0;
	cycles = 0.0f;
	cycleMsec = 0;
	spawnBunching = 0.0f;
	particleLife = 0.0f;
	timeOffset = 0.0f;
	deadTime = 0.0f;
	distributionType = PDIST_RECT;
	distributionParms[0] = distributionParms[1] = distributionParms[2] = distributionParms[3] = 0.0f;
	directionType = PDIR_CONE;
	directionParms[0] = directionParms[1] = directionParms[2] = directionParms[3] = 0.0f;
	// rvBSEParticleParm		speed;
	gravity = 0.0f;
	worldGravity = false;
	customPathType = PPATH_STANDARD;
	customPathParms[0] = customPathParms[1] = customPathParms[2] = customPathParms[3] = 0.0f;
	customPathParms[4] = customPathParms[5] = customPathParms[6] = customPathParms[7] = 0.0f;
	offset.Zero();
	animationFrames = 0;
	animationRate = 0.0f;
	randomDistribution = true;
	entityColor = false;
	initialAngle = 0.0f;
	// rvBSEParticleParm		rotationSpeed;
	orientation = POR_VIEW;
	orientationParms[0] = orientationParms[1] = orientationParms[2] = orientationParms[3] = 0.0f;
	// rvBSEParticleParm		size
	// rvBSEParticleParm		aspect
	color.Zero();
	fadeColor.Zero();
	fadeInFraction = 0.0f;
	fadeOutFraction = 0.0f;
	fadeIndexFraction = 0.0f;
	hidden = false;
	boundsExpansion = 0.0f;
	bounds.Clear();
}

/*
================
rvBSEParticleStage::Default

Sets the stage to a default state
================
*/
void rvBSEParticleStage::Default()
{
	material = declManager->FindMaterial("_default");
	totalParticles = 100;
	spawnBunching = 1.0f;
	particleLife = 1.5f;
	timeOffset = 0.0f;
	deadTime = 0.0f;
	distributionType = PDIST_RECT;
	distributionParms[0] = 8.0f;
	distributionParms[1] = 8.0f;
	distributionParms[2] = 8.0f;
	distributionParms[3] = 0.0f;
	directionType = PDIR_CONE;
	directionParms[0] = 90.0f;
	directionParms[1] = 0.0f;
	directionParms[2] = 0.0f;
	directionParms[3] = 0.0f;
	orientation = POR_VIEW;
	orientationParms[0] = 0.0f;
	orientationParms[1] = 0.0f;
	orientationParms[2] = 0.0f;
	orientationParms[3] = 0.0f;
	speed.from = 150.0f;
	speed.to = 150.0f;
	speed.table = NULL;
	gravity = 1.0f;
	worldGravity = false;
	customPathType = PPATH_STANDARD;
	customPathParms[0] = 0.0f;
	customPathParms[1] = 0.0f;
	customPathParms[2] = 0.0f;
	customPathParms[3] = 0.0f;
	customPathParms[4] = 0.0f;
	customPathParms[5] = 0.0f;
	customPathParms[6] = 0.0f;
	customPathParms[7] = 0.0f;
	offset.Zero();
	animationFrames = 0;
	animationRate = 0.0f;
	initialAngle = 0.0f;
	rotationSpeed.from = 0.0f;
	rotationSpeed.to = 0.0f;
	rotationSpeed.table = NULL;
	size.from = 4.0f;
	size.to = 4.0f;
	size.table = NULL;
	aspect.from = 1.0f;
	aspect.to = 1.0f;
	aspect.table = NULL;
	color.x = 1.0f;
	color.y = 1.0f;
	color.z = 1.0f;
	color.w = 1.0f;
    fadeColor.x = 0.0f;
    fadeColor.y = 0.0f;
    fadeColor.z = 0.0f;
    fadeColor.w = 0.0f;
#if 1
    fadeInFraction = 0.0f;
    fadeOutFraction = 0.0f;
#else
	fadeInFraction = 0.1f;
	fadeOutFraction = 0.25f;
#endif
	fadeIndexFraction = 0.0f;
	boundsExpansion = 0.0f;
	randomDistribution = true;
	entityColor = false;
	cycleMsec = (particleLife + deadTime) * 1000;

    fade = false;
}

/*
================
rvBSEParticleStage::NumQuadsPerParticle

includes trails and cross faded animations
================
*/
int rvBSEParticleStage::NumQuadsPerParticle() const
{
	int	count = 1;

	if (orientation == POR_AIMED) {
		int	trails = idMath::Ftoi(orientationParms[0]);
		// each trail stage will add an extra quad
		count *= (1 + trails);
	}

	// if we are doing strip-animation, we need to double the number and cross fade them
	if (animationFrames > 1) {
		count *= 2;
	}

	return count;
}

/*
===============
rvBSEParticleStage::ParticleOrigin
===============
*/
void rvBSEParticleStage::ParticleOrigin(rvBSE_particleGen_t *g, idVec3 &origin) const
{
	origin.Zero();

	if (customPathType == PPATH_STANDARD) {
		//
		// find intial origin distribution
		//
		float radiusSqr, angle1, angle2;

		switch (distributionType) {
			case PDIST_RECT: {	// ( sizeX sizeY sizeZ )
				origin[0] = ((randomDistribution) ? g->random.CRandomFloat() : 1.0f) * distributionParms[0];
				origin[1] = ((randomDistribution) ? g->random.CRandomFloat() : 1.0f) * distributionParms[1];
				origin[2] = ((randomDistribution) ? g->random.CRandomFloat() : 1.0f) * distributionParms[2];
				break;
			}
			case PDIST_CYLINDER: {	// ( sizeX sizeY sizeZ ringFraction )
				angle1 = ((randomDistribution) ? g->random.CRandomFloat() : 1.0f) * idMath::TWO_PI;

				idMath::SinCos16(angle1, origin[0], origin[1]);
				origin[2] = ((randomDistribution) ? g->random.CRandomFloat() : 1.0f);

				// reproject points that are inside the ringFraction to the outer band
				if (distributionParms[3] > 0.0f) {
					radiusSqr = origin[0] * origin[0] + origin[1] * origin[1];

					if (radiusSqr < distributionParms[3] * distributionParms[3]) {
						// if we are inside the inner reject zone, rescale to put it out into the good zone
						float f = sqrt(radiusSqr) / distributionParms[3];
						float invf = 1.0f / f;
						float newRadius = distributionParms[3] + f * (1.0f - distributionParms[3]);
						float rescale = invf * newRadius;

						origin[0] *= rescale;
						origin[1] *= rescale;
					}
				}

				origin[0] *= distributionParms[0];
				origin[1] *= distributionParms[1];
				origin[2] *= distributionParms[2];
				break;
			}
			case PDIST_SPHERE: {	// ( sizeX sizeY sizeZ ringFraction )
				// iterating with rejection is the only way to get an even distribution over a sphere
				if (randomDistribution) {
					do {
						origin[0] = g->random.CRandomFloat();
						origin[1] = g->random.CRandomFloat();
						origin[2] = g->random.CRandomFloat();
						radiusSqr = origin[0] * origin[0] + origin[1] * origin[1] + origin[2] * origin[2];
					} while (radiusSqr > 1.0f);
				} else {
					origin.Set(1.0f, 1.0f, 1.0f);
					radiusSqr = 3.0f;
				}

				if (distributionParms[3] > 0.0f) {
					// we could iterate until we got something that also satisfied ringFraction,
					// but for narrow rings that could be a lot of work, so reproject inside points instead
					if (radiusSqr < distributionParms[3] * distributionParms[3]) {
						// if we are inside the inner reject zone, rescale to put it out into the good zone
						float f = sqrt(radiusSqr) / distributionParms[3];
						float invf = 1.0f / f;
						float newRadius = distributionParms[3] + f * (1.0f - distributionParms[3]);
						float rescale = invf * newRadius;

						origin[0] *= rescale;
						origin[1] *= rescale;
						origin[2] *= rescale;
					}
				}

				origin[0] *= distributionParms[0];
				origin[1] *= distributionParms[1];
				origin[2] *= distributionParms[2];
				break;
			}
		}

		// offset will effect all particle origin types
		// add this before the velocity and gravity additions
		origin += offset;

		//
		// add the velocity over time
		//
		idVec3	dir;

		switch (directionType) {
			case PDIR_CONE: {
				// angle is the full angle, so 360 degrees is any spherical direction
				angle1 = g->random.CRandomFloat() * directionParms[0] * idMath::M_DEG2RAD;
				angle2 = g->random.CRandomFloat() * idMath::PI;

				float s1, c1, s2, c2;
				idMath::SinCos16(angle1, s1, c1);
				idMath::SinCos16(angle2, s2, c2);

				dir[0] = s1 * c2;
				dir[1] = s1 * s2;
				dir[2] = c1;
				break;
			}
			case PDIR_OUTWARD: {
				dir = origin;
				dir.Normalize();
				dir[2] += directionParms[0];
				break;
			}
		}

		// add speed
		float iSpeed = speed.Integrate(g->frac, g->random);
		origin += dir * iSpeed * particleLife;

	} else {
		//
		// custom paths completely override both the origin and velocity calculations, but still
		// use the standard gravity
		//
		float angle1, angle2, speed1, speed2;

		switch (customPathType) {
			case PPATH_HELIX: {		// ( sizeX sizeY sizeZ radialSpeed axialSpeed )
				speed1 = g->random.CRandomFloat();
				speed2 = g->random.CRandomFloat();
				angle1 = g->random.RandomFloat() * idMath::TWO_PI + customPathParms[3] * speed1 * g->age;

				float s1, c1;
				idMath::SinCos16(angle1, s1, c1);

				origin[0] = c1 * customPathParms[0];
				origin[1] = s1 * customPathParms[1];
				origin[2] = g->random.RandomFloat() * customPathParms[2] + customPathParms[4] * speed2 * g->age;
				break;
			}
			case PPATH_FLIES: {		// ( radialSpeed axialSpeed size )
				speed1 = idMath::ClampFloat(0.4f, 1.0f, g->random.CRandomFloat());
				speed2 = idMath::ClampFloat(0.4f, 1.0f, g->random.CRandomFloat());
				angle1 = g->random.RandomFloat() * idMath::PI * 2 + customPathParms[0] * speed1 * g->age;
				angle2 = g->random.RandomFloat() * idMath::PI * 2 + customPathParms[1] * speed1 * g->age;

				float s1, c1, s2, c2;
				idMath::SinCos16(angle1, s1, c1);
				idMath::SinCos16(angle2, s2, c2);

				origin[0] = c1 * c2;
				origin[1] = s1 * c2;
				origin[2] = -s2;
				origin *= customPathParms[2];
				break;
			}
			case PPATH_ORBIT: {		// ( radius speed axis )
				angle1 = g->random.RandomFloat() * idMath::TWO_PI + customPathParms[1] * g->age;

				float s1, c1;
				idMath::SinCos16(angle1, s1, c1);

				origin[0] = c1 * customPathParms[0];
				origin[1] = s1 * customPathParms[0];
				origin.ProjectSelfOntoSphere(customPathParms[0]);
				break;
			}
			case PPATH_DRIP: {		// ( speed )
				origin[0] = 0.0f;
				origin[1] = 0.0f;
				origin[2] = -(g->age * customPathParms[0]);
				break;
			}
			default: {
				common->Error("rvBSEParticleStage::ParticleOrigin: bad customPathType");
			}
		}

		origin += offset;
	}

	// adjust for the per-particle smoke offset
	origin *= g->axis;
	origin += g->origin;

	// add gravity after adjusting for axis
	if (worldGravity) {
		idVec3 gra(0, 0, -gravity);
		gra *= g->renderEnt->axis.Transpose();
		origin += gra * g->age * g->age;
	} else {
		origin[2] -= gravity * g->age * g->age;
	}
}

/*
==================
rvBSEParticleStage::ParticleVerts
==================
*/
extern void R_AxisToModelMatrix(const idMat3 &axis, const idVec3 &origin, float modelMatrix[16]);
extern void R_GlobalPointToLocal(const float modelMatrix[16], const idVec3 &in, idVec3 &out);
void R_LocalPointToGlobal(const float modelMatrix[16], const idVec3 &in, idVec3 &out);

static int ParticleVerts_line(const rvBSEParticleStage *stage, rvBSE_particleGen_t *g, idVec3 origin, idDrawVert *verts)
{
	bool useEndOrigin = stage->customPathParms[6] < 1.0f;

	float	modelMatrix[16];

	if(useEndOrigin)
	{
		idVec3	localView, localTarget;
		R_AxisToModelMatrix(g->renderEnt->axis, g->renderEnt->origin, modelMatrix);
		idVec3 target = idVec3(g->renderEnt->shaderParms[SHADERPARM_BEAM_WIDTH], g->renderEnt->shaderParms[SHADERPARM_BEAM_END_Y], g->renderEnt->shaderParms[SHADERPARM_BEAM_END_Z]);
		R_GlobalPointToLocal(modelMatrix, g->renderView->vieworg, localView);
		R_GlobalPointToLocal(modelMatrix, target, localTarget);

		idVec3	major = localTarget;
		idVec3	minor;

		idVec3	mid = 0.5f * localTarget;
		idVec3	dir = mid - localView;
		minor.Cross(major, dir);
		minor.Normalize();

		if (stage->customPathParms[5] != 0.0f) {
			minor *= stage->customPathParms[5] * 0.5f;
		}

		verts[0].xyz = minor;
		verts[1].xyz = localTarget + minor;
		verts[2].xyz = -minor;
		verts[3].xyz = localTarget - minor;
	}
	else
	{
		idVec3	localView, localTarget;
		idVec3 target = origin + g->renderEnt->axis[0] * stage->customPathParms[7];
		R_AxisToModelMatrix(g->renderEnt->axis, origin, modelMatrix);
		R_GlobalPointToLocal(modelMatrix, g->renderView->vieworg, localView);
		R_GlobalPointToLocal(modelMatrix, target, localTarget);

		idVec3	major = localTarget;
		idVec3	minor;

		idVec3	mid = 0.5f * localTarget;
		idVec3	dir = mid - localView;
		minor.Cross(major, dir);
		minor.Normalize();

		if (stage->customPathParms[5] != 0.0f) {
			minor *= stage->customPathParms[5] * 0.5f;
		}

		verts[0].xyz = origin + minor;
		verts[1].xyz = origin + localTarget + minor;
		verts[2].xyz = origin - minor;
		verts[3].xyz = origin + localTarget - minor;
	}

	return 4;
}

static int ParticleVerts_model(const rvBSEParticleStage *stage, rvBSE_particleGen_t *g, idVec3 origin, idDrawVert *verts)
{
	idRenderModel *model = stage->model;
	if(!model)
		return 0;
	if(!model->NumSurfaces())
		return 0;
	const modelSurface_t *msurf = model->Surface(0);
	if(!msurf)
		return 0;

	for(int i = 0; i < msurf->geometry->numVerts; i++)
	{
		idDrawVert *p = verts + i;
		p->xyz = msurf->geometry->verts[i].xyz + origin;
		p->st = msurf->geometry->verts[i].st;
		p->color[0] = 255;
		p->color[1] = 255;
		p->color[2] = 255;
		p->color[3] = 255;
	}

	return msurf->geometry->numVerts;
}

int	rvBSEParticleStage::ParticleVerts(rvBSE_particleGen_t *g, idVec3 origin, idDrawVert *verts) const
{
	if (rvptype == PTYPE_LINE)
	{
		return ParticleVerts_line(this, g, origin, verts);
	}
	else if (rvptype == PTYPE_MODEL)
	{
		return ParticleVerts_model(this, g, origin, verts);
	}
	float	psize = size.Eval(g->frac, g->random);
	float	paspect = aspect.Eval(g->frac, g->random);

	float	width = psize;
	float	height = psize * paspect;

	idVec3	left, up;

	if (orientation == POR_AIMED) {
		// reset the values to an earlier time to get a previous origin
		idRandom	currentRandom = g->random;
		float		currentAge = g->age;
		float		currentFrac = g->frac;
		idDrawVert *verts_p = verts;
		idVec3		stepOrigin = origin;
		idVec3		stepLeft;
		int			numTrails = idMath::Ftoi(orientationParms[0]);
		float		trailTime = orientationParms[1];

		if (trailTime == 0) {
			trailTime = 0.5f;
		}

		float height = 1.0f / (1 + numTrails);
		float t = 0;

		//LOGI("TTTTTTTT %d %f", numTrails, trailTime)
		for (int i = 0 ; i <= numTrails ; i++) {
			g->random = g->originalRandom;
			g->age = currentAge - (i + 1) * trailTime / (numTrails + 1);	// time to back up
			g->frac = g->age / particleLife;

			idVec3	oldOrigin;
			ParticleOrigin(g, oldOrigin);

			up = stepOrigin - oldOrigin;	// along the direction of travel

			idVec3	forwardDir;
			g->renderEnt->axis.ProjectVector(g->renderView->viewaxis[0], forwardDir);

			up -= (up * forwardDir) * forwardDir;

			up.Normalize();


			left = up.Cross(forwardDir);
			left *= psize;

			verts_p[0] = verts[0];
			verts_p[1] = verts[1];
			verts_p[2] = verts[2];
			verts_p[3] = verts[3];

			if (i == 0) {
				verts_p[0].xyz = stepOrigin - left;
				verts_p[1].xyz = stepOrigin + left;
			} else {
				verts_p[0].xyz = stepOrigin - stepLeft;
				verts_p[1].xyz = stepOrigin + stepLeft;
			}

			verts_p[2].xyz = oldOrigin - left;
			verts_p[3].xyz = oldOrigin + left;

			// modify texcoords
			verts_p[0].st[0] = verts[0].st[0];
			verts_p[0].st[1] = t;

			verts_p[1].st[0] = verts[1].st[0];
			verts_p[1].st[1] = t;

			verts_p[2].st[0] = verts[2].st[0];
			verts_p[2].st[1] = t+height;

			verts_p[3].st[0] = verts[3].st[0];
			verts_p[3].st[1] = t+height;

			//LOGI("TTTTT %f %f, %s | %s | %s | %s", speed.from, speed.to, verts_p[0].xyz.ToString(), verts_p[1].xyz.ToString(), verts_p[2].xyz.ToString(), verts_p[3].xyz.ToString())
			t += height;

			verts_p += 4;

			stepOrigin = oldOrigin;
			stepLeft = left;
		}

		g->random = currentRandom;
		g->age = currentAge;
		g->frac = currentFrac;

		return 4 * (numTrails+1);
	}

	//
	// constant rotation
	//
	float	angle;

	angle = (initialAngle) ? initialAngle : 360 * g->random.RandomFloat();

	float	angleMove = rotationSpeed.Integrate(g->frac, g->random) * particleLife;

	// have hald the particles rotate each way
	if (g->index & 1) {
		angle += angleMove;
	} else {
		angle -= angleMove;
	}

	angle = angle / 180 * idMath::PI;
	float c = idMath::Cos16(angle);
	float s = idMath::Sin16(angle);

	if (orientation  == POR_Z) {
		// oriented in entity space
		left[0] = s;
		left[1] = c;
		left[2] = 0;
		up[0] = c;
		up[1] = -s;
		up[2] = 0;
	} else if (orientation == POR_X) {
		// oriented in entity space
		left[0] = 0;
		left[1] = c;
		left[2] = s;
		up[0] = 0;
		up[1] = -s;
		up[2] = c;
	} else if (orientation == POR_Y) {
		// oriented in entity space
		left[0] = c;
		left[1] = 0;
		left[2] = s;
		up[0] = -s;
		up[1] = 0;
		up[2] = c;
	} else if (orientation == POR_DIR) {
		// oriented in entity space
		left[0] = orientationParms[2];
		left[1] = c * orientationParms[0];
		left[2] = s * orientationParms[1];
		up[0] = orientationParms[2];
		up[1] = -s * orientationParms[1];
		up[2] = c * orientationParms[0];
	} else {
		// oriented in viewer space
		idVec3	entityLeft, entityUp;

		g->renderEnt->axis.ProjectVector(g->renderView->viewaxis[1], entityLeft);
		g->renderEnt->axis.ProjectVector(g->renderView->viewaxis[2], entityUp);

		left = entityLeft * c + entityUp * s;
		up = entityUp * c - entityLeft * s;
	}

	left *= width;
	up *= height;

	verts[0].xyz = origin - left + up;
	verts[1].xyz = origin + left + up;
	verts[2].xyz = origin - left - up;
	verts[3].xyz = origin + left - up;

	return 4;
}

/*
==================
rvBSEParticleStage::ParticleTexCoords
==================
*/
void rvBSEParticleStage::ParticleTexCoords(rvBSE_particleGen_t *g, idDrawVert *verts) const
{
	float	s, width;
	float	t, height;

	if (animationFrames > 1) {
		width = 1.0f / animationFrames;
		float	floatFrame;

		if (animationRate) {
			// explicit, cycling animation
			floatFrame = g->age * animationRate;
		} else {
			// single animation cycle over the life of the particle
			floatFrame = g->frac * animationFrames;
		}

		int	intFrame = (int)floatFrame;
		g->animationFrameFrac = floatFrame - intFrame;
		s = width * intFrame;
	} else {
		s = 0.0f;
		width = 1.0f;
	}

	if(rvptype == PTYPE_MODEL)
		return;
	t = 0.0f;
	height = 1.0f;

	verts[0].st[0] = s;
	verts[0].st[1] = t;

	verts[1].st[0] = s+width;
	verts[1].st[1] = t;

	verts[2].st[0] = s;
	verts[2].st[1] = t+height;

	verts[3].st[0] = s+width;
	verts[3].st[1] = t+height;
}

/*
==================
rvBSEParticleStage::ParticleColors
==================
*/
void rvBSEParticleStage::ParticleColors(rvBSE_particleGen_t *g, idDrawVert *verts) const
{
	float	fadeFraction = fade ? g->frac : 1.0f;

	// most particles fade in at the beginning and fade out at the end
	if (g->frac < fadeInFraction) {
		fadeFraction *= (g->frac / fadeInFraction);
	}

	if (1.0f - g->frac < fadeOutFraction) {
		fadeFraction *= ((1.0f - g->frac) / fadeOutFraction);
	}

	// individual gun smoke particles get more and more faded as the
	// cycle goes on (note that totalParticles won't be correct for a surface-particle deform)
	if (fadeIndexFraction) {
		float	indexFrac = (totalParticles - g->index) / (float)totalParticles;

		if (indexFrac < fadeIndexFraction) {
			fadeFraction *= indexFrac / fadeIndexFraction;
		}
	}

	for (int i = 0 ; i < 4 ; i++) {
#if 0
		float	fcolor = ((entityColor) ? g->renderEnt->shaderParms[i] : color[i]) * fadeFraction + fadeColor[i] * (1.0f - fadeFraction);
#else
		float fcolor = (g->renderEnt->shaderParms[i] * color[i]) * fadeFraction + fadeColor[i] * (1.0f - fadeFraction);
#endif
		int		icolor = idMath::FtoiFast(fcolor * 255.0f);

		if (icolor < 0) {
			icolor = 0;
		} else if (icolor > 255) {
			icolor = 255;
		}

		verts[0].color[i] =
		        verts[1].color[i] =
		                verts[2].color[i] =
		                        verts[3].color[i] = icolor;
	}
}

/*
================
rvBSEParticleStage::CreateParticle

Returns 0 if no particle is created because it is completely faded out
Returns 4 if a normal quad is created
Returns 8 if two cross faded quads are created

Vertex order is:

0 1
2 3
================
*/
int rvBSEParticleStage::CreateParticle(rvBSE_particleGen_t *g, idDrawVert *verts) const
{
	idVec3	origin;

	if(rvptype != PTYPE_MODEL)
	{
        verts[0].Clear();
        verts[1].Clear();
        verts[2].Clear();
        verts[3].Clear();

        ParticleColors(g, verts);

        // if we are completely faded out, kill the particle
        if (verts[0].color[0] == 0 && verts[0].color[1] == 0 && verts[0].color[2] == 0 && verts[0].color[3] == 0) {
            return 0;
        }
	}

	ParticleOrigin(g, origin);

	ParticleTexCoords(g, verts);

	int	numVerts = ParticleVerts(g, origin, verts);

	if (animationFrames <= 1) {
		return numVerts;
	}

	// if we are doing strip-animation, we need to double the quad and cross fade it
	float	width = 1.0f / animationFrames;
	float	frac = g->animationFrameFrac;
	float	iFrac = 1.0f - frac;

	for (int i = 0 ; i < numVerts ; i++) {
		verts[numVerts + i] = verts[i];

		verts[numVerts + i].st[0] += width;

		verts[numVerts + i].color[0] *= frac;
		verts[numVerts + i].color[1] *= frac;
		verts[numVerts + i].color[2] *= frac;
		verts[numVerts + i].color[3] *= frac;

		verts[i].color[0] *= iFrac;
		verts[i].color[1] *= iFrac;
		verts[i].color[2] *= iFrac;
		verts[i].color[3] *= iFrac;
	}

	return numVerts * 2;
}

/*
==================
rvBSEParticleStage::GetCustomPathName
==================
*/
const char *rvBSEParticleStage::GetCustomPathName()
{
	int index = (customPathType < CustomParticleCount) ? customPathType : 0;
	return ParticleCustomDesc[index].name;
}

/*
==================
rvBSEParticleStage::GetCustomPathDesc
==================
*/
const char *rvBSEParticleStage::GetCustomPathDesc()
{
	int index = (customPathType < CustomParticleCount) ? customPathType : 0;
	return ParticleCustomDesc[index].desc;
}

/*
==================
rvBSEParticleStage::NumCustomPathParms
==================
*/
int rvBSEParticleStage::NumCustomPathParms()
{
	int index = (customPathType < CustomParticleCount) ? customPathType : 0;
	return ParticleCustomDesc[index].count;
}

/*
==================
rvBSEParticleStage::SetCustomPathType
==================
*/
void rvBSEParticleStage::SetCustomPathType(const char *p)
{
	customPathType = PPATH_STANDARD;

	for (int i = 0; i < CustomParticleCount; i ++) {
		if (idStr::Icmp(p, ParticleCustomDesc[i].name) == 0) {
			customPathType = static_cast<prtCustomPth_t>(i);
			break;
		}
	}
}

/*
==================
rvBSEParticleStage::operator=
==================
*/
void rvBSEParticleStage::operator=(const rvBSEParticleStage &src)
{
	material = src.material;
	totalParticles = src.totalParticles;
	cycles = src.cycles;
	cycleMsec = src.cycleMsec;
	spawnBunching = src.spawnBunching;
	particleLife = src.particleLife;
	timeOffset = src.timeOffset;
	deadTime = src.deadTime;
	distributionType = src.distributionType;
	distributionParms[0] = src.distributionParms[0];
	distributionParms[1] = src.distributionParms[1];
	distributionParms[2] = src.distributionParms[2];
	distributionParms[3] = src.distributionParms[3];
	directionType = src.directionType;
	directionParms[0] = src.directionParms[0];
	directionParms[1] = src.directionParms[1];
	directionParms[2] = src.directionParms[2];
	directionParms[3] = src.directionParms[3];
	speed = src.speed;
	gravity = src.gravity;
	worldGravity = src.worldGravity;
	randomDistribution = src.randomDistribution;
	entityColor = src.entityColor;
	customPathType = src.customPathType;
	customPathParms[0] = src.customPathParms[0];
	customPathParms[1] = src.customPathParms[1];
	customPathParms[2] = src.customPathParms[2];
	customPathParms[3] = src.customPathParms[3];
	customPathParms[4] = src.customPathParms[4];
	customPathParms[5] = src.customPathParms[5];
	customPathParms[6] = src.customPathParms[6];
	customPathParms[7] = src.customPathParms[7];
	offset = src.offset;
	animationFrames = src.animationFrames;
	animationRate = src.animationRate;
	initialAngle = src.initialAngle;
	rotationSpeed = src.rotationSpeed;
	orientation = src.orientation;
	orientationParms[0] = src.orientationParms[0];
	orientationParms[1] = src.orientationParms[1];
	orientationParms[2] = src.orientationParms[2];
	orientationParms[3] = src.orientationParms[3];
	size = src.size;
	aspect = src.aspect;
	color = src.color;
	fadeColor = src.fadeColor;
	fadeInFraction = src.fadeInFraction;
	fadeOutFraction = src.fadeOutFraction;
	fadeIndexFraction = src.fadeIndexFraction;
	hidden = src.hidden;
	boundsExpansion = src.boundsExpansion;
	bounds = src.bounds;

	rvptype = src.rvptype;
	model = src.model;
}

