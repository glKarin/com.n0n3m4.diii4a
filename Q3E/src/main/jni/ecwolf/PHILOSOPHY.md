Philosophy
==========

This document is designed to describe what falls under the scope of the project. Primarily these are guidelines for developers to determine if their goals align with the project and are not hard set rules.

General experience
------------------

When considering changes the following hierarchy should be observed:

1. ECWolf is first and foremost a Wolfenstein 3D (and derived games) source port. It is not a useful product if it isn't a good way to play the games that it supports.
2. The end-user/player is authoritative on their experience. Hint to what is the intended, but the player should be allowed to have the last say.
3. Finally, ECWolf is a tool for crafting new experiences.

In other words, while ECWolf proclaims its advanced modding capabilities, the focus is on the player experience first. For example ECWolf shouldn't have features that are intended to allow mod authors to tamper with control bindings, or prevent players from accessing debug mode.

Vanilla compatibility
---------------------

Vanilla here is defined as the original DOS wolf3d.exe, spear.exe, etc in their final form as provided by id Software (or equivalent developer for non-id games). This definition does not include any modified executables be it through source code changes or hex
editing.

ECWolf does aim to be compatible with vanilla mods including emulation of intentional use of engine exploits where possible.

ECWolf does not aim to be demo compatible with vanilla, and takes some liberties to change core code in such a way that while it may not produce 100% identical results, it should have the same high level behavior. The idea is to accelerate development of new features, allow APIs to conform better to user expectation, permit refactoring of core code, lift obvious limitations of the engine, and fix issues that would be considered by most people to be bugs in the game. This freedom should always be balanced against how likely a user is able to notice the change in behavior, and if noticeable to what degree the wider community would consider that change desirable.

As a general measuring stick for deviation, Fabien Sanglard's Wolfenstein 3D Game Engine Black Book should still be relevant reading as a high level of how ECWolf works.

Renderer
--------

ECWolf will always default to 8-bit paletted software ray casting. The ray caster should generally work in the same way as vanilla. The exception to this rule is if the SNES/Macintosh Wolfenstein 3D BSP renderer is implemented, then it may be used as the default when playing the Macintosh version of Wolfenstein 3D.

Alternative renderers are permitted as opt in features.

GZDoom compatibility
--------------------

Features in ECWolf which match those in GZDoom should strongly consider providing an API compatible solution. Due to the large degree of similarity between Wolfenstein 3D and Doom internals, there is not often a need to reinvent the wheel. By providing similar APIs ECWolf is able to leverage to some extent documentation, tutorials, and editing utilities already written for GZDoom.

Remember though that ECWolf is first and foremost a Wolfenstein 3D source port and, while compatibility is strongly advised, this guideline should not be taken to mean that deviation is not allowed.

Code should be shared between ECWolf and GZDoom where it makes sense, and changes should be synced upstream where relevant.

Backwards compatibility
-----------------------

During the 1.x series backwards compatibility with mods is not guaranteed. Some major features, such as full Z-axis support, still need to be implemented and ensuring API stability is not possible at this time.

Regardless, unless otherwise stated a break in compatibility with old mods should be considered a bug. Intentional changes should be documented on the wiki with instructions on what changes need to be made to mods.

This policy will change for 2.x and later.

Game support
------------

ECWolf intends to support all games derived from the Wolfenstein 3D DOS or Mac source code prior to the public source code release. Other Wolfenstein 3D engine derived games will be considered on a case by case basis. At this time the games based on the precursor engine are not being considered for inclusion, but this may be reevaluated.
