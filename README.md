 Build & Execute
==================
  $ ./build.py
  $ export LD_LIBRARY_PATH=build/zcl/libs/:build/r5l/libs/:build/r5l-client/libs
  $ ./build/r5l-server/r5l-server

 Dev Notes
==================
  $ valgrind --leak-check=yes --leak-check=full --show-reachable=yes --track-origins=yes ./build/r5l-server/r5l-server &> valgrind.log
