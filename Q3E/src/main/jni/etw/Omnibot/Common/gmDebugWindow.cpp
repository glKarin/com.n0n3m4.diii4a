#include "PrecompCommon.h"

#ifdef ENABLE_DEBUG_WINDOW

#include <guichan.hpp>
#include "gmbinder2.h"

//////////////////////////////////////////////////////////////////////////
#define GETMACHINE() ScriptManager::GetInstance()->GetMachine()
//////////////////////////////////////////////////////////////////////////

class DialogPanel : public gcn::contrib::AdjustingContainer, public gcn::ActionListener
{
public:

	void AddOption(const std::string &str, const std::string &act)
	{
		gcn::Button *btn = new gcn::Button(str);
		btn->setActionEventId(act);
		btn->addActionListener(this);
		btn->setWidth(mTextScrollArea->getWidth());
		btn->setAlignment(gcn::Graphics::LEFT);
		btn->setHeight(btn->getFont()->getHeight()*3);
		add(btn);
		setColumnAlignment(0,gcn::contrib::AdjustingContainer::CENTER);
		adjustSize();
	}

	void action(const gcn::ActionEvent& actionEvent)
	{
		/*gmMachine *pM = ScriptManager::GetInstance()->GetMachine();

		gmCall call;
		if(mActionCallback && call.BeginFunction(pM,mActionCallback,gmVariable(mThisObject)))
		{
			gmGCRoot<gmUserObject> r =
				gmBind2::Class<gcn::Widget>::WrapObject(pM, actionEvent.getSource(), true);

			call.AddParamUser(r.GetUserObject());
			call.AddParamString (actionEvent.getId().c_str());
			call.End();
		}*/
	}

	DialogPanel(const std::string &str)
	{
		mTextBox = new gcn::TextBox(str);
		mTextBox->setEditable(false);
		mTextScrollArea = new gcn::ScrollArea(mTextBox);
		mTextScrollArea->setSize(300, 200);
		add(mTextScrollArea);
	}
private:
	gcn::ScrollArea               *mTextScrollArea;
	gcn::TextBox                *mTextBox;
	gmGCRoot<gmFunctionObject>    mActionCallback;
};


//////////////////////////////////////////////////////////////////////////

class ScriptActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent)
	{
		gmVariable varCallback = mActionCallbacks->Get(mMachine,actionEvent.getId().c_str());
		gmFunctionObject *Fn = varCallback.GetFunctionObjectSafe();
		if(Fn)
		{
			gmCall call;
			if(call.BeginFunction(mMachine,Fn,mActionCallbacks->Get(mMachine,"_THIS")))
			{
				gmGCRoot<gmUserObject> r = 
					gmBind2::Class<gcn::Widget>::WrapObject(mMachine, actionEvent.getSource(), true);

				call.AddParamUser(r);
				call.AddParamString(actionEvent.getId().c_str());
				call.End();
			}
		}
	}
	ScriptActionListener(gmThread *a_thread)
	{
		DisableGCInScope gcEn(a_thread->GetMachine());
		mMachine = a_thread->GetMachine();
		mActionCallbacks.Set(mMachine->AllocTableObject(),a_thread->GetMachine());
		mActionCallbacks->Set(mMachine,"_THIS",a_thread->Param(0));
	}
	//////////////////////////////////////////////////////////////////////////
	gmMachine					*mMachine;
	gmGCRoot<gmTableObject>		mActionCallbacks;
};

class ScriptKeyListener : public gcn::KeyListener
{
public:
	void updateEventInfo(gcn::KeyEvent& keyEvent) 
	{
		mEventInfo->Set(GETMACHINE(),"ShiftPressed",gmVariable(keyEvent.isShiftPressed()?1:0));
		mEventInfo->Set(GETMACHINE(),"ControlPressed",gmVariable(keyEvent.isControlPressed()?1:0));
		mEventInfo->Set(GETMACHINE(),"AltPressed",gmVariable(keyEvent.isAltPressed()?1:0));
		mEventInfo->Set(GETMACHINE(),"MetaPressed",gmVariable(keyEvent.isMetaPressed()?1:0));
		mEventInfo->Set(GETMACHINE(),"NumericPad",gmVariable(keyEvent.isNumericPad()?1:0));
		mEventInfo->Set(GETMACHINE(),"Key",gmVariable(keyEvent.getKey().getValue()));
	}
	void keyPressed(gcn::KeyEvent& keyEvent)
	{
		gmCall call;
		if(mOnKeyPressed &&	call.BeginFunction(GETMACHINE(),mOnKeyPressed,gmVariable(mThisObject)))
		{
			gmGCRoot<gmUserObject> r = 
				gmBind2::Class<gcn::Widget>::WrapObject(GETMACHINE(), keyEvent.getSource(), true);

			updateEventInfo(keyEvent);
			call.AddParamTable(mEventInfo);
			call.End();
		}
	}
	void keyReleased(gcn::KeyEvent& keyEvent) 
	{
		gmCall call;
		if(mOnKeyReleased && call.BeginFunction(GETMACHINE(),mOnKeyReleased,gmVariable(mThisObject)))
		{
			gmGCRoot<gmUserObject> r = 
				gmBind2::Class<gcn::Widget>::WrapObject(GETMACHINE(), keyEvent.getSource(), true);

			updateEventInfo(keyEvent);
			call.AddParamTable(mEventInfo);
			call.End();
		}
	}
	ScriptKeyListener(gmThread *a_thread)
	{
		DisableGCInScope gcEn(a_thread->GetMachine());
		mThisObject.Set(a_thread->GetMachine()->AllocTableObject(),a_thread->GetMachine());
		mEventInfo.Set(a_thread->GetMachine()->AllocTableObject(),a_thread->GetMachine());
	}
	//////////////////////////////////////////////////////////////////////////
	gmGCRoot<gmTableObject>		mThisObject;
	gmGCRoot<gmTableObject>		mEventInfo;

	gmGCRoot<gmFunctionObject>	mOnKeyPressed;
	gmGCRoot<gmFunctionObject>	mOnKeyReleased;
};

class ScriptMouseListener : public gcn::MouseListener
{
public:
	void updateEventInfo(gcn::MouseEvent& mouseEvent, int wheel = 0) 
	{
		mEventInfo->Set(GETMACHINE(),"ShiftPressed",gmVariable(mouseEvent.isShiftPressed()?1:0));
		mEventInfo->Set(GETMACHINE(),"ControlPressed",gmVariable(mouseEvent.isControlPressed()?1:0));
		mEventInfo->Set(GETMACHINE(),"AltPressed",gmVariable(mouseEvent.isAltPressed()?1:0));
		mEventInfo->Set(GETMACHINE(),"MetaPressed",gmVariable(mouseEvent.isMetaPressed()?1:0));
		mEventInfo->Set(GETMACHINE(),"Button",gmVariable(mouseEvent.getButton()));
		mEventInfo->Set(GETMACHINE(),"ClickCount",gmVariable(mouseEvent.getClickCount()));
		mEventInfo->Set(GETMACHINE(),"X",gmVariable(mouseEvent.getX()));
		mEventInfo->Set(GETMACHINE(),"Y",gmVariable(mouseEvent.getY()));
		mEventInfo->Set(GETMACHINE(),"Wheel",wheel!=0 ? gmVariable(wheel) : gmVariable::s_null);
	}
	void mouseEntered(gcn::MouseEvent& mouseEvent)
	{
		gmCall call;
		if(mOnMouseEntered && call.BeginFunction(GETMACHINE(),mOnMouseEntered,gmVariable(mThisObject)))
		{
			gmGCRoot<gmUserObject> r = 
				gmBind2::Class<gcn::Widget>::WrapObject(GETMACHINE(), mouseEvent.getSource(), true);

			updateEventInfo(mouseEvent);
			call.AddParamTable(mEventInfo);
			call.End();
		}
	}
	void mouseExited(gcn::MouseEvent& mouseEvent)
	{
		gmCall call;
		if(mOnMouseExited && call.BeginFunction(GETMACHINE(),mOnMouseExited,gmVariable(mThisObject)))
		{
			gmGCRoot<gmUserObject> r = 
				gmBind2::Class<gcn::Widget>::WrapObject(GETMACHINE(), mouseEvent.getSource(), true);

			updateEventInfo(mouseEvent);
			call.AddParamTable(mEventInfo);
			call.End();
		}
	}
	void mousePressed(gcn::MouseEvent& mouseEvent)
	{
		gmCall call;
		if(mOnMousePressed && call.BeginFunction(GETMACHINE(),mOnMousePressed,gmVariable(mThisObject)))
		{
			gmGCRoot<gmUserObject> r = 
				gmBind2::Class<gcn::Widget>::WrapObject(GETMACHINE(), mouseEvent.getSource(), true);

			updateEventInfo(mouseEvent);
			call.AddParamTable(mEventInfo);
			call.End();
		}
	}
	void mouseReleased(gcn::MouseEvent& mouseEvent)
	{
		gmCall call;
		if(mOnMouseReleased && call.BeginFunction(GETMACHINE(),mOnMouseReleased,gmVariable(mThisObject)))
		{
			gmGCRoot<gmUserObject> r = 
				gmBind2::Class<gcn::Widget>::WrapObject(GETMACHINE(), mouseEvent.getSource(), true);

			updateEventInfo(mouseEvent);
			call.AddParamTable(mEventInfo);
			call.End();
		}
	}
	void mouseClicked(gcn::MouseEvent& mouseEvent)
	{
		gmCall call;
		if(mOnMouseClicked && call.BeginFunction(GETMACHINE(),mOnMouseClicked,gmVariable(mThisObject)))
		{
			gmGCRoot<gmUserObject> r = 
				gmBind2::Class<gcn::Widget>::WrapObject(GETMACHINE(), mouseEvent.getSource(), true);

			updateEventInfo(mouseEvent);
			call.AddParamTable(mEventInfo);
			call.End();
		}
	}
	void mouseWheelMovedUp(gcn::MouseEvent& mouseEvent)
	{
		gmCall call;
		if(mOnMouseWheel && call.BeginFunction(GETMACHINE(),mOnMouseWheel,gmVariable(mThisObject)))
		{
			gmGCRoot<gmUserObject> r = 
				gmBind2::Class<gcn::Widget>::WrapObject(GETMACHINE(), mouseEvent.getSource(), true);

			updateEventInfo(mouseEvent, 1);
			call.AddParamTable(mEventInfo);
			call.End();
		}
	}
	void mouseWheelMovedDown(gcn::MouseEvent& mouseEvent)
	{
		gmCall call;
		if(mOnMouseWheel && call.BeginFunction(GETMACHINE(),mOnMouseWheel,gmVariable(mThisObject)))
		{
			gmGCRoot<gmUserObject> r = 
				gmBind2::Class<gcn::Widget>::WrapObject(GETMACHINE(), mouseEvent.getSource(), true);

			updateEventInfo(mouseEvent, -1);
			call.AddParamTable(mEventInfo);
			call.End();
		}
	}
	void mouseMoved(gcn::MouseEvent& mouseEvent)
	{
		gmCall call;
		if(mOnMouseMoved && call.BeginFunction(GETMACHINE(),mOnMouseMoved,gmVariable(mThisObject)))
		{
			gmGCRoot<gmUserObject> r = 
				gmBind2::Class<gcn::Widget>::WrapObject(GETMACHINE(), mouseEvent.getSource(), true);

			updateEventInfo(mouseEvent);
			call.AddParamTable(mEventInfo);
			call.End();
		}
	}
	void mouseDragged(gcn::MouseEvent& mouseEvent)
	{
		gmCall call;
		if(mOnMouseDragged && call.BeginFunction(GETMACHINE(),mOnMouseDragged,gmVariable(mThisObject)))
		{
			gmGCRoot<gmUserObject> r = 
				gmBind2::Class<gcn::Widget>::WrapObject(GETMACHINE(), mouseEvent.getSource(), true);

			updateEventInfo(mouseEvent);
			call.AddParamTable(mEventInfo);
			call.End();
		}
	}

	ScriptMouseListener(gmThread *a_thread)
	{
		DisableGCInScope gcEn(a_thread->GetMachine());
		mThisObject.Set(a_thread->GetMachine()->AllocTableObject(),a_thread->GetMachine());
	}
	//////////////////////////////////////////////////////////////////////////
	gmGCRoot<gmTableObject>		mThisObject;
	gmGCRoot<gmTableObject>		mEventInfo;

	gmGCRoot<gmFunctionObject>	mOnMouseEntered;
	gmGCRoot<gmFunctionObject>	mOnMouseExited;
	gmGCRoot<gmFunctionObject>	mOnMousePressed;
	gmGCRoot<gmFunctionObject>	mOnMouseReleased;
	gmGCRoot<gmFunctionObject>	mOnMouseClicked;
	gmGCRoot<gmFunctionObject>	mOnMouseWheel;
	gmGCRoot<gmFunctionObject>	mOnMouseMoved;
	gmGCRoot<gmFunctionObject>	mOnMouseDragged;
};

class ScriptDeathListener : public gcn::DeathListener
{
public:
	void death(const gcn::Event& event)
	{
		gmCall call;
		if(mOnDeath && call.BeginFunction(GETMACHINE(),mOnDeath,gmVariable(mThisObject)))
		{
			gmGCRoot<gmUserObject> r = 
				gmBind2::Class<gcn::Widget>::WrapObject(GETMACHINE(), event.getSource(), true);

			call.End();
		}
	}
	ScriptDeathListener(gmThread *a_thread)
	{
		DisableGCInScope gcEn(a_thread->GetMachine());
		mThisObject.Set(a_thread->GetMachine()->AllocTableObject(),a_thread->GetMachine());
	}
	//////////////////////////////////////////////////////////////////////////
	gmGCRoot<gmTableObject>		mThisObject;

	gmGCRoot<gmFunctionObject>	mOnDeath;
};

class ScriptFocusListener : public gcn::FocusListener
{
public:
	void focusGained(const gcn::Event& event)
	{
		gmCall call;
		if(mOnFocusGained && call.BeginFunction(GETMACHINE(),mOnFocusGained,gmVariable(mThisObject)))
		{
			gmGCRoot<gmUserObject> r = 
				gmBind2::Class<gcn::Widget>::WrapObject(GETMACHINE(), event.getSource(), true);

			call.End();
		}
	}
	void focusLost(const gcn::Event& event)
	{
		gmCall call;
		if(mOnFocusLost && call.BeginFunction(GETMACHINE(),mOnFocusLost,gmVariable(mThisObject)))
		{
			gmGCRoot<gmUserObject> r = 
				gmBind2::Class<gcn::Widget>::WrapObject(GETMACHINE(), event.getSource(), true);

			call.End();
		}
	}
	ScriptFocusListener(gmThread *a_thread)
	{
		DisableGCInScope gcEn(a_thread->GetMachine());
		mThisObject.Set(a_thread->GetMachine()->AllocTableObject(),a_thread->GetMachine());
	}
	//////////////////////////////////////////////////////////////////////////
	gmGCRoot<gmTableObject>		mThisObject;

	gmGCRoot<gmFunctionObject>	mOnFocusGained;
	gmGCRoot<gmFunctionObject>	mOnFocusLost;
};

//////////////////////////////////////////////////////////////////////////

class ScriptListModel : public gcn::ListModel
{
public:
	int getNumberOfElements()
	{
		return mTable ? mTable->Count() : 0;
	}
	bool getColumnTitle(int column, std::string &title, int &columnwidth) 
	{
		if(mColumns)
		{
			gmTableObject *columnTable = mColumns->Get(gmVariable(column)).GetTableObjectSafe();
			if(columnTable)
			{
				const char *columnName = columnTable->Get(GETMACHINE(),"Name").GetCStringSafe();
				if(columnName)
					title = columnName;
				return columnName || columnTable->Get(GETMACHINE(),"Width").GetIntSafe(columnwidth);
			}
		}
		return false; 
	}
	std::string getElementAt(int i, int column)
	{
		if(mTable)
		{
			char buffer[256] = {};
			gmVariable v = mTable->Get(gmVariable(i));
			gmTableObject *subtbl = v.GetTableObjectSafe();
			if(subtbl)
			{
				gmVariable v2 = subtbl->Get(gmVariable(column));
				return v2.AsString(GETMACHINE(), buffer, 256);
			}
			else
			{
				return v.AsString(GETMACHINE(), buffer, 256);
			}
		}
		return "";
	}
	ScriptListModel(gmThread *a_thread) : mTable(0), mColumns(0)
	{
		gmTableObject *tbl = 0;
		
		tbl = a_thread->Param(0).GetTableObjectSafe();
		if(tbl)
			mTable = tbl;
		tbl = a_thread->Param(1).GetTableObjectSafe();
		if(tbl)
			mColumns = tbl;
	}
	//////////////////////////////////////////////////////////////////////////
	gmTableObject	*mTable;
	gmTableObject	*mColumns;
};

//////////////////////////////////////////////////////////////////////////

int Container_Ctr(gmThread *a_thread)
{
	gmBind2::Class<gcn::Container>::PushObject(a_thread,new gcn::Container(),true);
	return GM_OK;
}

int Label_Ctr(gmThread *a_thread)
{
	GM_STRING_PARAM(txt,0,"");
	gmBind2::Class<gcn::Label>::PushObject(a_thread,new gcn::Label(txt),true);
	return GM_OK;
}

int Button_Ctr(gmThread *a_thread)
{
	GM_STRING_PARAM(txt,0,"");
	gmBind2::Class<gcn::Button>::PushObject(a_thread,new gcn::Button(txt),true);
	return GM_OK;
}

int Checkbox_Ctr(gmThread *a_thread)
{
	GM_STRING_PARAM(txt,0,"");
	gmBind2::Class<gcn::CheckBox>::PushObject(a_thread,new gcn::CheckBox(txt),true);
	return GM_OK;
}

int Image_Ctr(gmThread *a_thread)
{
	GM_STRING_PARAM(imagefile,0,"");
	try
	{
		File InFile;
		if(InFile.OpenForRead(imagefile, File::Binary))
		{
			String contents;
			const obuint64 fileSize = InFile.ReadWholeFile(contents);
			OBASSERT(contents.size()==fileSize,"Error Reading File!");

			gcn::Image *img = gcn::Image::load(contents.c_str(),contents.size());
			gmBind2::Class<gcn::Image>::PushObject(a_thread, img, true);
		}
	}
	catch (gcn::Exception e)
	{
		GM_EXCEPTION_MSG(e.getMessage().c_str());
		return GM_EXCEPTION;
	}
	return GM_OK;
}

int Icon_Ctr(gmThread *a_thread)
{	
	gcn::Image *image = 0;
	if(!gmBind2::Class<gcn::Image>::FromParam(a_thread,0,image))
	{
		GM_EXCEPTION_MSG("expected Image as param 0, got %s",
			a_thread->GetMachine()->GetTypeName(a_thread->ParamType(0)));
		return GM_EXCEPTION;
	}
	gmBind2::Class<gcn::Icon>::PushObject(a_thread,new gcn::Icon(image),true);
	return GM_OK;
}

int RadioButton_Ctr(gmThread *a_thread)
{
	GM_STRING_PARAM(txt,0,"");
	GM_STRING_PARAM(group,1,"");
	GM_INT_PARAM(marked,2,0);
	gmBind2::Class<gcn::RadioButton>::PushObject(a_thread,new gcn::RadioButton(txt,group,marked!=0),true);
	return GM_OK;
}

int ScrollArea_Ctr(gmThread *a_thread)
{
	gcn::Widget *content = 0;
	if(!gmBind2::Class<gcn::Widget>::FromParam(a_thread,0,content))
	{
		GM_EXCEPTION_MSG("expected Widget as param 0, got %s",
			a_thread->GetMachine()->GetTypeName(a_thread->ParamType(0)));
		return GM_EXCEPTION;
	}
	gmBind2::Class<gcn::ScrollArea>::PushObject(a_thread,new gcn::ScrollArea(content),true);
	return GM_OK;
}

int TextBox_Ctr(gmThread *a_thread)
{
	GM_STRING_PARAM(txt,0,"");
	gmBind2::Class<gcn::TextBox>::PushObject(a_thread,new gcn::TextBox(txt),true);
	return GM_OK;
}

int TextField_Ctr(gmThread *a_thread)
{
	GM_STRING_PARAM(txt,0,"");
	gmBind2::Class<gcn::TextField>::PushObject(a_thread,new gcn::TextField(txt),true);
	return GM_OK;
}

int ScriptListModel_Ctr(gmThread *a_thread)
{
	gmBind2::Class<ScriptListModel>::PushObject(a_thread,new ScriptListModel(a_thread),true);
	return GM_OK;
}

int DropDown_Ctr(gmThread *a_thread)
{
	ScriptListModel *listmodel = 0;
	if(!gmBind2::Class<ScriptListModel>::FromParam(a_thread,0,listmodel))
	{
		GM_EXCEPTION_MSG("expected ListModel as param 0, got %s",
			a_thread->GetMachine()->GetTypeName(a_thread->ParamType(0)));
		return GM_EXCEPTION;
	}
	gmBind2::Class<gcn::DropDown>::PushObject(a_thread,new gcn::DropDown(listmodel),true);
	return GM_OK;
}

int ListBox_Ctr(gmThread *a_thread)
{
	ScriptListModel *listmodel = 0;
	if(!gmBind2::Class<ScriptListModel>::FromParam(a_thread,0,listmodel))
	{
		GM_EXCEPTION_MSG("expected ListModel as param 0, got %s",
			a_thread->GetMachine()->GetTypeName(a_thread->ParamType(0)));
		return GM_EXCEPTION;
	}
	gmBind2::Class<gcn::ListBox>::PushObject(a_thread,new gcn::ListBox(listmodel),true);
	return GM_OK;
}

int Window_Ctr(gmThread *a_thread)
{
	GM_STRING_PARAM(caption,0,"");
	gmBind2::Class<gcn::Window>::PushObject(a_thread,new gcn::Window(caption),true);
	return GM_OK;
}

int ScriptActionListener_Ctr(gmThread *a_thread)
{
	gmBind2::Class<ScriptActionListener>::PushObject(a_thread,new ScriptActionListener(a_thread),true);
	return GM_OK;
}

int ScriptKeyListener_Ctr(gmThread *a_thread)
{
	gmBind2::Class<ScriptKeyListener>::PushObject(a_thread,new ScriptKeyListener(a_thread),true);
	return GM_OK;
}

int ScriptMouseListener_Ctr(gmThread *a_thread)
{
	gmBind2::Class<ScriptMouseListener>::PushObject(a_thread,new ScriptMouseListener(a_thread),true);
	return GM_OK;
}

int ScriptDeathListener_Ctr(gmThread *a_thread)
{
	gmBind2::Class<ScriptDeathListener>::PushObject(a_thread,new ScriptDeathListener(a_thread),true);
	return GM_OK;
}

int ScriptFocusListener_Ctr(gmThread *a_thread)
{
	gmBind2::Class<ScriptFocusListener>::PushObject(a_thread,new ScriptFocusListener(a_thread),true);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

template<typename T>
void gcnTrace(T *a_var, gmMachine * a_machine, gmGarbageCollector* a_gc, const int a_workLeftToGo, int& a_workDone)
{

}

template<>
void gcnTrace(gcn::ListBox *a_var, gmMachine * a_machine, gmGarbageCollector* a_gc, const int a_workLeftToGo, int& a_workDone)
{
	ScriptListModel *lm = static_cast<ScriptListModel*>(a_var->getListModel());
	if(lm)
	{
		if(lm->mTable)
			a_gc->GetNextObject(lm->mTable);
		if(lm->mColumns)
			a_gc->GetNextObject(lm->mColumns);
	}
	a_workDone += 1;
}

template<>
void gcnTrace(gcn::DropDown *a_var, gmMachine * a_machine, gmGarbageCollector* a_gc, const int a_workLeftToGo, int& a_workDone)
{
	ScriptListModel *lm = static_cast<ScriptListModel*>(a_var->getListModel());
	if(lm)
	{
		if(lm->mTable)
			a_gc->GetNextObject(lm->mTable);
		if(lm->mColumns)
			a_gc->GetNextObject(lm->mColumns);
	}
	a_workDone += 1;
}

//////////////////////////////////////////////////////////////////////////

int Color_Ctr(gmThread *a_thread)
{
	gcn::Color *col = NULL;
	switch(a_thread->GetNumParams())
	{
	case 3:
		{
			GM_CHECK_INT_PARAM(r,0);
			GM_CHECK_INT_PARAM(g,1);
			GM_CHECK_INT_PARAM(b,2);
			GM_INT_PARAM(a,3,255);
			col = new gcn::Color();
			col->r = r;
			col->g = g;
			col->b = b;
			col->a = a;
			break;
		}
	case 1:
		{
			GM_CHECK_INT_PARAM(c,0);
			col = new gcn::Color(c);
			break;
		}
	default:
		GM_EXCEPTION_MSG("expected 1 or 3 params");
		return GM_EXCEPTION;
	}
	gmBind2::Class<gcn::Color>::PushObject(a_thread, col);
	return GM_OK;
}

int PointFacing_Ctr(gmThread *a_thread)
{	
	GM_CHECK_VECTOR_PARAM(pv, 0);
	GM_CHECK_VECTOR_PARAM(fv, 1);
	PointFacing *pf = new PointFacing;
	pf->mPosition = Vector3f(pv.x, pv.y, pv.z);
	pf->mFacing = Vector3f(fv.x, fv.y, fv.z);
	gmBind2::Class<PointFacing>::PushObject(a_thread, pf);
	return GM_OK;
}

void PointFacing_AsString(PointFacing *a_var, char * a_buffer, int a_bufferSize)
{
	_gmsnprintf(a_buffer, a_bufferSize, 
		"PointFacing(Vector3(%.3f, %.3f, %.3f),Vector3(%.3f, %.3f, %.3f))",
		a_var->mPosition.x, a_var->mPosition.y, a_var->mPosition.z,
		a_var->mFacing.x, a_var->mFacing.y, a_var->mFacing.z);
}

int DialogPanel_Ctr(gmThread *a_thread)
{
	GM_CHECK_STRING_PARAM(dialog,0);
	gmBind2::Class<DialogPanel>::PushObject(a_thread, new DialogPanel(dialog), true);
	return GM_OK;
}

int Slider_Ctr(gmThread *a_thread)
{
	GM_FLOAT_OR_INT_PARAM(minval,0,0.f);
	GM_FLOAT_OR_INT_PARAM(maxval,1,1.f);
	gmBind2::Class<gcn::Slider>::PushObject(a_thread, new gcn::Slider(minval,maxval), true);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

int gmfGetGuiTop(gmThread *a_thread)
{
	if(DW.mTop)
	{
		gmGCRoot<gmUserObject> r = gmBind2::Class<gcn::Container>::WrapObject(GETMACHINE(),DW.mTop,true);
		a_thread->PushUser(r);
	}
	return GM_OK;
}

int gmfGetGuiHud(gmThread *a_thread)
{
	if(DW.Hud.mUserObject)
	{
		a_thread->PushUser(DW.Hud.mUserObject);
	}
	return GM_OK;
}
int gmfAddPageButton(gmThread *a_thread)
{
	GM_CHECK_STRING_PARAM(btntext,0);
	if(DW.mTop)
	{
		gcn::Button *btn = DW.AddPageButton(btntext);
		gmGCRoot<gmUserObject> r = gmBind2::Class<gcn::Button>::WrapObject(GETMACHINE(),btn,false);
		a_thread->PushUser(r);
	}
	return GM_OK;
};
int gmfAddPageWindow(gmThread *a_thread)
{	
	GM_CHECK_STRING_PARAM(wintext,0);
	gcn::Button *togglebtn = 0;
	if(a_thread->GetNumParams() > 1)
	{
		if(!gmBind2::Class<gcn::Button>::FromParam(a_thread,1,togglebtn))
		{
			GM_EXCEPTION_MSG("expected Button as param 1, got %s",
				a_thread->GetMachine()->GetTypeName(a_thread->ParamType(1)));
			return GM_EXCEPTION;
		}
	}
	if(DW.mTop)
	{
		gcn::Window *w = DW.AddPageWindow(wintext,togglebtn);
		gmGCRoot<gmUserObject> r = gmBind2::Class<gcn::Window>::WrapObject(GETMACHINE(),w,false);
		a_thread->PushUser(r);
	}
	return GM_OK;
}

static gmFunctionEntry s_guiLib[] = 
{
	{"Top",					gmfGetGuiTop},
	{"Hud",					gmfGetGuiHud},
	{"AddPageButton",		gmfAddPageButton},
	{"AddPageWindow",		gmfAddPageWindow},
};

//////////////////////////////////////////////////////////////////////////

void gmBindDebugWindowLibrary(gmMachine *_m)
{
	//////////////////////////////////////////////////////////////////////////
	gmBind2::Class<PointFacing>("PointFacing",_m)
		.constructor(PointFacing_Ctr,0,"Gui")
		.asString(PointFacing_AsString)
		/*.var(&PointFacing::mPosition,	"Position")
		.var(&PointFacing::mFacing,		"Facing")*/
		;

	//////////////////////////////////////////////////////////////////////////
	gmBind2::Class<gcn::Color>("Color",_m)
		.constructor(Color_Ctr,0,"Gui")
		.var(&gcn::Color::r,	"r","int","red color component")
		.var(&gcn::Color::g,	"g","int","green color component")
		.var(&gcn::Color::b,	"b","int","blue color component")
		.var(&gcn::Color::a,	"a","int","alpha color component")
		;

	gmBind2::Class<gcn::Widget>("Widget",_m)
		.func(&gcn::Widget::isVisible,				"IsVisible","Checks if the widget is visible.")
		.func(&gcn::Widget::setVisible,				"SetVisible","Sets the visibility state of the widget.")
		.func(&gcn::Widget::setWidth,				"SetWidth","Sets the width of the widget.")
		.func(&gcn::Widget::getWidth,				"GetWidth","Gets the width of the widget.")
		.func(&gcn::Widget::setHeight,				"SetHeight","Sets the height of the widget.")
		.func(&gcn::Widget::getHeight,				"GetHeight","Gets the height of the widget.")
		.func(&gcn::Widget::setSize,				"SetSize","Sets the size of the widget.")
		.func(&gcn::Widget::setX,					"SetX","Set the x position of the widget.")
		.func(&gcn::Widget::getX,					"GetX","Get the x position of the widget.")
		.func(&gcn::Widget::setY,					"SetY","Set the y position of the widget.")
		.func(&gcn::Widget::getY,					"GetY","Get the y position of the widget.")
		.func(&gcn::Widget::setPosition,			"SetPosition","Set the position of the widget.")
		.func(&gcn::Widget::isEnabled,				"IsEnabled","Checks if the widget is enabled.")
		.func(&gcn::Widget::setEnabled,				"SetEnabled","Sets the enabled state of the widget.")
		.func(&gcn::Widget::setFrameSize,			"SetFrameSize","Sets the widget frame size.")
		.func(&gcn::Widget::getFrameSize,			"GetFrameSize","Gets the widget frame size.")
		.func(&gcn::Widget::getActionEventId,		"GetActionEventId","Gets the action event id of the widget.")
		.func(&gcn::Widget::setActionEventId,		"SetActionEventId","Sets the action event id of the widget.")
		.func(&gcn::Widget::addActionListener,		"AddActionListener","Adds an action listener to the widget.")
		.func(&gcn::Widget::removeActionListener,	"RemoveActionListener","Removes an action listener from the widget.")
		.func(&gcn::Widget::addKeyListener,			"AddKeyListener","Adds a key listener to the widget.")
		.func(&gcn::Widget::removeKeyListener,		"RemoveKeyListener","Removes a key listener from the widget.")
		.func(&gcn::Widget::addMouseListener,		"AddMouseListener","Adds a mouse listener to the widget.")
		.func(&gcn::Widget::removeMouseListener,	"RemoveMouseListener","Removes a mouse listener from the widget.")
		.func(&gcn::Widget::addDeathListener,		"AddDeathListener","Adds a death listener to the widget.")
		.func(&gcn::Widget::removeDeathListener,	"RemoveDeathListener","Removes a death listener from the widget.")
		.func(&gcn::Widget::addFocusListener,		"AddFocusListener","Adds a focus listener to the widget.")
		.func(&gcn::Widget::removeFocusListener,	"RemoveFocusListener","Removes a focus listener from the widget.")
		;

	gmBind2::Class<gcn::BasicContainer>("BasicContainer",_m)
		.base<gcn::Widget>()
		.func(&gcn::Container::moveToTop,			"MoveToTop","Moves a widget to the top of the container.")
		.func(&gcn::Container::moveToBottom,		"MoveToBottom","Moves a widget to the bottom of the container.")
		;

	gmBind2::Class<gcn::Container>("Container",_m)
		.base<gcn::BasicContainer>()
		.constructor(Container_Ctr,0,"Gui")
		.func((void (gcn::Container::*)(gcn::Widget*))&gcn::Container::add,"Add","Add a widget to the container.")
		.func((void (gcn::Container::*)(gcn::Widget*,int,int))&gcn::Container::add, "AddXY","Add a widget to the container at a position.")
		.func(&gcn::Container::remove,				"Remove","Removes a widget from the container.")
		.func(&gcn::Container::clear,				"Clear","Clears all widgets from the container.")
		.func(&gcn::Container::setOpaque,			"SetOpaque","Checks if the widget is opaque.")
		.func(&gcn::Container::isOpaque,			"IsOpaque","Set the widget to be opaque.")
		;

	gmBind2::Class<gcn::Label>("Label",_m)
		.base<gcn::Widget>()
		.constructor(Label_Ctr,0,"Gui")
		.func(&gcn::Label::setCaption,				"SetCaption","Sets the label text.")
		.func(&gcn::Label::getCaption,				"GetCaption","Gets the label text.")
		.func(&gcn::Label::setAlignment,			"SetAlignment","Sets the text alignment.")
		.func(&gcn::Label::getAlignment,			"GetAlignment","Gets the text alignment.")
		;

	gmBind2::Class<gcn::Button>("Button",_m)
		.base<gcn::Widget>()
		.constructor(Button_Ctr,0,"Gui")
		.func(&gcn::Button::setCaption,				"SetCaption","Sets the button text.")
		.func(&gcn::Button::getCaption,				"GetCaption","Gets the button text.")
		.func(&gcn::Button::setAlignment,			"SetAlignment","Sets the text alignment.")
		.func(&gcn::Button::getAlignment,			"GetAlignment","Gets the text alignment.")
		;

	gmBind2::Class<gcn::CheckBox>("CheckBox",_m)
		.base<gcn::Widget>()
		.constructor(Checkbox_Ctr,0,"Gui")
		.func(&gcn::CheckBox::setCaption,			"SetCaption","Sets the checkbox text.")
		.func(&gcn::CheckBox::getCaption,			"GetCaption","Gets the checkbox text.")
		.func(&gcn::CheckBox::isSelected,			"IsSelected","Checks if the box is selected.")
		.func(&gcn::CheckBox::setSelected,			"SetSelected","Sets the box selected state.")
		;

	gmBind2::Class<gcn::Image>("Image",_m)
		.constructor(Image_Ctr,0,"Gui")
		;

	gmBind2::Class<gcn::Icon>("Icon",_m)
		.base<gcn::Widget>()
		.constructor(Icon_Ctr,0,"Gui")
		;

	gmBind2::Class<gcn::RadioButton>("RadioButton",_m)
		.base<gcn::Widget>()
		.constructor(RadioButton_Ctr,0,"Gui")
		.func(&gcn::RadioButton::setCaption,		"SetCaption","Sets the button text.")
		.func(&gcn::RadioButton::getCaption,		"GetCaption","Gets the button text.")
		.func(&gcn::RadioButton::isSelected,		"IsSelected","Checks if the button is selected.")
		.func(&gcn::RadioButton::setSelected,		"SetSelected","Sets the button selected state.")
		.func(&gcn::RadioButton::setGroup,			"SetGroup","Sets the radio button group.")
		.func(&gcn::RadioButton::getGroup,			"GetGroup","Gets the radio button group.")
		;

	gmBind2::Class<gcn::ScrollArea>("ScrollArea",_m)
		.base<gcn::BasicContainer>()
		.constructor(ScrollArea_Ctr,0,"Gui")
		;

	gmBind2::Class<gcn::TextBox>("TextBox",_m)
		.base<gcn::Widget>()
		.constructor(TextBox_Ctr,0,"Gui")
		.func(&gcn::TextBox::setText,				"SetText","Sets the text box text.")
		.func(&gcn::TextBox::getText,				"GetText","Gets the text box text.")
		.func(&gcn::TextBox::setTextRow,			"SetTextRow","Sets the text of a given row.")
		.func(&gcn::TextBox::getTextRow,			"GetTextRow","Gets the text of a given row.")
		.func(&gcn::TextBox::getNumberOfRows,		"GetNumRows","Gets the number of rows in the text.")
		.func(&gcn::TextBox::getCaretPosition,		"GetCaretPos","Gets the position of the caret.")
		.func(&gcn::TextBox::setCaretPosition,		"SetCaretPos","Sets the position of the caret.")
		.func(&gcn::TextBox::getCaretRow,			"GetCaretRow","Gets the row of the caret.")
		.func(&gcn::TextBox::setCaretRow,			"SetCaretRow","Sets the row of the caret.")
		.func(&gcn::TextBox::getCaretColumn,		"GetCaretColumn","Gets the column of the caret.")
		.func(&gcn::TextBox::setCaretColumn,		"SetCaretColumn","Sets the column of the caret.")
		.func(&gcn::TextBox::setCaretRowColumn,		"SetCaretRowColumn","Sets the caret row and column.")
		.func(&gcn::TextBox::scrollToCaret,			"ScrollToCaret","Scrolls the text box to the caret.")
		.func(&gcn::TextBox::isEditable,			"IsEditable","Checks if the text box is editable.")
		.func(&gcn::TextBox::setEditable,			"SetEditable","Sets whether the text box is editable or not.")
		.func(&gcn::TextBox::addRow,				"AddRow","Append a row to the text box.")
		.func(&gcn::TextBox::isOpaque,				"IsOpaque","Set the widget to be opaque.")
		.func(&gcn::TextBox::setOpaque,				"SetOpaque","Checks if the widget is opaque.")
		;

	gmBind2::Class<gcn::TextField>("TextField",_m)
		.base<gcn::Widget>()
		.constructor(TextField_Ctr,0,"Gui")
		.func(&gcn::TextField::setText,				"SetText","Sets the text box text.")
		.func(&gcn::TextField::getText,				"GetText","Gets the text box text.")	
		.func(&gcn::TextField::getCaretPosition,	"GetCaretPos","Gets the position of the caret.")
		.func(&gcn::TextField::setCaretPosition,	"SetCaretPos","Sets the position of the caret.")
		;

	gmBind2::Class<gcn::Window>("Window",_m)
		.base<gcn::Container>()
		.trace(gcnTrace<gcn::Window>)
		.constructor(Window_Ctr,0,"Gui")
		.func(&gcn::Window::setAlignment,			"SetAlignment","Sets the text alignment.")
		.func(&gcn::Window::getAlignment,			"GetAlignment","Gets the text alignment.")
		.func(&gcn::Window::setPadding,				"SetPadding","Sets the window padding.")
		.func(&gcn::Window::getPadding,				"GetPadding","Gets the window padding.")
		.func(&gcn::Window::setTitleBarHeight,		"SetTitleBarHeight","Sets the window title bar height.")
		.func(&gcn::Window::getTitleBarHeight,		"GetTitleBarHeight","Gets the window title bar height.")
		.func(&gcn::Window::setMovable,				"SetMovable","Sets whether the window is movable or not.")
		.func(&gcn::Window::isMovable,				"IsMovable","Checks whether the window is movable.")
		.func(&gcn::Window::isOpaque,				"IsOpaque","Set the widget to be opaque.")
		.func(&gcn::Window::setOpaque,				"SetOpaque","Set the widget to be opaque.")
		.func(&gcn::Window::resizeToContent,		"ResizeToContent","Resizes window to the size of the content.")
		;

	gmBind2::Class<gcn::ActionListener>("ActionListenerBase",_m);
	gmBind2::Class<ScriptActionListener>("ActionListener",_m)
		.base<gcn::ActionListener>()
		.constructor(ScriptActionListener_Ctr,0,"Gui")
		.var(&ScriptActionListener::mActionCallbacks,"OnAction","callback","Function to call when the listener is triggered.")
		;

	gmBind2::Class<gcn::KeyListener>("KeyListenerBase",_m);
	gmBind2::Class<ScriptKeyListener>("KeyListener",_m)
		.base<gcn::KeyListener>()
		.constructor(ScriptKeyListener_Ctr,0,"Gui")
		.var(&ScriptKeyListener::mOnKeyPressed,		"OnKeyPressed","callback","Function to call when a key is pressed.")
		.var(&ScriptKeyListener::mOnKeyReleased,	"OnKeyReleased","callback","Function to call when a key is released.")
		;

	gmBind2::Class<gcn::MouseListener>("MouseListenerBase",_m);
	gmBind2::Class<ScriptMouseListener>("MouseListener",_m)
		.base<gcn::MouseListener>()
		.constructor(ScriptMouseListener_Ctr,0,"Gui")
		.var(&ScriptMouseListener::mOnMouseEntered,	"OnMouseEntered","callback","Function to call when the mouse enters a widget.")
		.var(&ScriptMouseListener::mOnMouseExited,	"OnMouseExited","callback","Function to call when the mouse exits a widget.")
		.var(&ScriptMouseListener::mOnMousePressed,	"OnMousePressed","callback","Function to call when the mouse is pressed.")
		.var(&ScriptMouseListener::mOnMouseReleased,"OnMouseReleased","callback","Function to call when the mouse is released.")
		.var(&ScriptMouseListener::mOnMouseClicked,	"OnMouseClicked","callback","Function to call when the mouse clicks.")
		.var(&ScriptMouseListener::mOnMouseWheel,	"OnMouseWheel","callback","Function to call when the mouse wheel is moved.")
		.var(&ScriptMouseListener::mOnMouseMoved,	"OnMouseMoved","callback","Function to call when the mouse is moved.")
		.var(&ScriptMouseListener::mOnMouseDragged,	"OnMouseDragged","callback","Function to call when the mouse is dragged.")
		;

	gmBind2::Class<gcn::DeathListener>("DeathListenerBase",_m);
	gmBind2::Class<ScriptDeathListener>("DeathListener",_m)
		.base<gcn::DeathListener>()
		.constructor(ScriptDeathListener_Ctr,0,"Gui")
		.var(&ScriptDeathListener::mOnDeath,		"OnDeath","callback","Function to call when the listener is triggered.")
		;

	gmBind2::Class<gcn::FocusListener>("FocusListenerBase",_m);
	gmBind2::Class<ScriptFocusListener>("FocusListener",_m)
		.base<gcn::FocusListener>()
		.constructor(ScriptFocusListener_Ctr,0,"Gui")
		.var(&ScriptFocusListener::mOnFocusGained,	"OnFocusGained","callback","Function to call when focus is gained.")
		.var(&ScriptFocusListener::mOnFocusLost,	"OnFocusLost","callback","Function to call when focus is lost.")
		;

	gmBind2::Class<gcn::ListModel>("ListModelBase",_m);
	gmBind2::Class<ScriptListModel>("ListModel",_m)
		.base<gcn::ListModel>()
		.trace(gcnTrace<ScriptListModel>)
		.constructor(ScriptListModel_Ctr,0,"Gui")
		//.constructorArgs(0,"Gui")
		.var(&ScriptListModel::mTable,	"ListTable","table","Table to display in the list.")
		;
	
	gmBind2::Class<gcn::DropDown>("DropDown",_m)
		.base<gcn::BasicContainer>()
		.trace(gcnTrace<gcn::DropDown>)
		.constructor(DropDown_Ctr,0,"Gui")
		.func(&gcn::DropDown::getSelected,			"GetSelected","Gets the index of the selected option.")
		.func(&gcn::DropDown::setSelected,			"SetSelected","Sets the index of the selected option.")
		.func(&gcn::DropDown::setListModel,			"SetListModel","Sets the list model to display in the dropdown.")
		.func(&gcn::DropDown::getListModel,			"GetListModel","Gets the list model to display in the dropdown.")
		.func(&gcn::DropDown::adjustHeight,			"AdjustHeight","Adjust the height of the drop down.")
		;

	gmBind2::Class<gcn::ListBox>("ListBox",_m)
		.base<gcn::Widget>()
		.trace(gcnTrace<gcn::ListBox>)
		.constructor(ListBox_Ctr,0,"Gui")		
		.func(&gcn::ListBox::getSelected,			"GetSelected","Gets the index of the selected option.")
		.func(&gcn::ListBox::setSelected,			"SetSelected","Sets the index of the selected option.")
		.func(&gcn::ListBox::setListModel,			"SetListModel","Sets the list model to display in the listbox.")
		.func(&gcn::ListBox::getListModel,			"GetListModel","Gets the list model to display in the listbox.")
		.func(&gcn::ListBox::adjustSize,			"AdjustSize","Adjust the size of the listbox.")
		.func(&gcn::ListBox::isWrappingEnabled,		"IsWrappingKeyboardSelection","Checks if selection wrapping is enabled.")
		.func(&gcn::ListBox::setWrappingEnabled,	"SetWrappingKeyboardSelection","Sets whether selection wrapping is enabled.")
		;

	using namespace gcn::contrib;

	gmBind2::Class<AdjustingContainer>("AdjustingContainer",_m)
		.constructorNativeOwned((const char *)0,"Gui")
		.base<gcn::Container>()
		/*.func((void (gcn::AdjustingContainer::*)(gcn::Widget*))&gcn::AdjustingContainer::add, "Add")
		.func((void (gcn::AdjustingContainer::*)(gcn::Widget*,int,int))&gcn::AdjustingContainer::add, "AddXY")
		.func(&gcn::AdjustingContainer::remove,					"Remove")
		.func(&gcn::AdjustingContainer::clear,					"Clear")*/
		.func(&AdjustingContainer::setNumberOfColumns,		"SetNumberOfColumns","Gets the number of columns.")
		.func(&AdjustingContainer::setColumnAlignment,		"SetColumnAlignment","Sets the alignment of a column.")
		.func(&AdjustingContainer::setPadding,				"SetPadding","Sets the padding of the container.")
		.func(&AdjustingContainer::setVerticalSpacing,		"SetVerticalSpacing","Sets the vertical spacing between elements.")
		.func(&AdjustingContainer::setHorizontalSpacing,	"SetHorizontalSpacing","Sets the horizontal spacing between elements.")
		.func(&AdjustingContainer::adjustContent,			"AdjustContent","Adjust the content in the container.")
		;

	gmBind2::Class<DialogPanel>("DialogPanel",_m)
		.base<AdjustingContainer>()
		.constructor(DialogPanel_Ctr,0,"Gui")
		.func(&DialogPanel::AddOption,				"AddOption","Add an option to the dialog panel.")		
		;

	gmBind2::Class<gcn::TabbedArea>("TabbedArea",_m)
		.constructorNativeOwned((const char *)0,"Gui")
		.base<gcn::BasicContainer>()
		.func((void (gcn::TabbedArea::*)(gcn::Tab*,gcn::Widget*))&gcn::TabbedArea::addTab, "AddTabWithWidget","Add tab to area and widget into tab.")
		.func((void (gcn::TabbedArea::*)(const std::string&,gcn::Widget*))&gcn::TabbedArea::addTab, "AddTabNameWithWidget","Add tab to area by name and widget into tab.")
		.func(&gcn::TabbedArea::removeTabWithIndex,		"RemoveTabByIndex","Remove a tab by its index.")
		.func(&gcn::TabbedArea::removeTab,				"RemoveTab","Remove a specific tab.")
		.func((bool (gcn::TabbedArea::*)(gcn::Tab*) const)&gcn::TabbedArea::isTabSelected, "IsTabSelected","Checks if a tab is selected.")
		.func((bool (gcn::TabbedArea::*)(int) const)&gcn::TabbedArea::isTabSelected, "IsTabIndexSelected","Checks if a tab is selected by index.")
		.func((void (gcn::TabbedArea::*)(gcn::Tab*))&gcn::TabbedArea::setSelectedTab, "SetSelectedTab","Sets the selected tab directly.")
		.func((void (gcn::TabbedArea::*)(int))&gcn::TabbedArea::setSelectedTab, "SetSelectedTabIndex","Sets the selected tab by index.")
		.func(&gcn::TabbedArea::getSelectedTabIndex,	"GetSelectedTabIndex","Gets index of selected tab.")
		.func(&gcn::TabbedArea::getSelectedTab,			"GetSelectedTab","Gets the selected tab.")
		;
	
	gmBind2::Class<gcn::Tab>("Tab",_m)
		.constructorNativeOwned((const char *)0,"Gui")
		.base<gcn::BasicContainer>()
		.func(&gcn::Tab::setTabbedArea,					"SetTabbedArea","Sets the tabbed area for the tab.")
		.func(&gcn::Tab::getTabbedArea,					"GetTabbedArea","Gets the tabbed area for the tab.")
		.func(&gcn::Tab::setCaption,					"SetCaption","Sets the tab title text.")
		.func(&gcn::Tab::getCaption,					"GetCaption","Gets the tab title text.")
		;	

	gmBind2::Class<gcn::Slider>("Slider",_m)
		.constructor(Slider_Ctr,(const char *)0,"Gui")
		.base<gcn::Widget>()
		.func(&gcn::Slider::setScale,					"SetScale","Sets the min and max scale of the slider.")
		.func(&gcn::Slider::getScaleStart,				"GetScaleStart","Gets the min scale of the slider.")
		.func(&gcn::Slider::getScaleEnd,				"GetScaleEnd","Gets the max scale of the slider.")
		.func(&gcn::Slider::setScaleStart,				"SetScaleStart","Sets the min scale of the slider.")
		.func(&gcn::Slider::setScaleEnd,				"SetScaleEnd","Sets the max scale of the slider.")
		.func(&gcn::Slider::getValue,					"GetValue","Gets the current value of the slider.")
		.func((void (gcn::Slider::*)(float))&gcn::Slider::setValue,	"SetValue","Sets the current value of the slider.")
		.func(&gcn::Slider::setStepLength,				"SetStepLength","Sets the step length of the slider.")
		.func(&gcn::Slider::getStepLength,				"GetStepLength","Gets the step length of the slider.")
		.func(&gcn::Slider::setMarkerLength,			"SetMarkerLength","Sets the marker length of the slider.")
		.func(&gcn::Slider::getMarkerLength,			"GetMarkerLength","Gets the marker length of the slider.")
		;

	//////////////////////////////////////////////////////////////////////////
	_m->RegisterLibrary(s_guiLib, sizeof(s_guiLib) / sizeof(s_guiLib[0]), "Gui", false);
}

void gmBindDebugWindowLibraryDocs(gmMachine *_m, gmBind2::TableConstructor &_tc)
{
#if(GMBIND2_DOCUMENT_SUPPORT)
	_tc.Push("Gui");
		_tc.Push("PointFacing");
			gmBind2::Class<PointFacing>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("Color");
			gmBind2::Class<gcn::Color>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("Widget");
			gmBind2::Class<gcn::Widget>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("BasicContainer");
			gmBind2::Class<gcn::BasicContainer>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("Container");
			gmBind2::Class<gcn::Container>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("Label");
			gmBind2::Class<gcn::Label>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("Button");
			gmBind2::Class<gcn::Button>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("CheckBox");
			gmBind2::Class<gcn::CheckBox>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("Image");
			gmBind2::Class<gcn::Image>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("Icon");
			gmBind2::Class<gcn::Icon>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("RadioButton");
			gmBind2::Class<gcn::RadioButton>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("ScrollArea");
			gmBind2::Class<gcn::ScrollArea>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("TextBox");
			gmBind2::Class<gcn::TextBox>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("TextField");
			gmBind2::Class<gcn::TextField>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("Window");
			gmBind2::Class<gcn::Window>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("ActionListener");
			gmBind2::Class<ScriptActionListener>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("KeyListener");
			gmBind2::Class<ScriptKeyListener>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("MouseListener");
			gmBind2::Class<ScriptMouseListener>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("DeathListener");
			gmBind2::Class<ScriptDeathListener>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("FocusListener");
			gmBind2::Class<ScriptFocusListener>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("ListModel");
			gmBind2::Class<ScriptListModel>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("DropDown");
			gmBind2::Class<gcn::DropDown>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("ListBox");
			gmBind2::Class<gcn::ListBox>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		using namespace gcn::contrib;
		_tc.Push("AdjustingContainer");
			gmBind2::Class<AdjustingContainer>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("DialogPanel");
			gmBind2::Class<DialogPanel>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("TabbedArea");
			gmBind2::Class<gcn::TabbedArea>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
		_tc.Push("Tab");
			gmBind2::Class<gcn::Tab>::GetPropertyTable(_m,_tc.Top());
		_tc.Pop();
	_tc.Pop();
#endif
}

#endif
