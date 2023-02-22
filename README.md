# <File Server and Client program implementation in C++>

[Description](#Description)

[Installation/Set-Up](#Installation/Set-Up)

[Usage](#Usage)

[Evaluation](#Evaluation)

[TL;DR](#TL;DR)

###NOTE!
To run the scripts included, you'll probably have to use "chmod +x script_name.sh"

##Description
This project includes 2 separate but functionally related programs: 
- a file server that can send files to clients

The server code sets up a socket to listen to incoming TCP connections and create a new thread to handle those connections. The server can listen on a user specified port from a configuration file or
from a passed argument. Once a connection is established, the `handle_connection` function is called in a newly created thread, and it begins by reading the data sent on the client socket. The messages 
consists of a message length (specified via a "header" struct) followed by the message. The message is expected to be a null-terminated string that will either be a command to be executed or a file name.
The only command that the server recognizes is "get_file_list", otherwise the message is considered to be a file name, which the server will attempt to locate in a folder called "files_in_server". If the
file exists, it will send a message back to the client saying the file was found, otherwise it will tell the client the file was not found and terminate the connection. In the case that the file is found
to exist, the server will send the client an MD5 hash of the file, and then the file content. After the file is sent, the server terminates the connection.

Within the "serverfiles/files_in_server/" directory, you can see the test files that will be available to be transferred by the server. There are 10
copies of 5 different file sises 


- a client to download files from the server

The client can be run in two ways, either "automatically" or "interactively". The usage of both is specified below. The basic functionality of the client is to get the list of files from the server (in "interactive" mode)
and download the file contents into a folder specified in a config file (or as an argument, check (#usage) for details). The program reads the client configuration file to obtain an IP address, port number, download attempts
(for the case of a corrupted download) and a download folder location. The client connects to the server using the specified IP address and port, and requests a file to download or the list of files that the server can send.
If a download is requested, the client first receives an MD5 hash of the file from the server. reads the file data from the socket connection, creates the file and writes the data to it. The client then generates an MD5 hash 
of the file, that it downloaded, and verifies the integrity of the download by comparing the hashes. In the event the hashes do not match, meaning the file was corrupted somehow, the client will attempt to re-download the file
a N number of times, with N being specified in the config file, or set to 3 by default. 

##Installation/Set-Up
To use the program(s), make sure you have g++ compiler installed, and that the program(s) are being executed in a linux environment. From the root directory of the project, type "make" into the terminal to compile the code and 
generate the executable binaries. They will be named "server.o" and "client.o". 

##Usage

You can start the server by typing "./server.o" into your terminal. There should be output indicating the config file was read successfully, and that the server is waiting for connections. The server will print out
certain information indicating the number of bytes sent and the time it took to write them to the socket. The server has a directory in "serverfiles/files_in_server" containing sample files of varying sizes for testing
purposes. The server uses this directory for files that it "hosts".

Open another terminal and navigate to the same directory, and type in  "./client.o" to run the client in interactive mode. The client will indicate that the config file was read successfully, and automatically 
attempt to connect to the server and execute the "get_file_list" command, and subsequently print them to the terminal. It will then ask the user if they'd like to download files, and request the files for download as
a comma separated list. For example: "test1.txt,test2.txt,test3.txt". It will then ask weather the user wants the files downloaded serially or in parallel. It will then attempt to download the specified files in 
the specified manner. It will print out when the file(s) are successfully downloaded or if there was an error. It will then terminate. 

In addition, the client can be run using passed arguments from the command line. The user must specify all the necessary arguments, or none at all. The format is as follows:
    
    ./client.o IPADDRESS:PORT SERIAL/PARALLEL FILES DIRECTORY
    
    For example:

    ./client.o 127.0.0.1:8081 serial example1.txt,example2.txt,example3.txt ./downloads/

It is important to note that both binaries must be executed in the projects "root" directory (i.e. the directory that the files are in, not the directories containing the source code)

##Evaluation

In addition, there are some scripts included that perform tests using the sample data in the "serverfiles/files_in_server" directory. These scripts are "clients_parallel.sh" and "clients_serial.sh" These scripts can be 
run in two ways, either with no argument or with a single integer argument. The argument specifies the number of clients to create to try and download files from the server simultaneously. As their names indicate, one
creates X number of clients to attempt downloads in parallel, while the other creates X number of clients to download the files in serial. Each client attempts to download 10 files, and it does this for each file size. 
(i.e. 10 files of size 128 bytes, then 10 files of size 512 bytes, and so on). The output of the clients is logged to the "logs" folder, and the results of the tests are put in the "results" directories. The "logs" consists
of the output generated by each of the clients created. Frankly, its a lot of data to sift through, but it is useful for seeing things like if a file download was re-attempted and such, and the difference between the parallel
and serial downloading. The "results" consists of a .txt file that aggregates the final output of each client that specifies the total bytes downloaded and the total time it took, as well as a .csv file that further aggregates
those results into a simple to read comma-separated-value format, which is useful for thing like graphing in excel. 

There is also a "perform_all_tests.sh" script that will automatically run the previously described two scripts for 1,4,8,and 12 clients each. The results will be put into 


##TL;DR

Run "make" in the root directory to compile the source files. In one terminal, run the "./server.o" file (from the directory it is compiled into), do the same with "./client.o" in another terminal. This will run the program
interactively. Follow the prompts.

You can also run the "run.sh" script to start the server in the background and only interact with the client. Just make sure to kill the process when you're done using pkill. 

Alternatively, run the "clients_parallel.sh" or "clients_serial.sh" scripts with either an integer argument or no argument, and behold as the script generates N number of clients to download files
from the server, and conveniently aggregates the results in the "results" directory!

Or, run "perform_all_test.sh" script to automatically run tests using a varied number of clients, and aggregates the results in "results" for you!
