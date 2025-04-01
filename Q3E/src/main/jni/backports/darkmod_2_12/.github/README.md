This is the official mirror of [TheDarkMod](https://www.thedarkmod.com) source code SVN repository.


### Origin

The original SVN repository is available at:

    https://svn.thedarkmod.com/publicsvn/darkmod_src

See also information in [the forum thread about development versions][1].


### Third Party

The history in this mirror is slightly cleaned up to reduce repository size.
Most importantly, it does **not** contain the binaries of third-party libs.

The binaries allow building the game easily on the officially supported platforms.
Without them, you'll have to build all third-party libs according to instructions in [ThirdParty/readme.md](../ThirdParty/readme.md).
The binaries are available in SVN repository, and you can download them from there.

The best approach is to checkout binaries as SVN repository:

    svn checkout https://svn.thedarkmod.com/publicsvn/darkmod_src/trunk/ThirdParty/artefacts/ ThirdParty/artefacts

After that, you can switch them to appropriate SVN revision like this (`XXXX` is revision number):

    svn update ThirdParty/artefacts -r XXXX

Note that third-party libs also change over time, but quite rarely.


### Build

The build instructions and information about workspace layout are available in [COMPILING.txt](../COMPILING.txt) file.


### Assets

In order to run the game, you also need compatible assets.
The best way to get assets is to install an appropriate dev build.
See the [forum thread about development builds][1] above for details.


 [1]: https://forums.thedarkmod.com/index.php?/topic/20824-public-access-to-development-versions
