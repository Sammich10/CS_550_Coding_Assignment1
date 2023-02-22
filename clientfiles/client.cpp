#include "dependencies.h"

#define PORT 8081
#define CONFIG_FILE_PATH "./clientfiles/client.config"

using namespace std;

char custom_ip[15] = "127.0.0.1";
int custom_port = 0;
int dl_attempts = 3;
string dl_folder = "./downloads/";

size_t total_dl_size = 0;
double total_dl_time = 0;

struct MessageHeader {
    uint32_t length; // Length of message data in bytes
};

typedef struct sockaddr SA;

bool loadConfig(){
	char real_path[PATH_MAX+1];
	realpath(CONFIG_FILE_PATH,real_path);
	ifstream in(real_path);
	if(!in.is_open()){
		cout << "config file could not be opened\n";
        in.close();
		return false;
	}
	string param;
	string value;

	while(!in.eof()){
		in >> param;
		in >> value;

		if(param == "PORT"){
			custom_port = stoi(value);
		}
        else if(param == "DL_FOLDER"){
            dl_folder = value;
        }
        else if(param == "DL_ATTEMPTS"){
            dl_attempts = stoi(value);
        }
		
	}
	in.close();
	return true;
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


//get the file list and print it to terminal
int getFileList(int sock){
    int valread;
    MessageHeader header;
    char message[] = "get_file_list";
    header.length = strlen(message);

    char read_buffer[2048] ={0};
    send(sock, &header, sizeof(header), 0);//send message header
    send(sock, message, header.length, 0); //send message data

    valread = read(sock, read_buffer, 2048);
    if(valread < 0){
        printf("error reading response from server"); 
        return -1;
    }
    printf("%s\n",read_buffer);

    return 0;
}

//connect to server and return socket
int sconnect(int port){
    int sock = 0, client_fd;
    struct sockaddr_in server_addr;

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("error creating socket\n");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if(inet_pton(AF_INET, custom_ip, &server_addr.sin_addr) <=0){
        printf("\nInvalid address or address not supported\n");
        return -1;
    }

    if((client_fd = connect(sock, (SA*)&server_addr, sizeof(server_addr))) < 0){
        printf("Connection to server on port %d failed!\n",custom_port);
        return -1;
    }

    //printf("Connection to server on port %d successful\n",custom_port);
    return sock;
}

//create a new socket and download a file specified
int downloadFile(char filename[]){
    
    int valread;
    char read_buffer[2048] ={0};

    int sock = sconnect(custom_port);

    MessageHeader h;
    h.length = strlen(filename);
    send(sock, &h, sizeof(h), 0); //send message header
    if((send(sock, filename, strlen(filename), 0))<0){ //send filename to server
        printf("error sending file name to server\n");
        return -1;
    }
    
    MessageHeader header;
    //receive message header
    valread = recv(sock, &header, sizeof(header),0);
    //receive message data
    valread = read(sock, read_buffer, header.length);
    if(valread < 0){
        printf("error reading response from server"); 
        close(sock);
        return -1;
    }
    
    if(strncmp(read_buffer, "File not found", strlen("File not found")) == 0){//if not, close connection and tell user
        printf("File not found: %s\n", filename);
        close(sock);
        return -1;
    }

    char md5_hash_server[32]; 
    if(strncmp(read_buffer, "file found", strlen("file found")) == 0){
        //printf("File found! ");
        char buf[32];
        //receive message header
        recv(sock,&header,sizeof(header),0);
        //receive message data
        read(sock,buf,header.length);
        strcpy(md5_hash_server,buf);
    }
    printf("Downloading file...\n");
    send(sock, "r", 1, 0);//signal to server we are ready to download

    //filename[strlen(filename)-1] = '\0';
    int l = dl_folder.length();
    l = std::strlen(filename) - 1;
    char* download_folder = new char[l];
    strcpy(download_folder, dl_folder.c_str());
    strcat(download_folder,filename);

    FILE *fp = fopen(download_folder, "w");
    //To time the speed of transfer
    clock_t start;
	double dur;
	start = clock();
	size_t bytes_recvd = 0;
    //
    while((valread = read(sock, read_buffer, 2048))>0){
        //printf("%s",read_buffer);
        fwrite(read_buffer, valread,1,fp);
        bytes_recvd+=valread;
    }
    //record speed and size of transfer
    dur = (clock()-start)/((double)CLOCKS_PER_SEC);
    printf("Received %zu bytes in %.16f seconds\n",bytes_recvd, dur);
    total_dl_size +=bytes_recvd;
    total_dl_time +=dur;
    fclose(fp);


    if(valread < 0){
        printf("error reading response from server\n\n"); 
        close(sock);
        return -1;
    }

    //calculate the md5 hash of the downloaded file
    char real_path[PATH_MAX+1];
    if(realpath(download_folder, real_path) == NULL){
			printf("Error: invalid path!\n%s\n",real_path);
    }
    std::string md5_hash = md5(real_path);

    //compare MD5 hash of the downloaded file to the MD5 hash 
    //that was generated by the server
    if(strncmp(md5_hash.c_str(), md5_hash_server,32) != 0){
        printf("Downloaded file corrupt!\n");
        close(sock);
        return 1;
    }else{
        printf("File download complete!\n\n");
        return 0;
    }
    
    close(sock);
    return 0;
}

void * t_downloadFile(void* p_filename){//attempt to download file in a thread, retry if failed up to dl_attempts times
    char* filename = new char[strlen((char*)p_filename)];
    strcpy(filename, (char*)p_filename);
    free(p_filename);
    filename[strlen(filename)-1] = '\0';
    int n = 0;
    for(n=0; n < dl_attempts; n++){
        int s=0;
        if((s = downloadFile(filename))!=1) break;
        printf("Reattempting download...\n");
    }
    if(n>=dl_attempts){
        printf("Failed to download file: %s\n",filename);
    }
    return NULL;
}


int getPort(){
    if(custom_port>0 && custom_port < 65535){
		return custom_port;
	}else{
        custom_port = PORT;
    }
    return custom_port;

}

void checkDownloadDirect(){
    struct stat st;
    char* d = new char[dl_folder.length()];
    strcpy(d, dl_folder.c_str());
    if(((stat(d,&st)) == 0) && (((st.st_mode) & S_IFMT) == S_IFDIR)){
        //directory exists
    }else{
        if(mkdir(d, S_IRWXU | S_IRWXG | S_IROTH)){
            printf("Failed to create directory %s\n",d);
            exit(EXIT_FAILURE);
        }
    }
}

void downloadSerial(vector<string>files_to_download){//download files in serial, attempt to download each file up to dl_attempts times
    for(int i = 0; (long unsigned int)i <files_to_download.size(); i++){
            string request = files_to_download[i];
            char *req = new char[request.length()];
            strcpy(req, request.c_str());
            int j = 0;
            for(j=0; j < dl_attempts; j++){
                if(downloadFile(req) != 1) break;
                printf("Reattempting download...\n");
            }
            if(j>=dl_attempts) printf("Failed to download file: %s\n", req);
        }
}

void downloadParallel(vector<string>files_to_download){
    pthread_t *t = new pthread_t[files_to_download.size()];
        for(int i = 0; (long unsigned int)i <files_to_download.size(); i++){
            string request = files_to_download[i]+'\n';
            char *req = new char[request.length()];
            strcpy(req, request.c_str());
            pthread_create(&t[i],NULL, t_downloadFile, req);
        }
        for(int j = 0; (long unsigned int)j < files_to_download.size(); j++){
            pthread_join(t[j],NULL);
        }
}

void run_no_interact(char* argv[]){
    char *ipv4, *port;
        char tlvbuf[80];
        strcpy(tlvbuf,argv[1]);
        ipv4 = strtok(tlvbuf, ":");
        port = strtok(NULL, ":");
        strcpy(custom_ip,ipv4);
        custom_port = atoi(port);
        printf("IP: %s\n",custom_ip);
        printf("Port: %d\n",custom_port);

        vector<string> files_to_download;
        char * tokens;
        char* input = argv[3];
        tokens = strtok(input," ,");
        while(tokens != NULL){
            files_to_download.push_back(tokens);
            tokens = strtok(NULL," ,");
        }
        cout << "Files:\n";
        for(auto i: files_to_download) cout<<i<<"\n";
        std::string cdl = argv[4];
        dl_folder = cdl;
        char* d = new char[dl_folder.length()];
        strcpy(d, dl_folder.c_str());
        printf("Directory: %s\n",argv[4]);
        checkDownloadDirect();

        char s = argv[2][0];
        switch(s){
            case 's':
                printf("downloading files serially\n");
                downloadSerial(files_to_download);
                printf("Total bytes downloaded: %zu\nTotal time spend downloading: %.16f\n",total_dl_size,total_dl_time);
                break;
            case 'p':
                printf("downloading files in parallel\n");
                downloadParallel(files_to_download);
                printf("Total bytes downloaded: %zu\nTotal time spend downloading: %.16f\n",total_dl_size,total_dl_time);
                break;
        } 
}

int main(int argc, char *argv[]){
    //specify usage if user passes some arguments but not enough to run fully automatically
    if(argc > 1 && argc < 5){
        printf("usage: address:port serial/parallel files(separated by comma) ./directory/\n\n");
        return 0;
    }
    //load the parameters in the configuration file
    if(loadConfig()){ 
		printf("configuration params loaded successfully\n");
	}
    //if all the arguments are specified in from command line, run the program with no user interaction
    if(argc == 5){
        run_no_interact(argv);
        return 0;
    }
    
    int port = getPort();
    int socket;
    if((socket = sconnect(port)) <0 ) 
    {
        perror("could not establish connection to server");
        return -1;
    }
    getFileList(socket);
    close(socket);

    cin.clear();
    cin.sync();

    //run program interactively
    printf("Would you like to download file(s)? (y/n): ");
    char an[5];
    cin.getline(an, 5, '\n');
    an[1]='\0';

    if(an[0] != 'y'){
        printf("Goodbye!\n\n");
        return 0;
    }
    checkDownloadDirect();
    printf("Which file(s) would you like to download?\nEnter the file name(s) separated by a comma: ");
    char input[1024];
    cin.getline(input, 1024, '\n');

    vector<string> files_to_download;
    char * tokens;
    tokens = strtok(input," ,");
    while(tokens != NULL){
        files_to_download.push_back(tokens);
        printf("%s\n",tokens);
        tokens = strtok(NULL," ,");
    }

    printf("would you like to receive the files in parallel or serially? (s/p): ");
    char a[5];
    cin.getline(a,5,'\n');

    if(a[0] == 's'){
        downloadSerial(files_to_download);
    }else if(a[0] == 'p'){
        downloadParallel(files_to_download);
    }    

    printf("Total bytes downloaded: %zu\nTotal time spend downloading: %.16f\nGoodbye!\n",total_dl_size,total_dl_time);
    
    return 0;
}