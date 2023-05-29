// Quake4's effects fx file simple parser

#define SKIP_UNKNOWN_TOKEN(x) \
	if(!token.Icmp(x)) { \
		Skip(); \
		continue; \
	}

#define SKIP_BRACED_SECTION() \
	if(!token.Cmp("{")) { \
		src.SkipBracedSection(); \
		continue; \
	}

#define IF_ZERO_SET(target, x) \
	if(target == 0.0) { \
		target = x; \
	}

#define BSE_MODEL_SUFFIX ".bse"

enum {
	STAGE_START = 1,
	STAGE_MOTION,
	STAGE_END
};

static idHashTable<rvBSEParticle *> particle_decl_hash;
rvBSEParticle * BSE_GetDeclParticle(const char *name)
{
	rvBSEParticle **res;

	bool b = particle_decl_hash.Get(name, &res);
	return b ? *res : NULL;
}

static rvBSEParticle * BSE_SetDeclParticle(const char *name, rvBSEParticle *particle = NULL)
{
	rvBSEParticle *p = BSE_GetDeclParticle(name);
	if(!particle)
		particle_decl_hash.Remove(name);
	else
		particle_decl_hash.Set(name, particle);
	return p;
}

// https://web.archive.org/web/20101225045355/http://iddevnet.com/quake4/Basic_FX_file_structure

static int genParticleStage = 0;

class rvDeclEffectParser
{
	public:
		explicit rvDeclEffectParser(rvDeclEffect *decl, idLexer &src);
		bool Parse(void);

	private:
		const char * GetName(void) const
		{
			return decl->GetName();
		}
		void Skip(void);
		rvBSEParticleStage * Alloc_rvBSEParticleStage(void);
		idStr BuildParticleSource(rvBSEParticle *particle, const char *name);
		int ParseVec(int *flagNum = 0);
		float ParseSize(rvBSEParticleStage *stage, int n);
		void ParsePosition(rvBSEParticleStage *stage);
		idVec3 ParseOffset(rvBSEParticleStage *stage);
		float ParseLength(rvBSEParticleStage *stage, bool *useEndOrigin = 0);
		// A segment consisting of polygons that always face the player.
		void ParseSprite(rvFXSingleAction &FXAction);
		// A segment made of polygons that face a direction specified in the segment properties. (Usually the normal of the effect.)
		void ParseOriented(rvFXSingleAction &FXAction);
		// A segment of lines. Lines differ from Sprites in that they have length and size (width), and can be made into very long rectangles. A sprite with a different height than width will be a diamond shape.
		void ParseLine(rvFXSingleAction &FXAction);
		void ParseStage(rvFXSingleAction &FXAction, rvBSEParticleStage *stage, int n);
		void ParseSpawner(rvFXSingleAction &FXAction);
		void ParseEmitter(rvFXSingleAction &FXAction);
		void Init_FXAction(rvFXSingleAction &FXAction);
		// Models can be placed in segments and given motion/gravity effects.
		void ParseModel(rvFXSingleAction &FXAction);
		// A pointlight within the effect that has size/color/light shader/timing options
		void ParseLight(rvFXSingleAction &FXAction);
		void ParseInnerLight(rvFXSingleAction &FXAction);
		// This segment places a decal on the surface the effect plays from. (note: some decal shaders have a specific lifetime in their material and as such are normally used only for weapon impact effects)
		void ParseDecal(rvFXSingleAction &FXAction);
		void ParseInnerDecal(rvFXSingleAction &FXAction);
		// Effects can have sound segments that play sounds at specific times during the effect.
		void ParseSound(rvFXSingleAction &FXAction);
		// Camera Shake, Double Vision and Tunnel Vision can all be added to an effect.
		void ParseShake(rvFXSingleAction &FXAction);
		void ParseDoubleVision(rvFXSingleAction &FXAction);
		bool HasVecFlag(const char *name) const;
		// Linked lines are used to have a set of lines linked together at their ends. Mostly used for arcing and curved effects attached to models or as projectiles such as the Quake4 Nailgun.
		void ParseLinked(rvFXSingleAction &FXAction);
		// Special case segment with Jitter and Forks settings allow easy simulation of electricity.
		void ParseElectricity(rvFXSingleAction &FXAction);
		// Debris defined in .def files can be used in place of models, in the event of more complex debris interactions. This segment allows the placement of Debris in an effect.
		void ParseDebris(rvFXSingleAction &FXAction);
		// Effects can be nested inside one another. By pressing the + plus button in the properties for this effect you can add complete effects to this segment, while the – minus button removes effects.
		void ParseEffect(void);
		void ParseInnerEffect(const char *name);
		// By using a delay, an entire effect can have its looping state altered to include a delay. A delay segment affects every segment within an effect.
		void ParseDelay(rvFXSingleAction &FXAction);
		void AppendDeclParticle(const char *name, rvBSEParticle *particle);
		void FinishDeclParticle(rvBSEParticle *particle, rvBSEParticleStage *stage, rvFXSingleAction &FXAction);
		void ParseTrail(rvFXSingleAction &FXAction);
		int ParseBox(idBounds &res, int *parmsCount = 0);
		int ParsePoint(idVec3 &res, int *parmsCount = 0);
		int ParseLine(idBounds &res, int *parmsCount = 0); // parse line geometry parmeters
		int ParseSphere(idSphere &res, int *parmsCount = 0);
		int ParseCylinder(idSphere &res, int *parmsCount = 0);
		int ParseSpiral(idBounds &res, float &i7, int *parmsCount = 0);
		void ParseVelocity(rvBSEParticleStage *stage, int n);
		void ParseFade(rvBSEParticleStage *stage, int n);
		idVec3 ParseTint(rvBSEParticleStage *stage, int n);
		float ParseRotate(rvBSEParticleStage *stage, int n);
		bool ParseGeneralParms(idToken &token, rvFXSingleAction &FXAction);
		bool ParseTrailParms(idToken &token, rvBSEParticleStage *stage);

	private:
		rvDeclEffect *decl;
		idLexer &src;
		idList<float> vec;
		idList<idStr> vecFlags;
};

rvDeclEffectParser::rvDeclEffectParser(rvDeclEffect *decl, idLexer &src)
	: decl(decl),
	src(src)
{
	vec.Resize(16);
}

bool rvDeclEffectParser::ParseTrailParms(idToken &token, rvBSEParticleStage *stage)
{
	if (!token.Icmp("trailCount")) {
		stage->orientation = POR_AIMED;
		ParseVec();
		stage->orientationParms[0] = vec[0];
		stage->distributionType = PDIST_RECT;
		return true;
	}

	if (!token.Icmp("trailTime")) {
		stage->orientation = POR_AIMED;
		ParseVec();
		stage->orientationParms[1] = vec[0];
		stage->distributionType = PDIST_RECT;
		return true;
	}
	if(!token.Icmp("trailType")) {
		Skip();
		return true;
	}
	if(!token.Icmp("trailMaterial")) {
		Skip();
		return true;
	}
	return false;
}

bool rvDeclEffectParser::ParseGeneralParms(idToken &token, rvFXSingleAction &FXAction)
{
	if (!token.Icmp("count")) {
		ParseVec();
		FXAction.count = vec[0];
		return true;
	}
	if (!token.Icmp("locked")) {
		FXAction.trackOrigin = true;
		return true;
	}
	if (!token.Icmp("constant")) {
		FXAction.restart = 1.0f;
		return true;
	}
	if (!token.Icmp("duration")) {
		ParseVec();
		FXAction.duration = vec[0];
		return true;
	}
	if (!token.Icmp("start")) {
		ParseVec();
		FXAction.delay = vec[0];
		return true;
	}
	if (!token.Icmp("density")) {
		ParseVec();
		if(FXAction.count == 0)
			FXAction.count = vec[0];
		return true;
	}
	return false;
}

int rvDeclEffectParser::ParseBox(idBounds &res, int *parmsCount)
{
	int count = ParseVec(parmsCount);
	if(count == 6)
	{
		res[0][0] = vec[0];
		res[0][1] = vec[1];
		res[0][2] = vec[2];
		res[1][0] = vec[3];
		res[1][1] = vec[4];
		res[1][2] = vec[5];
		return 3;
	}
	else if(count == 4)
	{
		res[0][0] = vec[0];
		res[0][1] = vec[1];
		res[0][2] = 0.0;
		res[1][0] = vec[2];
		res[1][1] = vec[3];
		res[1][2] = 0.0;
		return 2;
	}
	else if(count == 2)
	{
		res[0][0] = vec[0];
		res[0][1] = 0.0;
		res[0][2] = 0.0;
		res[1][0] = vec[1];
		res[1][1] = 0.0;
		res[1][2] = 0.0;
		return 1;
	}
	return 0;
}

int rvDeclEffectParser::ParsePoint(idVec3 &res, int *parmsCount)
{
	int count = ParseVec(parmsCount);
	if(count == 3)
	{
		res[0] = vec[0];
		res[1] = vec[1];
		res[2] = vec[2];
		return 3;
	}
	else if(count == 2)
	{
		res[0] = vec[0];
		res[1] = vec[1];
		res[2] = 0.0;
		return 2;
	}
	else if(count == 1)
	{
		res[0] = vec[0];
		res[1] = 0.0;
		res[2] = 0.0;
		return 1;
	}
	return 0;
}

int rvDeclEffectParser::ParseSpiral(idBounds &res, float &i7, int *parmsCount)
{
	int count = ParseVec(parmsCount);
	if(count == 7)
	{
		res[0][0] = vec[0];
		res[0][1] = vec[1];
		res[0][2] = vec[2];
		res[1][0] = vec[3];
		res[1][1] = vec[4];
		res[1][2] = vec[5];
		i7 = vec[6];
		return 3;
	}
	return 0;
}

int rvDeclEffectParser::ParseLine(idBounds &res, int *parmsCount)
{
	int count = ParseVec(parmsCount);
	if(count == 6)
	{
		res[0][0] = vec[0];
		res[0][1] = vec[1];
		res[0][2] = vec[2];
		res[1][0] = vec[3];
		res[1][1] = vec[4];
		res[1][2] = vec[5];
		return 3;
	}
	else if(count == 4)
	{
		res[0][0] = vec[0];
		res[0][1] = vec[1];
		res[0][2] = 0.0;
		res[1][0] = vec[2];
		res[1][1] = vec[3];
		res[1][2] = 0.0;
		return 2;
	}
	else if(count == 2)
	{
		res[0][0] = vec[0];
		res[0][1] = 0.0;
		res[0][2] = 0.0;
		res[1][0] = vec[1];
		res[1][1] = 0.0;
		res[1][2] = 0.0;
		return 1;
	}
	return 0;
}

int rvDeclEffectParser::ParseSphere(idSphere &res, int *parmsCount)
{
	int count = ParseVec(parmsCount);
	if(count == 6)
	{
		idVec3 a(vec[0], vec[1], vec[2]);
		idVec3 b(vec[3], vec[4], vec[5]);
		res.SetOrigin((b + a) / 2.0);
		res.SetRadius((b - a).LengthFast() / 2.0);
		return 1;
	}
	return 0;
}

int rvDeclEffectParser::ParseCylinder(idSphere &res, int *parmsCount)
{
	int count = ParseVec(parmsCount);
	if(count == 6)
	{
		idVec3 a(vec[0], vec[1], vec[2]);
		idVec3 b(vec[3], vec[4], vec[5]);
		res.SetOrigin((b + a) / 2.0);
		res.SetRadius((b - a).LengthFast() / 2.0);
		return 1;
	}
	return 0;
}

void rvDeclEffectParser::AppendDeclParticle(const char *name, rvBSEParticle *particle)
{
	decl->particles.Append(name);
	BSE_SetDeclParticle(name, particle);
}

void rvDeclEffectParser::FinishDeclParticle(rvBSEParticle *particle, rvBSEParticleStage *instage, rvFXSingleAction &FXAction)
{
	rvBSEParticleStage *stage;

	particle->depthHack = 0.0f;
	particle->bounds.Clear();

#if 1
	particle->stages.Append(instage);
	for(int i = 0; i < particle->stages.Num(); i++)
	{
		stage = particle->stages[i];
		stage->cycleMsec = (stage->particleLife + stage->deadTime) * 1000;
		stage->totalParticles = FXAction.count;
		//stage->cycles = FXAction.restart;
		//stage->timeOffset = FXAction.delay;
		// complete missing paramters
		if(stage->orientation == POR_AIMED)
		{
			if(stage->distributionType == PDIST_RECT)
			{
				IF_ZERO_SET(stage->distributionParms[0], 8);
				IF_ZERO_SET(stage->distributionParms[1], 8);
				IF_ZERO_SET(stage->distributionParms[2], 8);
				IF_ZERO_SET(stage->speed.from, 150.0f);
				IF_ZERO_SET(stage->speed.to, 150.0f);
			}
		}
		particle->GetStageBounds(stage);
		particle->bounds.AddBounds(stage->bounds);
	}
#else
	for(int i = 0; i < 1; i++)
	{
		stage = Alloc_rvBSEParticleStage();
		*stage = *instage;
		particle->stages.Append(stage);
		stage->cycleMsec = (stage->particleLife + stage->deadTime) * 1000;
		stage->totalParticles = 1;
		//stage->cycles = FXAction.restart;
		//stage->timeOffset = FXAction.delay;
		if(stage->orientation == POR_AIMED)
		{
			if(stage->distributionType == PDIST_RECT)
			{
				IF_ZERO_SET(stage->distributionParms[0], 8);
				IF_ZERO_SET(stage->distributionParms[1], 8);
				IF_ZERO_SET(stage->distributionParms[2], 8);
				IF_ZERO_SET(stage->speed.from, 150.0f);
				IF_ZERO_SET(stage->speed.to, 150.0f);
			}
		}
		particle->GetStageBounds(stage);
		particle->bounds.AddBounds(stage->bounds);
	}
	delete instage;
#endif

	if (particle->bounds.GetVolume() <= 0.1f) {
		particle->bounds = idBounds(vec3_origin).Expand(8.0f);
	}
}

bool rvDeclEffectParser::HasVecFlag(const char *name) const
{
	idStr str(name);
	str.ToLower();
	return vecFlags.FindIndex(str) != -1;
}

int rvDeclEffectParser::ParseVec(int *flagNum)
{
	idToken token;
	int i = 0;
	bool shouldReadNumber = true;
	vec.Clear();
	vecFlags.Clear();
	bool neg = false;
	float f;
	int m = 0;

	if(flagNum)
		*flagNum = 0;
	// read number
	while(1)
	{
		if (!src.ReadTokenOnLine(&token)) {
			break;
		}
		if(!idStr::Cmp(token, "}"))
			break;
		if(!idStr::Cmp(token, ","))
		{
			if(shouldReadNumber)
				break;
			shouldReadNumber = true;
			continue;
		}
		if(!idStr::Cmp(token, "-"))
		{
			neg = !neg;
			if(!shouldReadNumber)
				break;
			continue;
		}
		if(!shouldReadNumber)
			break;
		f = token.GetFloatValue();
		if(neg && f != 0.0f)
			f = -f;
		neg = false;
		vec.Append(f);
		i++;
		shouldReadNumber = false;
	}
	// read flag
	if(idStr::Cmp(token, "}"))
	{
		token.ToLower();
		vecFlags.Append(token);
		m++;
		while(1)
		{
			if (!src.ReadTokenOnLine(&token)) {
				break;
			}
			if(!idStr::Cmp(token, "}"))
				break;
			token.ToLower();
			vecFlags.Append(token);
			m++;
		}
		if(flagNum)
			*flagNum = m;
	}
	src.SkipRestOfLine();
	return i;
}

idVec3 rvDeclEffectParser::ParseTint(rvBSEParticleStage *stage, int n)
{
	idToken token;
	int count = 0;
	idVec3 p(0.0, 0.0, 0.0);

	if(!stage)
	{
		Skip();
		return p;
	}
	src.ExpectTokenString("{");
	src.ReadToken(&token);

	if(!idStr::Icmp(token, "point"))
	{
		count = ParsePoint(p);
	}
	else if(!idStr::Icmp(token, "line"))
	{
		idBounds b;
		b.Zero();
		count = ParseLine(b);
		p = (b[1] + b[0]) / 2.0;
	}
	else
	{
		LOGW_SKIP("Unknown tint property: %s -> %s", GetName(), token.c_str());
		src.SkipRestOfLine();
		return p;
	}

	if(count < 2)
		p[1] = 1.0;
	if(count < 3)
		p[2] = 1.0;
	p[0] = idMath::ClampFloat(0.0, 1.0, p[0]);
	p[1] = idMath::ClampFloat(0.0, 1.0, p[1]);
	p[2] = idMath::ClampFloat(0.0, 1.0, p[2]);
	if(n == STAGE_START)
	{
		stage->color[0] = p[0];
		stage->color[1] = p[1];
		stage->color[2] = p[2];
		stage->color[3] = 1.0;
		//stage->entityColor = false; // comment it, for line lighting
	}
#if 0
	else
	{
		stage->fadeColor[0] = p[0];
		stage->fadeColor[1] = p[1];
		stage->fadeColor[2] = p[2];
		stage->fadeColor[3] = 1.0;
	}
#endif
	return p;
}

void rvDeclEffectParser::ParseFade(rvBSEParticleStage *stage, int n)
{
	idToken token;
	int count;

	if(!stage)
	{
		Skip();
		return;
	}
	src.ExpectTokenString("{");
	src.ReadToken(&token);

	if(!idStr::Icmp(token, "point"))
	{
		idVec3 p(0.0f, 0.0f, 0.0f);
		ParsePoint(p);
		if(n == STAGE_START)
			stage->fadeInFraction = p[0];
		else
			stage->fadeOutFraction = p[0];
		stage->fadeOutFraction = p[0]; // TODO: always fade out
	}
	else if(!idStr::Icmp(token, "line"))
	{
		idBounds b;
		b.Zero();
		ParseLine(b);
		if(n == STAGE_START)
			stage->fadeInFraction = b[0][0];
		else
			stage->fadeOutFraction = b[0][0];
		stage->fadeOutFraction = b[0][0]; // TODO: always fade out
	}
	else
	{
		LOGW_SKIP("Unknown fade property: %s -> %s", GetName(), token.c_str());
		src.SkipRestOfLine();
	}
}

void rvDeclEffectParser::ParseVelocity(rvBSEParticleStage *stage, int n)
{
	idToken token;
	int count;

	if(!stage)
	{
		Skip();
		return;
	}
	src.ExpectTokenString("{");
	src.ReadToken(&token);

	if(!idStr::Icmp(token, "point"))
	{
		idVec3 p(0.0f, 0.0f, 0.0f);
		ParsePoint(p);
		if(n == STAGE_END)
			stage->speed.to = p.LengthFast();
		else
		{
			float v = p.LengthFast();
			stage->speed.from = v;
			if(stage->customPathType == PPATH_HELIX)
			{
				stage->customPathParms[3] = v;
				//stage->customPathParms[4] = v;
			}
		}
	}
	else if(!idStr::Icmp(token, "box"))
	{
		idBounds b;
		b.Zero();
		ParseBox(b);
		idVec3 p = b[1] - b[0];
		if(n == STAGE_END)
			stage->speed.to = p.LengthFast();
		else
		{
			float v = p.LengthFast();
			stage->speed.from = v;
			if(stage->customPathType == PPATH_HELIX)
			{
				stage->customPathParms[3] = v;
				//stage->customPathParms[4] = v;
			}
		}
	}
	else
	{
		LOGW_SKIP("Unknown velocity property: %s -> %s", GetName(), token.c_str());
		src.SkipRestOfLine();
	}
}

void rvDeclEffectParser::ParsePosition(rvBSEParticleStage *stage)
{
	idToken token;
	int count;

	if(!stage)
	{
		Skip();
		return;
	}
	src.ExpectTokenString("{");
	src.ReadToken(&token);

	if(!idStr::Icmp(token, "point"))
	{
		idVec3 p(0.0f, 0.0f, 0.0f);
		ParsePoint(p);
		stage->distributionType = PDIST_POINT;
	}
	else if(!idStr::Icmp(token, "box"))
	{
		idBounds b;
		b.Zero();
		ParseBox(b);
		stage->distributionParms[0] = b[1][0];
		stage->distributionParms[1] = b[1][1];
		stage->distributionParms[2] = b[1][2];
		stage->distributionType = PDIST_RECT;
	}
	else if(!idStr::Icmp(token, "line"))
	{
		idBounds b;
		b.Zero();
		ParseLine(b);
		stage->distributionParms[0] = b[1][0] - b[0][0];
		stage->distributionParms[1] = b[1][1] - b[0][1];
		stage->distributionParms[2] = b[1][2] - b[0][2];
		stage->distributionType = PDIST_RECT;
	}
	else if(!idStr::Icmp(token, "cylinder"))
	{
		idSphere s;
		s.Zero();
		ParseCylinder(s);
		const idVec3 &v = s.GetOrigin();
		stage->distributionParms[0] = v[0];
		stage->distributionParms[1] = v[1];
		stage->distributionParms[2] = v[2];
		stage->distributionParms[3] = s.GetRadius();
		stage->distributionType = PDIST_CYLINDER;
	}
	else if(!idStr::Icmp(token, "sphere"))
	{
		idSphere s;
		s.Zero();
		ParseSphere(s);
		const idVec3 &v = s.GetOrigin();
		stage->distributionParms[0] = v[0];
		stage->distributionParms[1] = v[1];
		stage->distributionParms[2] = v[2];
		stage->distributionParms[3] = s.GetRadius();
		stage->distributionType = PDIST_CYLINDER;
	}
	else if(!idStr::Icmp(token, "spiral"))
	{
		idBounds b;
		float i7 = 0.0;
		b.Zero();
		ParseSpiral(b, i7);
		stage->customPathType = PPATH_HELIX;
		idVec3 v = b.Size();
		stage->customPathParms[0] = v[0];
		stage->customPathParms[1] = v[1];
		stage->customPathParms[2] = v[2];
		stage->customPathParms[4] = i7;
	}
	else
	{
		LOGW_SKIP("Unknown position property: %s -> %s", GetName(), token.c_str());
		src.SkipRestOfLine();
	}
}

idVec3 rvDeclEffectParser::ParseOffset(rvBSEParticleStage *stage)
{
	idToken token;
	int count;
	idVec3 res(0.0f, 0.0f, 0.0f);

	src.ExpectTokenString("{");
	src.ReadToken(&token);

	if(!idStr::Icmp(token, "point"))
	{
		ParsePoint(res);
		stage->offset = res;
	}
	else if(!idStr::Icmp(token, "box"))
	{
		idBounds b;
		b.Zero();
		ParseBox(b);
		res = b[0];
		stage->offset = res;
	}
	else
	{
		LOGW_SKIP("Unknown offset property: %s -> %s", GetName(), token.c_str());
		src.SkipRestOfLine();
	}
	return res;
}

float rvDeclEffectParser::ParseLength(rvBSEParticleStage *stage, bool *useEndOrigin)
{
	idToken token;
	int count;
	float res = 0.0f;

	src.ExpectTokenString("{");
	src.ReadToken(&token);

	if(!idStr::Icmp(token, "point"))
	{
		idVec3 p(0.0f, 0.0f, 0.0f);
		ParsePoint(p);
		res = p[0];
	}
	else if(!idStr::Icmp(token, "line"))
	{
		idBounds b;
		b.Zero();
		ParseLine(b);
		res = b[0][0];
	}
	else if(!idStr::Icmp(token, "box"))
	{
		idBounds b;
		b.Zero();
		ParseBox(b);
		res = b[0][0];
	}
	else
	{
		LOGW_SKIP("Unknown length property: %s -> %s", GetName(), token.c_str());
		src.SkipRestOfLine();
	}

	if(useEndOrigin)
		*useEndOrigin = HasVecFlag("useEndOrigin");
	if(res < 0.0)
		res = -res;

	return res;
}

float rvDeclEffectParser::ParseSize(rvBSEParticleStage *stage, int n)
{
	idToken token;
	int count;
	float res = 0.0f;

	src.ExpectTokenString("{");
	src.ReadToken(&token);

	if(!idStr::Icmp(token, "point"))
	{
		idVec3 p(0.0f, 0.0f, 0.0f);
		ParsePoint(p);
		//res = p[0];
		res = p[0];
	}
	else if(!idStr::Icmp(token, "box"))
	{
		idBounds b;
		b.Zero();
		ParseBox(b);
		//res = b[0].LengthFast();
		//res = b[0][0];
		res = b[0][0];
	}
	else if(!idStr::Icmp(token, "line"))
	{
		idBounds b;
		b.Zero();
		ParseLine(b);
		//res = b[0].LengthFast();
		//res = b[0][0];
		res = b[0][0];
	}
	else
	{
		LOGW_SKIP("Unknown size property: %s -> %s", GetName(), token.c_str());
		src.SkipRestOfLine();
		return res;
	}

	if(stage)
	{
		if(n == STAGE_START)
			stage->size.from = res / 2.0;
		//else
			stage->size.to = res / 2.0;
	}
	return res;
}

float rvDeclEffectParser::ParseRotate(rvBSEParticleStage *stage, int n)
{
	idToken token;
	int count;
	float res = 0.0f;

	src.ExpectTokenString("{");
	src.ReadToken(&token);

	if(!idStr::Icmp(token, "point"))
	{
		idVec3 p(0.0f, 0.0f, 0.0f);
		ParsePoint(p);
		res = p[0];
	}
	else if(!idStr::Icmp(token, "box"))
	{
		idBounds b;
		b.Zero();
		ParseBox(b);
		res = b[0].LengthFast();
	}
	else if(!idStr::Icmp(token, "line"))
	{
		idBounds b;
		b.Zero();
		ParseLine(b);
		res = b[0].LengthFast();
	}
	else
	{
		LOGW_SKIP("Unknown rotate property: %s -> %s", GetName(), token.c_str());
		src.SkipRestOfLine();
		return res;
	}

	res *= 360.0f;
	if(stage)
	{
		if(n == STAGE_START)
			stage->rotationSpeed.from = res;
		//else
			stage->rotationSpeed.to = res;
	}
	return res;
}

void rvDeclEffectParser::ParseStage(rvFXSingleAction &FXAction, rvBSEParticleStage *stage, int n)
{
	idToken token;

	src.ExpectTokenString("{");

	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Icmp("size")) {
			FXAction.size = ParseSize(stage, n);
			continue;
		}

		if (!token.Icmp("rotate")) {
			FXAction.rotate = ParseRotate(stage, n);
			continue;
		}

		if (!token.Icmp("position")) {
			ParsePosition(stage);
			continue;
		}

		if (!token.Icmp("velocity")) {
			ParseVelocity(stage, n);
			continue;
		}

		if (!token.Icmp("fade")) {
			ParseFade(stage, n);
			continue;
		}

		if (!token.Icmp("offset")) {
			FXAction.offset = ParseOffset(stage);
			continue;
		}

		if (!token.Icmp("length")) {
			FXAction.length = ParseLength(stage, &FXAction.useEndOrigin);
			continue;
		}

		if (!token.Icmp("tint")) {
			ParseTint(stage, n);
			continue;
		}

		SKIP_UNKNOWN_TOKEN("acceleration");
		SKIP_UNKNOWN_TOKEN("angle");
		SKIP_UNKNOWN_TOKEN("friction");

		LOGW_SKIP("Skip stage: %s -> %s", GetName(), token.c_str());
		Skip();
	}
	if(stage && stage->customPathType == PPATH_HELIX)
	{
		FXAction.useEndOrigin = false;
	}
}

void rvDeclEffectParser::Skip(void)
{
	idToken token;

	if(!src.ReadTokenOnLine(&token))
		return;
	if(!idStr::Cmp(token, "{"))
		src.SkipBracedSection(false);
	else
		src.SkipRestOfLine();
}

void rvDeclEffectParser::ParseLinked(rvFXSingleAction &FXAction)
{
	ParseLine(FXAction);
	FXAction.ptype = PTYPE_LINKED;
	//FXAction.trackOrigin = true;
}

void rvDeclEffectParser::ParseElectricity(rvFXSingleAction &FXAction)
{
	ParseLine(FXAction);
	FXAction.ptype = PTYPE_ELECTRICITY;
}

void rvDeclEffectParser::ParseLine(rvFXSingleAction &FXAction)
{
	idToken token;
	rvFXSingleAction action;
	Init_FXAction(action);
	rvBSEParticleStage *stage = NULL;
	rvBSEParticle *particle;
	idStr name = idStr("effects/line/") + GetName() + "/" + FXAction.name + va("_%d", ++genParticleStage);
	idStr fileName = "particle/" + name + ".prt";

	particle = new rvBSEParticle;
	particle->depthHack = 0.0f;
	FXAction.ptype = PTYPE_LINE;
	stage = Alloc_rvBSEParticleStage();
	stage->rvptype = PTYPE_LINE;

	src.ExpectTokenString("{");

	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Icmp("duration")) {
			int count = ParseVec();
			stage->particleLife = vec[0];
			FXAction.duration = vec[0];
			continue;
		}

		if (!token.Icmp("gravity")) {
			ParseVec();
			stage->gravity = vec[0];
			continue;
		}

		if (!token.Icmp("material")) {
			src.ReadToken(&token);
			stage->material = declManager->FindMaterial(token.c_str());
			continue;
		}

		if (!token.Icmp("start")) {
			ParseStage(action, stage, STAGE_START);
			stage->customPathParms[5] = action.size;
			stage->customPathParms[6] = action.useEndOrigin ? 0.0f : 1.0f;
			stage->customPathParms[7] = action.length;
			continue;
		}

		if (!token.Icmp("motion")) {
			src.SkipBracedSection(true);
			continue;
		}

		if (!token.Icmp("end")) {
			ParseStage(action, stage, STAGE_END);
			//stage->customPathParms[5] = action.size;
			if(stage->customPathParms[7] == 0.0 && action.length != 0.0 && action.length < stage->customPathParms[7])
				stage->customPathParms[7] = action.length;
			continue;
		}

		if (!token.Icmp("impact")) {
			src.SkipBracedSection(true);
			continue;
		}

		if (!token.Icmp("persist")) {
			FXAction.trackOrigin = true;
			//FXAction.restart = 1.0f;
			continue;
		}

		SKIP_UNKNOWN_TOKEN("jitterRate");
		SKIP_UNKNOWN_TOKEN("jitterTable");
		SKIP_UNKNOWN_TOKEN("jitterSize");
		SKIP_UNKNOWN_TOKEN("fork");
		SKIP_UNKNOWN_TOKEN("generatedOriginNormal");
		SKIP_UNKNOWN_TOKEN("blend");
		SKIP_UNKNOWN_TOKEN("generatedLine");
		SKIP_UNKNOWN_TOKEN("flipNormal");
		SKIP_UNKNOWN_TOKEN("trailType");
		SKIP_UNKNOWN_TOKEN("trailTime");
		SKIP_UNKNOWN_TOKEN("trailCount");
		SKIP_UNKNOWN_TOKEN("trailMaterial");
		SKIP_UNKNOWN_TOKEN("tiling");
		SKIP_UNKNOWN_TOKEN("generatedNormal");

		LOGW_SKIP("Skip line: %s -> %s", GetName(), token.c_str());
		Skip();
	}

	FinishDeclParticle(particle, stage, FXAction);

	FXAction.data = name + BSE_MODEL_SUFFIX;
	FXAction.type = FX_PARTICLE;
	AppendDeclParticle(FXAction.data, particle);
}

void rvDeclEffectParser::ParseOriented(rvFXSingleAction &FXAction)
{
	idToken token;
	rvFXSingleAction action;
	Init_FXAction(action);
	rvBSEParticleStage *stage = NULL;
	rvBSEParticle *particle;
	idStr name = idStr("effects/oriented/") + GetName() + "/" + FXAction.name + va("_%d", ++genParticleStage);
	idStr fileName = "particle/" + name + ".prt";

	particle = new rvBSEParticle;
	FXAction.ptype = PTYPE_ORIENTED;
	stage = Alloc_rvBSEParticleStage();
	stage->orientation = POR_DIR;

	src.ExpectTokenString("{");

	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Icmp("duration")) {
			int count = ParseVec();
			stage->particleLife = vec[0];
			FXAction.duration = vec[0];
			continue;
		}

		if (!token.Icmp("gravity")) {
			ParseVec();
			stage->gravity = vec[0];
			continue;
		}

		if (!token.Icmp("material")) {
			src.ReadToken(&token);
			stage->material = declManager->FindMaterial(token.c_str());

			// precache the light material
			declManager->FindMaterial(action.data);
			continue;
		}

		if (!token.Icmp("start")) {
			ParseStage(action, stage, STAGE_START);
			stage->offset = action.offset;
			stage->orientationParms[0] = action.position[0] ? action.position[0] : 1.0;
			stage->orientationParms[1] = action.position[1] ? action.position[1] : 1.0;
			stage->orientationParms[2] = action.position[2];
			continue;
		}

		if (!token.Icmp("motion")) {
			src.SkipBracedSection(true);
			continue;
		}

		if (!token.Icmp("end")) {
			ParseStage(action, stage, STAGE_END);
			continue;
		}

		if (!token.Icmp("impact")) {
			src.SkipBracedSection(true);
			continue;
		}

		if(ParseTrailParms(token, stage))
			continue;

		if (!token.Icmp("persist")) {
			FXAction.trackOrigin = true;
			//FXAction.restart = 1.0f;
			continue;
		}

		SKIP_UNKNOWN_TOKEN("blend");
		SKIP_UNKNOWN_TOKEN("generatedNormal");
		SKIP_UNKNOWN_TOKEN("generatedOriginNormal");
		SKIP_UNKNOWN_TOKEN("generatedNormal");
		SKIP_UNKNOWN_TOKEN("flipNormal");

		LOGW_SKIP("Skip oriented: %s -> %s", GetName(), token.c_str());
		Skip();
	}

	FinishDeclParticle(particle, stage, FXAction);

	FXAction.data = name + BSE_MODEL_SUFFIX;
	FXAction.type = FX_PARTICLE;
	AppendDeclParticle(FXAction.data, particle);
}

void rvDeclEffectParser::ParseSprite(rvFXSingleAction &FXAction)
{
	idToken token;
	rvFXSingleAction action;
	Init_FXAction(action);
	rvBSEParticleStage *stage = NULL;
	rvBSEParticle *particle;
	idStr name = idStr("effects/sprite/") + GetName() + "/" + FXAction.name + va("_%d", ++genParticleStage);
	idStr fileName = "particles/" + name + ".prt";

	particle = new rvBSEParticle;
	FXAction.ptype = PTYPE_SPRITE;
	stage = Alloc_rvBSEParticleStage();

	src.ExpectTokenString("{");

	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Icmp("duration")) {
			int count = ParseVec();
			stage->particleLife = vec[0];
			FXAction.duration = vec[0];
			continue;
		}

		if (!token.Icmp("gravity")) {
			ParseVec();
			stage->gravity = vec[0];
			continue;
		}

		if (!token.Icmp("material")) {
			src.ReadToken(&token);
			stage->material = declManager->FindMaterial(token.c_str());
			continue;
		}

		if (!token.Icmp("start")) {
			ParseStage(action, stage, STAGE_START);
			stage->offset = action.offset;
			continue;
		}

		if (!token.Icmp("motion")) {
			src.SkipBracedSection(true);
			continue;
		}

		if (!token.Icmp("end")) {
			ParseStage(action, stage, STAGE_END);
			continue;
		}

		if (!token.Icmp("impact")) {
			src.SkipBracedSection(true);
			continue;
		}

		if(ParseTrailParms(token, stage))
			continue;

		if (!token.Icmp("persist")) {
			FXAction.trackOrigin = true;
			//FXAction.restart = 1.0f;
			continue;
		}

		SKIP_UNKNOWN_TOKEN("blend");
		SKIP_UNKNOWN_TOKEN("generatedNormal");
		SKIP_UNKNOWN_TOKEN("generatedOriginNormal");
		SKIP_UNKNOWN_TOKEN("generatedNormal");
		SKIP_UNKNOWN_TOKEN("flipNormal");

		LOGW_SKIP("Skip sprite: %s -> %s", GetName(), token.c_str());
		Skip();
	}

	FinishDeclParticle(particle, stage, FXAction);

	FXAction.data = name + BSE_MODEL_SUFFIX;
	FXAction.type = FX_PARTICLE;
	AppendDeclParticle(FXAction.data, particle);
}

void rvDeclEffectParser::ParseInnerEffect(const char *name)
{
	const rvDeclEffect *refEffect = declManager->FindEffect(name, false);
	if(!refEffect)
		return;
	return; // TODO
}

void rvDeclEffectParser::ParseEffect(void)
{
	idToken token;

	src.ExpectTokenString("{");
	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Icmp("effect")) {
			src.ReadToken(&token);
			ParseInnerEffect(token);
			continue;
		}

		if (!token.Icmp("start")) {
			Skip();
			continue;
		}

		LOGW_SKIP("Skip effect: %s -> %s", GetName(), token.c_str());
		Skip();
	}
}

void rvDeclEffectParser::ParseDebris(rvFXSingleAction &FXAction)
{
	FXAction.ptype = PTYPE_DEBRIS;
	src.SkipBracedSection(true);
}

void rvDeclEffectParser::ParseDelay(rvFXSingleAction &FXAction)
{
	FXAction.seg = SEG_DELAY;
	Skip();
}

rvBSEParticleStage * rvDeclEffectParser::Alloc_rvBSEParticleStage(void)
{
	rvBSEParticleStage *stage = new rvBSEParticleStage;
	stage->Default();
	stage->worldGravity = true;
	stage->entityColor = true;
	stage->gravity = 0.0f;
	stage->spawnBunching = 0.0f;
	stage->fadeInFraction = 0.0f;
	stage->fadeOutFraction = 0.0f;
	stage->rvptype = PTYPE_NONE;
	stage->distributionType = PDIST_POINT;
	stage->distributionParms[0] = 0;
	stage->distributionParms[1] = 0;
	stage->distributionParms[2] = 0;
	stage->distributionParms[3] = 0;
	stage->speed.from = 0;
	stage->speed.to = 0;
	stage->deadTime = 0;
	return stage;
}

void rvDeclEffectParser::ParseEmitter(rvFXSingleAction &FXAction)
{
	ParseSpawner(FXAction);
	FXAction.restart = 1.0f;
	FXAction.seg = SEG_EMITTER;
}

void rvDeclEffectParser::ParseSpawner(rvFXSingleAction &FXAction)
{
	idToken token;
	int count = 0;

	FXAction.seg = SEG_SPAWNER;
	src.ExpectTokenString("{");

	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}
		if(ParseGeneralParms(token, FXAction))
			continue;

		if (!token.Icmp("sprite")) { // as particle
			ParseSprite(FXAction);
			continue;
		}
		if (!token.Icmp("model")) {
			ParseModel(FXAction);
			continue;
		}
		if (!token.Icmp("line")) { // new
			ParseLine(FXAction);
			continue;
		}
		if (!token.Icmp("oriented")) {
			ParseOriented(FXAction);
			continue;
		}
		if (!token.Icmp("linked")) {
			ParseLinked(FXAction);
			continue;
		}
		if (!token.Icmp("electricity")) {
			ParseElectricity(FXAction);
			continue;
		}
		if (!token.Icmp("debris")) {
			ParseDebris(FXAction);
			continue;
		}

		SKIP_UNKNOWN_TOKEN("detail");
		SKIP_UNKNOWN_TOKEN("attenuateEmitter");
		SKIP_UNKNOWN_TOKEN("density");
		SKIP_UNKNOWN_TOKEN("particleCap");

		LOGW_SKIP("Skip spawner: %s -> %s", GetName(), token.c_str());
		Skip();
	}
}

void rvDeclEffectParser::ParseShake(rvFXSingleAction &FXAction)
{
	idToken token;

	src.ExpectTokenString("{");
	FXAction.type = FX_SHAKE;
	FXAction.seg = SEG_SHAKE;
	FXAction.data = "shake";
	FXAction.shakeAmplitude = 1.0f;
	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}
		if(ParseGeneralParms(token, FXAction))
			continue;

		if (!token.Icmp("attenuateEmitter")) {
			continue;
		}

		if (!token.Icmp("scale")) {
			ParseVec();
			FXAction.shakeAmplitude = vec[0];
			continue;
		}

		if (!token.Icmp("attenuation")) {
			ParseVec();
			FXAction.shakeDistance = vec[1] - vec[0];
			continue;
		}

		LOGW_SKIP("Skip shake: %s -> %s", GetName(), token.c_str());
		Skip();
	}
}

void rvDeclEffectParser::ParseDoubleVision(rvFXSingleAction &FXAction)
{
	idToken token;

	src.ExpectTokenString("{");
	FXAction.type = FX_SHAKE;
	FXAction.seg = SEG_SHAKE;
	FXAction.data = "doubleVision";
	FXAction.shakeAmplitude = 1.0f;
	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}
		if(ParseGeneralParms(token, FXAction))
			continue;

		if (!token.Icmp("attenuateEmitter")) {
			continue;
		}

		if (!token.Icmp("scale")) {
			ParseVec();
			FXAction.shakeAmplitude = vec[0];
			continue;
		}

		if (!token.Icmp("attenuation")) {
			ParseVec();
			FXAction.shakeDistance = vec[1] - vec[0];
			continue;
		}

		if (!token.Icmp("duration")) {
			ParseVec();
			FXAction.duration = vec[1] - vec[0];
			continue;
		}

		LOGW_SKIP("Skip doubleVision: %s -> %s", GetName(), token.c_str());
		Skip();
	}
}

void rvDeclEffectParser::ParseSound(rvFXSingleAction &FXAction)
{
	idToken token;

	src.ExpectTokenString("{");
	FXAction.type = FX_SOUND;
	FXAction.seg = SEG_SOUND;
	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Icmp("soundShader")) {
			src.ReadToken(&token);
			FXAction.data = token;

			// precache it
			declManager->FindSound(FXAction.data);
			continue;
		}

		if (!token.Icmp("volume")) { // f|f,f
			src.SkipRestOfLine();
			continue;
		}

		LOGW_SKIP("Skip sound: %s -> %s", GetName(), token.c_str());
		Skip();
	}
}

void rvDeclEffectParser::ParseInnerDecal(rvFXSingleAction &FXAction)
{
	idToken token;

	src.ExpectTokenString("{");
	FXAction.type = FX_DECAL;
	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Icmp("duration")) {
			ParseVec();
			FXAction.duration = vec[0];
			continue;
		}

		if (!token.Icmp("material")) {
			src.ReadToken(&token);
			FXAction.data = token;

			// precache the light material
			declManager->FindMaterial(FXAction.data);
			continue;
		}

		if (!token.Icmp("start")) {
			ParseStage(FXAction, 0, STAGE_START);
			continue;
		}

		if (!token.Icmp("motion")) {
			src.SkipBracedSection(true);
			continue;
		}

		if (!token.Icmp("end")) {
			src.SkipBracedSection(true);
			continue;
		}

		LOGW_SKIP("Skip decal::decal: %s -> %s", GetName(), token.c_str());
		Skip();
	}
}

void rvDeclEffectParser::ParseDecal(rvFXSingleAction &FXAction)
{
	idToken token;

	src.ExpectTokenString("{");
	FXAction.type = FX_DECAL;
	FXAction.seg = SEG_DECAL;
	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Icmp("decal")) {
			ParseInnerDecal(FXAction);
			continue;
		}

		LOGW_SKIP("Skip decal: %s -> %s", GetName(), token.c_str());
		Skip();
	}
}

void rvDeclEffectParser::ParseInnerLight(rvFXSingleAction &FXAction)
{
	idToken token;

	src.ExpectTokenString("{");
	FXAction.type = FX_LIGHT;
	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Icmp("duration")) {
			ParseVec();
			FXAction.duration = vec[0];
			continue;
		}

		if (!token.Icmp("material")) {
			src.ReadToken(&token);
			FXAction.data = token;

			// precache the light material
			declManager->FindMaterial(FXAction.data);
			continue;
		}

		if (!token.Icmp("shadows")) {
			FXAction.noshadows = false;
			continue;
		}

		if (!token.Icmp("start")) {
			ParseStage(FXAction, 0, STAGE_START);
			FXAction.lightRadius = FXAction.size;
			continue;
		}

		if (!token.Icmp("motion")) {
			src.SkipBracedSection(true);
			continue;
		}

		if (!token.Icmp("end")) {
			rvFXSingleAction action;
			Init_FXAction(action);
			ParseStage(action, 0, STAGE_END);
			if(FXAction.size < action.size)
			{
				FXAction.lightRadius = FXAction.size = action.size;
			}
			continue;
		}

		SKIP_UNKNOWN_TOKEN("flipNormal");
		SKIP_UNKNOWN_TOKEN("blend");
		SKIP_UNKNOWN_TOKEN("specular");

		LOGW_SKIP("Skip light::light: %s -> %s", GetName(), token.c_str());
		Skip();
	}
}

void rvDeclEffectParser::ParseLight(rvFXSingleAction &FXAction)
{
	idToken token;

	src.ExpectTokenString("{");
	FXAction.type = FX_LIGHT;
	FXAction.seg = SEG_LIGHT;
	FXAction.noshadows = true;
	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}
		if(ParseGeneralParms(token, FXAction))
			continue;

		if (!token.Icmp("light")) {
			ParseInnerLight(FXAction);
			continue;
		}

		SKIP_UNKNOWN_TOKEN("detail");

		LOGW_SKIP("Skip light: %s -> %s", GetName(), token.c_str());
		Skip();
	}
}

idStr rvDeclEffectParser::BuildParticleSource(rvBSEParticle *particle, const char *name)
{
	idFile_Memory f;

	f.WriteFloatString("particle %s {\n", name);

	if (particle->depthHack) {
		f.WriteFloatString("\tdepthHack\t%f\n", particle->depthHack);
	}

	for (int i = 0; i < particle->stages.Num(); i++) {
		particle->WriteStage(&f, particle->stages[i]);
	}

	f.WriteFloatString("}");

	const char *text = f.GetDataPtr();

	idStr str;
	str.Append(text, idStr::Length(text));
	return str;
}

void rvDeclEffectParser::ParseTrail(rvFXSingleAction &FXAction)
{
	ParseSpawner(FXAction);
	FXAction.seg = SEG_TRAIL;
	rvBSEParticle *particle = BSE_GetDeclParticle(FXAction.data.c_str());
	if(particle && particle->stages.Num() > 0)
	{
		rvBSEParticleStage *stage = particle->stages[0];
		if(stage)
		{
			stage->orientation = POR_AIMED;
			stage->orientationParms[0] = FXAction.count;
			stage->orientationParms[1] = FXAction.duration;
			stage->distributionType = PDIST_RECT;
		}
	}
}

void rvDeclEffectParser::ParseModel(rvFXSingleAction &FXAction)
{
	idToken token;
	rvFXSingleAction action;
	Init_FXAction(action);
	rvBSEParticleStage *stage = NULL;
	rvBSEParticle *particle;
	idStr name = idStr("effects/model/") + GetName() + "/" + FXAction.name + va("_%d", ++genParticleStage);
	idStr fileName = "particles/" + name + ".prt";

	particle = new rvBSEParticle;
	FXAction.ptype = PTYPE_MODEL;
	stage = Alloc_rvBSEParticleStage();
	stage->rvptype = PTYPE_MODEL;

	src.ExpectTokenString("{");

	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (!token.Icmp("duration")) {
			int count = ParseVec();
			stage->particleLife = vec[0];
			FXAction.duration = vec[0];
			continue;
		}

		if (!token.Icmp("gravity")) {
			ParseVec();
			stage->gravity = vec[0];
			continue;
		}

		if (!token.Icmp("material")) {
			src.ReadToken(&token);
			stage->material = declManager->FindMaterial(token.c_str());
			continue;
		}

		if (!token.Icmp("model")) {
			src.ReadToken(&token);
			stage->model = renderModelManager->FindModel(token.c_str());
			continue;
		}

		if (!token.Icmp("start")) {
			ParseStage(action, stage, STAGE_START);
			stage->offset = action.offset;
			continue;
		}

		if (!token.Icmp("motion")) {
			src.SkipBracedSection(true);
			continue;
		}

		if (!token.Icmp("end")) {
			ParseStage(action, stage, STAGE_END);
			continue;
		}

		if (!token.Icmp("impact")) {
			src.SkipBracedSection(true);
			continue;
		}

		if (!token.Icmp("trailCount")) {
			ParseVec();
			//stage->orientation = POR_AIMED;
			//stage->orientationParms[0] = vec[0];
			continue;
		}

		if (!token.Icmp("trailTime")) {
			ParseVec();
			//stage->orientation = POR_AIMED;
			//stage->orientationParms[1] = vec[0];
			continue;
		}

		if (!token.Icmp("persist")) {
			FXAction.trackOrigin = true;
			//FXAction.restart = 1.0f;
			continue;
		}

		SKIP_UNKNOWN_TOKEN("blend");
		SKIP_UNKNOWN_TOKEN("generatedNormal");
		SKIP_UNKNOWN_TOKEN("generatedOriginNormal");
		SKIP_UNKNOWN_TOKEN("generatedNormal");
		SKIP_UNKNOWN_TOKEN("flipNormal");
		SKIP_UNKNOWN_TOKEN("trailType");
		SKIP_UNKNOWN_TOKEN("trailMaterial");

		LOGW_SKIP("Skip model: %s -> %s", GetName(), token.c_str());
		Skip();
	}

	FinishDeclParticle(particle, stage, FXAction);

	FXAction.data = name + BSE_MODEL_SUFFIX;
	FXAction.type = FX_PARTICLE;
	AppendDeclParticle(FXAction.data, particle);
}

void rvDeclEffectParser::Init_FXAction(rvFXSingleAction &FXAction)
{
	FXAction.type = -1;
	FXAction.sibling = -1;

	FXAction.data = "<none>";
	FXAction.name = "<none>";
	FXAction.fire = "<none>";

	FXAction.delay = 0.0f;
	FXAction.duration = 1.0f;
	FXAction.restart = 0.0f;
	FXAction.size = 0.0f;
	FXAction.fadeInTime = 0.0f;
	FXAction.fadeOutTime = 0.0f;
	FXAction.shakeTime = 0.0f;
	FXAction.shakeAmplitude = 0.0f;
	FXAction.shakeDistance = 0.0f;
	FXAction.shakeFalloff = false;
	FXAction.shakeImpulse = 0.0f;
	FXAction.shakeIgnoreMaster = false;
	FXAction.lightRadius = 0.0f;
	FXAction.rotate = 0.0f;
	FXAction.random1 = 0.0f;
	FXAction.random2 = 0.0f;

	FXAction.lightColor = vec3_origin;
	FXAction.offset = vec3_origin;
	FXAction.axis = mat3_identity;

	FXAction.bindParticles = false;
	FXAction.explicitAxis = false;
	FXAction.noshadows = false;
	FXAction.particleTrackVelocity = true;
	FXAction.trackOrigin = false;
	FXAction.soundStarted = false;

	FXAction.seg = SEG_NONE;
	FXAction.ptype = PTYPE_NONE;
	FXAction.position = vec3_origin;
	FXAction.count = 0;
	FXAction.useEndOrigin = false;
	FXAction.length = 0.0f;

	//FXAction.trackOrigin = true;
	FXAction.lightColor.Set(1.0f, 1.0f, 1.0f);
}

bool rvDeclEffectParser::Parse(void)
{
	idToken token;

	src.SkipUntilString("{");

	BSE_DEBUG("Parse effect file: %s %p", GetName(), this);
	// scan through, identifying each individual parameter
	decl->mFlags = 0;
	while (1) {

		if (!src.ReadToken(&token)) {
			break;
		}

		if (token == "}") {
			break;
		}

		if (!token.Icmp("size")) {
			src.ReadToken(&token);
			continue;
		}

		if (!token.Icmp("spawner")) {
			src.ReadToken(&token);
			rvFXSingleAction action;
			Init_FXAction(action);
			action.name = token;
			ParseSpawner(action);
			decl->events.Append(action);
			continue;
		}

		if (!token.Icmp("emitter")) {
			src.ReadToken(&token);
			rvFXSingleAction action;
			Init_FXAction(action);
			action.name = token;
			ParseEmitter(action);
			decl->events.Append(action);
			continue;
		}

		if (!token.Icmp("decal")) {
			src.ReadToken(&token);
			rvFXSingleAction action;
			Init_FXAction(action);
			action.name = token;
			ParseDecal(action);
			decl->events.Append(action);
			continue;
		}

		if (!token.Icmp("sound")) {
			src.ReadToken(&token);
			rvFXSingleAction action;
			Init_FXAction(action);
			action.name = token;
			ParseSound(action);
			decl->events.Append(action);
			decl->mFlags |= ETFLAG_HAS_SOUND;
			continue;
		}

		if (!token.Icmp("light")) {
			src.ReadToken(&token);
			rvFXSingleAction action;
			Init_FXAction(action);
			action.name = token;
			ParseLight(action);
			decl->events.Append(action);
			continue;
		}

		if (!token.Icmp("shake")) {
			src.ReadToken(&token);
			rvFXSingleAction action;
			Init_FXAction(action);
			action.name = token;
			ParseShake(action);
			decl->events.Append(action);
			continue;
		}

		if (!token.Icmp("doubleVision")) {
			src.ReadToken(&token);
			rvFXSingleAction action;
			Init_FXAction(action);
			action.name = token;
			ParseDoubleVision(action);
			decl->events.Append(action);
			continue;
		}

		if (!token.Icmp("delay")) {
			src.ReadToken(&token);
			src.SkipBracedSection(true);
			continue;
		}
		if (!token.Icmp("trail")) {
			src.ReadToken(&token);
			rvFXSingleAction action;
			Init_FXAction(action);
			action.name = token;
			ParseTrail(action);
			decl->events.Append(action);
			continue;
		}
		if (!token.Icmp("effect")) {
			src.ReadToken(&token);
			src.SkipBracedSection(true);
			continue;
		}

		src.Warning("FX decl '%s' had a parse error -> %s", GetName(), token.c_str());
		Skip();
	}
	//decl->Print();

	return true;
}
