#include "dependencies.h"

#define PORT 8081
#define BUFSIZE 2048
#define SERVER_BACKLOG 200
#define CONFIG_FILE_PATH "./serverfiles/server.config"

int custom_port = 0;

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

struct MessageHeader {
    uint32_t length; // Length of message data in bytes
};

bool loadConfig(){
	char real_path[PATH_MAX+1];
	realpath(CONFIG_FILE_PATH,real_path);
	std::ifstream in(real_path);
	if(!in.is_open()){
		std::cout << "config file could not be opened\n";
		return false;
	}
	std::string param;
	int value;

	while(!in.eof()){
		in >> param;
		in >> value;

		if(param == "PORT"){
			custom_port = value;
		}
		
	}
	in.close();

	return true;
}

void * handle_connection(void* p_client_socket);//function prototype
void * update();

int main(int argc, char *argv[]){//main function
	int opt = 1;
	if(loadConfig()){ //load configuration parameters from file
		printf("configuration params loaded successfully\n");
	}

	int server_socket, client_socket, addr_size;
	SA_IN server_addr, client_addr;

	if((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){//create socket
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){//set socket options
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	server_addr.sin_family = AF_INET;//configure server
	server_addr.sin_addr.s_addr = INADDR_ANY;
	if(argc >= 2){//if a port is specified as an argument
		custom_port = atoi(argv[1]);
	}

	if(custom_port >= 1 && custom_port <= 65535){
			server_addr.sin_port = htons(custom_port);
	}else{
		server_addr.sin_port = htons(PORT);
		custom_port = PORT;
	}

	if((bind(server_socket,(SA*)&server_addr,sizeof(server_addr)))<0){//bind socket
		perror("failed to bind socket");
		exit(EXIT_FAILURE);
	}
	if((listen(server_socket, SERVER_BACKLOG))<0){//listen for connections
		perror("failed to listen");
		exit(EXIT_FAILURE);
	}
	
	printf("Waiting for connections on port %d...\n",custom_port);
	while(true){//accept connections
		addr_size = sizeof(SA_IN);
		if((client_socket = accept(server_socket, (SA*)&client_addr,(socklen_t*)&addr_size))<0){
			perror("failed to accept client connection");
		}
		//printf("Connection successful!\n");

		pthread_t t;
		int *pclient = (int *)malloc(sizeof(int));
		*pclient = client_socket;
		pthread_create(&t, NULL, handle_connection, pclient);//create thread to handle connection
	}

	return 0;
}

//function to calculate md5 hash of a file
std::string md5(const std::string& file_path) {
    std::string md5_str;
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_get_digestbyname("md5");
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;

    if (!mdctx || !md) {
        perror("!mdctx || !md");
        return md5_str;
    }

    if (EVP_DigestInit_ex(mdctx, md, NULL) != 1) {
        perror("EVP_DigestInit_ex");
        EVP_MD_CTX_free(mdctx);
        return md5_str;
    }

    FILE* file = fopen(file_path.c_str(), "rb");
    if (!file) {
        perror("md5 fopen");
        EVP_MD_CTX_free(mdctx);
        return md5_str;
    }

    const size_t buf_size = 4096;
    unsigned char buf[buf_size];
    size_t n;
    while ((n = fread(buf, 1, buf_size, file)) > 0) {
        if (EVP_DigestUpdate(mdctx, buf, n) != 1) {
            perror("EVP_DigestUpdate");
            EVP_MD_CTX_free(mdctx);
            fclose(file);
            return md5_str;
        }
    }

    if (ferror(file)) {
        perror("md5 ferror");
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        return md5_str;
    }

    fclose(file);

    if (EVP_DigestFinal_ex(mdctx, md_value, &md_len) != 1) {
        perror("EVP_DigestFinal_ex");
        EVP_MD_CTX_free(mdctx);
        return md5_str;
    }

    EVP_MD_CTX_free(mdctx);

    // Convert md_value to a hex string
    std::stringstream ss;
    for (unsigned int i = 0; i < md_len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)md_value[i];
    }
    md5_str = ss.str();

    return md5_str;
}


void * handle_connection(void* p_client_socket){//function to handle connections
	int client_socket = *((int*)p_client_socket);
	free(p_client_socket);
	char buffer[BUFSIZE];
	size_t bytes_read;
	char real_path[PATH_MAX+1];
	MessageHeader h;
	recv(client_socket, &h, sizeof(h), 0);
	recv(client_socket, buffer, h.length, 0);
	int c;
	c = strcmp(buffer,"get_file_list");//check if client wants file list
	if(c==0){
		std::string msg = "File List:\n";
		std::vector<std::string> availableFiles = getFileList();
		for(int i = 0; (unsigned long int)i < availableFiles.size(); i++){
			msg+= availableFiles[i] + "\n";//add each file to the message
		}
		char* response = new char[msg.length()+1];
		strcpy(response, msg.c_str());
		send(client_socket,response,strlen(response),0);//send the message
	}else{
		std::string file_location = "serverfiles/files_in_server/";
		//std::string file_location = "files_in_server/";
		int i = 0;
		while(i < 2048){//get the file name
			file_location+=buffer[i];
			i++;
			if(buffer[i] == '\n' || buffer[i] == '\0') break;
		}

		file_location += '\0';//add null terminator
		const int length = file_location.length();//get the length of the file name
		char* file_to_send = new char[length+1];//create a char array to hold the file name
		strcpy(file_to_send, file_location.c_str());//copy the file name to the char array

		MessageHeader header;//create a message header
		//get the abs path to the specified file if it exists
		if(realpath(file_to_send, real_path) == NULL){
			printf("Error: invalid path!\n%s\n",real_path);
			char errmsg[] = "File not found\n";
			header.length = sizeof(errmsg);
			//send message header
			send(client_socket, &header, sizeof(header),0);
			//send message data
			send(client_socket, errmsg, strlen(errmsg),0);
			close(client_socket);
			return NULL;
		}

		char message[] = "file found";
		header.length = strlen(message);
		//send message header
		send(client_socket, &header, sizeof(header),0);
		//send message data
		send(client_socket, message, header.length, 0);
		//get the md5 hash of the file
		std::string md5_hash = md5(real_path);
		if(md5_hash.empty()){
			perror("error generating md5 hash");
			return NULL;
		}

		//send the md5 hash
		header.length = md5_hash.length();
		//send message header
		send(client_socket, &header, sizeof(header),0);
		//send message data
		send(client_socket, md5_hash.c_str(), md5_hash.length(),0);

		//read the file specified by the client
		FILE* fp = fopen(real_path, "r");
		if(fp==NULL){
			char errmsg[] = "File not found\n";
			send(client_socket, errmsg, strlen(errmsg),0);
			printf("Error: cannot open file%s\n",buffer);
			close(client_socket);
			return NULL;
		}
		
		
		//wait for client to signal it is ready
		//to receive file content
		read(client_socket,buffer,1);

		//rewind file pointer to beginning of file
		fseek(fp, 0, SEEK_SET);
		//time the speed of transfer
		std::clock_t start;
		double dur;
		start = std::clock();
		size_t bytes_sent = 0;

		while((bytes_read = fread(buffer,1,BUFSIZE,fp)) > 0){//send file content as long as there is data
			//printf("sending %zu bytes\n",bytes_read);
			bytes_sent += bytes_read;
			write(client_socket, buffer, bytes_read);
		}

		dur = (std::clock() - start) / ((double) CLOCKS_PER_SEC);
		printf("Sent %zu bytes in %.16f seconds\n",bytes_sent, dur);

		fclose(fp);
	}
	fflush(stdout);
	close(client_socket);
	//printf("connection closed\n");
	return NULL;

}

