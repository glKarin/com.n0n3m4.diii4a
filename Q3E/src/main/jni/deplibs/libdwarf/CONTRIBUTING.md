Thank you for considering contributing to libdwarf
development or documentation!

Any and all email mentions here mean send to
libdwarf -at- linuxmail -dot- org.

## Testing
Contributions should provide open-source (small)
test cases and instructions to
prove the change is correct, or should
mention what existing tests show correctness,
or should explain how to test the change.
It's not necessary to actually modify
libdwarf-regressiontests, but documenting
(in some way) how this change can be tested is important.

## Formalities
If your contribution is just a few lines
there is no paperwork to deal with.
Generate a github pull request or generate
a git diff and email it.

If your intended contribution is more than a
few lines and someone in your organization/company
has already established a relationship with the
project there is no paperwork needed.
Email to enquire
if you are already covered (name the relevant
company/organization you are working for).

Otherwise
there is a simple form to fill out and
sign (email a pdf of the signed document).
Either https://www.prevanders.net/generic_takebacklite.txt
or
https://www.prevanders.net/special_takebacklite.txt
will do. We will sign to and return via email
a copy with all signatures.
It is intended to assure the project that
you have the right to contribute your code, that it's not
code owned by anyone else.
These forms were prepared by SGI / SUN (respectively)
company lawyers 20+ years ago,
not by the library authors ourselves....

If you are thinking of making a contribution of more than a few
lines drop an email and describe what you have in mind.

The file CODINGSTYLE.md describes the coding style
in the project.  Formatting is simple and
the basic rule is 4 character
indents and no tabs.

All contributions should be correct for all the supported
platforms  (Linux (all), MacOS, and Windows Msys2) and
supported endianness.
That's nearly always trivial and needs no special effort.


## Tools for formatting
A simple C++ tool is in the project libdwarf-dicheck
containing the dicheck and trimtrailing program source.
Compile the two protrams and run dicheck  on any source
file you change and fix any warnings (the trimtrailing C++
program will remove trailing whitespace for you.).

## Python Formatting
All python code in the project is formatted with
the python3-standard formatter ```black```












