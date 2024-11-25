#ifndef __REGULATOR_H__
#define __REGULATOR_H__

// class: Regulator
//		Simple Regulator class that simply provides a true/false return
//		type through IsReady() for simple checking if time interval has elapsed
//		It is also self resetting
class Regulator
{
public:

	bool IsReady();
	void SetMsInterval(int _msInterval) { m_NextUpdateTime += _msInterval - m_UpdateInterval;  m_UpdateInterval = _msInterval; }
	void SetTickRate(int _hz) { SetMsInterval(1000 / _hz); }
	int GetInterval() const { return m_UpdateInterval; }

	Regulator(int _msInterval = 0) :
			m_UpdateInterval(_msInterval),
			m_NextUpdateTime(0)
	  {
	  }
protected:
	int		m_UpdateInterval;
	int		m_NextUpdateTime;
};

// typedef: RegulatorPtr
//		Smart pointer to a regulator
#if __cplusplus >= 201103L //karin: using C++11 instead of boost
typedef std::shared_ptr<Regulator> RegulatorPtr;
#else
typedef boost::shared_ptr<Regulator> RegulatorPtr;
#endif

#endif
