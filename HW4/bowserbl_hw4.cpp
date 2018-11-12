/* 
 * File:   bowserbl_hw4.cpp
 * Author: bowserbl
 *
 * Copyright 2018 Bowserbl
 */

#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace std;

void myExec(std::vector<string> argList) {
    // Execute the argument list
    std::vector<char*> args;
    for (size_t i = 0; (i < argList.size()); i++) {
        args.push_back(&argList[i][0]);
    }
    args.push_back(nullptr);
    execvp(args[0], &args[0]);
}

bool exitCheck(string input) {
    // Check if exit command is sent
    if (input == "exit") {
        return true;
    } else {
        return false;
    }
}

bool blankCheck(string input) {
    // Check for a blank line with no input
    if (input == "") {
        return true;
    } else {
        return false;
    }
}

bool commentCheck(string input) {
    // Check for comments
    if (input.substr(0, 1) == "#") {
        return true;
    } else {
        return false;
    }
}

std::vector<string> inputProcessor(string input) {
    // Process user input from a string and print data
    std::vector<string> argsList;
    std::istringstream dataStream(input);
    std::string word;

    while (dataStream >> std::quoted(word)) {
        argsList.push_back(word);
    }
    if (argsList[0] != "SERIAL" && argsList[0] != "PARALLEL") {
        std::cout << "Running:";
    }

    for (size_t i = 0; i < argsList.size(); i++) {
        if (argsList[i] == "SERIAL" && argsList[0] != "PARALLEL") {
            i += 2;
        }
        if (argsList[0] != "SERIAL" && argsList[0] != "PARALLEL") {
            std::cout << " " + argsList[i];
        }
    }
    if (argsList[0] != "SERIAL" && argsList[0] != "PARALLEL") {
        std::cout << std::endl;
    }

    return argsList;
}

int execFork(std::vector<string> argsList) {
    // Execute commands from vector
    const int pid = fork();
    if (pid == 0) {
        myExec(argsList);
    } else {
        return pid;
    }
    return pid;
}

void pidCheck(int pid) {
    // Get PID from process
    int code;
    waitpid(pid, &code, 0);
    std::cout << "Exit code: " << code << std::endl;
}

void SerialPar(std::vector<string> argsList, std::vector<int> pidList) {
    // If argument is Serial or Parallel, run here
    bool par = false;
    if (argsList[0] == "PARALLEL") {
        par = true;
    }
    std::ifstream inputFile(argsList[1]);
    string copy;
    std::vector<int> returnCode;
    while (std::getline(inputFile, copy)) {
        if (!commentCheck(copy) && !blankCheck(copy) && !exitCheck(copy)) {
            std::vector<string> argsList = inputProcessor(copy);
            int pid = execFork(argsList);

            if (par) {
                pidList.push_back(pid);
            } else {
                pidCheck(pid);
            }
        }
    }
    if (par) {
        for (size_t i = 0; i < pidList.size(); i++) {
            pidCheck(pidList[i]);
        }
    }
}

void programConsole() {
    bool stop = false;
    while (!stop) {
        string input;
        std::cout << "> ";
        getline(cin, input);

        if (exitCheck(input)) {
            stop = true;
            break;
        }

        if (!blankCheck(input)) {
            if (!commentCheck(input)) {
                std::vector<string> argsList = inputProcessor(input);
                std::vector<int> pidList;

                if (argsList[0] == "SERIAL" || argsList[0] == "PARALLEL") {
                    SerialPar(argsList, pidList);
                } else {
                    // Not serial or parallel
                    pidCheck(execFork(argsList));
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    programConsole();
    return 0;
}
