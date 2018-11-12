/* 
 * File:   bowserbl_hw2.cpp
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
#include <vector>
using namespace std;

std::unordered_map<int, std::string> groupGID(std::istream& line) {
    std::string copy;
    
    std::unordered_map<int, std::string> userMap;

    while (std::getline(line, copy)) {
        std::replace(copy.begin(), copy.end(), ':', ' ');
        std::istringstream is(copy);
        std::string login, pass;
        int uid;
        is >> login >> pass >> uid;  // or gid
        userMap[uid] = login;  // or gid
    }
    return userMap;
}

std::unordered_map<int, std::vector<int>> gidVectorMembers(std::istream& line) {
    std::string copy;
    
    std::unordered_map<int, std::vector<int>> userIDs;

    
    while (std::getline(line, copy)) {
        std::replace(copy.begin(), copy.end(), ':', ' ');
        std::replace(copy.begin(), copy.end(), ',', ' ');
        std::istringstream is(copy);
        
        std::string group, pass;
        int gid, uid;
        
        is >> group >> pass >> gid;
        std::vector<int> uids;
        while (is >> uid) {  // while a stream still has words
             uids.push_back(uid);
        }
        userIDs[gid] = uids;
    }
    return userIDs;
}

void processInput(std::vector<int> dataInputs) {
    std::ifstream groupsFile("groups"); 
    std::ifstream groupsFile2("groups");
    std::ifstream passFile("passwd");
    std::unordered_map<int, std::string> passMap;
    std::unordered_map<int, std::vector<int>> vectorMembers;
    std::unordered_map<int, std::string> groupMap;
    passMap = groupGID(passFile);
    vectorMembers = gidVectorMembers(groupsFile);
    groupMap = groupGID(groupsFile2);

    for (int i = 0; i < static_cast<int>(dataInputs.size()); i++) {
        try { 
            string groupID = groupMap.at(dataInputs[i]);
            std::vector<int> uniqueID = vectorMembers.at(dataInputs[i]);
            std::cout << dataInputs[i] << " = " << groupID << ":";
            for (int i = 0; i < static_cast<int>(uniqueID.size()); i++) {
                string name = passMap.at(uniqueID[i]);
                std::cout << " " << name << "(" << uniqueID[i] << ")";
            }
        std::cout << "\n";
            }
        catch (std::out_of_range ex) {
            std::cout << dataInputs[i] << " = Group not found.\n";
        }
    }
}

int main(int argc, char *argv[]) {
    int totalInputs = argc;
    std::vector<int> dataInputs;
    for (int i = 1; i < totalInputs; i++) {
       dataInputs.push_back(atoi(argv[i]));
    }
    processInput(dataInputs);
    return 0;
}
