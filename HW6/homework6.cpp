/* 
 * File:   homework6.cpp
 * Author: bowserbl
 *
 * Copyright 2018 Bowserbl
 */

/**
 * A program to use multiple threads to count words from data obtained
 * via a given URL.
 *
 */

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp> 
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <fstream>
#include <iterator>
#include <cctype>
#include <algorithm>
#include <thread>
#include <mutex>  
#include <sstream>

// Using namespace to streamline working with Boost socket.

using namespace boost::asio;
using namespace boost::system;
using namespace boost::asio::ip;

// Shortcut to a vector of strings.
using StrVec = std::vector<std::string>;
using FileList = std::vector<std::string>;
using URLOutput = std::vector<std::string>;
using ThreadList = std::vector<std::thread>;

/** Return a sorted list of words from a file to use as an dictionary.
 *
 * \param[in] filePath Path to the dictionary file to be used.
 *
 * \return A vector containing a sorted list of words loaded from the
 * given file.
 */
StrVec loadDictionary(const std::string& filePath = "english.txt") {
    std::ifstream englishWords(filePath);
    std::istream_iterator<std::string> in(englishWords), eof;
    StrVec dictionary(in, eof);
    std::sort(dictionary.begin(), dictionary.end());
    return dictionary;
}

/**
 * Check if a given word is a valid English word.
 *
 * \param[in] dictionary A sorted dictionary of words to be used for
 * checking.  NOTE: This list *must* be sorted in order to use it with
 * binary_search.
 *
 * \param[in] word The word to be checked.
 *
 * \return This method returns true if the word was found in the
 * dictionary.  Otherwise it returns false.
 */
bool isValidWord(const StrVec& dictionary, std::string word) {
    // Convert the word to lower case to check against the dictionary.
    std::transform(word.begin(), word.end(), word.begin(), tolower);
    // Use binary search to find word in the dictionary.
    return std::binary_search(dictionary.begin(), dictionary.end(), word);
}

/**
 * Helper method to change punctuation in a given line to blank
 * spaces.
 *
 * This method replaces puctuations and special characters in a line
 * with blank spaces. For example, "<div class='line'>and,</div>" is
 * transformed to " div class line and div".
 *
 * \param[in] line The line of data in which punctuation are to be
 * removed.
 *
 * \return A string in which punctuation and special characters have
 * been replaced with blank spaces to ease extracting words.
 */
std::string changePunct(std::string line) {
    std::replace_if(line.begin(), line.end(), ispunct, ' ');
    return line;
}

// Counts the total number of words in a string, regardless of if they are
// actually valid words in a language or not
int countWords(std::string s) {
    int count = 0;
    std::stringstream ss(s);
    std::string word;
    while (ss >> word) {
        count++;
    }
    return count;
}

// Counts the number of english words by comparing each word to the
// dictionary file of all words
int countEnglish(std::string s, const StrVec& dictionary) {
    int count = 0;
    std::stringstream ss(s);
    std::string word;
    while (ss >> word) {
        if (isValidWord(dictionary, word)) {
            count++;
        }
    }
    return count;
}

// Removes excess whitespace and replaces them with single spaces
std::string cutSpaces(const std::string& s) {
  std::string result = boost::trim_copy(s);
  while (result.find("  ") != result.npos) {
    boost::replace_all(result, "  ", " ");
  }
  return result;
}

// Takes data in and counts words after cutting whitespace and checking if
// words are in english. A stream is used to write a string that will be
// sent as output.
std::string process(std::istream& is, std::ostream& os, std::string file) {
    int wordCount = 0;
    int english = 0;
    std::string line;
    StrVec dictionary = loadDictionary();
    while (std::getline(is, line), line != "\r") {}
    // Print HTTP response body with simple processing to filter-out
    // chunk sizes in chunked responses.
    while (std::getline(is, line)) {
        line = changePunct(line);
        line = cutSpaces(line);
        wordCount += countWords(line);
        english += countEnglish(line, dictionary);
    }
    std::ostringstream out;
    out << "URL: http://ceclnx01.cec.miamioh.edu/~raodm/SlowGet.cgi?file=";
    out << file << ", words: " << wordCount << ", English words: " << english;
    std::string urlOutput = out.str();
    return urlOutput;
}

// Does the actual work of connecting to the server with boost, then sends
// the get request with headers 
std::string execution(std::string URL) {
    std::string output;
    const std::string host = 
    "ceclnx01.cec.miamioh.edu";
    ip::tcp::iostream stream(host, "80");
    if (!stream) {
        return "Error connecting\n";
    } else {
    // Set HTTP request to the server.
    stream << "GET " << "/~raodm/SlowGet.cgi?file=" <<
            URL << " HTTP/1.1\r\n";
    stream << "Host: " << host << "\r\n";
    stream << "Connection: Close\r\n\r\n";
    // Process response from the server.  Skip header lines first.
        output = process(stream, stream, URL);
    }
    return output;
}

// Takes in the threads and lists, then stores the result of the word counts
// in the results vector
void thrMain(const FileList& list, URLOutput& results, const int startIdx,
    const int count) {
    int end = (startIdx + count);
    for (int i = startIdx; (i < end); i++) {
        // Executes the program to count words
        results[i] = execution(list[i]);
    }
}
// Takes in a list of files to count words in, a blank vector results to write
// output to, and the number of threads the user entered
void threadRun(const FileList& list, URLOutput& results, int threadCount) {
    // If threadCount is odd, add one more thread
    if (threadCount % 2) {
        threadCount++;
    }
    const int count = list.size() / threadCount;
    results.resize(list.size());
    ThreadList thrList;
    
    for (int start = 0, thr = 0; (thr < threadCount); thr++, start += count) {
        thrList.push_back(std::thread(thrMain, std::ref(list),
                std::ref(results), start, count));
    }
    for (auto& t : thrList) {
        t.join();
    }
}

int main(int argc, char** argv) {
    int totalInputs = argc;
    int count = std::stoi(argv[1]);
    std::vector<std::string> dataInputs;
    for (int i = 2; i < totalInputs; i++) {
       dataInputs.push_back(argv[i]);
    }
    if (count > 1) { 
        std::vector<std::string> results;
        std::vector<std::string> list;
        threadRun(dataInputs, results, count);
        for (size_t i = 0; i < results.size(); i++) {
            std::cout << results[i] << std::endl;
        }
    } else {
        for (size_t i = 0; i < dataInputs.size(); i++) {
            std::string input = dataInputs[i];
            std::cout << execution(input) << std::endl;
        }
    }
    return 0;
}
