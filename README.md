
This is an alternative version of Rory Russell and Yezhou Zhang's CS165 final project. In final week, we don't have much time to communicate, so the branches don't have chance to be merged. 

But the frame of our code is shared and based on our co-work. the important functions, HRW hashing, bloomfilter, TLS API usage and main fucntion logic are completely same.

Contribution:
Rory Russell figured out libtls and wrote rw.h and related files
Yezhou Zhang figured out the bloom filter and rendezvous hashing and wrote bloomfilter.h and hash.h

the main difference is how we organize the code, this version is developed from starter code. So the usage is the same with starter code.

Example:
need libressl installed on local machine.

in build/

./server -port 3000
./proxy -port 2001 -localhost:3000
./client -port 2000 test.txt

Put the files in src/server/ for server to fetch.

Only this version has been fully tested on both of our machines. 

our github repository: 
Rory's branch: https://github.com/rruss003/tls/tree/master
Yezhou's branch: https://github.com/zhangyezhou/tls
or https://github.com/rruss003/tls/tree/yzhan736