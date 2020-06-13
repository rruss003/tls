#!/bin/sh
if pgrep server 2>/dev/null; then
    pkill server
fi >/dev/null;
if pgrep proxy 2>/dev/null; then
    pkill proxy
fi >/dev/null;
# rm test >/dev/null;
./build/server 4951 & ./build/proxy -port 4952 -servername:4951 & ./build/client -port 4951 testfile