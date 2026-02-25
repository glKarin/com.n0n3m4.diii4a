// Copyright (c) 2012 Cygnus Software
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
// Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

// For more details see the original blog post at:
// http://randomascii.wordpress.com/2012/04/21/exceptional-floating-point/

#ifdef _WIN32
#include <float.h> // For _clearfp and _controlfp_s

//#define ENABLE_FP_EXCEPTIONS 1

// Declare an object of this type in a scope in order to suppress
// all floating-point exceptions temporarily. The old exception
// state will be reset at the end.
// From http://randomascii.wordpress.com/2012/04/21/exceptional-floating-point/
class FPExceptionDisabler
{
public:
    FPExceptionDisabler()
    {
        // Retrieve the current state of the exception flags. This
        // must be done before changing them.
        _controlfp_s(&mOldValues, 0, 0);
        // Set all of the exception flags, which suppresses FP
        // exceptions on the x87 and SSE units. _MCW_EM is a bit
        // mask representing all available exception masks.
        _controlfp_s(0, _MCW_EM, _MCW_EM);
    }
    ~FPExceptionDisabler()
    {
        // Clear any pending FP exceptions. This must be done
        // prior to enabling FP exceptions since otherwise there
        // may be a 'deferred crash' as soon the exceptions are
        // enabled.
        _clearfp();

        // Reset (possibly enabling) the exception status.
        _controlfp_s(0, mOldValues, _MCW_EM);
    }

private:
    unsigned int mOldValues;

    // Make the copy constructor and assignment operator private
    // and unimplemented to prohibit copying.
    FPExceptionDisabler(const FPExceptionDisabler&);
    FPExceptionDisabler& operator=(const FPExceptionDisabler&);
};

// Declare an object of this type in a scope in order to enable a
// specified set of floating-point exceptions temporarily. The old
// exception state will be reset at the end.
// This class can be nested.
// From http://randomascii.wordpress.com/2012/04/21/exceptional-floating-point/
class FPExceptionEnabler
{
public:
    // Overflow, divide-by-zero, and invalid-operation are the FP
    // exceptions most frequently associated with bugs.
    FPExceptionEnabler(unsigned int enableBits = _EM_OVERFLOW |
                _EM_ZERODIVIDE | _EM_INVALID)
    {
        // Retrieve the current state of the exception flags. This
        // must be done before changing them.
        _controlfp_s(&mOldValues, 0, 0);

        // Make sure no non-exception flags have been specified,
        // to avoid accidental changing of rounding modes, etc.
        enableBits &= _MCW_EM;

        // Clear any pending FP exceptions. This must be done
        // prior to enabling FP exceptions since otherwise there
        // may be a 'deferred crash' as soon the exceptions are
        // enabled. _MCW_EM is a bit
        // mask representing all available exception masks.
        _clearfp();

        // Zero out the specified bits, leaving other bits alone.
        _controlfp_s(0, ~enableBits, enableBits);
    }
    ~FPExceptionEnabler()
    {
        // Reset the exception state.
        _controlfp_s(0, mOldValues, _MCW_EM);
    }

private:
    unsigned int mOldValues;

    // Make the copy constructor and assignment operator private
    // and unimplemented to prohibit copying.
    FPExceptionEnabler(const FPExceptionEnabler&);
    FPExceptionEnabler& operator=(const FPExceptionEnabler&);
};
#endif
