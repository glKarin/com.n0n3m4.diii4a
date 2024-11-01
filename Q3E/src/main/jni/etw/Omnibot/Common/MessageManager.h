#ifndef __MESSAGEMANAGER_H__
#define __MESSAGEMANAGER_H__

#include "CommandReciever.h"
#include "GoalManager.h"

// class: MessageManager
//		Handles triggers recieved from the game. Calls necessary script
//		or function callbacks for any trigger that has callbacks registered
//class MessageManager : public CommandReciever
//{
//public:
//
//	
//protected:
//
//	MessageManager();
//	~MessageManager();
//	MessageManager &operator=(const MessageManager&);
//};

template<typename MSG_TYPE, int MAX_MSGS = 128, typename SUBSCRIBER_TYPE = obuint64, bool THREADSAFE = false>
class MessageDepot
{
public:
	static const int MAX_SUBSCRIBERS = sizeof(SUBSCRIBER_TYPE) * 8;

	const char *GetSubScriberName(int _subscriber)
	{
		return m_SubscriberName[_subscriber] ? m_SubscriberName[_subscriber] : "";
	}

	int GetNumMessagesForSubscriber(int _subscriber) const
	{
		return m_NumMessagesForSubscriber[_subscriber];
	}

	int Subscribe(const char *_name)
	{
		// look for a free subscriber slot
		for(int i = 0; i < MAX_SUBSCRIBERS; ++i)
		{
			if(!(m_CurrentSubscriberMask>>i) & 1)
			{
				m_NumMessagesForSubscriber[i] = 0;
				m_SubscriberName[i] =_name;
				m_CurrentSubscriberMask |= ((SUBSCRIBER_TYPE)1<<i);
				return i;
			}
		}
		assert(0 && "No Subscriber Slots Available!");
		return -1;
	}

	void UnSubscribe(int &_subscriber)
	{
		if(_subscriber == -1)
			return;

		bool bPack = false;

		// clear this subscriber from the masks
		const SUBSCRIBER_TYPE tmask = ~((SUBSCRIBER_TYPE)1<<_subscriber);

		m_SubscriberName[_subscriber] = 0;

		// open this subscriber slot to re-use.
		m_CurrentSubscriberMask &= tmask;

		// mark all messages read for this subscriber
		if(m_NumMessagesForSubscriber[_subscriber] != 0)
		{
			for(int i = 0; i < m_CurrentMsgIndex; ++i)
			{
				// clear this subscriber from all messages
				m_MsgMask[i] &= tmask;

				// if this message waits on nothing else, get rid of it
				if(m_MsgMask[i] == 0)
					bPack = true;
			}
		}

		if(bPack)
			Pack();

		_subscriber = -1;
	}

	bool Post(const MSG_TYPE &_message, int _subscribermask = 0)
	{
		if(m_CurrentSubscriberMask)
		{
			//if(THREADSAFE)

			if(m_CurrentMsgIndex >= MAX_MSGS)
			{
				OBASSERT(0, "Message Queue Full!");
			}
			else
			{
				// store the message in the list
				m_Messages[m_CurrentMsgIndex] = _message;

				// mark this message with the mask representing what subscribers it should got to
				m_MsgMask[m_CurrentMsgIndex] = _subscribermask ? _subscribermask : m_CurrentSubscriberMask;

				// increment the message counts for all subscribers
				int iIndex = 0;
				SUBSCRIBER_TYPE subscribers = m_MsgMask[m_CurrentMsgIndex];
				while(subscribers)
				{
					m_NumMessagesForSubscriber[iIndex++] += subscribers&1 ? 1 : 0;
					subscribers>>=1;
				}

				++m_CurrentMsgIndex;

#ifdef _DEBUG
				if(m_CurrentMsgIndex > m_BufferMax)
				{
					m_BufferMax = m_CurrentMsgIndex;
					MaxChanged();
				}
#endif
				return true;
			}
		}
		return false;
	}

	int Collect( MSG_TYPE* _messageout, int _maxmsgs, int _subscriber )
	{
		bool bPack = false;

		int iWriteIndex = 0;

		if(m_NumMessagesForSubscriber[_subscriber] > 0)
		{
			SUBSCRIBER_TYPE subMask = (1<<_subscriber);
			for(int i = 0; i < m_CurrentMsgIndex && iWriteIndex < _maxmsgs; ++i)
			{
				if(m_MsgMask[i] & subMask)
				{
					_messageout[iWriteIndex++] = m_Messages[i];
					m_NumMessagesForSubscriber[_subscriber]--;

					m_MsgMask[i] &= ~subMask;
					if(!m_MsgMask[i])
						bPack = true;
				}
			}
		}

		if(bPack)
			Pack();

		return iWriteIndex;
	}

#ifdef _DEBUG
	void MaxChanged()
	{
		Utils::OutputDebug(kInfo, "Max Buffer Size: %d", m_BufferMax);
	}
#endif

	MessageDepot() 
		: m_CurrentMsgIndex(0)
		, m_CurrentSubscriberMask(0)
#ifdef _DEBUG
		, m_BufferMax(0)
#endif
	{
	}
private:
	void Pack()
	{
		int iSource = 0, iDestination = 0, iEnd = 0;
		for(; iDestination < m_CurrentMsgIndex; ++iDestination)
		{
			if(iSource <= iDestination)
				iSource = iDestination+1;   
			while(!m_MsgMask[iDestination] && iSource < m_CurrentMsgIndex)
			{
				// look for a valid message to move over
				for(; iSource < m_CurrentMsgIndex; ++iSource)
				{
					if(m_MsgMask[iSource])
					{
						m_Messages[iDestination] = m_Messages[iSource];
						m_MsgMask[iDestination] = m_MsgMask[iSource];
						m_MsgMask[iSource] = 0;
						++iSource;
						break;
					}                   
				}
			}

			if(m_MsgMask[iDestination])
				iEnd = iDestination+1;
		}
		m_CurrentMsgIndex = iEnd;
	}

	MSG_TYPE			m_Messages[MAX_MSGS];
	SUBSCRIBER_TYPE		m_MsgMask[MAX_MSGS];

	int					m_NumMessagesForSubscriber[MAX_SUBSCRIBERS];
	const char *		m_SubscriberName[MAX_SUBSCRIBERS];

	int                m_CurrentMsgIndex;            // next available message slot
	SUBSCRIBER_TYPE    m_CurrentSubscriberMask;    // mask of active subscribers

#ifdef _DEBUG
	int					m_BufferMax;
#endif
};


#endif