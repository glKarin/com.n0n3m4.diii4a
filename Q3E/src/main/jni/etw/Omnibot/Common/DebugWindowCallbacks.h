
#ifndef __DEBUGWINDOWCALLBACKS_H__
#define __DEBUGWINDOWCALLBACKS_H__

namespace ScriptConsoleCallbacks
{
	void OnAutoCompleteSelectionChanged(const char *_str);
	void OnScriptInputChanged(const char *_str);
	void OnScriptInputEntered(const char *_str);
	void OnScriptInputCycleHistory(bool _forward);
	void OnPlayerSelectionChanged(const int _gameId);
};

#include <FL/Fl_Group.H>
#include <FLU/Flu_Tree_Browser.h>

class ProfilerGroup : public Fl_Group
{
public:

	static ProfilerGroup *s_instance;

	ProfilerGroup(int X,int Y,int W,int H,const char *l = 0);
	~ProfilerGroup();
protected:
	void draw();

private:

	int handle(int evt);
	int handle_key();

};

class BehaviorTree : public Flu_Tree_Browser
{
public:

	void Refresh();

	BehaviorTree(int X,int Y,int W,int H,const char *l = 0);
	~BehaviorTree();
protected:
	void draw();
private:
};

class BehaviorTreeInfo : public Flu_Tree_Browser
{
public:

	void Init();
	void Refresh();

	BehaviorTreeInfo(int X,int Y,int W,int H,const char *l = 0);
	~BehaviorTreeInfo();
protected:
	void draw();
private:
};

class MapViewport : public Fl_Group
{
public:

	Vector3f FromWorld(const AABB &_world, const Vector3f &_worldpos);

	MapViewport(int X,int Y,int W,int H,const char *l = 0);
	~MapViewport();
protected:
	void draw();
private:
};

#endif
