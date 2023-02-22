
if [ $# -eq 1 ]
then
    a=$1
else
    echo "Enter number of clients:"
    read a
fi

if [ $a -lt 4 ]
then
    t=$a
else
    t=$(($a / 2))
fi


make

if [ ! -d "logs" ]; then
  mkdir "logs"
fi
if [ ! -d "results" ]; then
  mkdir "results"
fi

rm logs/*
rm results/${a}_clients_parallel_result.txt
rm results/${a}_clients_parallel_result.csv

sleep 1

./server.o > logs/serverlog.txt &
echo letting server start up
sleep ${t}
echo generating ${a} clients to download 128 bytes serially
for (( i = 1; i <= ${a}; i++ ))
do
    ./client.o 127.0.0.1:8081 serial t128b1.txt,t128b2.txt,t128b3.txt,t128b4.txt,t128b5.txt,t128b6.txt,t128b7.txt,t128b8.txt,t128b9.txt,t128b10.txt ./downloads/ > logs/clientlog128buser${i}.txt &
done

sleep ${t}
echo generating ${a} clients to download 512 bytes serially
for (( i = 1; i <= ${a}; i++ ))
do
    ./client.o 127.0.0.1:8081 serial t512b1.txt,t512b2.txt,t512b3.txt,t512b4.txt,t512b5.txt,t512b6.txt,t512b7.txt,t512b8.txt,t512b9.txt,t512b10.txt ./downloads/ > logs/clientlog512buser${i}.txt &
done

sleep ${t}
echo generating ${a} clients to download 2 kilobytes serially
for (( i = 1; i <= ${a}; i++ ))
do
    ./client.o 127.0.0.1:8081 serial t2kb1.txt,t2kb2.txt,t2kb3.txt,t2kb4.txt,t2kb5.txt,t2kb6.txt,t2kb7.txt,t2kb8.txt,t2kb9.txt,t2kb10.txt ./downloads/ > logs/clientlog2kuser${i}.txt &
done

sleep ${t}
echo generating ${a} clients to download 8 kilobytes serially
for (( i = 1; i <= ${a}; i++ ))
do
    ./client.o 127.0.0.1:8081 serial t8kb1.txt,t8kb2.txt,t8kb3.txt,t8kb4.txt,t8kb5.txt,t8kb6.txt,t8kb7.txt,t8kb8.txt,t8kb9.txt,t8kb10.txt ./downloads/ > logs/clientlog8kuser${i}.txt &
done

sleep ${t}
echo generating ${a} clients to download 32 kilobytes serially
for (( i = 1; i <= ${a}; i++ ))
do
    ./client.o 127.0.0.1:8081 serial t32kb1.txt,t32kb2.txt,t32kb3.txt,t32kb4.txt,t32kb5.txt,t32kb6.txt,t32kb7.txt,t32kb8.txt,t32kb9.txt,t32kb10.txt ./downloads/ > logs/clientlog32kuser${i}.txt &
done

sleep ${t}

echo shutting down server
pkill -f ./server.o

echo putting results in results/

for i in {1..5}
do
    if [ $i -eq 1 ]
    then
        s=128b
    elif [ $i -eq 2 ]
    then
        s=512b
    elif [ $i -eq 3 ]
    then
        s=2k
    elif [ $i -eq 4 ]
    then
        s=8k
    elif [ $i -eq 5 ]
    then
        s=32k
    fi
    for (( j = 1; j <= ${a}; j++ ))
    do
        tail -3 logs/clientlog${s}user${j}.txt >> results/${a}_clients_serial_result.txt
    done
done

sleep 1

awk '/Total bytes downloaded:/ {b=$NF} /Total time spend downloading:/ {print b","$NF}' results/${a}_clients_serial_result.txt > results/${a}_clients_serial_result.csv

awk '{print $2","$5}' logs/serverlog.txt > results/${a}_clients_serial_server_result.csv
