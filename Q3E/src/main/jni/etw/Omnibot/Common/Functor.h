#ifndef __FUNCTOR_H__
#define __FUNCTOR_H__

// class: Functor
//		Base functor class, used for overloading operator() to do custom processing
class Functor
{
public:
    virtual int operator()()=0;
};

#if __cplusplus >= 201103L //karin: using C++11 instead of boost
typedef std::shared_ptr<Functor> FunctorPtr;
#else
typedef boost::shared_ptr<Functor> FunctorPtr;
#endif
typedef std::list<FunctorPtr> FunctorList;
typedef std::map<std::string, FunctorPtr> FunctorMap;

//////////////////////////////////////////////////////////////////////////

struct toLower
{	
	char operator() (char c) const  
	{
#ifdef WIN32
		return (char)tolower(c); 
#else
		return std::tolower(c); 
#endif
	}
};

struct toUpper
{
	char operator() (char c) const  
	{
#ifdef WIN32
		return (char)toupper(c); 
#else
		return std::toupper(c); 
#endif
	}
};

#endif
