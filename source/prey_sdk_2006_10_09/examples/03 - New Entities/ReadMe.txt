In this example, we'll be adding a new trigger, which adds health to the player while he's standing in it. The basic
framework is fairly barebones (you could dress it up a lot, by adding, perhaps, a debounce time and some sounds), but
the intent is to demonstrate how to create a new type of entity and place it in your map.

Included in the "healthzone" subdirectory are 3 files. 2 of them are replacements for their ./src/Prey/ counterparts
(game_zone.cpp and game_zone.h), with the changes already made that we will be making in this example. The
healthzone.pk4 file contains new def and map files. The def file contains an entity entry for the new class we will
be creating, and the map has the health trigger placed and ready to test.

So, to sum up what we will be doing to get this example all up and running:

1) Merge the included game_zone.cpp and game_zone.h into your copy of the Prey SDK source.
2) Compile, and create a game00.pk4 as described in the Basic Mod example.
3) Create a mod folder as described in Basic Mod, something like "healthzone". Don't forget to create a
description.txt file in that folder as well, as per the Basic Mod example.
4) Copy the included healthzone.pk4 into that mod folder, along with your game00.pk4.
5) Start Prey up and activate your healthzone mod through the menu. Bring down the console and type "map healthzone".
Now pick up the autocannon and some grenades and hurt yourself. Then, step onto the platform in the room and observe
as your health shoots back up.

The actual code changes involved here are minimal. You should take a look at the contents in the new game_zone.cpp
and game_zone.h files, and search for the //healthzone begin and //healthzone end comments. These mark all of the
changes that were necessary to add this healthzone entity.

Note that the hhHealthZone inherits from the hhZone class. The hhZone class handles all of the main functionality for
determining if an entity is within it, and giving us callbacks. All we have to do is override the ValidEntity and
EntityEncroaching functions, in order to perform our own logic there. This is generally possible with many types of
entities that you will want to add. You should note, however, that you can go so far as to inherit directly from
idEntity or even idClass when you are creating drastically new types of objects or game systems.

You should also examine the contents of the def/mymoddefs.def file in the healthzone.pk4 file. It demonstrates how
you can add a new entity entry without modifying existing files (you should always do this when possible for
distributing maps with custom entity types - it allows your map to work without interfering with other custom maps
or the normal game), as well as demonstrating how the entityDef hooks up to the new hhHealthZone class that we added.

Hopefully this will get you well on your way to creating new game content. Good luck.


Rich Whitehouse
( http://www.telefragged.com/thefatal/ )
