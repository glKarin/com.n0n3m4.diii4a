#ifndef __NAMEMANAGER_H__
#define __NAMEMANAGER_H__

class NameReference
{
public:
	friend class NameManager;

	const String &GetName() { return m_Name; }
	const String &GetProfileName() { return m_ProfileName; }

	NameReference(const String &_name = "", const String &_profile = "");
	~NameReference();
private:
	String		m_Name;
	String		m_ProfileName;
};

#if __cplusplus >= 201103L //karin: using C++11 instead of boost
typedef std::shared_ptr<NameReference> NamePtr;
#else
typedef boost::shared_ptr<NameReference> NamePtr;
#endif


// class: NameManager
class NameManager
{
public:
	
	bool AddName(const String &_name, const String &_profile);
	void DeleteName(const String &_name);
	void ClearNames();
    
	void SetProfileForClass(const int _class, const String &_name);

	NamePtr GetName(const String &_preferred = "");

	const String GetProfileForName(const String &_name) const;
	const String GetProfileForClass(int _class) const;

	//void LoadBotNames();

	static NameManager *GetInstance();
	static void DeleteInstance();
protected:

	typedef std::map<String, NamePtr> NamesMap;

	NamesMap	m_NamesMap;

	// typedef: DefaultProfileMap
	typedef std::map<int, String> DefaultProfileMap;
	DefaultProfileMap		m_ProfileMap;

	static NameManager		*m_Instance;
	NameManager() {};
	~NameManager() {};
	NameManager &operator=(const NameManager&);
};

#endif

