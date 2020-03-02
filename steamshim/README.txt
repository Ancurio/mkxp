What is this?

- This is a small piece of code, written in C++, that acts as a bridge between
a child process and the Steamworks SDK. The child process links against a
small piece of code, written in C, to facilitate communication with the
bridge.

What would I ever need this for?

- You have a GPL-licensed game and can't link directly against the Steamworks
SDK. The child process links against the simple, open source C code, which
talks to the open source C++ code via a pipe, which talks to Steamworks. You
can now add Steam achievements to your game without violating the GPL.


How does it work?

- You get a copy of the Steamworks SDK, and link steamshim_parent.cpp against
it. You ship that program and the steam_api.dll (or whatever) with your game.

- The parent process (the C++ code) gets launched as if it were your game. It
initializes Steamworks, creates some pipes and launches your actual game, then
waits on your game to talk to it over those pipes.

- Your game links against steamshim_child.c and at some point #includes
steamshim_child.h to use this functionality.

- Your game calls STEAMSHIM_init() at startup to prepare the system. It can
then make other STEAMSHIM_*() calls to interact with the parent process and
Steamworks.

- Your game, once a frame, calls STEAMSHIM_pump() until it returns NULL. Each
time it doesn't return NULL is a new message (event) from the parent process.
Often times, this is results from a command you asked the parent process to
do earlier, or some other out-of-band event from Steamworks.

- Your game, when shutting down normally, calls STEAMSHIM_deinit(), so the
parent process knows you're going away. It says goodbye, shuts down the pipe,
waits for your game to terminate, and then terminates itself.


Is this all of Steamworks?

- No! It's actually just enough to deal with stats and achievements right now,
but it can definitely be extended to offer more things. Take a look at


How do I get the Steamworks SDK?

- Go to https://partner.steamgames.com/ and login with your Steam account. You
can agree to some terms and then download the SDK.


Is there an example?

- That would be testapp.c. This example expects you to own Postal 1 on Steam and
will RESET ALL YOUR ACHIEVEMENTS, so be careful running it. But hey, if you
lose your work, it's a good exercise in SteamShim usage to put them back
again.  :)

Questions? Ask me.

--ryan.
icculus@icculus.org


