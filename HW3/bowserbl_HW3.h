/* 
 * File:   bowserbl_hw3.h
 * Author: bowserbl
 *
 * Copyright 2018 Bowserbl
 */

#ifndef BOWSERBL_HW3_H
#define BOWSERBL_HW3_H
#include <string>

std::unordered_map<int, int> pidPPIDStorage(std::istream& line);
std::unordered_map<int, std::string> pidCMDStorage(std::istream& line);
void processInput(int input);

#endif /* BOWSERBL_HW3_H */

