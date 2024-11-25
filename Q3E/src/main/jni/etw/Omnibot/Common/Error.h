#ifndef __ERROR_H__
#define __ERROR_H__

// class: Error
//		Wraps more detailed error information to be used as return values for
//		various functions
class Error
{
public:

    enum Enum
    {
        // Messages that are passed around between any objects

		// enum: UNKNOWN
		//		Unknown error has occurred
        UNKNOWN,
		// enum: FILE_NOT_FOUND
		//		File not found error has occurred
        FILE_NOT_FOUND,
		// enum: INVALID_PARAMETER_TYPE
		//		Invalid parameter type error has occurred
        INVALID_PARAMETER_TYPE,
		// enum: FILE_NOT_FOUND
		//		Wrong number of parameters error has occurred
        WRONG_NUM_PARAMS,
        // This must be the last entry.
        NUM_ERRORS
    };

    Enum operator=(int _num);
    Enum operator=(Enum _enum);

    bool operator==(int _num);
    bool operator==(Error _error);

	// function: GetString
	//		Get the string name of this error
    const char *GetString();

    Error(void);
    Error(int _num);
    virtual ~Error(void) {};
private:
    Enum    m_Enum;

    static const char           *m_ErrorTxt[];
    static const unsigned int   m_NumItems;

};

#endif
