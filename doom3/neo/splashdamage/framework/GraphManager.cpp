// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "GraphManager.h"

class sdGraphManagerLocal : public sdGraphManager {
public:
	sdGraphManagerLocal();
	virtual					~sdGraphManagerLocal();

	virtual void			Init();
	virtual void			Shutdown();

	virtual sdGraph*		AllocGraph( const char* name );
	virtual sdGraph*		FindGraph( const char* name );
	virtual void			FreeGraph( const char* name );

	virtual void			Draw();
};

static sdGraphManagerLocal graphManagerLocal;

sdGraphManager* graphManager = &graphManagerLocal;

sdGraph::sdGraph() {
}

void sdGraph::AddSample( const char* name, double value, double rangeMin, double rangeMax, const idVec4& color ) {
}

void sdGraph::SetMaxSamples( int max ) {
}

void sdGraph::Draw( int x, int y, int w, int h ) {
}

void sdGraph::Clear() {
}




sdGraphManagerLocal::sdGraphManagerLocal() {
}

sdGraphManagerLocal::~sdGraphManagerLocal() {
}

void sdGraphManagerLocal::Init() {
}

void sdGraphManagerLocal::Shutdown() {
}

sdGraph* sdGraphManagerLocal::AllocGraph( const char* name ) {
	return NULL;
}

sdGraph* sdGraphManagerLocal::FindGraph( const char* name ) {
	return NULL;
}

void sdGraphManagerLocal::FreeGraph( const char* name ) {
}

void sdGraphManagerLocal::Draw() {
}
