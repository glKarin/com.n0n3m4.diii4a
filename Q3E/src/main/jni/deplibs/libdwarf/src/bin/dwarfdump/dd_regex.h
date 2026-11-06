/*
    regex - Regular expression pattern matching  and replacement

    By:  Ozan S. Yigit (oz)
        Dept. of Computer Science
        York University

    These routines are the PUBLIC DOMAIN equivalents of regex
    routines as found in 4.nBSD UN*X, with minor extensions.

    These routines are derived from various implementations found
    in software tools books, and Conroy's grep. They are NOT derived
    from licensed/restricted software.

    [comment by DavidAnderson]
    We are skipping the replacement function, which
    dwarfdump does not require.
*/
#ifndef DD_REGEX_H
#define DD_REGEX_H

int dd_re_comp(const char *);
int dd_re_exec(char *);

#endif /* DD_REGEX_H */
