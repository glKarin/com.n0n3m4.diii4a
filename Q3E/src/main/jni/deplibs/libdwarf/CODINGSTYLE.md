# Libdwarf/dwarfdump etc  coding style.

Latest update (minor clarifications): 7 July 2023

Adapted from the Cairo coding style document

Libdwarf/dwarfdump etc  coding style.

In "Tools that help" we provide a tool (dicheck)
that enforces some parts of the coding style.
Please don't submit changes (longer
than a few lines) that cause messages from dicheck.

This document is intended to be a short description of the
preferred coding style for the libdwarf and
related source code. Good style
requires good taste, which means this can't all be reduced
to automated rules, and there are exceptions.

We want the code to be easy to understand and maintain, and
consistent style plays an important part in that, even if
some of the specific details seem trivial. If nothing else,
this document gives a place to put consistent answers for
issues that would otherwise be arbitrary.

We avoid using features of C standardised
after ANSI C89,ISO C90.

Most of the guidelines here are demonstrated by examples,
(which means this document is quicker to read than it might
appear given its length). Most of the examples are positive
examples that you should imitate. The few negative examples
are clearly marked with a comment of /* Yuck! */. Please
don't submit code for libdwarf that looks like any Yuck!

Section list:

    Tab characters
    Braces
    Whitespace
    Struct Format and Content
    Managing nested blocks
    Test or Loop with Side Effect
    Switch statements
    Singly Linked Lists
    Macro Tests Commented
    Lookup Tables
    Memory allocation
    Naming, namespace
    Checking For Overflow
    Dwarfdump Flags Data
    Never use strcpy strcat strncpy
    Static data and functions
    Argument Lists
    Tools that help

### Indentation
Each new level is indented 4 more spaces than the previous level,
and an if is followed by a space and a left-parenthesis:

    if (condition) {
        do_something();
    }
Done solely with space characters.

Similarly, for is to have a single space after the r followed by
a left parenthesis.

    for (v; condition; v2) {
        do_something();
    }

### Tab characters

The tab character must never appear except in
a printf string.

### Braces
Most of the code in libdwarf uses bracing in the style of K&R:

    if (condition) {
        do_this();
        do_that();
    } else {
        do_the_other();
    }

and that is the preferred style.

Even if all of the substatements of an if statement are single
statements, the optional braces must always appear.

    if (condition) {
        do_this();
    } else {
        do_that();
    }

Never
    if (condition)      /* Yuck */
        do_this();      /* Yuck */
    else                /* Yuck */
        do_the_other(); /* Yuck */

Note that this last example also shows a situation in which the
opening brace can be on its own line, though usually it looks
like this example. The following can be difficult to read.

	if (condition &&
	    other_condition &&
	    yet_another) { /* But we usually do put the brace here */	
	    do_something();
	}

### Whitespace

Separate logically distinct chunks with a single newline. This
obviously applies between functions, but also applies within
a function or block and can even be used to good effect
within a structure definition. But don't get carried away
with blank lines:

    struct _Dwarf_Data_s {
        struct Something_s  op;
        double         tolerance;
        Dwarf_Unsigned line_width;
        Dwarf_Unsigned line_height;
        Dwarf_Unsigned line_thickness;

        /*  Notice the _s on struct declarations
            And notice indent of this comment
            and the blank line above this comment to
            make it clear the comment applies to fill_rule. */
        struct Reg_Struct_s  fill_rule;
        Dwarf_Signed   distance_from_end;
        ...
    };

Having both CamelCase in names and _ in the names
used by the library
is perhaps unusual, but it's been that way
in the libdwarf source since the library was
first written.
So we stick with it in most cases.

Never use a space before a function-call left parenthesis
or a macro-call left parenthesis.

Don't remove newlines just because things would still fit
on one line if removal breaks the expected visual structure of
the code.

Eliminate trailing whitespace on any line. Also, avoid putting
initial or final blank lines into any file, and never use
multiple blank lines instead of a single blank line.
The trimtrailing program in the libdwarf-dicheck
project will fix such issues.

Use dicheck tools below to find trailing whitespace
and indentation inconsistencies.

You might find the git-stripspace utility helpful which
acts as a filter to remove trailing whitespace as well as
initial, final, and duplicate blank lines.

As a special case of the bracing and whitespace guidelines,
function definitions  in libdwarf should always take the following form:

    int
    my_function (Dwarf_Debug dbg, Dwarf_Unsigned second_arg,
        Dwarf_Unsigned* ret_value;
        Dwarf_Error *error)
    {
        do_my_things();
        /*  The meaning of life */
        *ret_value = 42;
        return DW_DLV_OK;
    }

In dwarfdump, the code tends to follow this form
too (but not always).

Function prototypes inside libdwarf headers
(as opposed to .c files)
usually have the return type
(and associated specifiers and qualifiers) the same
line as the function name. If the line gets long
additional lines are appropriate.

In .c files the function name starts a line as shown above.

Notice, above, how a too-long argument line gets folded
with standard indentation(4).

Break up long lines (> 70 characters) and use whitespace to
align things nicely indenting by
four spaces. For example the arguments in a long list
to a function call should all (but the first) be aligned with
each other:

    align_function_arguments (argument_the_first,
        argument_the_second,
        argument_the_third);

And as a special rule, in a function prototype, (as well as
in the definition), whitespace should be inserted between
the parameter types and names so that the names are aligned:

    void
    align_parameter_names_in_prototypes(const char *char_star_arg,
        int	     int_arg,
        double	*double_star_arg,
        double	 double_arg);

Parameters with a * prefix are aligned one character
to the left so that the actual names are aligned.

### Struct Format and Content

Usually, avoid adding a comment alongside declarations
unless it is a very short comment.  The following example
uses meaningless x y z field names (sorry), but but does
show a short prefix related to the struct name.  Generally
struct members should have a prefix common to that struct and
hopefully distinct from other struct declarations.  This short
prefix makes it much easier to find all uses of a particular
field in the code (with grep).

    struct Dwarf_Nothing_Special_s {
        int ns_x; /* usually avoid comment here this is Yuck!
            and this is even worse as continuation.  Yuck! */

        /*  Comment here about ns_y with blank line above
            and below to make it clear which line
            referred to. */
        unsigned ns_y;

        unsigned ns_z;
    };

### Managing nested blocks

Long blocks that are deeply nested make the code very hard
to read. Fortunately such blocks often indicate logically
distinct chunks of functionality that are begging to be split
into their own functions. Please listen to the blocks when
they beg.  You will notice many exceptions to this in the
code, unfortunately.

In other cases, gratuitous nesting comes about because the
primary functionality gets buried in a nested block rather
than living at the primary level where it belongs. Consider
the following:

    foo = malloc (sizeof (foo_t));
    if (foo) { /* Yuck! The error return becomes
               hard to see.  */
        ...
        /* lots of code to initialize foo */
        ...
        return DW_DLV_OK;
    }
    _dwarf_error(dbg,error,DW_DLE_MALLOC_RETURNS_NULL);
    return DW_DLV_ERROR;

This kind of gratuitous nesting can be avoided by following
a pattern of handling exceptional cases early and returning:

    foo = malloc (sizeof (foo_t));
    /*  A test foo == NULL is fine but the following
        is better as there is  no danger of
        turning == into = by accident. */
    if (!foo) {
        _dwarf_error(dbg,error,DW_DLE_MALLOC_RETURNS_NULL);
        return DW_DLV_ERROR;
    }
    ...
    lots of code to initialize foo */
	
    return DW_DLV_OK;

In otherwords, refactor deeply nested code.
There are places in the library and dwarfdump
where appropriate refactoring has not yet been
done.

The return statement is often the best thing to use in a
pattern like this. If it's not available due to additional
nesting above which require some cleanup after the current
block, then consider splitting the current block into a new
function before using goto.

### Test or Loop with Side Effect

Never do:

    if ((foo = malloc (sizeof (foo_t)))) {

or

    if (foo = malloc (sizeof (foo_t))) {

as the reader has to think carefully about it,
whereas

    foo = malloc (sizeof (foo_t));
    if (foo) {

is more transparent (in some sense) and
makes it easier to stop( in debugger) or
add a printf in case this is a point where
things might be going wrong somehow.

Note the space between if and the left
parenthesis.

Also see "Managing nested blocks" just above.

### Switch statements

We use the following indent practice:

    switch(x) {
    case a: {
        /*  Define&use local variables,  do something */
    }
    case b:
        /*  Do something, no new local variable
            needed. */
        break;
    ...
    default: /* do something or break*/
    }

The default case is used everywhere (instead
of being omitted when it could be omitted)
to satisfy a Cobra rule. One thinks
consistency reads well.

dwarf_names.c (for example) is full of functions
where all known values do a return on each case:
and there the last entry is default: break;
followed outside the switch by the return for
the error case.

### Singly Linked Lists

Many places in libdwarf build singly-linked lists.
And do it without a check for NULL. Stephen Macguire, in
Writing Solid Code (Microsoft Press, 1993) explains
how and why one should do this on page 126.
We assume it's well known. The same book's
"Candy Machine Interfaces" chapter lead to the design
of the return values used in the library.

### Macro Tests Commented

    #ifdef SOME_MACRO
    #else /* !SOME_MACRO */
    #endif /* !SOME_MACRO */

    #ifdef OTHER_MACRO
    #endif /* OTHER_MACRO */

The comments add clarity when one is not familiar with code in
the area but are not required if the #else #endif are within a
couple lines of the #if(def).  In a spot with nested #if(def)
the comments become necessary to avoid confusing the reader.
There is nothing wrong with adding them everywhere.

### Lookup Tables

Any situation requiring a lookup table should use one
or the other tsearch functions. The practical ones are
dwarf\_tsearchhash.c and dwarf\_tsearchbal.c.  dwarfdump and
libdwarf each use one of them and only one of them.  See the
tsearch-code project to see the full set available.

These use the traditional UNIX tsearch arguments and return
values even though those are not good designs by current
standards. Expanded to have destroy() functions whose prototype
was copied from GNU man pages.

GNU libc has much nicer (in the sense of much nicer interface
designs)  non-standard tsearch functions, but we've ignored
those to keep to the official Single Unix Specification
standard interfaces.

### Memory allocation

In general, be wary of performing any arithmetic operations
in an argument to malloc.  You should explicitly check for
integer overflow yourself in any more complex situations.

In libdwarf most allocations use \_dwarf\_alloc() instead of
malloc.  And the tables of valid types (which are predefined
in libdwarf) allow for constructor/destructor functions.
The \_dwarf\_alloc() code keeps a record of what is allocated
so a careless user can simply call dwarf\_finish() at the end
and all the allocated data will be freed that was not already
freed via  user calls to dwarf\_dealloc().

### Naming, namespace
Avoid conflicting with other libraries
or with user code.  Anything publicly visible
in headers or in the library (.a or .so or .dll)
must have only certain prefixes to names
visible in dwarf.h or libdwarf.h or
a library a user links in (.a or .so or .dll).
Prefixes are:

    DW_  (this is from the DWARF Standard and dwarf.h)
    Dwarf_  (for types)
    dwarf_  (for functions)
    _dwarf_ (for non-static functions in the library but not
        for user code to call, so far this has worked
        acceptably). For a .so or .dll current builds
        (meaning September 2021 and later)
        ensure such names are invisible to callers.
    dw_ For public function argument names.

dwarf.h and libdwarf.h functions must have all arguments named
with a leading dw\_ so that doxygen has names to work with
and to make the function prototypes more readable.
Ensure new functions or changes are reflected in
the libdwarf.h doxygen comments.

Function names should be all lower-case and should
use underbar(s) for readability.

Function-local variables should be lower-case with
underbars for readability.   It's ok for a small loop
with counters to use single letter names like i or k or m.

structure members should have a struct-specific
2-character prefix to the name (followed by
an underbar). This convention makes it much
easier to grep for uses of members.


### Checking For Overflow

Libdwarf and dwarfdump are often dealing with offsets and
indexes read from disk object files and all such should be
checked before use.  That approach means that dwarfdump (and
libdwarf callers in general) need not check the things that
libdwarf has returned (at least those libdwarf could check).

When it is possible that  X +Y might overflow make every effort
to check X an Y independently before attempting an addition.
If you are confident X and Y are sensible (given available data
like section sizes) you can add them and then determine if
the sum (say an offset) is still within the relevant section
or data-item usable range.

There are many libdwarf-internal functions to read data from
an object, and all of them require an end-pointer argument
so the code can easily check for corrupt object-file or DWARF
values without duplicating the error-code-setting.

### Dwarfdump Flags Data

Dwarfdump has a large number of options and nearly all  the
option  values are recorded as fields in a single global
structure  glflags (see dwarfdump.c).  Option data not in that
structure should be moved into it, in general. This makes
it much simpler to find instances using flags and to know,
reading dwarfdump source, when flags are being referred to..

The text in this document (not the examples!)  was formatted
with the vi command "!}fmt -64"...without the quotes.

### Never use strcpy strcat strncpy

dwarfdump has dd\_safe\_strcpy().

libdwarf has \_dwarf\_safe\_strcpy().

These functions eliminate the need for the three traditional
string functions strcpy, strcat, strncpy and do exactly what
is needed safely while doing nothing extra.

### Static data and functions

Any  data or function not referenced outside the
defining source file should be declared 'static'.

In the libdwarf library itself static data is
not appropriate except for data which is also
const.
Multiple Dwarf_Debug
may be open at the same time in a single
program (dwarfdump or user code) so static data
will eventually be a bug.
### Argument Lists

Beginning February 2023 we are moving to presenting
argument lists with no space between any
pointer * and the immediately following argument name.
And with such lists mostly lining up the names
(with the first argument usually not lined up with anything).

	functionname(Dwarf_Debug x,
        Dwarf_Off   *out_value,
        Dwarf_Error *error)

This format is suggested, not required.


### Tools that help

The

    github.com/davea42/libdwarf-dicheck project

is a tool that checks indentation and a few other things to
find instances of  failures to follow codingstyle rules. The
program trimtrailing in that project removes trailing
whitespace and changes a sequence of blank lines to a single
blank line.

The open-source project

    github.com/nimble-code/Cobra

package is being used (beginning December 2021) as an aid to
finding problematic or not-best-practices code.

It does not compile anything, it does checking via source
scans with regular expressions.  Not everything it mentions
is necessarily a bug or a defect.

The primary commands used (in a source directory) are

    cobra -f basic *.h
    cobra -f basic *.c
    cobra -terse -f stats *.[ch]
    cobra -terse -f metrics *.[ch]
    # The cwe command generates lots of spurious output as they
    # check things we do not care much about.
    # But some of it has been useful.
    cwe   *.h
    cwe   *.c
