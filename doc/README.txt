The code is written such that the programs should be called from the base directory and not from inside build.
e.g. use ./build/client instead of cd build && ./client. A script has been provided in the base directory named "c" that can be used to test the program once it is built.

The work distribution was approximately as follows:
Rory Russell figured out libtls and wrote rw.h and related files
Yezhou Zhang figured out the bloom filter and rendezvous hashing and wrote bloomfilter.h and hash.h.
We mainly cooperated through GitHub, but also used Visual Studio Live Share and Slack.