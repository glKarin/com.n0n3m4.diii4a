#include "PrecompCommon.h"

//////////////////////////////////////////////////////////////////////////

#include "DebugWindow.h"
#ifdef ENABLE_DEBUG_WINDOW
#include "NavigationManager.h"
#include "PathPlannerWaypoint.h"
#include "PathPlannerNavMesh.h"
#include "PathPlannerFloodFill.h"
#include "PathPlannerRecast.h"
#include "ScriptManager.h"
#endif

static RenderOverlayType gOverlayType = OVERLAY_GAME;

RenderOverlay *CreateOpenGLRenderOverlay();
//RenderOverlay *CreateDXRenderOverlay();
RenderOverlay *CreateGameRenderOverlay();

RenderOverlay *gOverlay = NULL;

void SetRenderOverlayType(RenderOverlayType _t)
{
	gOverlayType = _t;
}

#ifdef ENABLE_DEBUG_WINDOW

#include <SFML/Graphics.hpp>
#include <guichan/sfml.hpp>

#include "detours.h"

#undef DELETE

//#pragma comment(lib,"sfml-main.lib")
#pragma comment(lib,"sfml-window.lib")
#pragma comment(lib,"sfml-graphics.lib")
#pragma comment(lib,"sfml-system.lib")
//#pragma comment(lib,"freetype.lib")

//////////////////////////////////////////////////////////////////////////

DebugWindow_s DW = {};

gcn::MouseCache g_MapMouse;

int				g_SelectedClient = -1;
MapGoalPtr		g_LastMapGoal;

ClientPtr GetSelectedClient()
{
	return g_SelectedClient != -1 ?
		IGameManager::GetInstance()->GetGame()->GetDebugWindowClient(g_SelectedClient)
		: ClientPtr();
}

//////////////////////////////////////////////////////////////////////////

WeaponSystem_DW		*gWeaponWindow = 0;
SensoryMemory_DW	*gSensoryWindow = 0;
Aimer_DW			*gAimerWindow = 0;

void ShowStateWindow(obuint32 name)
{
	if(name == Utils::Hash32("WeaponSystem"))
		gWeaponWindow->setVisible(!gWeaponWindow->isVisible());
	if(name == Utils::Hash32("SensoryMemory"))
		gSensoryWindow->setVisible(!gSensoryWindow->isVisible());
	if(name == Utils::Hash32("Aimer"))
		gAimerWindow->setVisible(!gAimerWindow->isVisible());
}

//////////////////////////////////////////////////////////////////////////

namespace DrawBuffer
{
	typedef std::vector<gcn::Graphics::Line> LineList;
	typedef std::vector<gcn::Graphics::Point> PointList;

	LineList		Lines;
	PointList		Points;

	void FlushLines(gcn::Graphics *graphics)
	{
		if(!Lines.empty())
		{
			graphics->drawLines(&Lines[0], Lines.size());
			Lines.resize(0);
		}
	}
	void FlushPoints(gcn::Graphics *graphics)
	{
		if(!Points.empty())
		{
			graphics->drawPoints(&Points[0], Points.size());
			Points.resize(0);
		}
	}
	void Flush(gcn::Graphics *graphics)
	{
		FlushLines(graphics);
		FlushPoints(graphics);
	}
	
	void Add(gcn::Graphics *graphics, const gcn::Graphics::Line &l)
	{
		Lines.push_back(l);
	}
	void Add(gcn::Graphics *graphics, const gcn::Graphics::Point &p)
	{
		Points.push_back(p);
	}
	void Reset()
	{
		Lines.resize(0);
		Points.resize(0);
	}
	void Init()
	{
		Lines.reserve(8192);
		Points.reserve(2048);
	}
	void Shutdown()
	{
		Lines.clear();
		Points.clear();
	}
}

//////////////////////////////////////////////////////////////////////////

namespace Listeners
{
	class Console : public gcn::KeyListener, public gcn::SelectionListener
	{
	public:
		//////////////////////////////////////////////////////////////////////////
		bool AnyKeyPressed() const
		{
			return m_AnyKeyPressed;
		}
		void ResetKeyPressed()
		{
			m_AnyKeyPressed = false;
		}
		//////////////////////////////////////////////////////////////////////////
		void valueChanged(const gcn::SelectionEvent& selEvent)
		{
			if(selEvent.getSource()==DW.Console.mAutoComplete)
			{
				static bool bDo = false;
				if(bDo)
					DW.Console.UpdateWordAtCaret();				
			}
		}
		//////////////////////////////////////////////////////////////////////////
		virtual void keyPressed(gcn::KeyEvent& keyEvent)
		{
			using namespace gcn;

			Key key = keyEvent.getKey();
			switch(key.getValue())
			{
			case Key::TAB:
				{
					DW.Console.UpdateWordAtCaret();
					keyEvent.consume();
					break;
				}
			case Key::ALT_GR:
				{
					DW.Console.mInput->setText("");
					break;
				}
			case Key::ENTER:
				{
					keyEvent.consume();
					String s = DW.Console.mInput->getText();
					Utils::StringTrimCharacters(s, "\r\n");
					ScriptManager::GetInstance()->ExecuteString(s);
					DW.Console.mInput->setText("");				
					m_History.push_back(s);
					break;
				}
			case Key::UP:
				{
					int iSel = DW.Console.mAutoComplete->getSelected();
					DW.Console.mAutoComplete->setSelected(iSel-1);
					break;
				}
			case Key::DOWN:
				{
					int iSel = DW.Console.mAutoComplete->getSelected();
					DW.Console.mAutoComplete->setSelected(iSel+1);
					break;
				}
			case Key::PAGE_UP:
				if(!m_History.empty())
				{
					keyEvent.consume();
					m_History.push_front(m_History.back());
					DW.Console.mInput->setText(m_History.back());
					DW.Console.mInput->setCaretToEnd();
					m_History.pop_back();
					break;
				}
			case Key::PAGE_DOWN:
				if(!m_History.empty())
				{
					keyEvent.consume();
					m_History.push_back(m_History.front());
					DW.Console.mInput->setText(m_History.front());
					m_History.pop_front();
					break;
				}
			case Key::DELETE:
				{
					if(keyEvent.isShiftPressed())
					{
						m_History.clear();
					}
					break;
				}
			}

			m_AnyKeyPressed = true;
		}

		Console() : m_AnyKeyPressed(false) {}
	private:
		StringList	m_History;
		bool		m_AnyKeyPressed;
	};
	//////////////////////////////////////////////////////////////////////////
	class GenericActionListener : public gcn::ActionListener, public gcn::KeyListener, public gcn::MouseListener
	{
	public:
		struct KeyCombo
		{
			gcn::Widget*	mButton;
			int				mKeyCode;
			bool			mAlt;
			bool			mCtrl;
			const char *	mName;

			gcn::Widget	*	mWidget;

			KeyCombo(gcn::Widget *button, int k = 0, bool alt = false, bool ctrl = false, gcn::Widget *w = 0, const char *name = 0) 
				: mButton(button), mKeyCode(k), mAlt(alt), mCtrl(ctrl), mWidget(w), mName(name) {}
		};
		typedef std::list<KeyCombo> ToggleWidgets;
		ToggleWidgets mToggleKeyMap;

		void addToggleKeyMap(gcn::Widget *button, int key, bool alt, bool ctrl, gcn::Widget *w, const char *name)
		{
			if(name)
			{
				Options::SetValue("Debug Window",name,"false",false);

				bool l = true;
				if(Options::GetValue("Debug Window","ShowGoals",l))
				{
					w->setVisible(l);
				}
			}

			KeyCombo kc(button,key,alt,ctrl,w,name);
			mToggleKeyMap.push_back(kc);
		}

		void action(const gcn::ActionEvent& actionEvent)
		{
			for(ToggleWidgets::iterator it = mToggleKeyMap.begin();
				it != mToggleKeyMap.end();
				++it)
			{
				if((*it).mButton == actionEvent.getSource())
				{
					const bool isvis = !(*it).mWidget->isVisible();
					(*it).mWidget->setVisible(isvis);

					if((*it).mName)
						Options::SetValue("Debug Window",(*it).mName,isvis?"true":"false",true);
				}
			}
		}

		void keyPressed(gcn::KeyEvent& keyEvent)
		{
			gcn::Key key = keyEvent.getKey();

			for(ToggleWidgets::iterator it = mToggleKeyMap.begin();
				it != mToggleKeyMap.end();
				++it)
			{
				if((*it).mKeyCode == key.getValue())
				{
					if((*it).mAlt && !keyEvent.isAltPressed())
						return;
					if((*it).mCtrl && !keyEvent.isControlPressed())
						return;

					gcn::Widget *w = (*it).mWidget;
					w->setVisible(!w->isVisible());
				}
			}
		}
	};
	//////////////////////////////////////////////////////////////////////////
	class MapActionListener : public gcn::ActionListener, public gcn::KeyListener, public gcn::MouseListener
	{
	public:
		void action(const gcn::ActionEvent& actionEvent)
		{
		}
		void keyPressed(gcn::KeyEvent& keyEvent)
		{
			using namespace gcn;

			gcn::Key key = keyEvent.getKey();
			switch(key.getValue())
			{
			case Key::LEFT_SHIFT:
			case Key::RIGHT_SHIFT:
				{
					mShiftHeld = true;
					break;
				}
			}
		}
		void keyReleased(gcn::KeyEvent& keyEvent)
		{
			using namespace gcn;

			gcn::Key key = keyEvent.getKey();
			switch(key.getValue())
			{
			case Key::LEFT_SHIFT:
			case Key::RIGHT_SHIFT:
				{
					mShiftHeld = false;
					break;
				}
			}
		}
		void mousePressed(gcn::MouseEvent& mouseEvent)
		{
			mMouseX = mouseEvent.getX();
			mMouseY = mouseEvent.getY();
		}
		void mouseWheelMovedUp(gcn::MouseEvent& mouseEvent)
		{
			mMapZoom += 0.05f * (mShiftHeld ? 10.f : 5.f);
		}
		void mouseWheelMovedDown(gcn::MouseEvent& mouseEvent)
		{
			mMapZoom -= 0.05f * (mShiftHeld ? 10.f : 5.f);
		}
		void mouseMoved(gcn::MouseEvent& mouseEvent)
		{
		}
		void mouseDragged(gcn::MouseEvent& mouseEvent)
		{
			int speed = 1;
			switch(mouseEvent.getButton())
			{
			case gcn::MouseInput::LEFT:
				break;
			case gcn::MouseInput::RIGHT:
				speed = 5;
				break;
			case gcn::MouseInput::MIDDLE:
				speed = 10;
				break;
			}
			
			mOffsetX += (mouseEvent.getX() - mMouseX) * speed;
			mOffsetY -= (mouseEvent.getY() - mMouseY) * speed;
			mMouseX = mouseEvent.getX();
			mMouseY = mouseEvent.getY();
		}

		MapActionListener()
			: mMapZoom(1.f)
			, mMouseX(0)
			, mMouseY(0)
			, mOffsetX(0)
			, mOffsetY(0)
			, mShiftHeld(false)
		{
		}

		float		mMapZoom;
		int			mMouseX, mMouseY;
		int			mOffsetX, mOffsetY;
		bool		mShiftHeld;
	};
}

Listeners::Console consoleListener;
Listeners::GenericActionListener dwActionListener;
Listeners::MapActionListener mapActionListener;

bool _GoalNameLT(const MapGoalPtr _pt1, const MapGoalPtr _pt2);

namespace ListModels
{
	class ClientListModel : public gcn::ListModel
	{
	public:
		virtual int getNumberOfColumns() { return 1; };

		virtual bool getColumnTitle(int column, std::string &title, int &columnwidth)
		{
			columnwidth = 100;
			title = "Clients";
			return true;
		}

		virtual int getColumnBorderSize() { return 1; };

		virtual int getNumberOfElements()
		{
			return IGameManager::GetInstance()->GetGame()->GetDebugWindowNumClients();
		}

		std::string getElementAt(int i, int column)
		{
			ClientPtr cl = IGameManager::GetInstance()->GetGame()->GetDebugWindowClient(i);
			if(cl)
				return cl->GetName();
			return std::string("");
		}
	};

	class GoalsListModel : public gcn::ListModel
	{
	public:

		enum Columns
		{
			GoalName,
			GoalAvailability1,
			GoalAvailability2,
			GoalAvailability3,
			GoalAvailability4,

			NumColumns
		};		

		virtual int getNumberOfColumns() { return NumColumns; };

		virtual bool getColumnTitle(int column, std::string &title, int &columnwidth)
		{
			switch(column)
			{
			case GoalName:
				columnwidth = 300;
				title = "Goals";
				break;
			case GoalAvailability1:
				columnwidth = 8;
				title = "1";
				break;
			case GoalAvailability2:
				columnwidth = 8;
				title = "2";
				break;
			case GoalAvailability3:
				columnwidth = 8;
				title = "3";
				break;
			case GoalAvailability4:
				columnwidth = 8;
				title = "4";
				break;
			}			
			return true;
		}

		virtual int getColumnBorderSize() { return 1; };

		virtual int getNumberOfElements()
		{
			m_GoalList = GoalManager::GetInstance()->GetGoalList();
			std::sort(m_GoalList.begin(), m_GoalList.end(), _GoalNameLT);
			return m_GoalList.size();
		}

		std::string getElementAt(int i, int column)
		{
			switch(column)
			{
			case GoalName:
				return m_GoalList[i]->GetName();
			case GoalAvailability1:
				return m_GoalList[i]->IsAvailable(OB_TEAM_1) ? "1":"0";
			case GoalAvailability2:
				return m_GoalList[i]->IsAvailable(OB_TEAM_2) ? "1":"0";
			case GoalAvailability3:
				return m_GoalList[i]->IsAvailable(OB_TEAM_3) ? "1":"0";
			case GoalAvailability4:
				return m_GoalList[i]->IsAvailable(OB_TEAM_4) ? "1":"0";
			}
			return "";
		}

		/*bool getElementColor(int i, int column, Color &c)
		{
			m_GoalList[i]->IsE
			c = gcn::Color();
			return true; 
		}*/

		MapGoalList m_GoalList;
	};

	class AutoCompleteListModel : public gcn::ListModel
	{
	public:
		virtual int getNumberOfColumns() { return 1; };
		virtual bool getColumnTitle(int column, std::string &title, int &columnwidth)
		{
			title="Suggestions"; 
			columnwidth = 400;
			return false;
		}
		virtual int getColumnBorderSize()
		{
			return 0; 
		}
		virtual int getNumberOfElements()
		{
			if(consoleListener.AnyKeyPressed())
			{
				consoleListener.ResetKeyPressed();

				m_Completions.resize(0);

				// remember last selection
				String sSelected = DW.Console.mAutoComplete->getSelectedText();

				int iStart = 0, iEnd = 0;
				String s;
				DW.Console.mInput->getWordPosAtCaret(iStart,iEnd,s);
				ScriptManager::GetInstance()->GetAutoCompleteList(DW.Console.mInput->getText(),m_Completions);

				if(sSelected.empty() || DW.Console.mAutoComplete->setSelectedText(sSelected))
				{
					// select first item.
					if(DW.Console.mAutoComplete->getSelected()==-1)
						DW.Console.mAutoComplete->setSelected(0);
				}
			}
			return m_Completions.size(); 
		}
		std::string getElementAt(int i, int column)
		{
			return m_Completions[i];
		}
		StringVector m_Completions;
	};
}

ListModels::ClientListModel clientList;
ListModels::GoalsListModel goalList;
ListModels::AutoCompleteListModel autoCompleteList;

//////////////////////////////////////////////////////////////////////////

extern "C"
{
#include "prof_internal.h"
	int int_to_string_has_init();
	void int_to_string_init();
	void float_to_string(char *buf, float num, int precision);
	int get_colors(float factor,float text_color_ret[3],float glow_color_ret[3],float *glow_alpha_ret);
};

namespace gcn
{
	class Profiler : public Widget, public KeyListener
	{
	public:
		Profiler()
		{
			setFocusable(true);
			addKeyListener(this);
			setFrameSize(1);
		}

		void keyPressed(KeyEvent& keyEvent)
		{
			Key key = keyEvent.getKey();

			switch(key.getValue())
			{
			case Key::LEFT:
				Prof_select_parent();
				keyEvent.consume();
				break;
			case Key::RIGHT:
				Prof_select();
				keyEvent.consume();
				break;
			case Key::UP:
				Prof_move_cursor(-1);
				keyEvent.consume();
				break;
			case Key::DOWN:
				Prof_move_cursor(1);
				keyEvent.consume();
				break;
			case Key::PAGE_UP:
				{
					int iMode = Prof_get_report_mode();
					iMode = (iMode+1) % Prof_Last_Mode;
					Prof_set_report_mode((Prof_Report_Mode)iMode);
					keyEvent.consume();
					break;
				}
			case Key::PAGE_DOWN:
				{
					int iMode = Prof_get_report_mode();
					iMode = (iMode-1) < 0 ? Prof_Last_Mode : iMode-1;
					Prof_set_report_mode((Prof_Report_Mode)iMode);
					keyEvent.consume();
					break;
				}			
			}
		}

		virtual void draw(Graphics* graphics)
		{
			Prof(ProfilerRender);

			graphics->setColor(getForegroundColor());
			graphics->setFont(getFont());

			int sx = getX(), sy = getY();
			int full_width = getWidth()-5, height = getHeight()/2;
			int line_spacing = getFont()->getHeight();
			int precision = 2;

			int i,j,n,o;
			int backup_sy;

			int field_width = getFont()->getWidth("5555.55");
			int name_width  = full_width - field_width * 3;
			int plus_width  = getFont()->getWidth("+");

			int max_records;

			Prof_Report *pob = NULL;

			if (!int_to_string_has_init()) 
				int_to_string_init();

			if (precision < 1) precision = 1;
			if (precision > 4) precision = 4;

			pob = Prof_create_report();

			for (i=0; i < NUM_TITLE; ++i) {
				if (pob->title[i]) 
				{
					int header_x0 = sx;
					int header_x1 = header_x0 + full_width;

					if (i == 0)
					{
						graphics->setColor(Color(25,76,0));
					}
					else
					{
						graphics->setColor(Color(51,25,51));
					}			

					graphics->fillRectangle(Rectangle(header_x0, sy+2, header_x1, sy+line_spacing+2));

					if (i == 0)
					{
						graphics->setColor(Color(153,102,0));
					}
					else
					{
						graphics->setColor(Color(204,25,25));
					}	

					graphics->drawText(pob->title[i], sx+2, sy);

					sy += (int)(1.5f*(float)line_spacing);
					height -= (int)(abs((float)line_spacing)*1.5f);
				}
			}

			max_records = height / abs(line_spacing);

			o = 0;
			n = pob->num_record;
			if (n > max_records) n = max_records;
			if (pob->hilight >= o + n) 
			{
				o = pob->hilight - n + 1;
			}

			backup_sy = sy;

			// Draw the background colors for the zone data.
			graphics->fillRectangle(Rectangle(sx, sy, sx + full_width, sy + line_spacing));
			sy += line_spacing;

			for (i = 0; i < n; i++) 
			{
				if (i & 1) 
				{
					graphics->setColor(Color(25,25,51));
				} 
				else 
				{
					graphics->setColor(Color(25,25,76));
				}
				if(i+o == pob->hilight)
				{
					graphics->setColor(Color(76,76,25));
				}

				int y0 = sy;
				int y1 = sy + line_spacing;

				graphics->fillRectangle(Rectangle(sx, y0, sx + full_width, y1));
				sy += line_spacing;
			}

			sy = backup_sy;

			graphics->setColor(Color(178,178,178));

			if (pob->header[0])
				graphics->drawText(pob->header[0], sx+8, sy);

			for (j=1; j < NUM_HEADER; ++j)
				if (pob->header[j])
					graphics->drawText(pob->header[j], sx + name_width + field_width * (j-1) + field_width/2 - getFont()->getWidth(pob->header[j])/2, sy);

			sy += line_spacing;

			for (i = 0; i < n; i++) 
			{
				char buf[256], *b = buf;
				Prof_Report_Record *r = &pob->record[i+o];
				float text_color[3], glow_color[3];
				float glow_alpha;
				int x = sx + getFont()->getWidth(" ") * r->indent + plus_width/2;
				if (r->prefix) 
				{
					buf[0] = r->prefix;
					++b;
				} 
				else 
				{
					x += plus_width;
				}
				if (r->number)
					sprintf(b, "%s (%d)", r->name, r->number);
				else
					sprintf(b, "%s", r->name);
				if (get_colors((float)r->heat, text_color, glow_color, &glow_alpha)) 
				{
					graphics->setColor(Color(
						(int)(255.f*glow_color[0]),
						(int)(255.f*glow_color[1]),
						(int)(255.f*glow_color[2]),
						(int)(255.f*glow_alpha)));
					graphics->drawText(buf, x+2, sy-1);
				}

				graphics->setColor(Color(
					(int)(255.f*text_color[0]),
					(int)(255.f*text_color[1]),
					(int)(255.f*text_color[2])));
				graphics->drawText(buf, x + 1, sy);

				for (j=0; j < NUM_VALUES; ++j) 
				{
					if (r->value_flag & (1 << j)) 
					{
						int pad;
						float_to_string(buf, (float)r->values[j], j == 2 ? 2 : precision);
						pad = field_width- plus_width - getFont()->getWidth(buf);
						if (r->indent) pad += plus_width;

						graphics->drawText(buf, sx + pad + name_width + field_width * j, sy);
					}
				}
				sy += line_spacing;
			}
			Prof_free_report(pob);
		}

		void drawBorder(Graphics* graphics)
		{
			Color faceColor = getBaseColor();
			Color highlightColor, shadowColor;
			int alpha = getBaseColor().a;
			int width = getWidth() + getFrameSize() * 2 - 1;
			int height = getHeight() + getFrameSize() * 2 - 1;
			highlightColor = faceColor + 0x303030;
			highlightColor.a = alpha;
			shadowColor = faceColor - 0x303030;
			shadowColor.a = alpha;

			for (int i = 0; i < getFrameSize(); ++i)
			{
				graphics->setColor(shadowColor);
				graphics->drawLine(i,i, width - i, i);
				graphics->drawLine(i,i + 1, i, height - i - 1);
				graphics->setColor(highlightColor);
				graphics->drawLine(width - i,i + 1, width - i, height - i);
				graphics->drawLine(i,height - i, width - i - 1, height - i);
			}
		}
	protected:
	};

	class Map : public Widget, public MouseListener, public KeyListener
	{
	public:
		Map()
		{
			setFocusable(true);
			addMouseListener(this);
			addKeyListener(this);
			setFrameSize(1);
		}

		/*void keyPressed(KeyEvent& keyEvent)
		{
		}*/

		void mouseClicked(MouseEvent& mouseEvent)
		{
			switch(mouseEvent.getButton())
			{
			case MouseEvent::LEFT:
				g_MapMouse.MouseClicked[MouseCache::Left] = true;
				break;
			case MouseEvent::MIDDLE:
				g_MapMouse.MouseClicked[MouseCache::Mid] = true;
				break;
			case MouseEvent::RIGHT:
				g_MapMouse.MouseClicked[MouseCache::Right] = true;
				break;
			}
		}

		void mouseWheelMovedUp(MouseEvent& mouseEvent)
		{
			g_MapMouse.WheelMoved = -1;
		}

		void mouseWheelMovedDown(MouseEvent& mouseEvent)
		{
			g_MapMouse.WheelMoved = 1;
		}

		void mouseMoved(MouseEvent& mouseEvent)
		{
			g_MapMouse.X = mouseEvent.getX();
			g_MapMouse.Y = mouseEvent.getY();
		}

		virtual void draw(Graphics* graphics)
		{
			Prof(map_draw);
			graphics->setColor(getForegroundColor());
			graphics->setFont(getFont());			
			NavigationManager::GetInstance()->GetCurrentPathPlanner()->RenderToMapViewPort(this,graphics);
			g_MapMouse.Reset();
			DrawBuffer::Flush(graphics);
			DrawBuffer::Reset();
		}

		void drawBorder(Graphics* graphics)
		{
			Color faceColor = getBaseColor();
			Color highlightColor, shadowColor;
			int alpha = getBaseColor().a;
			int width = getWidth() + getFrameSize() * 2 - 1;
			int height = getHeight() + getFrameSize() * 2 - 1;
			highlightColor = faceColor + 0x303030;
			highlightColor.a = alpha;
			shadowColor = faceColor - 0x303030;
			shadowColor.a = alpha;

			for (int i = 0; i < getFrameSize(); ++i)
			{
				graphics->setColor(shadowColor);
				graphics->drawLine(i,i, width - i, i);
				graphics->drawLine(i,i + 1, i, height - i - 1);
				graphics->setColor(highlightColor);
				graphics->drawLine(width - i,i + 1, width - i, height - i);
				graphics->drawLine(i,height - i, width - i - 1, height - i);
			}
		}
	protected:		
	};

	class RayVision : public Widget
	{
	public:
		enum { R_WIDTH = 150, R_HEIGHT = 100 };

		virtual void draw(Graphics* graphics)
		{
			Prof(rayvision_draw);

			ClientPtr cl = IGameManager::GetInstance()->GetGame()->GetDebugWindowClient(g_SelectedClient);
			if(cl)
			{
				Prof(rayvision_raytrace);

				Vector3f vEnd, vEye = cl->GetEyePosition();
				
				Vector3f vFacing, vRight, vUp;
				EngineFuncs::EntityOrientation(cl->GetGameEntity(), vFacing, vRight, vUp);
				Matrix3f orien = Matrix3f(vRight, vFacing, vUp, true);

				obTraceResult tr;

				static float YAWSPAN = Mathf::DegToRad(45.f);
				static float PITCHSPAN = Mathf::DegToRad(45.f);
				static float PROBEDIST = 2048.f;

				Quaternionf qhl, qhr;
				qhl.FromAxisAngle(Vector3f::UNIT_Z, YAWSPAN);
				qhr.FromAxisAngle(Vector3f::UNIT_Z, -YAWSPAN);

				Quaternionf qvt, qvb;
				qvt.FromAxisAngle(Vector3f::UNIT_X, -PITCHSPAN);
				qvb.FromAxisAngle(Vector3f::UNIT_X, PITCHSPAN);

				bool bDraw = false;
				static int NEXT_DRAW = 0;
				if(IGame::GetTime() > NEXT_DRAW)
					bDraw = true;

				for(int h = 0; h < R_HEIGHT; ++h)
				{
					for(int w = 0; w < R_WIDTH; ++w)
					{
						Quaternionf qy, qp;
						qy.Slerp((float)w/(float)(R_WIDTH-1), qhl, qhr);
						qp.Slerp((float)h/(float)(R_HEIGHT-1), qvt, qvb);
						
						vEnd = vEye + (qy*qp).Rotate(vFacing) * PROBEDIST;							

						if(bDraw)
						{
							NEXT_DRAW = IGame::GetTime() + 2000;

							if(h==0 && w==0)
							{
								Utils::DrawLine(vEye,vEnd,COLOR::BLUE,2.f);
							}
							else if(h==R_HEIGHT-1 && w==R_WIDTH-1)
							{
								Utils::DrawLine(vEye,vEnd,COLOR::BLUE,2.f);
							}
							else if(h==0 && w==R_WIDTH-1)
							{
								Utils::DrawLine(vEye,vEnd,COLOR::BLUE,2.f);
							}
							else if(h==R_HEIGHT-1 && w==0)
							{
								Utils::DrawLine(vEye,vEnd,COLOR::BLUE,2.f);
							}
						}						

						EngineFuncs::TraceLine(tr,vEye,vEnd,0,TR_MASK_SHOT,cl->GetGameID(),False);

						obuint8 z = (obuint8)(255.f * ClampT(tr.m_Fraction, 0.f, 1.f));
						m_Pixels[h*R_WIDTH+w].c = gcn::Color(z,z,z);
					}
				}
			}
			{
				Prof(rayvision_draw);
				graphics->drawPoints(m_Pixels, R_WIDTH*R_HEIGHT);
			}
		}

		void drawBorder(Graphics* graphics)
		{
			Color faceColor = getBaseColor();
			Color highlightColor, shadowColor;
			int alpha = getBaseColor().a;
			int width = getWidth() + getFrameSize() * 2 - 1;
			int height = getHeight() + getFrameSize() * 2 - 1;
			highlightColor = faceColor + 0x303030;
			highlightColor.a = alpha;
			shadowColor = faceColor - 0x303030;
			shadowColor.a = alpha;

			for (int i = 0; i < getFrameSize(); ++i)
			{
				graphics->setColor(shadowColor);
				graphics->drawLine(i,i, width - i, i);
				graphics->drawLine(i,i + 1, i, height - i - 1);
				graphics->setColor(highlightColor);
				graphics->drawLine(width - i,i + 1, width - i, height - i);
				graphics->drawLine(i,height - i, width - i - 1, height - i);
			}
		}

		RayVision()
		{
			setFrameSize(1);
			for(int h = 0; h < R_HEIGHT; ++h)
			{
				for(int w = 0; w < R_WIDTH; ++w)
				{
					m_Pixels[h*150+w].x = w;
					m_Pixels[h*150+w].y = h;
					m_Pixels[h*150+w].c = gcn::Color(255,0,255);
				}
			}
		}
		~RayVision()
		{
		}
	protected:
		gcn::Graphics::Point m_Pixels[R_WIDTH*R_HEIGHT];
	};
	//////////////////////////////////////////////////////////////////////////

	StateTree::StateTree()
	{
		setFocusable(true);
		addMouseListener(this);
		addKeyListener(this);
		setFrameSize(1);
	}

	void StateTree::logic()
	{
		Widget::logic();

		int iSelected = DW.StateTree.mClientList->getSelected();
		ClientPtr cl = IGameManager::GetInstance()->GetGame()->GetDebugWindowClient(iSelected);
		if(cl)
			g_SelectedClient = iSelected;
		else
			g_SelectedClient = -1;
	}

	void StateTree::draw(Graphics* graphics)
	{
		Prof(statetree_draw);

		if(g_SelectedClient==-1)
			return;

		ClientPtr cl = IGameManager::GetInstance()->GetGame()->GetDebugWindowClient(g_SelectedClient);
		if(!cl)
		{
			g_SelectedClient = -1;
			return;
		}

		graphics->setColor(getForegroundColor());
		graphics->setFont(getFont());

		int iLine = 0;
		DrawInfo di(this,graphics,iLine,0);
		di.Mouse = mMouse;
		cl->GetStateRoot()->RenderDebugWindow(di);
		mMouse.Reset();

		setHeight(di.m_Line * getFont()->getHeight());
	}

	void StateTree::drawBorder(Graphics* graphics)
	{
		Color faceColor = getBaseColor();
		Color highlightColor, shadowColor;
		int alpha = getBaseColor().a;
		int width = getWidth() + getFrameSize() * 2 - 1;
		int height = getHeight() + getFrameSize() * 2 - 1;
		highlightColor = faceColor + 0x303030;
		highlightColor.a = alpha;
		shadowColor = faceColor - 0x303030;
		shadowColor.a = alpha;

		for (int i = 0; i < getFrameSize(); ++i)
		{
			graphics->setColor(shadowColor);
			graphics->drawLine(i,i, width - i, i);
			graphics->drawLine(i,i + 1, i, height - i - 1);
			graphics->setColor(highlightColor);
			graphics->drawLine(width - i,i + 1, width - i, height - i);
			graphics->drawLine(i,height - i, width - i - 1, height - i);
		}
	}

	void StateTree::mouseEntered(MouseEvent& mouseEvent)
	{
	}

	void StateTree::mouseExited(MouseEvent& mouseEvent)
	{
	}

	void StateTree::mousePressed(MouseEvent& mouseEvent)
	{
	}

	void StateTree::mouseReleased(MouseEvent& mouseEvent)
	{
	}

	void StateTree::mouseClicked(MouseEvent& mouseEvent)
	{
		switch(mouseEvent.getButton())
		{
		case MouseEvent::LEFT:
			mMouse.MouseClicked[MouseCache::Left] = true;
			break;
		case MouseEvent::MIDDLE:
			mMouse.MouseClicked[MouseCache::Mid] = true;
			break;
		case MouseEvent::RIGHT:
			mMouse.MouseClicked[MouseCache::Right] = true;
			break;
		}
	}

	void StateTree::mouseWheelMovedUp(MouseEvent& mouseEvent)
	{
		mMouse.WheelMoved = -1;
	}

	void StateTree::mouseWheelMovedDown(MouseEvent& mouseEvent)
	{
		mMouse.WheelMoved = 1;
	}

	void StateTree::mouseMoved(MouseEvent& mouseEvent)
	{
		mMouse.X = mouseEvent.getX();
		mMouse.Y = mouseEvent.getY();
	}

	void StateTree::mouseDragged(MouseEvent& mouseEvent)
	{
	}
}

//////////////////////////////////////////////////////////////////////////
gcn::Button *DebugWindow_s::AddPageButton(const String &_btntext)
{
	gcn::Button *btn = new gcn::Button(_btntext);
	btn->setPosition(mButtonXOffset, 0);
	btn->addActionListener(&dwActionListener);
	btn->setY(mTop->getHeight()-btn->getHeight());
	mButtonXOffset += btn->getWidth();
	mTop->add(btn);
	return btn;
}

gcn::Window *DebugWindow_s::AddPageWindow(const String &_wintext, gcn::Button *togglebutton)
{
	gcn::Window *w = new gcn::Window(_wintext);
	w->setDimension(gcn::Rectangle(0,0,mTop->getWidth(), mTop->getHeight() - mTop->getFont()->getHeight() * 2));
	w->setFrameSize(1);

	if(togglebutton)
		dwActionListener.addToggleKeyMap(togglebutton,0,false,false,w,NULL);
	mTop->add(w);
	return w;
}

void DebugWindow_s::Init(const WindowProps &_mainwindow)
{
	//////////////////////////////////////////////////////////////////////////
	Script.Init(_mainwindow);
	Map.Init(_mainwindow);	
	StateTree.Init(_mainwindow);
	Log.Init(_mainwindow);
	Console.Init(_mainwindow);
	Profiler.Init(_mainwindow);
	Goals.Init(_mainwindow);
	Hud.Init(_mainwindow);
	Nav.Init(_mainwindow);

	//////////////////////////////////////////////////////////////////////////
	// Create the page buttons
	mButtonXOffset = 0;

	mConsoleBtn = AddPageButton("Console");
	mProfilerBtn = AddPageButton("Profiler");
	mStateTreeBtn = AddPageButton("StateTree");
	mMapBtn = AddPageButton("Map");
	mLogBtn = AddPageButton("Log");
	mScriptBtn = AddPageButton("Script");
	mGoalsBtn = AddPageButton("Goals");
	mHudBtn = AddPageButton("HUD");
	mNavBtn = AddPageButton("Nav");

	dwActionListener.addToggleKeyMap(mConsoleBtn, gcn::Key::ALT_GR, true, false, Console.mContainer, "ShowConsole");
	dwActionListener.addToggleKeyMap(mGoalsBtn, gcn::Key::F1, true, false, Goals.mContainer, "ShowGoals");
	dwActionListener.addToggleKeyMap(mProfilerBtn, gcn::Key::F2, true, false, Profiler.mContainer, "ShowProfiler");
	dwActionListener.addToggleKeyMap(mMapBtn, gcn::Key::F3, true, false, Map.mContainer, "ShowMap");
	dwActionListener.addToggleKeyMap(mStateTreeBtn, gcn::Key::F4, true, false, StateTree.mContainer, "ShowStateTree");
	dwActionListener.addToggleKeyMap(mLogBtn, gcn::Key::F5, true, false, Log.mContainer, "ShowLog");
	dwActionListener.addToggleKeyMap(mScriptBtn, gcn::Key::F6, true, false, Script.mContainer, "ShowScript");
	dwActionListener.addToggleKeyMap(mHudBtn, gcn::Key::F7, true, false, Hud.mContainer, "ShowHud");
	dwActionListener.addToggleKeyMap(mNavBtn, gcn::Key::F8, true, false, Nav.mAdjCtr, "ShowNav");

	//////////////////////////////////////////////////////////////////////////
}

void DebugWindow_s::Shutdown()
{
	OB_DELETE(mHudBtn);
	OB_DELETE(mGoalsBtn);
	OB_DELETE(mScriptBtn);
	OB_DELETE(mProfilerBtn);
	OB_DELETE(mStateTreeBtn);
	OB_DELETE(mMapBtn);
	OB_DELETE(mLogBtn);
	OB_DELETE(mConsoleBtn); 
	OB_DELETE(mNavBtn);

	Hud.Shutdown();
	Goals.Shutdown();
	Profiler.Shutdown();
	Console.Shutdown();
	Log.Shutdown();
	StateTree.Shutdown();
	Map.Shutdown();
	Script.Shutdown();
	Nav.Shutdown();

	OB_DELETE(mTop);

	OB_DELETE(Core.mGui);
	OB_DELETE(Core.mFont);
	OB_DELETE(Core.mInput);
	OB_DELETE(Core.mGraphics);
	OB_DELETE(Core.mImageLoader);
	OB_DELETE(Core.mMainWindow);	
}

void DebugWindow_s::Update()
{
	Goals.Update();
	Profiler.Update();
	Console.Update();
	Log.Update();
	StateTree.Update();
	Map.Update();
	Script.Update();
	Hud.Update();
	Nav.Update();
}

//////////////////////////////////////////////////////////////////////////

void DebugWindow_s::Profiler_s::Init(const WindowProps &_mainwindow)
{
	using namespace gcn;

	mContainer = new Window("Profiler");
	mContainer->setDimension(gcn::Rectangle(0,0,_mainwindow.mWidth, _mainwindow.mHeight));
	mContainer->setFrameSize(1);
	//mContainer->setMovable(false);

	mProfiler = new gcn::Profiler();
	mProfiler->setDimension(gcn::Rectangle(0,0,_mainwindow.mWidth, _mainwindow.mHeight));
	mContainer->add(mProfiler);

	DW.mTop->add(mContainer);
}
void DebugWindow_s::Profiler_s::Shutdown()
{
	OB_DELETE(mContainer);
	OB_DELETE(mProfiler);	
}

//////////////////////////////////////////////////////////////////////////
MapGoalPtr gSelectedMapGoal;
void DebugWindow_s::Hud_s::SetActiveMapGoal(MapGoalPtr mg)
{
	if(mPropertySheet)
	{
		if(gSelectedMapGoal != mg)
		{
			if(gSelectedMapGoal)
				gSelectedMapGoal->DeAllocGui();

			gSelectedMapGoal = mg;

			mPropertySheet->clear();

			if(gSelectedMapGoal)
			{
				gSelectedMapGoal->HudDisplay();
			}
		}

		if(gSelectedMapGoal)
		{
			gSelectedMapGoal->UpdatePropertiesToGui();
		}
	}
}

void DebugWindow_s::Hud_s::Init(const WindowProps &_mainwindow)
{
	using namespace gcn;

	const int fontHeight = DW.mTop->getFont()->getHeight();

	mContainer = new Window("HUD");
	mContainer->setDimension(gcn::Rectangle(0,0,_mainwindow.mWidth, _mainwindow.mHeight));
	mContainer->setFrameSize(1);
	//mContainer->setMovable(false);

	mPropertySheet = new gcn::contrib::PropertySheet;
	mScrollArea = new gcn::ScrollArea(mPropertySheet);
	mScrollArea->setVerticalScrollPolicy(ScrollArea::SHOW_ALWAYS);
	mScrollArea->setSize(_mainwindow.mWidth, _mainwindow.mHeight - fontHeight);
	mScrollArea->setOpaque(false);
	mContainer->add(mScrollArea);

	DW.mTop->add(mContainer);

	gmMachine *machine = ScriptManager::GetInstance()->GetMachine();
	mUserObject = gmBind2::Class<gcn::Window>::WrapObject(machine,mContainer);
}
void DebugWindow_s::Hud_s::Shutdown()
{
	mUserObject = NULL;
	OB_DELETE(mContainer);
	OB_DELETE(mPropertySheet);
	OB_DELETE(mScrollArea);

	if(gSelectedMapGoal)
		gSelectedMapGoal->DeAllocGui();

	gSelectedMapGoal.reset();
}

void DebugWindow_s::Hud_s::Update()
{	
}

//////////////////////////////////////////////////////////////////////////

void DebugWindow_s::Nav_s::Init(const WindowProps &_mainwindow)
{
	using namespace gcn;

	mAdjCtr = new gcn::contrib::AdjustingContainer();
	
	//mContainer->setMovable(false);

	DW.mTop->add(mAdjCtr);

	// TEMP
	NavigationManager::GetInstance()->GetCurrentPathPlanner()->CreateGui();

}
void DebugWindow_s::Nav_s::Shutdown()
{
	OB_DELETE(mAdjCtr);
}

void DebugWindow_s::Nav_s::Update()
{
	NavigationManager::GetInstance()->GetCurrentPathPlanner()->UpdateGui();
}

//////////////////////////////////////////////////////////////////////////

namespace ScriptThreads
{
	struct ThreadInfo
	{
		enum	{ BufferSize=128 };
		char	mFunction[BufferSize];
		char	mThis[BufferSize];
		char	mFile[BufferSize];
		float	mTime;
		float	mPeakTime;
		int		mLineNum;
		int		mState;
	};

	std::vector<ThreadInfo>	Threads;

	bool ThreadSort(const ThreadInfo &_p1, const ThreadInfo &_p2)
	{
		return _p1.mPeakTime > _p2.mPeakTime;
	}

	//////////////////////////////////////////////////////////////////////////

	bool ThreadListIter(gmThread * a_thread, void * a_context);
	class ThreadListModel : public gcn::ListModel
	{
	public:
		enum Columns
		{
			ThreadCurrentState,
			ThreadFunction,
			ThreadThis,
			ThreadFile,
#if(GM_USE_THREAD_TIMERS)
			ThreadTime,
#endif
			NumColumns
		};

		virtual int getNumberOfColumns() { return NumColumns; };

		virtual bool getColumnTitle(int column, std::string &title, int &columnwidth)
		{
			switch(column)
			{
			case ThreadCurrentState:
				columnwidth = 70;
				title = "State";
				break;
			case ThreadFunction:
				columnwidth = 150;
				title = "Function";
				break;
			case ThreadThis:
				columnwidth = 350;
				title = "'this'";
				break;
			case ThreadFile:
				columnwidth = 200;
				title = "File";
				break;
#if(GM_USE_THREAD_TIMERS)
			case ThreadTime:
				columnwidth = 200;
				title = "Time (last/peak)";
				break;
#endif
			}
			return true;
		}

		virtual int getColumnBorderSize() { return 1; };

		virtual int getNumberOfElements()
		{
			static int lastTime = 0;
			if(IGame::GetTime() != lastTime)
			{
				gmMachine *machine = ScriptManager::GetInstance()->GetMachine();
				Threads.resize(0);
				machine->ForEachThread(ThreadListIter,this);
				lastTime = IGame::GetTime();

#if(GM_USE_THREAD_TIMERS)
				std::sort(Threads.begin(), Threads.end(), ThreadSort);
#endif
			}
			return (int)Threads.size();
		}

		std::string getElementAt(int i, int column)
		{
			switch(column)
			{
			case ThreadCurrentState:
				{
					switch(Threads[i].mState)
					{
					case gmThread::RUNNING:
						return "Running";
					case gmThread::SLEEPING:
						return "Sleeping";
					case gmThread::BLOCKED:
						return "Blocked";
					case gmThread::EXCEPTION:
						return "Exception";
					case gmThread::KILLED:
						return "Killed";
					}
					break;
				}
			case ThreadFunction:
				{
					return Threads[i].mFunction;
				}
			case ThreadThis:
				{
					return Threads[i].mThis;
				}
			case ThreadFile:
				{
					return Threads[i].mFile;
				}
#if(GM_USE_THREAD_TIMERS)
			case ThreadTime:
				{
					return va("%.3f / %.3f", Threads[i].mTime, Threads[i].mPeakTime).c_str();
				}
#endif
			}
			return std::string("");
		}
	};
	//////////////////////////////////////////////////////////////////////////
	bool ThreadListIter(gmThread * a_thread, void * a_context)
	{
		ThreadInfo currentTi;
		// state
		currentTi.mState = a_thread->GetState();
		// func name
		const gmFunctionObject *pFn = a_thread->GetFunctionObject();
		const char *pFuncName = pFn ? pFn->GetDebugName() : 0;
		Utils::StringCopy(currentTi.mFunction, pFuncName?pFuncName:"<unknown>",ThreadInfo::BufferSize);
		// 'this'
		a_thread->GetThis()->AsString(a_thread->GetMachine(),currentTi.mThis,ThreadInfo::BufferSize);
		// file
		const char *pSource = 0, *pFileName = 0;
		if ( pFn ) {
			a_thread->GetMachine()->GetSourceCode(pFn->GetSourceId(), pSource, pFileName);
		}
		// trim path, just leave filename
		if(pFileName)
		{
			const char * ptr = pFileName;
			while(*ptr)
			{
				if(*ptr == '/')
					pFileName = ptr+1;
				++ptr;
			}
		}
		Utils::VarArgs(currentTi.mFile,ThreadInfo::BufferSize,"%s",pFileName?pFileName:"<unknown file>");
#if(GM_USE_THREAD_TIMERS)
		currentTi.mTime = a_thread->GetRealThreadTime().LastExecTime;
		currentTi.mPeakTime = a_thread->GetRealThreadTime().PeakTime;
#endif
		Threads.push_back(currentTi);
		return true;
	}
	ThreadListModel threadListModel;
};
void DebugWindow_s::Script_s::Init(const WindowProps &_mainwindow)
{
	using namespace gcn;

	mContainer = new Window("Scripting");
	mContainer->setDimension(gcn::Rectangle(0,0,_mainwindow.mWidth, _mainwindow.mHeight));
	mContainer->setFrameSize(1);
	//mContainer->setMovable(false);

	mThreadList = new ListBox(&ScriptThreads::threadListModel);
	mThreadList->setDimension(gcn::Rectangle(0,0,_mainwindow.mWidth,_mainwindow.mHeight));
	mThreadList->setFrameSize(0);
	mThreadList->addActionListener(&dwActionListener);
	mThreadList->setFocusable(true);

	mOutputScroll = new ScrollArea(mThreadList);
	mOutputScroll->setFrameSize(0);
	mOutputScroll->setDimension(
		gcn::Rectangle(
		0,
		0,
		mContainer->getWidth(),
		mContainer->getHeight() - mContainer->getFont()->getHeight() * 4));

	mContainer->add(mOutputScroll);

	DW.mTop->add(mContainer);
}
void DebugWindow_s::Script_s::Shutdown()
{	
	OB_DELETE(mThreadList);
	OB_DELETE(mOutputScroll);
	OB_DELETE(mContainer);
}
//////////////////////////////////////////////////////////////////////////

void DebugWindow_s::Log_s::Init(const WindowProps &_mainwindow)
{
	using namespace gcn;

	mContainer = new Window("Log");
	mContainer->setDimension(gcn::Rectangle(0,0,_mainwindow.mWidth, _mainwindow.mHeight));
	mContainer->setFrameSize(1);
	//mContainer->setMovable(false);

	mOutput = new TextBox("");
	mOutput->setEditable(false);
	mOutput->setOpaque(false);
	//mOutput->setBackgroundColor(inputColor);
	mOutput->setFrameSize(0);

	mOutputScroll = new ScrollArea(mOutput);
	mOutputScroll->setFrameSize(0);
	mOutputScroll->setDimension(
		gcn::Rectangle(
		0,
		0,
		mContainer->getWidth(),
		mContainer->getHeight() - mContainer->getFont()->getHeight() * 4));

	mContainer->add(mOutputScroll);

	mCbNormal = new gcn::CheckBox("Normal", true);
	mCbInfo = new gcn::CheckBox("Info", true);
	mCbWarning = new gcn::CheckBox("Warning", true);
	mCbError = new gcn::CheckBox("Error", true);
	mCbDebug = new gcn::CheckBox("Debug", true);
	mCbScript = new gcn::CheckBox("Script", false);
	
	int xPos = mOutputScroll->getX();
	int yPos = mOutputScroll->getY() + mOutputScroll->getHeight() + 2;
	mCbNormal->setPosition(xPos, yPos);
	xPos += mCbNormal->getWidth();
	mCbInfo->setPosition(xPos, yPos);
	xPos += mCbInfo->getWidth();
	mCbWarning->setPosition(xPos, yPos);
	xPos += mCbWarning->getWidth();
	mCbError->setPosition(xPos, yPos);
	xPos += mCbError->getWidth();
	mCbDebug->setPosition(xPos, yPos);
	xPos += mCbDebug->getWidth();
	mCbScript->setPosition(xPos, yPos);
	xPos += mCbScript->getWidth();

	mContainer->add(mCbNormal);
	mContainer->add(mCbInfo);
	mContainer->add(mCbWarning);
	mContainer->add(mCbError);
	mContainer->add(mCbDebug);
	mContainer->add(mCbScript);

	DW.mTop->add(mContainer);
}
void DebugWindow_s::Log_s::Shutdown()
{
	OB_DELETE(mCbScript);
	OB_DELETE(mCbNormal);
	OB_DELETE(mCbInfo);
	OB_DELETE(mCbWarning);
	OB_DELETE(mCbError);
	OB_DELETE(mCbDebug);
	OB_DELETE(mOutput);
	OB_DELETE(mOutputScroll);
	OB_DELETE(mContainer);
}
void DebugWindow_s::Log_s::AddLine(eMessageType _type, const String &_s)
{
	if(mOutput)
	{
		if(_type==kNormal && !mCbNormal->isSelected())
			return;
		if(_type==kInfo && !mCbInfo->isSelected())
			return;
		if(_type==kWarning && !mCbWarning->isSelected())
			return;
		if(_type==kError && !mCbError->isSelected())
			return;
		if(_type==kDebug && !mCbDebug->isSelected())
			return;
		if(_type==kScript && !mCbScript->isSelected())
			return;

		String s = _s;
		Utils::StringTrimCharacters(s, "\r\n");
		mOutput->addRow(s);
		mOutputScroll->setVerticalScrollAmount(mOutputScroll->getVerticalMaxScroll());
	}
}
void DebugWindow_s::Log_s::AddLine(eMessageType _type, const char* _msg, ...)
{
	if(mOutput)
	{
		if(_type==kNormal && !mCbNormal->isSelected())
			return;
		if(_type==kInfo && !mCbInfo->isSelected())
			return;
		if(_type==kWarning && !mCbWarning->isSelected())
			return;
		if(_type==kError && !mCbError->isSelected())
			return;
		if(_type==kDebug && !mCbDebug->isSelected())
			return;
		if(_type==kScript && !mCbScript->isSelected())
			return;

		char buffer[8192] = {0};
		va_list list;
		va_start(list, _msg);
#ifdef WIN32
		_vsnprintf(buffer, 8192, _msg, list);	
#else
		vsnprintf(buffer, 8192, _msg, list);
#endif
		va_end(list);

		String s = buffer;
		s += "\n";
		Utils::StringTrimCharacters(s, "\r\n");
		mOutput->addRow(s);
		mOutputScroll->setVerticalScrollAmount(mOutputScroll->getVerticalMaxScroll());
	}
}

//////////////////////////////////////////////////////////////////////////

void DebugWindow_s::Map_s::Init(const WindowProps &_mainwindow)
{
	using namespace gcn;

	mContainer = new Window("Map");
	mContainer->setDimension(gcn::Rectangle(0,0,_mainwindow.mWidth, _mainwindow.mHeight));
	mContainer->setFrameSize(1);
	//mContainer->setMovable(false);

	mMap = new gcn::Map();
	mMap->setDimension(gcn::Rectangle(0,0,_mainwindow.mWidth, _mainwindow.mHeight));
	mContainer->add(mMap);

	DW.mTop->add(mContainer);

	mMap->addActionListener(&mapActionListener);
	mMap->addKeyListener(&mapActionListener);
	mMap->addMouseListener(&mapActionListener);
}
void DebugWindow_s::Map_s::Shutdown()
{
	OB_DELETE(mContainer);
	OB_DELETE(mMap);
}

//////////////////////////////////////////////////////////////////////////

void DebugWindow_s::Goals_s::Init(const WindowProps &_mainwindow)
{
	using namespace gcn;
	using namespace gcn::contrib;

	//const int GOALS_WIDTH = 300;

	mContainer = new Window("Goals");
	mContainer->setDimension(gcn::Rectangle(0,0,_mainwindow.mWidth, _mainwindow.mHeight));
	mContainer->setFrameSize(1);
	//mContainer->setMovable(false);

	mGoalList = new ListBox(&goalList);
	mGoalList->setPosition(0,0);
	mGoalList->setSize(_mainwindow.mWidth, _mainwindow.mHeight/2);
	mGoalList->setFrameSize(0);
	mGoalList->addActionListener(&dwActionListener);
	mGoalList->setFocusable(true);

	mGoalInfoContainer = new AdjustingContainer();
	mGoalInfoContainer->setNumberOfColumns(1);
	mGoalInfoContainer->setColumnAlignment(0,AdjustingContainer::LEFT);
	//mGoalInfoContainer->addActionListener(&dwActionListener);
	mGoalInfoContainer->setFocusable(true);

	mInfoScrollArea = new ScrollArea(mGoalInfoContainer);
	mInfoScrollArea->setPosition(0,_mainwindow.mHeight/2);
	mInfoScrollArea->setSize(_mainwindow.mWidth, _mainwindow.mHeight/2);

	mContainer->add(mGoalList);
	mContainer->add(mInfoScrollArea);
	DW.mTop->add(mContainer);
}
void DebugWindow_s::Goals_s::Shutdown()
{
	OB_DELETE(mGoalInfoContainer);
	OB_DELETE(mInfoScrollArea);
	OB_DELETE(mGoalList);
	OB_DELETE(mContainer);
}

void DebugWindow_s::Goals_s::Update()
{
	if(!mContainer || !mContainer->isVisible())
		return;

	const int iSelected = mGoalList->getSelected();
	const MapGoalList &mgl = goalList.m_GoalList;

	if(iSelected<0 || iSelected>=(int)mgl.size())
		return;
	
	MapGoalPtr mg = mgl[iSelected];

	/*if(mg != g_LastMapGoal)
	{
		if(g_LastMapGoal)
		{
			g_LastMapGoal->DeAllocGui();
		}
		g_LastMapGoal = mg;
		if(g_LastMapGoal)
		{
			mg->CheckPropertiesBound();
			g_LastMapGoal->AllocGui();

			mGoalInfoContainer->clear();
			mg->GatherProperties(mGoalInfoContainer);
		}
	}
	
	mg->UpdatePropertiesToGui();*/
	mGoalInfoContainer->adjustContent();
}

//////////////////////////////////////////////////////////////////////////

void DebugWindow_s::StateTree_s::Init(const WindowProps &_mainwindow)
{
	using namespace gcn;

	const int CLIENT_WIDTH = 150;
	const int CLIENT_HEIGHT = 300;
	const int TREE_PANE_WIDTH = 450;
	const int BOTTOM_BUFFER = 40;

	mContainer = new Window("StateTree");
	mContainer->setDimension(gcn::Rectangle(0,0,_mainwindow.mWidth, _mainwindow.mHeight));
	mContainer->setFrameSize(1);
	//mContainer->setMovable(false);

	mClientList = new ListBox(&clientList);
	mClientList->setDimension(gcn::Rectangle(_mainwindow.mWidth-CLIENT_WIDTH,0,CLIENT_WIDTH,_mainwindow.mHeight));
	mClientList->setFrameSize(0);
	mClientList->addActionListener(&dwActionListener);
	mClientList->setFocusable(true);
	
	mClientScroll = new gcn::ScrollArea(mClientList);
	mClientScroll->setDimension(gcn::Rectangle(_mainwindow.mWidth-CLIENT_WIDTH,0,CLIENT_WIDTH,CLIENT_HEIGHT));
	mClientScroll->setFrameSize(1);

	mStateTree = new gcn::StateTree();
	mStateTree->setDimension(gcn::Rectangle(0,0,TREE_PANE_WIDTH,_mainwindow.mHeight-BOTTOM_BUFFER));
	mStateTree->setFrameSize(1);
	
	mStateScroll = new gcn::ScrollArea(mStateTree);
	mStateScroll->setDimension(gcn::Rectangle(0,0,TREE_PANE_WIDTH,_mainwindow.mHeight-BOTTOM_BUFFER));
	mStateScroll->setWidth(TREE_PANE_WIDTH);
	mStateScroll->setFrameSize(1);

	mContainer->add(mStateScroll);
	mContainer->add(mClientScroll);

	/*RayVision *rv = new RayVision;
	rv->setSize(150,100);
	mContainer->add(rv);*/

	//////////////////////////////////////////////////////////////////////////
	using namespace AiState;
	gWeaponWindow = new WeaponSystem_DW;
	mContainer->add(gWeaponWindow);
	gSensoryWindow = new SensoryMemory_DW;
	mContainer->add(gSensoryWindow);
	gAimerWindow = new Aimer_DW;
	mContainer->add(gAimerWindow);
	//////////////////////////////////////////////////////////////////////////

	DW.mTop->add(mContainer);
}
void DebugWindow_s::StateTree_s::Shutdown()
{
	OB_DELETE(gAimerWindow);
	OB_DELETE(gSensoryWindow);
	OB_DELETE(gWeaponWindow);

	OB_DELETE(mClientScroll);
	OB_DELETE(mClientList);
	OB_DELETE(mStateScroll);
	OB_DELETE(mStateTree);
	OB_DELETE(mContainer);
}

//////////////////////////////////////////////////////////////////////////

void DebugWindow_s::Console_s::Init(const WindowProps &_mainwindow)
{
	using namespace gcn;

	const int fontHeight = DW.mTop->getFont()->getHeight();
	const int INPUT_HEIGHT = fontHeight * 3;
	const int BORDER_SIZE = 1;
	const int BOTTOM_BUFFER = fontHeight * 2;
	const int AUTOCOMPLETE_WIDTH = 300;
	const int AUTOCOMPLETE_HEIGHT = fontHeight * 15;

	mContainer = new Window("Console");
	mContainer->setSize(_mainwindow.mWidth, _mainwindow.mHeight - BOTTOM_BUFFER);
	mContainer->setFrameSize(0);
	//mContainer->setMovable(false);

	mOutput = new TextBox("");
	mOutput->setEditable(false);
	mOutput->setOpaque(false);
	//mOutput->setBackgroundColor(inputColor);
	mOutput->setFrameSize(0);
	
	mOutputScroll = new ScrollArea(mOutput);
	mOutputScroll->setFrameSize(0);
	mOutputScroll->setDimension(
		gcn::Rectangle(
		0,
		0,
		mContainer->getWidth(),
		mContainer->getHeight() - INPUT_HEIGHT));
	
	mAutoComplete = new ListBox(&autoCompleteList);
	mAutoComplete->addSelectionListener(&consoleListener);
	//mAutoComplete->setSelectionColor(inputColor);
	mAutoComplete->setFrameSize(0);	
	mAutoComplete->setSize(AUTOCOMPLETE_WIDTH,AUTOCOMPLETE_HEIGHT);
	mAutoComplete->setPosition(mOutputScroll->getWidth()-AUTOCOMPLETE_WIDTH,mOutputScroll->getHeight()-AUTOCOMPLETE_HEIGHT);
	mAutoComplete->setBackgroundColor(gcn::DefaultBgColor+gcn::Color(20,20,20));

	mInput = new TextBox("");
	mInput->setOpaque(false);
	//mInput->setBackgroundColor(inputColor);
	mInput->setFrameSize(0);

	mInputScroll = new ScrollArea(mInput);
	mInputScroll->setFrameSize(0);
	mInputScroll->setDimension(
		gcn::Rectangle(
		0,
		mOutputScroll->getHeight() + BORDER_SIZE,
		_mainwindow.mWidth,
		INPUT_HEIGHT));	
	
	mContainer->add(mOutputScroll);
	mContainer->add(mInputScroll);
	mContainer->add(mAutoComplete);

	mInput->addKeyListener(&consoleListener);

	DW.mTop->add(mContainer);

	DW.Core.mGui->addGlobalKeyListener(&dwActionListener);

	mContainer->setVisible(false);

	IGame *game = IGameManager::GetInstance()->GetGame();
	AddLine("Omni-bot - version: %s, Date: %s", game->GetVersion(), game->GetVersionDateTime());
	AddLine("Game: %s", game->GetGameName());
}
void DebugWindow_s::Console_s::Update()
{
}
void DebugWindow_s::Console_s::DumpConsoleToFile(const char *filename)
{
	File f;
	if(f.OpenForWrite(va("user/%s",filename),File::Text))
	{
		String s = mOutput->getText();
		f.WriteString(s);		
		f.Close();
	}
}

void DebugWindow_s::Console_s::ClearInput()
{
	if(mInput)
		mInput->setText("");
}

void DebugWindow_s::Console_s::ClearOutput()
{
	if(mOutput)
		mOutput->setText("");
}

void DebugWindow_s::Console_s::UpdateWordAtCaret()
{
	String autoComplete = DW.Console.mAutoComplete->getSelectedText();
	// trim off anything before last .
	
	if(!autoComplete.empty())
	{
		size_t lastDot = autoComplete.find_last_of(".");
		if(lastDot != String::npos)
			autoComplete = autoComplete.substr(lastDot);
		DW.Console.mInput->replaceWordAtCaret(autoComplete);
	}
}

void DebugWindow_s::Console_s::Shutdown()
{
	OB_DELETE(mAutoComplete);
	OB_DELETE(mContainer);
	OB_DELETE(mOutput);
	OB_DELETE(mInput);
	OB_DELETE(mOutputScroll);
	OB_DELETE(mInputScroll);
}

void DebugWindow_s::Console_s::AddLine(const String &_s)
{
	if(mOutput)
	{
		String s = _s + "\n";

		Utils::StringTrimCharacters(s, "\r");
		mOutput->setText( mOutput->getText() + s );
		mOutputScroll->setVerticalScrollAmount(mOutputScroll->getVerticalMaxScroll());
	}
}

void DebugWindow_s::Console_s::AddLine(const char* _msg, ...)
{
	if(mOutput)
	{
		char buffer[8192] = {0};
		va_list list;
		va_start(list, _msg);
#ifdef WIN32
		_vsnprintf(buffer, 8192, _msg, list);	
#else
		vsnprintf(buffer, 8192, _msg, list);
#endif
		va_end(list);

		String s = buffer;
		s += "\n";
		Utils::StringTrimCharacters(s, "\r");
		mOutput->setText( mOutput->getText() + s );
		mOutputScroll->setVerticalScrollAmount(mOutputScroll->getVerticalMaxScroll());
	}
}

//////////////////////////////////////////////////////////////////////////

//struct Triangle
//{
//	int i0,i1,i2;
//};
//
//typedef std::vector<Triangle> TriangleList;
//
//Vector3List TestVerts;
//TriangleList TestTris;
//
//void LoadObjFile(const char *_file, Vector3List &verts, TriangleList &tris)
//{
//	verts.resize(0);
//	tris.resize(0);
//
//	String line;
//
//	File f;
//	if(f.OpenForRead(_file,File::Text))
//	{		
//		while(f.ReadLine(line))
//		{
//			Vector3f v;
//			if(sscanf(line.c_str(),"v %f %f %f",&v.x,&v.y,&v.z)==3)
//			{
//				verts.push_back(v);
//			}
//
//			Triangle tri;
//			int vi0,vi1,vi2,vi3;
//			int vn0,vn1,vn2,vn3;
//			if(sscanf(line.c_str(),"f %d/%d %d/%d %d/%d %d/%d",&vi0,&vn0,&vi1,&vn1,&vi2,&vn2,&vi3,&vn3)==8)
//			{
//				vi0--; vi1--; vi2--; vi3--;
//
//				tri.i0 = vi0;
//				tri.i1 = vi1;
//				tri.i2 = vi2;
//				tris.push_back(tri);
//				tri.i0 = vi0;
//				tri.i1 = vi2;
//				tri.i2 = vi3;
//				tris.push_back(tri);
//			}
//			line.resize(0);
//		}
//	}
//}

//////////////////////////////////////////////////////////////////////////

namespace DebugWindow
{
	//////////////////////////////////////////////////////////////////////////
	void Create(int _width, int _height, int _bpp)
	{
#ifdef ENABLE_DEBUG_WINDOW
		if (gOverlay) 
		{
			EngineFuncs::ConsoleError("debug window already enabled");
			return;
		}

		switch(gOverlayType)
		{
		case OVERLAY_OPENGL:
			gOverlay = CreateOpenGLRenderOverlay();
			break;
		case OVERLAY_GAME:
			gOverlay = CreateGameRenderOverlay();
			break;
		};

		if(gOverlay != NULL && gOverlay->Initialize(_width, _height))
		{
			DW.mTop = new gcn::Container();
			DW.mTop->setDimension(gcn::Rectangle(0, 0, _width, _height));

			DW.Core.mGui = new gcn::Gui();
			DW.Core.mGui->setGraphics(DW.Core.mGraphics);
			DW.Core.mGui->setInput(DW.Core.mInput);
			DW.Core.mGui->setTop(DW.mTop);

			DW.Core.mGui->getTop()->setSize(DW.Core.mMainWindow->GetWidth(), DW.Core.mMainWindow->GetHeight());

			// Load the image font.
			try
			{
				fs::path basePath = FileSystem::GetRealPath("gui/default_font.bmp");
				DW.Core.mFont = new gcn::ImageFont(basePath.string(), " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-+/():;%&''*#=[]\"<>{}^~|_@$\\");
				OBASSERT(DW.Core.mFont,"Failed to Load Font!");
				//g_Font = new gcn::ImageFont(basePath.string(), " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz{|}~");
				gcn::Widget::setGlobalFont(DW.Core.mFont);
				DW.Core.mGraphics->setFont(DW.Core.mFont);
			}
			catch(const gcn::Exception &ex)
			{
				ex;
				DW.Core.mFont = 0;			
			}

			WindowProps wp;
			wp.mWidth = _width;
			wp.mHeight = _height - DW.mTop->getFont()->getHeight() * 2;
			DW.Init(wp);

			gOverlay->PostInitialize();

			// Reserve some space in the render buffers
			DrawBuffer::Init();

			int threadId = GM_INVALID_THREAD;
			ScriptManager::GetInstance()->ExecuteFile("gui.gm", threadId);
		}
#endif
	}

	void Destroy() 
	{
#ifdef ENABLE_DEBUG_WINDOW
		OB_DELETE(gOverlay);

		DrawBuffer::Shutdown();

		DW.Shutdown();
#endif
	}
	void Update()
	{
#ifdef ENABLE_DEBUG_WINDOW
		Prof(Debug_Window);
		Prof_update(gOverlay ? 1 : 0);
		if(gOverlay)
			gOverlay->Update();
		DW.Update();
		
		if(DW.Core.mGui)
		{
			Prof(input);

			sf::Event Event;
			while(DW.Core.mMainWindow->GetEvent(Event))
				DW.Core.mInput->pushInput(Event);

			DW.Core.mGui->logic();
		}
#else
		Prof_update(0);
#endif
	}
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
};

void drawCircle(gcn::Graphics *graphics, const Vector3f &p, float radius, gcn::Color color)
{
	Prof(draw_circle);
	static bool bDrawCircles = true;
	if(bDrawCircles)
	{
		if(radius==1)
		{
			DrawBuffer::Add(graphics, gcn::Graphics::Point((int)p.x, (int)p.y, color));
			return;
		}

		enum { MaxLines = 32 };
		gcn::Graphics::Line lines[MaxLines];

		static int iStepAngle = 30;

		Quaternionf q;
		q.FromAxisAngle(Vector3f::UNIT_Z, Mathf::DegToRad((float)iStepAngle));

		Vector3f vFirstPt = Vector3f::UNIT_X * radius;
		Vector3f vPt = vFirstPt;
		for(int i = 0; i < 360/iStepAngle; ++i)
		{
			Vector3f vNextPt = q.Rotate(vPt);

			DrawBuffer::Add(graphics, gcn::Graphics::Line(
				(int)(vPt.x+p.x), 
				(int)(vPt.y+p.y), 
				(int)(vNextPt.x+p.x), 
				(int)(vNextPt.y+p.y), color));

			vPt = vNextPt;
		}
	}
}

Vector3f FromWorld(const AABB &_world, const Vector3f &_wp)
{
	const int MapX = DW.Map.mMap->getX();
	const int MapY = DW.Map.mMap->getY();
	const int MapWidth = DW.Map.mMap->getWidth();
	const int MapHeight = DW.Map.mMap->getHeight();

	const Vector3f vViewPortOrigin((float)MapX, (float)(MapY+MapHeight), 0.f);
	const Vector3f vViewPortSize((float)MapWidth, (float)MapHeight, 0.f);

	Vector3f worldPos = _wp;

	// offset position
	static bool bDoOffset = true;
	if(bDoOffset)
	{
		worldPos.x += mapActionListener.mOffsetX;
		worldPos.y += mapActionListener.mOffsetY;
	}

	// modify based on zoom.
	static bool bDoZoom = true;
	if(bDoZoom)
	{
		worldPos.x *= mapActionListener.mMapZoom;
		worldPos.y *= mapActionListener.mMapZoom;
	}	

	Vector3f vRelativePosition = Vector3f::ZERO;
	vRelativePosition.x = (worldPos.x - _world.m_Mins[0]) / _world.GetAxisLength(0);
	vRelativePosition.y = (worldPos.y - _world.m_Mins[1]) / _world.GetAxisLength(1);

	Vector3f vPosition = vViewPortOrigin;
	vPosition.x += vRelativePosition.x * vViewPortSize.x;
	vPosition.y -= vRelativePosition.y * vViewPortSize.y;
	return Vector3f(vPosition.x, vPosition.y, 0.f);
}

void PathPlannerWaypoint::RenderToMapViewPort(gcn::Widget *widget, gcn::Graphics *graphics)
{
	using namespace gcn;

	AABB navextents;
	for(obuint32 i = 0; i < m_WaypointList.size(); ++i)
	{
		Waypoint *pWp = m_WaypointList[i];
		if(i==0)
			navextents.SetCenter(pWp->GetPosition());
		else
			navextents.Expand(pWp->GetPosition());
	}

	// Expand buffer to the viewports aspect ratio
	//const int MapX = DW.Map.mMap->getX();
	//const int MapY = DW.Map.mMap->getY();
	const int MapWidth = DW.Map.mMap->getWidth();
	const int MapHeight = DW.Map.mMap->getHeight();

	const float fDesiredAspectRatio = (float)MapWidth / (float)MapHeight;

	//////////////////////////////////////////////////////////////////////////

	float fCurrentAspect = navextents.GetAxisLength(0) / navextents.GetAxisLength(1);
	if(fCurrentAspect < fDesiredAspectRatio)
	{
		float fDesiredHeight = navextents.GetAxisLength(0) * fDesiredAspectRatio/fCurrentAspect;
		navextents.ExpandAxis(0, (fDesiredHeight-navextents.GetAxisLength(0)) * 0.5f);
	}
	else if(fCurrentAspect > fDesiredAspectRatio)
	{
		float fDesiredHeight = navextents.GetAxisLength(1) * fCurrentAspect/fDesiredAspectRatio;
		navextents.ExpandAxis(1, (fDesiredHeight-navextents.GetAxisLength(1)) * 0.5f);
	}

	// Expand a small edge buffer
	navextents.ExpandAxis(0, navextents.GetAxisLength(0) * 0.01f);
	navextents.ExpandAxis(1, navextents.GetAxisLength(1) * 0.01f);

	// Draw it!
	const float fRadiusScaler = MapWidth/navextents.GetAxisLength(0);

	//////////////////////////////////////////////////////////////////////////

	Color colWaypoint(255, 255, 255);
	Color colConnections(0,255,255);
	Color colEntities(0,0,0);
	Color colGoalBounds(255,127,0);
	Color colGoalRadius(0,0,0);
	Color colBot(0,0,255);
	Color colSelectedBot(0,0,255);
	Color colBotPath(0,255,0);
	Color colBotPathRadius(0,255,0);

	//////////////////////////////////////////////////////////////////////////

	{
		Prof(draw_wps);
		
		for(obuint32 i = 0; i < m_WaypointList.size(); ++i)
		{
			Waypoint *pWp = m_WaypointList[i];

			Vector3f vP = FromWorld(navextents, pWp->GetPosition());
			DrawBuffer::Add(graphics, Graphics::Point((int)vP.x, (int)vP.y, colWaypoint));

			Waypoint::ConnectionList::iterator it = pWp->m_Connections.begin();
			while(it != pWp->m_Connections.end())
			{
				Vector2f vP1 = FromWorld(navextents, pWp->GetPosition()).As2d();
				Vector2f vP2 = FromWorld(navextents, it->m_Connection->GetPosition()).As2d(); 

				DrawBuffer::Add(graphics, Graphics::Line((int)vP1.x, (int)vP1.y, (int)vP2.x, (int)vP2.y, colConnections));

				++it;
			}
		}
	}

	//if(m_DrawPlayers->value())
	{
		Prof(draw_clients);
		ClientPtr selectedPlayer;
		if(g_SelectedClient != -1)
			selectedPlayer = IGameManager::GetInstance()->GetGame()->GetDebugWindowClient(g_SelectedClient);
		
		for(int i = 0; i < Constants::MAX_PLAYERS; ++i)
		{
			ClientPtr c = IGameManager::GetInstance()->GetGame()->GetClientByIndex(i);
			if(c)
			{
				Vector3f vViewPortPos = FromWorld(navextents, c->GetPosition());
				Vector3f vViewPortFace = FromWorld(navextents, c->GetPosition() + c->GetFacingVector() * 50.f);
				
				DrawBuffer::Add(graphics, Graphics::Line((int)vViewPortPos.x, (int)vViewPortPos.y, (int)vViewPortFace.x, (int)vViewPortFace.y, colBot));
				
				if(selectedPlayer && selectedPlayer==c)
					drawCircle(graphics, vViewPortPos, 3.f+Mathf::Sin(IGame::GetTimeSecs()*3.f), colSelectedBot);
				else
					drawCircle(graphics, vViewPortPos, 2, colBot);

				//if(m_DrawBotPaths->value())
				{
					using namespace AiState;
					FINDSTATE(follow, FollowPath, c->GetStateRoot());
					if(follow)
					{
						Vector3f vLastPosition = c->GetPosition();

						Path::PathPoint ppt;
						Path pth = follow->GetCurrentPath();
						
						while(pth.GetCurrentPt(ppt) && !pth.IsEndOfPath())
						{
							Vector3f vP1 = FromWorld(navextents, vLastPosition);
							Vector3f vP2 = FromWorld(navextents, ppt.m_Pt);

							DrawBuffer::Add(graphics, Graphics::Line((int)vP1.x, (int)vP1.y, (int)vP2.x, (int)vP2.y, colBotPath));

							drawCircle(graphics, FromWorld(navextents, ppt.m_Pt), ppt.m_Radius*fRadiusScaler, colBotPath);

							vLastPosition =  ppt.m_Pt;
							pth.NextPt();
						}
					}
				}
			}
		}
	}

	{
		Prof(draw_entities);
		
		int iNumEntities = 0;
		IGame::EntityIterator ent;
		while(IGame::IterateEntity(ent))
		{
			++iNumEntities;
			Vector3f vPos;
			if(EngineFuncs::EntityPosition(ent.GetEnt().m_Entity, vPos))
			{
				Vector3f vViewPortPos = FromWorld(navextents, vPos);
				drawCircle(graphics, vViewPortPos, 
					1.f*fRadiusScaler * mapActionListener.mMapZoom, colEntities);
				
				if(Length(Vector3f((float)g_MapMouse.X, (float)g_MapMouse.Y, 0.f), vViewPortPos) < 3.f)
				{

				}
			}
		}
	}	
	
	{
		Prof(draw_goals);
		
		const MapGoalList &glist = GoalManager::GetInstance()->GetGoalList();
		MapGoalList::const_iterator mgIt = glist.begin(), mgItEnd = glist.end();
		for(; mgIt != mgItEnd; ++mgIt)
		{
			MapGoalPtr mg = (*mgIt);
			const float fRadius = mg->GetRadius();
			const Vector3f vPos = mg->GetPosition();

			drawCircle(graphics, FromWorld(navextents, vPos), Mathf::Max(2.f, fRadius*fRadiusScaler * mapActionListener.mMapZoom), colGoalRadius);

			const Box3f worldbounds = mg->GetWorldBounds();
			//if(!worldbounds.IsZero())
			{
				Vector3f vC[8];
				worldbounds.ComputeVertices( vC );

				vC[0] = FromWorld(navextents, vC[0]);
				vC[1] = FromWorld(navextents, vC[1]);
				vC[2] = FromWorld(navextents, vC[2]);
				vC[3] = FromWorld(navextents, vC[3]);

				DrawBuffer::Add(graphics, Graphics::Line((int)vC[0].x, (int)vC[0].y, (int)vC[1].x, (int)vC[1].y, colGoalBounds));
				DrawBuffer::Add(graphics, Graphics::Line((int)vC[1].x, (int)vC[1].y, (int)vC[2].x, (int)vC[2].y, colGoalBounds));
				DrawBuffer::Add(graphics, Graphics::Line((int)vC[2].x, (int)vC[2].y, (int)vC[3].x, (int)vC[3].y, colGoalBounds));
				DrawBuffer::Add(graphics, Graphics::Line((int)vC[3].x, (int)vC[3].y, (int)vC[0].x, (int)vC[0].y, colGoalBounds));
			
				//////////////////////////////////////////////////////////////////////////
				//const int mapX = g_MapMouse.X + mapActionListener.mOffsetX;
				//const int mapY = g_MapMouse.Y + mapActionListener.mOffsetY;
				//gcn::Rectangle r((int)vC[0].x, (int)vC[0].y, (int)(vC[2].x - vC[0].x), (int)(vC[3].y - vC[1].y));
				//if(r.isPointInRect(mapX, mapY))
				{
					Vector3f center = FromWorld(navextents, worldbounds.Center);
					gcn::Graphics::Alignment alignment = gcn::Graphics::CENTER;
					/*if(g_MapMouse.X > DW.Map.mMap->getWidth())
						alignment = gcn::Graphics::RIGHT;*/
					graphics->drawText(mg->GetName(), (int)center.x, (int)center.y, alignment);
				}
				//////////////////////////////////////////////////////////////////////////
			}
		}
	}
}

void PathPlannerNavMesh::RenderToMapViewPort(gcn::Widget *widget, gcn::Graphics *graphics)
{
	using namespace gcn;

	AABB navextents;
	for(obuint32 i = 0; i < m_NavSectors.size(); ++i)
	{
		const NavSector &ns = m_NavSectors[i];
		for(obuint32 p = 0; p < ns.m_Boundary.size(); ++p)
		{
			if(i==0&&p==0)
				navextents.SetCenter(ns.m_Boundary[p]);
			else
				navextents.Expand(ns.m_Boundary[p]);
		}
	}

	// Expand buffer to the viewports aspect ratio
	//const int MapX = DW.Map.mMap->getX();
	//const int MapY = DW.Map.mMap->getY();
	const int MapWidth = DW.Map.mMap->getWidth();
	const int MapHeight = DW.Map.mMap->getHeight();

	const float fDesiredAspectRatio = (float)MapWidth / (float)MapHeight;

	// TODO: Flip the axis if the map is taller than wide.

	//////////////////////////////////////////////////////////////////////////

	float fCurrentAspect = navextents.GetAxisLength(0) / navextents.GetAxisLength(1);
	if(fCurrentAspect < fDesiredAspectRatio)
	{
		float fDesiredHeight = navextents.GetAxisLength(0) * fDesiredAspectRatio/fCurrentAspect;
		navextents.ExpandAxis(0, (fDesiredHeight-navextents.GetAxisLength(0)) * 0.5f);
	}
	else if(fCurrentAspect > fDesiredAspectRatio)
	{
		float fDesiredHeight = navextents.GetAxisLength(1) * fCurrentAspect/fDesiredAspectRatio;
		navextents.ExpandAxis(1, (fDesiredHeight-navextents.GetAxisLength(1)) * 0.5f);
	}

	// Expand a small edge buffer
	navextents.ExpandAxis(0, navextents.GetAxisLength(0) * 0.01f);
	navextents.ExpandAxis(1, navextents.GetAxisLength(1) * 0.01f);

	//////////////////////////////////////////////////////////////////////////
	Vector3f vLocalPos, vLocalAim, vAimPos, vAimNormal;
	Utils::GetLocalEyePosition(vLocalPos);
	Utils::GetLocalFacing(vLocalAim);
	Utils::GetLocalAimPoint(vAimPos, &vAimNormal);
	//////////////////////////////////////////////////////////////////////////

	// Draw it!
	const float fRadiusScaler = MapWidth/navextents.GetAxisLength(0);

	//////////////////////////////////////////////////////////////////////////

	Color colWaypoint(255, 255, 255);
	Color colConnections(0,255,255);
	Color colEntities(0,0,0);
	Color colGoalBounds(255,127,0);
	Color colGoalRadius(0,0,0);
	Color colBot(0,0,255);
	Color colSelectedBot(0,0,255);
	Color colBotPath(0,255,0);
	Color colBotPathRadius(0,255,0);

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// Draw our nav sectors
	{
		Prof(draw_sectors);

		for(obuint32 i = 0; i < m_NavSectors.size(); ++i)
		{
			const NavSector &ns = m_NavSectors[i];
			/*if(i==iCurrentSector)
				continue;*/

			//////////////////////////////////////////////////////////////////////////
			if(ns.m_Boundary.size()>1)
			{
				for(obuint32 l = 1; l <= ns.m_Boundary.size(); ++l)
				{
					if(l < ns.m_Boundary.size())
					{
						Vector3f vP1 = FromWorld(navextents, ns.m_Boundary[l-1]);
						Vector3f vP2 = FromWorld(navextents, ns.m_Boundary[l]);
						DrawBuffer::Add(graphics, gcn::Graphics::Line((int)vP1.x,(int)vP1.y,(int)vP2.x,(int)vP2.y, 
							graphics->getColor()));
					}
					else
					{
						Vector3f vP1 = FromWorld(navextents, ns.m_Boundary[l-1]);
						Vector3f vP2 = FromWorld(navextents, ns.m_Boundary[0]);
						DrawBuffer::Add(graphics, gcn::Graphics::Line((int)vP1.x,(int)vP1.y,(int)vP2.x,(int)vP2.y,
							graphics->getColor()));
					}
				}
			}
			//////////////////////////////////////////////////////////////////////////
			Vector3f vMid = Vector3f::ZERO;
			for(obuint32 v = 0; v < ns.m_Boundary.size(); ++v)
				vMid += ns.m_Boundary[v];
			vMid /= (float)ns.m_Boundary.size();
		}
	}	
	//////////////////////////////////////////////////////////////////////////
	//if(m_DrawPlayers->value())
	{
		Prof(draw_clients);
		ClientPtr selectedPlayer;
		if(g_SelectedClient != -1)
			selectedPlayer = IGameManager::GetInstance()->GetGame()->GetDebugWindowClient(g_SelectedClient);

		for(int i = 0; i < Constants::MAX_PLAYERS; ++i)
		{
			ClientPtr c = IGameManager::GetInstance()->GetGame()->GetClientByIndex(i);
			if(c)
			{
				Vector3f vViewPortPos = FromWorld(navextents, c->GetPosition());
				Vector3f vViewPortFace = FromWorld(navextents, c->GetPosition() + c->GetFacingVector() * 50.f);

				DrawBuffer::Add(graphics, Graphics::Line((int)vViewPortPos.x, (int)vViewPortPos.y, (int)vViewPortFace.x, (int)vViewPortFace.y, colBot));

				if(selectedPlayer && selectedPlayer==c)
					drawCircle(graphics, vViewPortPos, 3.f+Mathf::Sin(IGame::GetTimeSecs()*3.f), colSelectedBot);
				else
					drawCircle(graphics, vViewPortPos, 2, colBot);

				//if(m_DrawBotPaths->value())
				{
					using namespace AiState;
					FINDSTATE(follow, FollowPath, c->GetStateRoot());
					if(follow)
					{
						Vector3f vLastPosition = c->GetPosition();

						Path::PathPoint ppt;
						Path pth = follow->GetCurrentPath();

						while(pth.GetCurrentPt(ppt) && !pth.IsEndOfPath())
						{
							Vector3f vP1 = FromWorld(navextents, vLastPosition);
							Vector3f vP2 = FromWorld(navextents, ppt.m_Pt);

							DrawBuffer::Add(graphics, Graphics::Line((int)vP1.x, (int)vP1.y, (int)vP2.x, (int)vP2.y, colBotPath));

							drawCircle(graphics, FromWorld(navextents, ppt.m_Pt), ppt.m_Radius*fRadiusScaler, colBotPath);

							vLastPosition =  ppt.m_Pt;
							pth.NextPt();
						}
					}
				}
			}
		}
	}

	{
		Prof(draw_entities);

		int iNumEntities = 0;
		IGame::EntityIterator ent;
		while(IGame::IterateEntity(ent))
		{
			++iNumEntities;
			Vector3f vPos;
			if(EngineFuncs::EntityPosition(ent.GetEnt().m_Entity, vPos))
			{
				Vector3f vViewPortPos = FromWorld(navextents, vPos);
				drawCircle(graphics, vViewPortPos, 
					1.f*fRadiusScaler * mapActionListener.mMapZoom, colEntities);

				if(Length(Vector3f((float)g_MapMouse.X, (float)g_MapMouse.Y, 0.f), vViewPortPos) < 3.f)
				{

				}
			}
		}
	}	

	{
		Prof(draw_goals);

		const MapGoalList &glist = GoalManager::GetInstance()->GetGoalList();
		MapGoalList::const_iterator mgIt = glist.begin(), mgItEnd = glist.end();
		for(; mgIt != mgItEnd; ++mgIt)
		{
			MapGoalPtr mg = (*mgIt);
			const float fRadius = mg->GetRadius();
			const Vector3f vPos = mg->GetPosition();

			drawCircle(graphics, FromWorld(navextents, vPos), Mathf::Max(2.f, fRadius*fRadiusScaler * mapActionListener.mMapZoom), colGoalRadius);

			const Box3f worldbounds = mg->GetWorldBounds();
			//if(!worldbounds.IsZero())
			{
				Vector3f vC[8];
				worldbounds.ComputeVertices( vC );

				vC[0] = FromWorld(navextents, vC[0]);
				vC[1] = FromWorld(navextents, vC[1]);
				vC[2] = FromWorld(navextents, vC[2]);
				vC[3] = FromWorld(navextents, vC[3]);

				DrawBuffer::Add(graphics, Graphics::Line((int)vC[0].x, (int)vC[0].y, (int)vC[1].x, (int)vC[1].y, colGoalBounds));
				DrawBuffer::Add(graphics, Graphics::Line((int)vC[1].x, (int)vC[1].y, (int)vC[2].x, (int)vC[2].y, colGoalBounds));
				DrawBuffer::Add(graphics, Graphics::Line((int)vC[2].x, (int)vC[2].y, (int)vC[3].x, (int)vC[3].y, colGoalBounds));
				DrawBuffer::Add(graphics, Graphics::Line((int)vC[3].x, (int)vC[3].y, (int)vC[0].x, (int)vC[0].y, colGoalBounds));

				//////////////////////////////////////////////////////////////////////////
				/*gcn::Rectangle r((int)vC[0].x, (int)vC[0].y, (int)(vC[2].x - vC[0].x), (int)(vC[3].y - vC[1].y));
				if(r.isPointInRect(g_MapMouse.X, g_MapMouse.Y))*/
				{
					Vector3f center = FromWorld(navextents, worldbounds.Center);
					gcn::Graphics::Alignment alignment = gcn::Graphics::CENTER;
					/*if(g_MapMouse.X > DW.Map.mMap->getWidth())
					alignment = gcn::Graphics::RIGHT;*/
					graphics->drawText(mg->GetName(), (int)center.x, (int)center.y, alignment);
				}
				//////////////////////////////////////////////////////////////////////////
			}
		}
	}
}

void PathPlannerFloodFill::CreateGui()
{
	gcn::Color oldBgColor = gcn::DefaultBgColor;
	gcn::Color oldFgColor = gcn::DefaultFgColor;

	gcn::DefaultBgColor.a = 128;
	gcn::DefaultFgColor.a = 128;

	DW.Nav.mAdjCtr->clear(true);

	DW.Nav.mAdjCtr->setNumberOfColumns(2);

	gcn::DefaultBgColor = oldBgColor;
	gcn::DefaultFgColor = oldFgColor;
}

void PathPlannerFloodFill::UpdateGui()
{
	if(!DW.Nav.mAdjCtr)
		return;
}

void PathPlannerFloodFill::RenderToMapViewPort(gcn::Widget *widget, gcn::Graphics *graphics)
{
	using namespace gcn;

	
}

//////////////////////////////////////////////////////////////////////////

class WeaponListModel : public gcn::ListModel
{
public:
	enum Columns
	{
		WeaponName,
		WeaponClipPrimary,
		WeaponAmmoPrimary,
		WeaponChargePrimary,
		WeaponBurstPrimary,
		WeaponExtra,

		NumColumns
	};

	virtual int getNumberOfColumns() { return NumColumns; };

	virtual bool getColumnTitle(int column, std::string &title, int &columnwidth)
	{
		switch(column)
		{
		case WeaponName:
			columnwidth = 150;
			title = "Weapon Name";
			break;
		case WeaponClipPrimary:
			columnwidth = 50;
			title = "Clip";
			break;
		case WeaponAmmoPrimary:
			columnwidth = 50;
			title = "Ammo";
			break;
		case WeaponChargePrimary:
			columnwidth = 50;
			title = "Charge";
			break;
		case WeaponBurstPrimary:
			columnwidth = 50;
			title = "Burst";
			break;
		case WeaponExtra:
			columnwidth = 150;
			title = "Misc";
			break;
		}
		return true;
	}

	virtual int getColumnBorderSize() { return 1; };

	virtual int getNumberOfElements()
	{
		using namespace AiState;
		ClientPtr bot = GetSelectedClient();
		if(bot)
		{
			FINDSTATE(ws,WeaponSystem,bot->GetStateRoot());
			if(ws)
				return ws->GetWeaponList().size();
		}
		return 0;
	}

	std::string getElementAt(int i, int column)
	{
		using namespace AiState;

		ClientPtr bot = GetSelectedClient();
		if(!bot) return "";

		FINDSTATE(ws,WeaponSystem,bot->GetStateRoot());
		if ( ws != NULL ) {
			WeaponPtr wpn = ws->GetWeaponByIndex(i);
			if(wpn)
			{
				const int curWpnId = ws->GetCurrentWeaponID();
				switch(column)
				{
				case WeaponName:
					{
						return String(va("%s (%d)", wpn->GetWeaponName().c_str(),wpn->GetWeaponID()));
					}
				case WeaponClipPrimary:
					{
						const Weapon::WeaponFireMode &m = wpn->GetFireMode(Primary);
						if(wpn->GetWeaponID() != curWpnId)
							return "-";
						if(!m.CheckFlag(Weapon::RequiresAmmo))
							return "NA";
						return String(va("%d/%d", m.GetCurrentClip(), m.GetMaxClip()));
					}
				case WeaponAmmoPrimary:
					{
						const Weapon::WeaponFireMode &m = wpn->GetFireMode(Primary);
						if(!m.CheckFlag(Weapon::RequiresAmmo))
							return "NA";
						return String(va("%d/%d", m.GetCurrentAmmo(), m.GetMaxAmmo()));
					}
				case WeaponChargePrimary:
					{
						const Weapon::WeaponFireMode &m = wpn->GetFireMode(Primary);
						if(m.IsCharging())
							return String(va("%.1f", Utils::MillisecondsToSeconds(m.GetChargeTime()-IGame::GetTime())));
						else
							return "-";
					}
				case WeaponBurstPrimary:
					{
						const Weapon::WeaponFireMode &m = wpn->GetFireMode(Primary);
						const Weapon::WeaponFireMode::BurstWindow &bw = m.GetBurstWindow();
						if(bw.m_BurstRounds)
						{
							if(m.IsBurstDelaying())
								return String(va("%.1f", Utils::MillisecondsToSeconds(m.GetBurstTime()-IGame::GetTime())));
							else
								return String(va("%d/%d", m.GetBurstRound(),bw.m_BurstRounds));
						}
						else
							return "-";
					}
				case WeaponExtra:
					{
						return "";
					}
				}
			}
		}
		return std::string("");
	}
};
WeaponListModel weaponListModel;

WeaponSystem_DW::WeaponSystem_DW()
	: gcn::Window("WeaponSystem")
{
	int fWidth = 500, fHeight = 300;
	setSize(fWidth,fHeight);
	setVisible(false);

	mWeaponList = new gcn::ListBox(&weaponListModel);
	mWeaponList->setSize(fWidth,fHeight);
	
	mWeaponScroll = new gcn::ScrollArea(mWeaponList);
	mWeaponScroll->setDimension(gcn::Rectangle(0,0,fWidth,fHeight));
	mWeaponScroll->setFrameSize(1);

	add(mWeaponList);
}

WeaponSystem_DW::~WeaponSystem_DW()
{
	OB_DELETE(mWeaponScroll);
	OB_DELETE(mWeaponList);
}

void WeaponSystem_DW::logic()
{
}

//////////////////////////////////////////////////////////////////////////

ScriptGoal_DW::ScriptGoal_DW()
	: gcn::Window("WeaponSystem")
{
	int fWidth = 500, fHeight = 300;
	setSize(fWidth,fHeight);
	setVisible(false);
}

ScriptGoal_DW::~ScriptGoal_DW()
{
}

void ScriptGoal_DW::logic()
{
}


//////////////////////////////////////////////////////////////////////////
class SensoryListModel : public gcn::ListModel
{
public:
	enum Columns
	{
		EntTypeName,
		EntId,
		EntName,
		EntHealth,
		EntState,
		NumColumns
	};

	virtual int getNumberOfColumns() { return NumColumns; };

	virtual bool getColumnTitle(int column, std::string &title, int &columnwidth)
	{
		switch(column)
		{
		case EntTypeName:
			columnwidth = 150;
			title = "Type";
			break;
		case EntId:
			columnwidth = 50;
			title = "Id";
			break;
		case EntName:
			columnwidth = 50;
			title = "Name";
			break;
		case EntHealth:
			columnwidth = 50;
			title = "Health";
			break;
		case EntState:
			columnwidth = 50;
			title = "Flags";
			break;		
		}
		return true;
	}

	virtual int getColumnBorderSize() { return 1; };

	virtual int getNumberOfElements()
	{
		using namespace AiState;
		ClientPtr bot = GetSelectedClient();
		if(bot)
		{
			FINDSTATE(sm,SensoryMemory,bot->GetStateRoot());
			if(sm)
				return sm->GetAllRecords(mRecords,SensoryMemory::NumRecords);
		}
		return 0;
	}

	std::string getElementAt(int i, int column)
	{
		using namespace AiState;

		switch(column)
		{
		case EntTypeName:
			{
				const char *pName = Utils::FindClassName(mRecords[i].m_TargetInfo.m_EntityClass);
				return String(va("%s", pName?pName:"<UNKNOWN>"));
			}
		case EntId:
			{
				return Utils::FormatEntityString(mRecords[i].GetEntity());
			}
		case EntName:
			{
				return EngineFuncs::EntityName(mRecords[i].GetEntity(),"");
			}
		case EntHealth:
			{
				Msg_HealthArmor m;
				if(InterfaceFuncs::GetHealthAndArmor(mRecords[i].GetEntity(),m))
					return String(va("%d/%d", m.m_CurrentHealth, m.m_MaxHealth));
				else
					return "-ERR-";
			}
		case EntState:
			{
				const BitFlag64 &bf = mRecords[i].m_TargetInfo.m_EntityFlags;
				return String(va("%s%s%s%s%s%s%s%s%s%s%s%s"
					, bf.CheckFlag(ENT_FLAG_DEAD)||bf.CheckFlag(ENT_FLAG_DISABLED)?"Dead":"Alive"
					, bf.CheckFlag(ENT_FLAG_HUMANCONTROLLED)?" Human":""
					, bf.CheckFlag(ENT_FLAG_CROUCHED)?" Crouch":""
					, bf.CheckFlag(ENT_FLAG_PRONED)?" Prone":""
					, bf.CheckFlag(ENT_FLAG_VISTEST)?" VisTest":""
					, bf.CheckFlag(ENT_FLAG_RELOADING)?" Reloading":""
					, bf.CheckFlag(ENT_FLAG_CARRYABLE)?" Carryable":""
					, bf.CheckFlag(ENT_FLAG_ONGROUND)?" Ground":""
					, bf.CheckFlag(ENT_FLAG_ONLADDER)?" Ladder":""
					, bf.CheckFlag(ENT_FLAG_INWATER)?" InWater":""
					, bf.CheckFlag(ENT_FLAG_UNDERWATER)?" UnderWater":""
					, bf.CheckFlag(ENT_FLAG_INVEHICLE)?" Vehicle":""
					));
			}
		}
		return std::string("");
	}

	MemoryRecord	mRecords[AiState::SensoryMemory::NumRecords];
};
SensoryListModel sensoryListModel;

SensoryMemory_DW::SensoryMemory_DW()
	: gcn::Window("SensoryMemory")
{
	int fWidth = 600, fHeight = 300;
	setSize(fWidth,fHeight);
	setVisible(false);

	mSensoryList = new gcn::ListBox(&sensoryListModel);
	mSensoryList->setSize(fWidth,fHeight);
	
	mShowPerception = new gcn::CheckBox("Show Perception",false);
	mShowPerception->setDimension(gcn::Rectangle(0,0,100,20));
	mShowRecords = new gcn::CheckBox("Show Records",true);
	mShowRecords->setDimension(gcn::Rectangle(100,0,100,20));

	mSensoryScroll = new gcn::ScrollArea(mSensoryList);
	mSensoryScroll->setDimension(gcn::Rectangle(0,20,fWidth,fHeight-mShowPerception->getHeight()));
	mSensoryScroll->setFrameSize(1);

	add(mShowPerception);
	add(mShowRecords);
	add(mSensoryScroll);
}

SensoryMemory_DW::~SensoryMemory_DW()
{
	OB_DELETE(mSensoryScroll);
	OB_DELETE(mSensoryList);
	OB_DELETE(mShowPerception);
	OB_DELETE(mShowRecords);
}

void SensoryMemory_DW::logic()
{
	ClientPtr cl = IGameManager::GetInstance()->GetGame()->GetDebugWindowClient(g_SelectedClient);
	if(cl)
	{
		using namespace AiState;
		SensoryMemory *sm = static_cast<SensoryMemory*>(cl->GetStateRoot()->FindState("SensoryMemory"));
		if(sm)
		{
			sm->m_DebugFlags.SetFlag(SensoryMemory::Dbg_ShowPerception,mShowPerception->isSelected());
			sm->m_DebugFlags.SetFlag(SensoryMemory::Dbg_ShowEntities,mShowRecords->isSelected());			
		}
	}

}

//////////////////////////////////////////////////////////////////////////
bool AimReqPriority(const AiState::Aimer::AimRequest &_p1, const AiState::Aimer::AimRequest &_p2)
{
	return _p1.m_Priority > _p2.m_Priority;
}
//////////////////////////////////////////////////////////////////////////
class AimerListModel : public gcn::ListModel
{
public:
	enum Columns
	{
		AimerPriority,
		AimerOwner,
		AimerType,
		AimerVec,

		NumColumns
	};

	virtual int getNumberOfColumns() { return NumColumns; };

	virtual bool getColumnTitle(int column, std::string &title, int &columnwidth)
	{
		switch(column)
		{
		case AimerPriority:
			columnwidth = 50;
			title = "Priority";
			break;
		case AimerOwner:
			columnwidth = 100;
			title = "Owner";
			break;
		case AimerType:
			columnwidth = 100;
			title = "Type";
			break;
		case AimerVec:
			columnwidth = 150;
			title = "Vector";
			break;	
		}
		return true;
	}

	virtual int getColumnBorderSize() { return 1; };
	virtual int getNumberOfElements()
	{
		using namespace AiState;
		ClientPtr bot = GetSelectedClient();
		if(bot)
		{
			FINDSTATE(aim,Aimer,bot->GetStateRoot());
			if(aim)
			{
				int iNum = aim->GetAllRequests(mAimRequests,Aimer::MaxAimRequests);
				std::sort(mAimRequests, mAimRequests+Aimer::MaxAimRequests, AimReqPriority);
				return iNum;
			}
		}
		return 0;
	}

	std::string getElementAt(int i, int column)
	{
		using namespace AiState;

		switch(column)
		{
		case AimerPriority:
			{
				return String(va("%s(%d)", 
					Priority::AsString(mAimRequests[i].m_Priority), 
					mAimRequests[i].m_Priority));
			}
		case AimerOwner:
			{
				return String(va("%s", Utils::HashToString(mAimRequests[i].m_Owner).c_str()));
			}
		case AimerType:
			{
				switch(mAimRequests[i].m_AimType)
				{
				case Aimer::WorldPosition:
					return "WorldPosition";
				case Aimer::WorldFacing:
					return "WorldFacing";
				case Aimer::MoveDirection:
					return "MoveDirection";
				case Aimer::UserCallback:
					return "Callback";
				}
			}
		case AimerVec:
			{
				Vector3f v = mAimRequests[i].m_AimVector;
				return String(va("( %.1f, %.1f, %.1f )", v.x, v.y, v.z));
			}
		}
		return std::string("");
	}

	AiState::Aimer::AimRequest		mAimRequests[AiState::Aimer::MaxAimRequests];
};
AimerListModel aimerListModel;

Aimer_DW::Aimer_DW()
	: gcn::Window("Aimer")
{
	int fWidth = 500, fHeight = 300;
	setSize(fWidth,fHeight);
	setVisible(false);

	mAimerList = new gcn::ListBox(&aimerListModel);
	mAimerList->setSize(fWidth,fHeight);

	mAimerScroll = new gcn::ScrollArea(mAimerList);
	mAimerScroll->setDimension(gcn::Rectangle(0,0,fWidth,fHeight));
	mAimerScroll->setFrameSize(1);

	add(mAimerScroll);
}

Aimer_DW::~Aimer_DW()
{
	OB_DELETE(mAimerScroll);
	OB_DELETE(mAimerList);
}

void Aimer_DW::logic()
{
}

//////////////////////////////////////////////////////////////////////////

#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
