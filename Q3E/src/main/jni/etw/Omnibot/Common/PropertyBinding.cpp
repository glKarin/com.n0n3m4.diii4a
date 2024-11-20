#include "PrecompCommon.h"
#include "PropertyBinding.h"

//////////////////////////////////////////////////////////////////////////

enum PropType 
{
	PropBool,
	PropCString,
	PropString,
	PropVector,
	PropInt,
	PropIntBitflag32,
	PropFloat,
	PropEntity,
	PropAABB,
	PropMat,
	PropFunc,
	PropNum,
};

static const char *dbgNames[] =
{
	"boolean",
	"string",
	"string",
	"vector",
	"int",
	"bitflag",
	"float",
	"entity",
	"aabb",
	"matrix",
	"func",
};
BOOST_STATIC_ASSERT(sizeof(dbgNames)/sizeof(dbgNames[0]) == PropNum);
//////////////////////////////////////////////////////////////////////////
#ifdef ENABLE_DEBUG_WINDOW
class Property : public gcn::ActionListener
#else
class Property
#endif
{
public:
	virtual PropType GetPropertyType() const = 0;
	String GetDebugTypeName() { return dbgNames[GetPropertyType()]; }
	
	String GetName() const { return m_Name; }
	bool Check(obuint32 _f) const { return (m_Flags&_f)!=0; }

	virtual bool FromString(const String &_str) = 0;
	virtual bool FromGMVar(gmMachine *_m, const gmVariable &_str) = 0;
	
	//void SetBinding(PropertyBinding *_bind) { m_Binding = _bind; }
	//PropertyBinding *GetBinding() const { return m_Binding; }
	Property(const String &_name, obuint32 _flags) 
		: m_Flags(_flags)
		, m_Name(_name)
	{
	}
	virtual ~Property() {}

	protected:
		obuint32							m_Flags;
	private:
		String								m_Name;
		//PropertyBinding						*m_Binding;
public:
#ifdef ENABLE_DEBUG_WINDOW
	virtual void AddToGui(gcn::contrib::PropertySheet *_propsheet) { }
	virtual void DeAllocGui() {}
	virtual void UpdateToGui() {}
#endif
};

//////////////////////////////////////////////////////////////////////////
class PropertyBool : public Property
{
public:
	virtual PropType GetPropertyType() const { return PropBool; }
	bool GetValue() const { return m_Value; }
	bool FromString(const String &_str)
	{
		if(Utils::StringToFalse(_str))
			m_Value = false;
		else if(Utils::StringToTrue(_str))
			m_Value = true;
		else
			return false;
		return true;
	}
	bool FromGMVar(gmMachine *_m, const gmVariable &_v)
	{
		if(_v.IsInt())
		{
			m_Value = _v.GetInt()!=0;
			return true;
		}		
		return false;
	}
#ifdef ENABLE_DEBUG_WINDOW
	void action(const gcn::ActionEvent& actionEvent)
	{
		if(actionEvent.getSource()==m_CheckBox)
		{
			m_Value = m_CheckBox->isSelected();
		}
	}
	void AddToGui(gcn::contrib::PropertySheet *_propsheet) 
	{
		m_CheckBox = new gcn::CheckBox;
		if(Check(Prop::PF_READONLY))
			m_CheckBox->setEnabled(false);
		_propsheet->addProperty(GetName(),m_CheckBox);
	}
	void DeAllocGui()
	{
		OB_DELETE(m_CheckBox);
	}
	void UpdateToGui() 
	{
		if(m_CheckBox)
		{
			m_CheckBox->setCaption(m_Value?"true":"false");
			m_CheckBox->setSelected(m_Value);
			m_CheckBox->adjustSize();
		}
	}
#endif
	PropertyBool(const String &_name, bool &v, obuint32 _flags = 0) 
		: Property(_name,_flags)
		, m_Value(v)
#ifdef ENABLE_DEBUG_WINDOW
		, m_CheckBox(0)
#endif
	{
	}
	~PropertyBool() {}
private:
	bool &			m_Value;
#ifdef ENABLE_DEBUG_WINDOW
	gcn::CheckBox	*m_CheckBox;
#endif
};
//////////////////////////////////////////////////////////////////////////
class PropertyCString : public Property
{
public:
	virtual PropType GetPropertyType() const { return PropCString; }

	const char *GetValue() const { return m_Value; }

	bool FromString(const String &_str)
	{
		return false;
	}
	bool FromGMVar(gmMachine *_m, const gmVariable &_v)
	{		
		return false;
	}
#ifdef ENABLE_DEBUG_WINDOW
	void action(const gcn::ActionEvent& actionEvent)
	{
	}
	void AddToGui(gcn::contrib::PropertySheet *_propsheet) 
	{
		m_TextField = new gcn::TextField;
		m_TextField->setEnabled(false);
		m_TextField->setReadOnly(true);
		_propsheet->addProperty(GetName(),m_TextField);
	}
	void DeAllocGui()
	{
		OB_DELETE(m_TextField);
	}
	void UpdateToGui() 
	{
		if(m_TextField)
		{
			m_TextField->setText(m_Value?m_Value:"");
			m_TextField->adjustSize();
		}
	}
#endif
	PropertyCString(const String &_name, const char *&v, obuint32 _flags = 0) 
		: Property(_name,_flags)
		, m_Value(v) 
#ifdef ENABLE_DEBUG_WINDOW
		, m_TextField(0)
#endif
	{
	}
	~PropertyCString() {}
private:
	const char *&	m_Value;
#ifdef ENABLE_DEBUG_WINDOW
	gcn::TextField *m_TextField;
#endif
};
//////////////////////////////////////////////////////////////////////////
class PropertyString : public Property
{
public:
	virtual PropType GetPropertyType() const { return PropString; }

	String GetValue() const { return m_Value; }

	bool FromString(const String &_str)
	{
		m_Value = _str;
		return true;
	}
	bool FromGMVar(gmMachine *_m, const gmVariable &_v)
	{
		const char *s = _v.GetCStringSafe(0);
		if(s)
		{
			m_Value = s;
			return true;
		}		
		return false;
	}
#ifdef ENABLE_DEBUG_WINDOW
	void action(const gcn::ActionEvent& actionEvent)
	{
		if(actionEvent.getSource()==m_TextField)
		{
			m_Value = m_TextField->getText();
		}
	}
	void AddToGui(gcn::contrib::PropertySheet *_propsheet) 
	{
		m_TextField = new gcn::TextField;
		if(Check(Prop::PF_READONLY))
			m_TextField->setEnabled(false);
		_propsheet->addProperty(GetName(),m_TextField);
	}
	void DeAllocGui()
	{
		OB_DELETE(m_TextField);
	}
	void UpdateToGui() 
	{
		if(m_TextField)
		{
			m_TextField->setText(m_Value);
			m_TextField->adjustSize();
		}
	}
#endif
	PropertyString(const String &_name, String &v, obuint32 _flags = 0) 
		: Property(_name,_flags)
		, m_Value(v)
#ifdef ENABLE_DEBUG_WINDOW
		, m_TextField(0)
#endif
	{
	}
	~PropertyString() {}
private:
	String &		m_Value;
#ifdef ENABLE_DEBUG_WINDOW
	gcn::TextField *m_TextField;
#endif
};
//////////////////////////////////////////////////////////////////////////
class PropertyVector : public Property
{
public:
	virtual PropType GetPropertyType() const { return PropVector; }
	Vector3f GetValue() const { return m_Value; }
	bool FromString(const String &_str)
	{
		Vector3f v;
		if(Utils::ConvertString(_str,v))
		{
			m_Value = v;
			return true;
		}
		return false;
	}
	bool FromGMVar(gmMachine *_m, const gmVariable &_v)
	{
		if(_v.IsVector())
		{
			_v.GetVector(m_Value);
			return true;
		}		
		return false;
	}
#ifdef ENABLE_DEBUG_WINDOW
	void action(const gcn::ActionEvent &evt)
	{
		if(evt.getSource()==m_BtnSetPos)
		{
			Utils::GetLocalFacing(m_Value);
		}
		else if(evt.getSource()==m_BtnSetFacing)
		{
			Utils::GetLocalPosition(m_Value);
		}
		else if(evt.getSource()==m_BtnSetAimPos)
		{
			Utils::GetLocalAimPoint(m_Value);
		}
	}
	void AddToGui(gcn::contrib::PropertySheet *_propsheet) 
	{
		m_Container = new gcn::contrib::AdjustingContainer;

		m_TextField = new gcn::TextField;
		if(Check(Prop::PF_READONLY))
			m_TextField->setEnabled(false);
		m_Container->add(m_TextField);

		m_BtnSetPos = new gcn::Button("Set Position");
		m_BtnSetPos->addActionListener(this);
		m_Container->add(m_BtnSetPos);

		m_BtnSetFacing = new gcn::Button("Set Facing");
		m_BtnSetFacing->addActionListener(this);
		m_Container->add(m_BtnSetFacing);

		m_BtnSetAimPos = new gcn::Button("Set Aim Position");
		m_BtnSetAimPos->addActionListener(this);
		m_Container->add(m_BtnSetAimPos);

		_propsheet->addProperty(GetName(),m_Container);
	}
	void UpdateToGui() 
	{
		if(m_TextField)
		{
			m_TextField->setText(Utils::FormatVectorString(m_Value));
			m_TextField->adjustSize();
		}
	}
#endif
	PropertyVector(const String &_name, Vector3f &v, obuint32 _flags = 0) 
		: Property(_name,_flags)
		, m_Value(v)
#ifdef ENABLE_DEBUG_WINDOW
		, m_Container(0)
		, m_TextField(0)
		, m_BtnSetPos(0)
		, m_BtnSetFacing(0)
		, m_BtnSetAimPos(0)
#endif
	{
	}
	~PropertyVector() {}
private:
	Vector3f &							m_Value;
#ifdef ENABLE_DEBUG_WINDOW
	gcn::contrib::AdjustingContainer	*m_Container;
	gcn::TextField						*m_TextField;
	gcn::Button							*m_BtnSetPos;
	gcn::Button							*m_BtnSetFacing;
	gcn::Button							*m_BtnSetAimPos;
#endif
};
//////////////////////////////////////////////////////////////////////////
class PropertyMatrix : public Property
{
public:
	virtual PropType GetPropertyType() const { return PropMat; }
	Matrix3f GetValue() const { return m_Value; }
	bool FromString(const String &_str)
	{
		/*Matrix3f v;
		if(Utils::ConvertString(_str,v))
		{
			m_Value = v;
			return true;
		}*/
		return false;
	}
	bool FromGMVar(gmMachine *_m, const gmVariable &_v)
	{
		/*if(_v.IsVector())
		{
			_v.GetVector(m_Value);
			return true;
		}	*/	
		return false;
	}
#ifdef ENABLE_DEBUG_WINDOW
	void action(const gcn::ActionEvent& actionEvent)
	{
	}
	void AddToGui(gcn::contrib::PropertySheet *_propsheet) 
	{
		m_TextField = new gcn::TextField;
		if(Check(Prop::PF_READONLY))
			m_TextField->setEnabled(false);
		_propsheet->addProperty(GetName(),m_TextField);
	}
	void DeAllocGui()
	{
		OB_DELETE(m_Button1);
		OB_DELETE(m_Button2);
		OB_DELETE(m_Button3);
		OB_DELETE(m_TextField);
	}
	void UpdateToGui() 
	{
		if(m_TextField)
		{
			m_TextField->setText(Utils::FormatMatrixString(m_Value));
			m_TextField->adjustSize();
		}
	}
#endif
	PropertyMatrix(const String &_name, Matrix3f &v, obuint32 _flags = 0) 
		: Property(_name,_flags)
		, m_Value(v)
#ifdef ENABLE_DEBUG_WINDOW
		, m_TextField(0)
		, m_Button1(0)
		, m_Button2(0)
		, m_Button3(0)
#endif
	{
	}
	~PropertyMatrix() {}
private:
	Matrix3f		&m_Value;
#ifdef ENABLE_DEBUG_WINDOW
	gcn::TextField	*m_TextField;
	gcn::Button		*m_Button1;
	gcn::Button		*m_Button2;
	gcn::Button		*m_Button3;
#endif
};
//////////////////////////////////////////////////////////////////////////
#ifdef ENABLE_DEBUG_WINDOW
class PropertyInt : public Property, public gcn::ListModel
#else
class PropertyInt : public Property
#endif
{
public:
	virtual PropType GetPropertyType() const { return PropInt; }
	int GetValue() const { return m_Value; }
	bool FromString(const String &_str)
	{
		int v;
		if(Utils::ConvertString(_str,v))
		{
			if(Check(Prop::PF_MS_TO_SECONDS))
				m_Value = v * 1000;
			else
				m_Value = v;
			return true;
		}
		else if(m_Enum && m_EnumNum)
		{
			//const obuint32 uiKey = Utils::Hash32(_str);
			for(int i = 0; i < m_EnumNum; ++i)
			{
				if(!Utils::StringCompareNoCase(m_Enum[i].m_Key,_str.c_str()))
				{
					m_Value = m_Enum[i].m_Value;
					return true;
				}
			}
		}
		return false;
	}
	bool FromGMVar(gmMachine *_m, const gmVariable &_v)
	{
		if(_v.IsInt())
		{
			if(Check(Prop::PF_MS_TO_SECONDS))
				m_Value = _v.GetInt() * 1000;
			else
				m_Value = _v.GetInt();
			return true;
		} 
		else if(m_Enum && m_EnumNum)
		{
			const char *key = _v.GetCStringSafe(0);
			if(key)
			{
				//const obuint32 uiKey = Utils::Hash32(key);
				for(int i = 0; i < m_EnumNum; ++i)
				{
					if(!Utils::StringCompareNoCase(m_Enum[i].m_Key,key))
					{
						m_Value = m_Enum[i].m_Value;
						return true;
					}
				}
			}
		}
		return false;
	}
#ifdef ENABLE_DEBUG_WINDOW
	void action(const gcn::ActionEvent& actionEvent)
	{
		if(actionEvent.getSource()==m_DropDown)
		{
			m_Value = m_DropDown->getSelected();			
		}
		if(actionEvent.getSource()==m_TextField)
		{
			float val = (float)atof(m_TextField->getText().c_str());
			if(Check(Prop::PF_MS_TO_SECONDS))
				val *= 1000.f;

			m_Value = (int)val;
		}
	}
	void AddToGui(gcn::contrib::PropertySheet *_propsheet) 
	{
		if(!m_TextField)
		{
			if(m_Enum)
			{
				m_DropDown = new gcn::DropDown(this);
				m_DropDown->setSelected(m_Value);
				m_DropDown->adjustHeight();
				if(Check(Prop::PF_READONLY))
					m_DropDown->setEnabled(false);

				// resize to fit the biggest value.
				int width = m_DropDown->getWidth();
				for(int i = 0; i < m_EnumNum; ++i)
				{
					int w = m_DropDown->getFont()->getWidth(m_Enum[i].m_Key);
					if(w > width)
						width = w;
				}
				m_DropDown->setWidth(width);
				m_DropDown->setSelected(m_Value);
				_propsheet->addProperty(GetName(),m_DropDown);
			}
			else
			{
				m_TextField = new gcn::TextField();
				if(Check(Prop::PF_READONLY))
					m_TextField->setEnabled(false);
				_propsheet->addProperty(GetName(),m_TextField);
			}
		}
	}
	void UpdateToGui() 
	{
		if(m_Enum)
		{
			m_Value = m_DropDown->getSelected();
		}
		else if(m_TextField)
		{
			if(Check(Prop::PF_MS_TO_SECONDS))
				m_TextField->setText((String)va("%g",(float)m_Value / 1000.f));
			else
				m_TextField->setText((String)va("%d",m_Value));
			//m_TextField->adjustSize();
		}
	}
#endif
	int getNumberOfElements() { return m_EnumNum; }
	std::string getElementAt(int i, int column) { return m_Enum[i%m_EnumNum].m_Key; }
	PropertyInt(const String &_name, int &v, obuint32 _flags = 0, const IntEnum *_enum = 0, int _numenum = 0) 
		: Property(_name,_flags)
		, m_Value(v) 
		, m_Enum(_enum)
		, m_EnumNum(_numenum)
#ifdef ENABLE_DEBUG_WINDOW
		, m_TextField(0)
		, m_DropDown(0)
#endif
	{
	}
	~PropertyInt() {}
private:
	int								&m_Value;
	const IntEnum	*m_Enum;
	int								m_EnumNum;
#ifdef ENABLE_DEBUG_WINDOW
	gcn::TextField	*m_TextField;
	gcn::DropDown	*m_DropDown;
#endif
};
//////////////////////////////////////////////////////////////////////////
#ifdef ENABLE_DEBUG_WINDOW
class PropertyBitflag32 : public Property, public gcn::ListModel
#else
class PropertyBitflag32 : public Property
#endif
{
public:
	virtual PropType GetPropertyType() const { return PropIntBitflag32; }
	BitFlag32 GetValue() const { return m_Value; }
	bool FromString(const String &_str)
	{
		if(m_Enum && m_EnumNum)
		{
			StringVector sv;
			Utils::Tokenize(_str,"|,:",sv);

			// for each string in the token
			BitFlag32 bf;
			for(obuint32 s = 0; s < sv.size(); ++s)
			{
				bool bRecognized = false;

				for(int e = 0; e < m_EnumNum; ++e)
				{
					if(m_Enum[e].m_Key)
					{
						if(!Utils::StringCompareNoCase(m_Enum[e].m_Key,sv[s].c_str()))
						{
							bRecognized = true;
							bf.SetFlag(m_Enum[e].m_Value,true);
							break;
						}
					}
				}

				if(!bRecognized)
				{
					return false;
				}
			}
			m_Value = bf;
		}
		return false;
	}
	bool FromGMVar(gmMachine *_m, const gmVariable &_v)
	{
		if(_v.IsInt())
		{
			m_Value = BitFlag32(_v.GetInt());
			return true;
		} 
		else if(m_Enum && m_EnumNum)
		{
			const char *key = _v.GetCStringSafe(0);
			if(key)
			{
				StringVector sv;
				Utils::Tokenize(key,"|,:",sv);

				// for each string in the token
				BitFlag32 bf;
				for(obuint32 s = 0; s < sv.size(); ++s)
				{
					bool bRecognized = false;

					for(int e = 0; e < m_EnumNum; ++e)
					{
						if(m_Enum[e].m_Key)
						{
							if(!Utils::StringCompareNoCase(m_Enum[e].m_Key,sv[s].c_str()))
							{
								bRecognized = true;
								bf.SetFlag(m_Enum[e].m_Value,true);
								break;
							}
						}
					}

					if(!bRecognized)
					{
						return false;
					}
				}

				m_Value = bf;
				return true;
			}
		}
		return false;
	}
#ifdef ENABLE_DEBUG_WINDOW
	void action(const gcn::ActionEvent& actionEvent)
	{
		gcn::CheckBox *cb = static_cast<gcn::CheckBox*>(m_Container->findWidgetById(actionEvent.getId()));
		const int bit = atoi(actionEvent.getId().c_str());
		m_Value.SetFlag(bit, cb->isSelected());
	}
	void AddToGui(gcn::contrib::PropertySheet *_propsheet) 
	{
		m_Container = new gcn::contrib::AdjustingContainer;
		m_Container->setNumberOfColumns(2);
		for(int i = 0; i < m_EnumNum; ++i)
		{
			gcn::CheckBox *cb = new gcn::CheckBox(m_Enum[i].m_Key,m_Value.CheckFlag(i));
			cb->setId((String)va("%d",i));
			m_Container->add(cb);
		}
		_propsheet->addProperty(GetName(),m_Container);
	}
	void UpdateToGui() 
	{
	}
#endif
	int getNumberOfElements() { return m_EnumNum; }
	std::string getElementAt(int i, int column) { return m_Enum[i%m_EnumNum].m_Key; }
	PropertyBitflag32(const String &_name, BitFlag32 &v, obuint32 _flags = 0, const IntEnum *_enum = 0, int _numenum = 0) 
		: Property(_name,_flags)
		, m_Value(v) 
		, m_Enum(_enum)
		, m_EnumNum(_numenum)
#ifdef ENABLE_DEBUG_WINDOW
		, m_Container(0)
#endif
	{
	}
	~PropertyBitflag32() {}
private:
	BitFlag32						&	m_Value;
	const IntEnum	*	m_Enum;
	int									m_EnumNum;
#ifdef ENABLE_DEBUG_WINDOW
	gcn::contrib::AdjustingContainer	*m_Container;
#endif
};
//////////////////////////////////////////////////////////////////////////
class PropertyFloat : public Property
{
public:
	virtual PropType GetPropertyType() const { return PropFloat; }
	float GetValue() const { return m_Value; }
	bool FromString(const String &_str)
	{
		float v;
		if(Utils::ConvertString(_str,v))
		{
			m_Value = v;
			return true;
		}
		return false;
	}
	bool FromGMVar(gmMachine *_m, const gmVariable &_v)
	{
		if(_v.IsNumber())
		{
			m_Value = _v.GetFloatSafe();
			return true;
		}		
		return false;
	}
#ifdef ENABLE_DEBUG_WINDOW
	void action(const gcn::ActionEvent& actionEvent)
	{
		if(actionEvent.getSource() == m_TextField)
		{
			m_Value = (float)atof(m_TextField->getText().c_str());
		}
	}
	void AddToGui(gcn::contrib::PropertySheet *_propsheet) 
	{
		m_TextField = new gcn::TextField;
		if(Check(Prop::PF_READONLY))
			m_TextField->setEnabled(false);
		_propsheet->addProperty(GetName(),m_TextField);
	}
	void DeAllocGui()
	{
		OB_DELETE(m_TextField);
	}
	void UpdateToGui() 
	{
		if(m_TextField)
		{
			m_TextField->setText((String)va("%g",m_Value));
		}
	}
#endif
	PropertyFloat(const String &_name, float &v, obuint32 _flags = 0) 
		: Property(_name,_flags)
		, m_Value(v)
#ifdef ENABLE_DEBUG_WINDOW
		, m_TextField(0)
#endif
	{
	}
	~PropertyFloat() {}
private:
	float &			m_Value;
#ifdef ENABLE_DEBUG_WINDOW
	gcn::TextField *m_TextField;
#endif
};
//////////////////////////////////////////////////////////////////////////
class PropertyEntity : public Property
{
public:
	virtual PropType GetPropertyType() const { return PropEntity; }
	GameEntity GetValue() const { return m_Value; }
	bool FromString(const String &_str)
	{
		int v;
		if(Utils::ConvertString(_str,v))
		{
			m_Value = g_EngineFuncs->EntityFromID(v);
			return true;
		}
		return false;
	}
	bool FromGMVar(gmMachine *_m, const gmVariable &_v)
	{
		if(_v.IsEntity())
		{
			m_Value.FromInt(_v.GetEntity());
			return true;
		}		
		return false;
	}
#ifdef ENABLE_DEBUG_WINDOW
	void action(const gcn::ActionEvent& actionEvent)
	{
		if(actionEvent.getSource() == m_TextField)
		{
		}
	}
	void AddToGui(gcn::contrib::PropertySheet *_propsheet) 
	{
		m_TextField = new gcn::TextField;
		if(Check(Prop::PF_READONLY))
			m_TextField->setEnabled(false);
		_propsheet->addProperty(GetName(),m_TextField);
	}
	void DeAllocGui()
	{
		OB_DELETE(m_TextField);
	}
	void UpdateToGui() 
	{
		if(m_TextField)
		{
			m_TextField->setText(Utils::FormatEntityString(m_Value));
			m_TextField->adjustSize();
		}
	}
#endif
	PropertyEntity(const String &_name, GameEntity &v, obuint32 _flags = 0) 
		: Property(_name,_flags)
		, m_Value(v)
#ifdef ENABLE_DEBUG_WINDOW
		, m_TextField(0)
#endif
	{
	}
	~PropertyEntity() {}
private:
	GameEntity	&	m_Value;
#ifdef ENABLE_DEBUG_WINDOW
	gcn::TextField *m_TextField;
#endif
};
//////////////////////////////////////////////////////////////////////////
class PropertyAABB : public Property
{
public:
	virtual PropType GetPropertyType() const { return PropAABB; }
	AABB GetValue() const { return m_Value; }
	bool FromString(const String &_str)
	{
		/*AABB v;
		if(Utils::ConvertString(_str,v))
		{
			m_Value = v;
			return true;
		}*/
		return false;
	}
	bool FromGMVar(gmMachine *_m, const gmVariable &_v)
	{
		/*if(_v.IsNumber())
		{
			m_Value = _v.GetFloatSafe();
			return true;
		}*/		
		return false;
	}
#ifdef ENABLE_DEBUG_WINDOW
	void action(const gcn::ActionEvent& actionEvent)
	{
		if(actionEvent.getSource() == m_TextField_Mins)
		{
		}
		if(actionEvent.getSource() == m_TextField_Maxs)
		{
		}
	}
	void AddToGui(gcn::contrib::PropertySheet *_propsheet) 
	{
		/*m_TextField = new gcn::TextField;
		if(Check(Prop::PF_READONLY))
		m_TextField->setEnabled(false);
		_propsheet->addProperty(GetName(),m_TextField);*/
	}
	void DeAllocGui()
	{
		OB_DELETE(m_TextField_Mins);
		OB_DELETE(m_TextField_Maxs);
	}
	void UpdateToGui() 
	{
		if(m_TextField_Mins)
		{
			m_TextField_Mins->setText(Utils::FormatVectorString(m_Value.m_Mins));
			m_TextField_Mins->adjustSize();
		}
		if(m_TextField_Maxs)
		{
			m_TextField_Maxs->setText(Utils::FormatVectorString(m_Value.m_Maxs));
			m_TextField_Maxs->adjustSize();
		}
	}
#endif
	PropertyAABB(const String &_name, AABB &v, obuint32 _flags = 0) 
		: Property(_name,_flags)
		, m_Value(v) 
#ifdef ENABLE_DEBUG_WINDOW
		, m_TextField_Mins(0)
		, m_TextField_Maxs(0)
#endif
	{
	}
	~PropertyAABB() {}
private:
	AABB	&		m_Value;
#ifdef ENABLE_DEBUG_WINDOW
	gcn::TextField *m_TextField_Mins;
	gcn::TextField *m_TextField_Maxs;
#endif
};
//////////////////////////////////////////////////////////////////////////
class PropertyFunction : public Property
{
public:
	virtual PropType GetPropertyType() const { return PropFunc; }
	bool FromString(const String &_str)
	{
		return false;
	}
	bool FromGMVar(gmMachine *_m, const gmVariable &_v)
	{
		return false;
	}
#ifdef ENABLE_DEBUG_WINDOW
	void action(const gcn::ActionEvent &evt)
	{
		if(evt.getSource()==m_Button)
		{
			StringVector params;
			(*m_Function)(params);			
		}
	}
	void AddToGui(gcn::contrib::PropertySheet *_propsheet) 
	{
		m_Button = new gcn::Button;
		if(Check(Prop::PF_READONLY))
			m_Button->setEnabled(false);
		m_Button->addActionListener(this);
		_propsheet->addProperty(GetName(),m_Button);
	}
	void DeAllocGui()
	{
		OB_DELETE(m_Button);
	}
	void UpdateToGui() 
	{
	}
#endif
	PropertyFunction(const String &_name, CommandFunctorPtr _ptr, obuint32 _flags = 0) 
		: Property(_name,_flags)
		, m_Function(_ptr)
#ifdef ENABLE_DEBUG_WINDOW
		, m_Button(0)
#endif
	{
	}
	~PropertyFunction() {}
private:
	CommandFunctorPtr	m_Function;
#ifdef ENABLE_DEBUG_WINDOW
	gcn::Button		*	m_Button;
#endif
};
//////////////////////////////////////////////////////////////////////////
PropertyBinding::PropertyBinding() 
{
}

bool PropertyBinding::FromPropertyMap(const PropertyMap &_propmap, StringStr &errorOut)  
{  
	bool bGood = true;
	bool bHandelledProp = false; 
	PropertyList::iterator it = m_PropertyList.begin();  
	for(; it != m_PropertyList.end(); ++it)  
	{  
		String n = (*it)->GetName();
		OBASSERT(!n.empty(),"Unknown String Hash");  

		PropertyMap::ValueMap::const_iterator pIt = _propmap.GetProperties().begin();  
		for(; pIt != _propmap.GetProperties().end(); ++pIt)  
		{  
			if(!Utils::StringCompareNoCase(pIt->first.c_str(),n.c_str()))  
			{  
				const bool bSuccess = (*it)->FromString(pIt->second);  
				bHandelledProp |= bSuccess; 

				if(!bSuccess && (*it)->Check(Prop::PF_REQUIRED))  
				{                      
					String t = (*it)->GetDebugTypeName();  
					errorOut << "Required Property " << n.c_str() << " as " << t.c_str() << std::endl;  
					bGood = false;  
				}  
			}
		}  
	}

	if(!bHandelledProp) 
		bGood = false;

	return bGood;  
} 

bool PropertyBinding::FromScriptTable(gmMachine *_machine, gmTableObject *_tbl, StringStr &errorOut)
{
	bool bGood = true;

	gmTableIterator tIt;
	gmTableNode *pNode = _tbl->GetFirst(tIt);
	while(pNode)
	{
		const char *PropName = pNode->m_key.GetCStringSafe(0);
		if(PropName)
		{
			FromScriptVar(_machine,PropName,pNode->m_value,errorOut);
		}
		pNode = _tbl->GetNext(tIt);
	}	
	return bGood;
}

bool PropertyBinding::FromScriptVar(gmMachine *_machine, const char *_key, gmVariable &_var, StringStr &errorOut)
{
	bool bGood = false;
	PropertyList::iterator it = m_PropertyList.begin();
	for(; it != m_PropertyList.end(); ++it)
	{
		String n = (*it)->GetName();
		if(!Utils::StringCompareNoCase(n,_key))
		{
			const bool bSuccess = (*it)->FromGMVar(_machine,_var);
			if(bSuccess)
			{
				bGood = true;
			}
			else
			{
				String t = (*it)->GetDebugTypeName();
				errorOut << "Expected Property " << n.c_str() << " as " << t.c_str() << std::endl;
			}
		}
	}
	return bGood;
}

void PropertyBinding::BindProperty(const String &_name, bool &_val, obuint32 _flags)
{
	m_PropertyList.push_back(PropertyPtr(new PropertyBool(_name,_val)));
	//m_PropertyList.back()->SetBinding(this);
}
void PropertyBinding::BindProperty(const String &_name, const char *&_val, obuint32 _flags)
{
	m_PropertyList.push_back(PropertyPtr(new PropertyCString(_name,_val)));
	//m_PropertyList.back()->SetBinding(this);
}

void PropertyBinding::BindProperty(const String &_name, String &_val, obuint32 _flags)
{
	m_PropertyList.push_back(PropertyPtr(new PropertyString(_name,_val,_flags)));
	//m_PropertyList.back()->SetBinding(this);
}

void PropertyBinding::BindProperty(const String &_name, Vector3f &_val, obuint32 _flags)
{
	m_PropertyList.push_back(PropertyPtr(new PropertyVector(_name,_val,_flags)));
	//m_PropertyList.back()->SetBinding(this);
}
void PropertyBinding::BindProperty(const String &_name, Matrix3f &_val, obuint32 _flags)
{
	m_PropertyList.push_back(PropertyPtr(new PropertyMatrix(_name,_val,_flags)));
	//m_PropertyList.back()->SetBinding(this);
}
void PropertyBinding::BindProperty(const String &_name, int &_val, obuint32 _flags, const IntEnum *_enum, int _numenum)
{
	m_PropertyList.push_back(PropertyPtr(new PropertyInt(_name,_val,_flags,_enum,_numenum)));
	//m_PropertyList.back()->SetBinding(this);
}
void PropertyBinding::BindProperty(const String &_name, BitFlag32 &_val, obuint32 _flags, const IntEnum *_enum, int _numenum)
{
	m_PropertyList.push_back(PropertyPtr(new PropertyBitflag32(_name,_val,_flags,_enum,_numenum)));
	//m_PropertyList.back()->SetBinding(this);
}
void PropertyBinding::BindProperty(const String &_name, float &_val, obuint32 _flags)
{
	m_PropertyList.push_back(PropertyPtr(new PropertyFloat(_name,_val,_flags)));
	//m_PropertyList.back()->SetBinding(this);
}
void PropertyBinding::BindProperty(const String &_name, GameEntity &_val, obuint32 _flags)
{
	m_PropertyList.push_back(PropertyPtr(new PropertyEntity(_name,_val)));
	//m_PropertyList.back()->SetBinding(this);
}
void PropertyBinding::BindProperty(const String &_name, AABB &_val, obuint32 _flags)
{
	m_PropertyList.push_back(PropertyPtr(new PropertyAABB(_name,_val,_flags)));
	//m_PropertyList.back()->SetBinding(this);
}
void PropertyBinding::BindFunction(const String _name, CommandFunctorPtr _functor)
{
	m_PropertyList.push_back(PropertyPtr(new PropertyFunction(_name,_functor,0)));
	//m_PropertyList.back()->SetBinding(this);
}
//////////////////////////////////////////////////////////////////////////

PropertyBinding::PropertyPtr PropertyBinding::Get(const String &_name)
{
	PropertyList::iterator it = m_PropertyList.begin();
	for(; it != m_PropertyList.end(); ++it)
	{
		if((*it)->GetName()==_name)
			return *it;
	}
	return PropertyPtr();
}

bool PropertyBinding::GetProperty(const String &_name, bool &_val)
{
	PropertyPtr pp = Get(_name);
	if(pp && pp->GetPropertyType()==PropBool)
	{
		PropertyBool *p = static_cast<PropertyBool*>(pp.get());
		_val = p->GetValue();
		return true;
	}
	return false;
}

bool PropertyBinding::GetProperty(const String &_name, const char *&_val)
{
	PropertyPtr pp = Get(_name);
	if(pp && pp->GetPropertyType()==PropCString)
	{
		PropertyCString *p = static_cast<PropertyCString*>(pp.get());
		_val = p->GetValue();
		return true;
	}
	return false;
}

bool PropertyBinding::GetProperty(const String &_name, String &_val)
{
	PropertyPtr pp = Get(_name);
	if(pp && pp->GetPropertyType()==PropString)
	{
		PropertyString *p = static_cast<PropertyString*>(pp.get());
		_val = p->GetValue();
		return true;
	}
	return false;
}

bool PropertyBinding::GetProperty(const String &_name, Vector3f &_val)
{
	PropertyPtr pp = Get(_name);
	if(pp && pp->GetPropertyType()==PropVector)
	{
		PropertyVector *p = static_cast<PropertyVector*>(pp.get());
		_val = p->GetValue();
		return true;
	}
	return false;
}

bool PropertyBinding::GetProperty(const String &_name, int &_val)
{
	PropertyPtr pp = Get(_name);
	if(pp && pp->GetPropertyType()==PropInt)
	{
		PropertyInt *p = static_cast<PropertyInt*>(pp.get());
		_val = p->GetValue();
		return true;
	}
	return false;
}

bool PropertyBinding::GetProperty(const String &_name, float &_val)
{
	PropertyPtr pp = Get(_name);
	if(pp && pp->GetPropertyType()==PropFloat)
	{
		PropertyFloat *p = static_cast<PropertyFloat*>(pp.get());
		_val = p->GetValue();
		return true;
	}
	return false;
}

bool PropertyBinding::GetProperty(const String &_name, GameEntity &_val)
{
	PropertyPtr pp = Get(_name);
	if(pp && pp->GetPropertyType()==PropEntity)
	{
		PropertyEntity *p = static_cast<PropertyEntity*>(pp.get());
		_val = p->GetValue();
		return true;
	}
	return false;
}

bool PropertyBinding::GetProperty(const String &_name, AABB &_val)
{
	PropertyPtr pp = Get(_name);
	if(pp && pp->GetPropertyType()==PropAABB)
	{
		PropertyAABB *p = static_cast<PropertyAABB*>(pp.get());
		_val = p->GetValue();
		return true;
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////
#ifdef ENABLE_DEBUG_WINDOW
void PropertyBinding::GatherProperties(gcn::contrib::PropertySheet *_container)
{
	PropertyList::iterator it = m_PropertyList.begin();
	for(; it != m_PropertyList.end(); ++it)
	{
		(*it)->AddToGui(_container);
		(*it)->UpdateToGui();
	}
}
void PropertyBinding::DeAllocGui()
{
	PropertyList::iterator it = m_PropertyList.begin();
	for(; it != m_PropertyList.end(); ++it)
	{
		(*it)->DeAllocGui();
	}
}
void PropertyBinding::UpdatePropertiesToGui()
{
	PropertyList::iterator it = m_PropertyList.begin();
	for(; it != m_PropertyList.end(); ++it)
	{
		(*it)->UpdateToGui();
	}
}
#endif

//void PropertyBinding::action(const gcn::ActionEvent& actionEvent)
//{
//	PropertyPtr pptr = Get(actionEvent.getId());
//	if(pptr)
//	{
//
//	}
//}
