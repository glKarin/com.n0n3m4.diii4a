
// With GCC and xCode, a bool is 4 bytes. There are two approaches to 
// making bool one byte. You could #define bool as an unsigned char. The downside
// is that a function that take bool and one that takes a char will no longer have a unique function
// signature. You could #define bool to custom class that defines a bool operator, which
// fixes the function signature but includes others problems such as bool bitfields, 
// classes that have a bool operator, C function that accept ... as a param, the keyword
// volatile.

// The following approach works for the best for Doom because of the above issues, bitfields
// especially

#ifdef bool
#undef bool
#endif

#define bool unsigned char

