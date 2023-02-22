#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <unistd.h>
#include <algorithm>

namespace fs = std::filesystem;

std::vector<std::string> fileList;

void updateFileList(){
    std::string path = get_current_dir_name();
    //path += "/files_in_server"; //for testing
    path += "/serverfiles/files_in_server/";
    int i = 0;
    for(const auto & entry : fs::directory_iterator(path)){
        //std::cout << entry.path() << std::endl;
        if(std::find(fileList.begin(),fileList.end(), entry.path().filename())==fileList.end()){//if the file path does not exist
            fileList.push_back(entry.path().filename());
            i++;
        }
    }
}

std::vector<std::string> getFileList(){
    updateFileList();
    return fileList;
}

void * update(){
    while(true){
        updateFileList();
        sleep(30);
    }
    return NULL;
}