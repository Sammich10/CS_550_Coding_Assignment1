make
./server.o > server.log &

echo server started in background. Run pkill -f ./server.o to kill it when done.
echo sleeping 3 seconds to let server start up
sleep 1
echo 2
sleep 1
echo 1
sleep 1

./client.o
