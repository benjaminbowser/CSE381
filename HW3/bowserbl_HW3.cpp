/* 
 * File:   bowserbl_hw3.cpp
 * Author: bowserbl
 *
 * Copyright 2018 Bowserbl
 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <stack>
#include "bowserbl_HW3.h"
using namespace std;

/*
 * Takes in an istream object to store data from a text file
 * into a map. PID is the key and PPID is the data stored.
 */
std::unordered_map<int, int> pidPPIDStorage(std::istream& line) {
    std::string copy;
    
    std::unordered_map<int, int> pidPPID;
    while (std::getline(line, copy)) {
        std::replace(copy.begin(), copy.end(), '\t', ' ');
        std::replace(copy.begin(), copy.end(), ' ', ' ');
        std::istringstream is(copy);
        std::string uid;
        int pid, ppid;
        is >> uid >> pid >> ppid;  
        pidPPID[pid] = ppid;  
    }
    return pidPPID;
}

/*
 * Takes in an istream object to store data from a text file
 * into a map. PID is the key and the command is the data stored.
 */
std::unordered_map<int, std::string> pidCMDStorage(std::istream& line) {
    std::string copy;
    std::unordered_map<int, std::string> pidCMD;

    while (std::getline(line, copy)) {
        std::replace(copy.begin(), copy.end(), '\t', ' ');
        std::istringstream is(copy);
        std::string uid, STIME, TTY, TIME, cmd, cmd2;
        int pid, ppid, C;
        is >> uid >> pid >> ppid >> C >> STIME >> TTY >> TIME >> cmd;
        getline(is, cmd2);
        
        pidCMD[pid] = cmd + cmd2;  
    }
    return pidCMD;
}

/*
 * Takes in an int to start tracing the process tree at and a 
 * string of the filename entered in argv[1]. Writes out the
 * tree of processes in the desired format.
 */
void processInput(int input, std::string file) {
    std::ifstream process(file); 
    std::ifstream process2(file);
    
    std::unordered_map<int, int> pidPPID;
    std::unordered_map<int, std::string> pidCMD;
    
    pidPPID = pidPPIDStorage(process);
    pidCMD = pidCMDStorage(process2);
    
    int current = input;
    std::stack<string> output; 
    
    // Place data into the stack until PPID == 0
    do {
        std::stringstream stream;
        stream << current << "\t" << pidPPID[current] << "\t ";
        stream << pidCMD[current] << "\n";
        std::string data = stream.str();
        output.push(data);
        
        current = pidPPID[current];
    } while (current != 0);
    
    // Output Titles
    std::cout << "Process tree for PID: " << input;
    std::cout << "\n" << "PID\t" << "PPID\t" << "CMD\n";

    // Pop stack to display data
    while (output.size() > 0) {
        string stacked = output.top();
        std::cout << stacked;
        output.pop();
    }
}

int main(int argc, char** argv) {
    std::string fileName = argv[1];
    int input = (atoi(argv[2]));
    processInput(input, fileName);
    return 0;
}
