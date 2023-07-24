// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GRAPH_MANAGER_H__
#define __GRAPH_MANAGER_H__

/*
=====================================

sdGraphManager
This is still WIP
A basic class to manage named graphs
Multiple data sets can be managed in a single graph

TODO:
+Graph layout on screen
+Expose a way to adjust graph size
+More efficient drawing than using deviceContext
+Labeled axes

=====================================
*/

class sdGraph;
class sdGraphManager {
public:
	virtual					~sdGraphManager() {}

	virtual void			Init() = 0;
	virtual void			Shutdown() = 0;

	virtual sdGraph*		AllocGraph( const char* name ) = 0;
	virtual sdGraph*		FindGraph( const char* name ) = 0;
	virtual void			FreeGraph( const char* name ) = 0;

	virtual void			Draw() = 0;
};

extern sdGraphManager* graphManager;

class sdGraph {
public:
							sdGraph();

	virtual void			AddSample( const char* name, double value, double rangeMin, double rangeMax, const idVec4& color );
	virtual void			SetMaxSamples( int max );
	virtual void			Draw( int x, int y, int w, int h );
	virtual void			Clear();

private:
	struct category_t {
		category_t() {
			samples.Fill( 800, 0.0 );
			minValue	= idMath::INFINITY;
			maxValue	= -idMath::INFINITY;
			insertPoint = 0;
		}
		idVec4				color;
		idList< double >	samples;
		int					insertPoint;
		double				minValue;
		double				maxValue;
		double				rangeMin;
		double				rangeMax;
	};

	typedef sdHashMapGeneric< idStr, category_t > categories_t;
	categories_t			categories;
};

class sdGraphMS_Scoped {
public:
	sdGraphMS_Scoped( const char* name, const char* sample, double minRange = 0.0, double maxRange = 100.0, const idVec4& color = colorGreen, bool active = true ) :
		name( name ),
		sample( sample ),
		minRange( minRange ),
		maxRange( maxRange ),
		color( color ),
		active( active ) {
		if( active ) {
			profile.Start();
		}
	}

	~sdGraphMS_Scoped() {
		if( !active ) {
			return;
		}
		profile.Stop();
		if( sdGraph* graph = graphManager->FindGraph( name.c_str() ) ) {
			graph->AddSample( sample.c_str(), profile.Milliseconds(), minRange, maxRange, color );
		}
	}

private:
	idTimer			profile;
	const idStr		name;
	const idStr		sample;
	const idVec4	color;
	const double	minRange;
	const double	maxRange;
	const bool		active;
};

#endif // __GRAPH_MANAGER_H__
