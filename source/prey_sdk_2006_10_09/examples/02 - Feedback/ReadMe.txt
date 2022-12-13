Now that we've accomplished everything involved in getting a mod up and running with Basic Mod, let's do something
that produces more immediate feedback and actually changes the game behavior. We'll start out doing something very
simple, before we move onto something more advanced in the next example. Fire up the codebase again, and open up
game_player.cpp. Head to the function hhPlayer::GetViewPos, and you'll see a line that looks like this:

	axis = TransformToPlayerSpace( (GetUntransformedViewAngles() + viewBobAngles + playerView.AngleOffset(kickSpring, kickDamping)).ToMat3() );

Let's replace it with this chunk of code:

	//viewswagger begin
	idAngles swaggerOffset;
	const float swagger = 10.0f;
	swaggerOffset.roll = 0.0f;
	swaggerOffset.pitch = idMath::Sin(MS2SEC(gameLocal.time))*swagger;
	swaggerOffset.yaw = idMath::Cos(MS2SEC(gameLocal.time))*swagger*0.5f;
	axis = TransformToPlayerSpace( (swaggerOffset + GetUntransformedViewAngles() + viewBobAngles + playerView.AngleOffset(kickSpring, kickDamping)).ToMat3() );		
	//viewswagger end

Now compile and take the gamex86.dll, and do the same thing we did with it in the Basic Mod example (though you will
probably want to use something other than mymod, such as, say, viewswagger). When you load a map up, you will now
notice that Tommy has a little view swagger going on. This is accomplished by adding some simple wave-based angles to
the final view axis in the code above. Now that we have observed our first changes in game behavior, it's time to
move onto the next example, where we actually get to add our own new type of entity.


Rich Whitehouse
( http://www.telefragged.com/thefatal/ )
