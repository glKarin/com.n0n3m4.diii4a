#ifndef __PROPERTYBINDING_H__
#define __PROPERTYBINDING_H__

class Property;

#ifdef ENABLE_DEBUG_WINDOW
#include <guichan.hpp>
#endif

namespace Prop
{
	enum BaseFlags
	{
		PF_READONLY = (1<<0),
		PF_REQUIRED = (1<<1),

		// must be last!
		PF_LAST		= (1<<2),
	};

	enum NumberFlags
	{
		PF_MS_TO_SECONDS	= (PF_LAST<<0),
	};

	enum Vector3Flags
	{
		PF_POSITION = (PF_LAST<<0),
		PF_FACING	= (PF_LAST<<1),

		PF_VEC_LAST	= (PF_LAST<<2),
	};

	enum MatrixFlags
	{
		PF_FULLTRANS= (PF_VEC_LAST<<0),
	};
}

//////////////////////////////////////////////////////////////////////////

struct IntEnum
{
	const char *m_Key;
	int			m_Value;
	IntEnum(const char *_key = 0, int _val = 0) : m_Key(_key), m_Value(_val) {}
};

class PropertyBinding //: public gcn::ActionListener
{
public:
#if __cplusplus >= 201103L //karin: using C++11 instead of boost
	typedef std::shared_ptr<Property> PropertyPtr;
#else
	typedef boost::shared_ptr<Property> PropertyPtr;
#endif
	typedef std::vector<PropertyPtr> PropertyList;
	
	void BindProperty(const String &_name, bool &_val, obuint32 _flags = 0);
	void BindProperty(const String &_name, const char *&_val, obuint32 _flags = 0);
	void BindProperty(const String &_name, String &_val, obuint32 _flags = 0);	
	void BindProperty(const String &_name, Vector3f &_val, obuint32 _flags = Prop::PF_POSITION);
	void BindProperty(const String &_name, Matrix3f &_val, obuint32 _flags = 0);
	void BindProperty(const String &_name, int &_val, obuint32 _flags = 0, const IntEnum *_enum = 0, int _numenum = 0);
	void BindProperty(const String &_name, BitFlag32 &_val, obuint32 _flags = 0, const IntEnum *_enum = 0, int _numenum = 0);
	void BindProperty(const String &_name, float &_val, obuint32 _flags = 0);
	void BindProperty(const String &_name, GameEntity &_val, obuint32 _flags = 0);
	void BindProperty(const String &_name, AABB &_val, obuint32 _flags = 0);

	template <typename T, typename Fn>
	void BindFunction(const String _name, T *_src, Fn _fn)
	{
		BindFunction(_name,CommandFunctorPtr(new Delegate0<T,Fn>(_src,_fn)));
	}
	void BindFunction(const String _name, CommandFunctorPtr _functor);

	bool GetProperty(const String &_name, bool &_val);
	bool GetProperty(const String &_name, const char *&_val);
	bool GetProperty(const String &_name, String &_val);	
	bool GetProperty(const String &_name, Vector3f &_val);
	bool GetProperty(const String &_name, int &_val);
	bool GetProperty(const String &_name, float &_val);
	bool GetProperty(const String &_name, GameEntity &_val);
	bool GetProperty(const String &_name, AABB &_val);

	bool FromPropertyMap(const PropertyMap &_propmap, StringStr &errorOut);
	bool FromScriptTable(gmMachine *_machine, gmTableObject *_tbl, StringStr &errorOut);
	bool FromScriptVar(gmMachine *_machine, const char *_key, gmVariable &_var, StringStr &errorOut);

#ifdef ENABLE_DEBUG_WINDOW
	void DeAllocGui();
	void GatherProperties(gcn::contrib::PropertySheet *_propsheet);
	void UpdatePropertiesToGui();
#endif

	//void action(const gcn::ActionEvent& actionEvent);

	PropertyBinding();
	virtual ~PropertyBinding() {}
private:
	PropertyList		m_PropertyList;

	PropertyPtr Get(const String &_name);
};

//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////

#endif
