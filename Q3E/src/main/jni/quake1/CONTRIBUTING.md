# DarkPlaces Contributing Guidelines
-------------------------------------------------------------------------------

1. ### Do not break Quake or its mods, and any other game using the engine.

   The engine has known compatibility issues with Quake and many community
   mods. All code must not make the situation worse. This is analogous to the policy
   of the Linux kernel to not break userspace.

2. ### Sign off all of your commits if they are to be included upstream.

   You must use a valid, permanent email address.

2. ### All code submitted should follow the Allman style for the most part.

	1. In statements, the curly brace should be placed on the next line at the
	   same indentation level as the statement. If the statement only involves
	   a single line, do not use curly braces.

		```c
		// Example:
		if(foo == 1)
		{
			Do_Something();
			Do_Something_Else();
		}
		else
			Do_Something_Else_Else();

		if(bar == 1)
			Do_Something_Else_Else_Else();
		```

	2. Use tabs for indentation.

	   Use spaces subsequently for alignment of the
	   parameters of multi-line function calls or declarations, or statements.

		```c
		// Example:
		if(very_long_statement &&
		   very_long_statement_aligned &&
		   foo)
		{
			if(indented)
			{
				Do_Something();
				Do_Something_Else();
			}
		}
		```

	3. If possible, try to keep individual lines of code less than 100
	   characters.

	4. Pointer operators should be placed on the right-hand side.

		```c
		int foo = 1;
		int *bar = &foo;
		```

	5. Type casts should have a space between the type and the pointer.

		```c
		int *foo = (int *)malloc(5);
		```

	6. Use spaces after each comma when listing parameters or variables of a
	   struct.

	7. Multi-line comments should be formatted like so:

		```c
		/*
		 * This is a multi-line comment.
		 * Sometimes, I dream about cheese.
		 */
		```

	   But this is okay too:

		```c
		/* This is another multi-line comment.
		 * Hiya! How are you?
		 */
		```

	8. If following these guidelines in any manner would make any code less
	   readable or harder to follow, ***ignore them***. This section is only
	   guidelines, not rules. We're not gonna beat you over the head in pull
	   requests because you misplaced a dereference operator.

	   For example, in some situations, placing the block on the same line as
	   the condition would be okay because it looks cleaner:

		```c
		if (foo)  Do_Something();
		if (bar)  Do_Something_Else();
		if (far)  near();
		if (boo)  AHH("!!!\n");
		```

4. DarkPlaces is written in the common subset of C and C++. This means it is
   (usually) both valid C and C++. We historically wanted to keep it that way,
   but don't worry about it unless you care enough and/or are a maintainer.

   Most of the differences are caught by `-Wc++-compat` but some are subtle
   and would cause behavior differences between the two compilers, or are not
   caught by that warning. The things to look out for are:

	1. Character constants are `int`-sized in C but `char`-sized in C++. This
       means `sizeof('a')` will be 4 in C, but 1 in C++.

	2. `goto label;` cannot jump over a variable initialization. This will
       cause a compiler error as C++ but is not caught by `-Wc++-compat`.
	   This is nevertheless bad code, so avoid this anyway.

   If, for some reason, you care enough, compatibility can always be tested
   affirmatively by setting CC=g++. CC=clang++ may not work in the future, if
   it didn't already stop working in current versions of LLVM, as it otherwise
   spams deprecation warnings for using a C file as input to a C++ compiler.

> [!NOTE]
> We do not officially support building C code with a C++ compiler and may not
> address issue reports for buggy behavior that does not occur when compiled
> with a C compiler. That said, there have been fleeting ideas for converting
> either the entire engine or parts of it to C++, which is what `-Wc++-compat`
> would make a little easier. So, we at least *do not discourage* such issue
> reports, especially for things the warning doesn't catch. They will be noted
> as they would become relevant in the event we do decide to convert to C++.