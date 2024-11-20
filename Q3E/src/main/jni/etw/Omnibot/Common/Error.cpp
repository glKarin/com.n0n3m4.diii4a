#include "PrecompCommon.h"
#include "Error.h"

const char *Error::m_ErrorTxt[] =
{
    "Unknown Error",
    "File Not Found",
    "Invalid Parameter Type",   
    "Wrong Number of Parameters",
};

const unsigned int Error::m_NumItems = sizeof(Error::m_ErrorTxt) / sizeof(Error::m_ErrorTxt[0]);

Error::Error(void) : 
    m_Enum(UNKNOWN) 
{
    BOOST_STATIC_ASSERT(sizeof(m_ErrorTxt) / sizeof(m_ErrorTxt[0]) == NUM_ERRORS);
}

Error::Error(int _num)
{
    m_Enum = (_num >= 0 && _num < NUM_ERRORS) ? (Enum)_num : UNKNOWN;
}

bool Error::operator==(int _num)
{
    return m_Enum == _num;
}

bool Error::operator==(Error _error)
{
    return m_Enum == _error.m_Enum;
}

const char *Error::GetString()
{
    return m_ErrorTxt[m_Enum];
}

Error::Enum Error::operator=(int _num)
{
    m_Enum = (_num >= 0 && _num < NUM_ERRORS) ? (Enum)_num : UNKNOWN;
    return m_Enum;
}

Error::Enum Error::operator=(Enum _enum)
{
    m_Enum = _enum;
    return m_Enum;
}
