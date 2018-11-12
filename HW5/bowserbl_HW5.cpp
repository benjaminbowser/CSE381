/* 
 * File:   bowserbl_hw4.cpp
 * Author: bowserbl
 *
 * Copyright 2018 Bowserbl
 */
#include <ext/stdio_filebuf.h>
#include <unistd.h>
#include <sys/wait.h> 
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <algorithm>

using namespace std;

const int READ = 0;
const int WRITE = 1;  

void child(int pipefd[]);
void parent(int pipefd[]); 

// Takes in a string and returns the appropriate file type string
void contentProcessor(string contentType) {
    std::string type = "";
    if (contentType == "html") {
        type = "text/html\r\n";
    }
    if (contentType == "png") {
        type = "image/png\r\n";
    }
    if (contentType == "jpg") {
        type = "image/jpeg\r\n";
    }
    if (contentType == "txt") {
        type = "text/plain\r\n";
    }
    if (type == "") {
        type = "text/plain\r\n";
    }
    std::cout << "Content-Type: " << type;
}

// Decodes URL strings
std::string url_decode(std::string str) {
    size_t pos = 0;
    while ((pos = str.find_first_of("%+", pos)) != std::string::npos) {
        switch (str.at(pos)) {
            case '+': str.replace(pos, 1, " ");
            break;
            
            case '%': {
                std::string hex = str.substr(pos + 1, 2);
                char ascii = std::stoi(hex, nullptr, 16);
                str.replace(pos, 3, 1, ascii);
            }
        }
        pos++;
    }
    return str;
}

// Process the header data after the HTTP request
void headerProcessor(std::vector<string> inputs, bool ok, std::string file) {
    std::cout << "Transfer-Encoding: chunked\r\n";
    std::cout << "X-Client-Header-Count: " << inputs.size()-1 << "\r\n";
    std::cout << "Connection: Close\r\n";
    std::cout << "\r\n";
    if (file.substr(0, 7) == "cgi-bin") {
        ok = true;
    }
    if (!ok) {
        std::cout << "2a\r\n";
        std::cout << "The following file was not found: " << file << "\r\n";
    }
}

// Takes in the file and processes the data from the istream
void processFile(std::istream& in) {
    std::string input;
    while (std::getline(in, input)) {        
        // Send chunk size to client.
        std::cout << std::hex << input.size()+1 << "\r\n";
        // Write the actual data for the line.
        std::cout << input << "\r\n";
        std::cout << "\r\n";
    }
    std::cout << "0\r\n";  // Last line
}

// Child method to run with the parent function
void child(int pipefd[], string command, std::vector<string> contents) {
    std::vector<char*> args;
    args.push_back(&command[0]);
        for (size_t i = 0; i < contents.size(); i++) {
            args.push_back(&contents[i][0]);
        }
    args.push_back(nullptr);
    
    close(pipefd[READ]);  // READ is constant 0 (zero)
    dup2(pipefd[WRITE], WRITE);
    __gnu_cxx::stdio_filebuf<char> fb(pipefd[WRITE], std::ios::out, 1);
    std::ostream os(&fb);
    fflush(stdout);
    execvp(&command[0], &args[0]);
    fflush(stdout);
    close(pipefd[WRITE]);
}

// Parent function to output data from execvp
void parent(int pipefd[]) {  //  std::vector<char*> data) {
    close(pipefd[WRITE]);  // WRITE is constant 1 (one)
    __gnu_cxx::stdio_filebuf<char> fb(pipefd[READ], std::ios::in, 1);
    std::istream is(&fb);
    std::string line;
    while (getline(is, line)) {
        std::cout << std::hex << line.size()+1 << "\r\n";
        std::cout << line << "\r\n\r\n";
    }
    std::string exit = "Exit code: 0\r\n\r\n";
    std::cout << std::hex << exit.size() << "\r\n\r\n";
    std::cout << exit;
    close(pipefd[READ]);
}

// Method to call parent and child functions
void execute(string command, std::vector<string> args) {
    int pipefd[2];
    pipe(pipefd);
    pid_t pid = fork();
    if (pid == 0) {
        parent(pipefd);
    } else {
        child(pipefd, command, args);
        waitpid(pid, nullptr, 0);
    }    
}

// Used if input is CGI executable
void cgiInput(string command, std::vector<string> inputs) {  
    std::string cmd = command; 
    std::string args = command;
    cmd = cmd.erase(0, 17);
    cmd = cmd.substr(0, cmd.find("&"));
    args = args.erase(0, args.find(("&"))+6);
    args = args.erase(args.find(" "), args.length());
    args = url_decode(args);
    std::string temp;
    std::istringstream is(args);
    std::vector<string> contents;
    while (is >> quoted(temp)) {
        contents.push_back(temp);
    }
    execute(cmd, contents);
}

// Check if a file is valid and if it is a CGI file
bool validFile(std::istream& input, std::string fileName, 
        std::string firstLine, std::vector<string> inputs) {
        fileName = fileName.substr(0, 7);
        bool cgi = false;
        if (fileName == "cgi-bin") {
            cgi = true;
            fflush(stdout);
            std::cout << "HTTP/1.1 200 OK\r\n";
            std::cout << "Content-Type: text/plain\r\n";
            std::cout << "Transfer-Encoding: chunked\r\n";
            std::cout << "X-Client-Header-Count: " << inputs.size()-1 << "\r\n";
            std::cout << "Connection: Close\r\n";
            std::cout << "\r\n";
            fflush(stdout);
            cgiInput(firstLine, inputs);
            return cgi;
        } else {
            if (!input.good()) {
            std::cout << "HTTP/1.1 404 Not Found\r\n";
        } else {
            std::cout << "HTTP/1.1 200 OK\r\n";
        }
  }
        return cgi;
}

int main(int argc, char** argv) {
    std::string line;
    std::vector<string> inputs;
    std::string file;
    
    getline(std::cin, line);
    while (!line.empty()) {
        inputs.push_back(line);
        getline(std::cin, line);
    }
    string get = inputs[0];
    string firstLine;
    firstLine = get.erase(0, get.find("/")+1);
    get.erase(get.find(" "), get.length());
    std::ifstream input(get);  // Open file
    file = get;
    std::string contentType = get.erase(0, get.find(".")+1);
    bool cgi = validFile(input, get, firstLine, inputs);
    if (!cgi) {
        contentProcessor(contentType);
        headerProcessor(inputs, input.good(), file);
    }
    processFile(input);
    std::cout << "\r\n";  // Last line
    return 0;
}
