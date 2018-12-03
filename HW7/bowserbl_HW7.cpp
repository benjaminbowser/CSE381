/* 
 * A simple web-server.  
 * 
 * The web-server performs the following tasks:
 * 
 *     1. Accepts connection from a client.
 *     2. Processes cgi-bin GET request.
 *     3. If not cgi-bin, it responds with the specific file or a 404.
 * 
 * Copyright (C) 2018 raodm@miamiOH.edu
 */

#include <ext/stdio_filebuf.h>
#include <unistd.h>
#include <sys/wait.h>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include <thread>

// Using namespaces to streamline code below
using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;

// Forward declaration for method defined further below
void serveClient(std::istream& is, std::ostream& os, bool genFlag);

// shared_ptr is a garbage collected pointer!
using TcpStreamPtr = std::shared_ptr<tcp::iostream>;

/** Simple method to be run from a separate thread.
 *
 * @param client The client socket to be processed.
 */
void threadMain(TcpStreamPtr client) {
    // Call routine/regular helper method.
    serveClient(*client, *client, true);
}

/**
 * Runs the program as a server that listens to incoming connections.
 * 
 * @param port The port number on which the server should listen.
 */
void runServer(int port) {
    // Setup a server socket to accept connections on the socket
    io_service service;
    // Create end point
    tcp::endpoint myEndpoint(tcp::v4(), port);
    // Create a socket that accepts connections
    tcp::acceptor server(service, myEndpoint);
    std::cout << "Server is listening on " << port 
              << " & ready to process clients...\n";
    // Process client connections one-by-one...forever
    while (true) {
        // Create garbage-collect object on heap
        TcpStreamPtr client = std::make_shared<tcp::iostream>();
        // Wait for a client to connect
        server.accept(*client->rdbuf());
        // Create a separate thread to process the client.
        std::thread thr(threadMain, client);
        thr.detach();
    }
}

// Forward declaration for method used further below.
std::string url_decode(std::string);

// Named-constants to keep pipe code readable below
const int READ = 0, WRITE = 1;

// The default file to return for "/"
const std::string RootFile = "index.html";

/**
 * This method is a convenience method that extracts file or command
 * from a string of the form: "GET <path> HTTP/1.1"
 * 
 * @param req The request from which the file path is to be extracted.
 * @return The path to the file requested
 */
std::string getFilePath(const std::string& req) {
    // std::cout << req << std::endl;
    size_t spc1 = req.find(' '), spc2 = req.rfind(' ');
    if ((spc1 == std::string::npos) || (spc2 == std::string::npos)) {
        return "";  // Invalid or unhandled type of request.
    }
    std::string path = req.substr(spc1 + 2, spc2 - spc1 - 2);
    if (path == "") {
        return RootFile;  // default root file
    }
    return path;
}

/** Helper method to send HTTP 404 message back to the client.

    This method is called in cases where the specified file name is
    invalid.  This method uses the specified path and sends a suitable
    404 response back to client.

    \param[out] os The output stream to where the data is to be
    written.

    \param[in] path The file path that is invalid.
 */
void send404(std::ostream& os, const std::string& path) {
    const std::string msg = "The following file was not found: " + path;
    // Send a fixed message back to the client.
    os << "HTTP/1.1 404 Not Found\r\n"
       << "Content-Type: text/plain\r\n"
       << "Transfer-Encoding: chunked\r\n"        
       << "Connection: Close\r\n\r\n";
    // Send the chunked data to client.
    os << std::hex << msg.size() << "\r\n";
    // Write the actual data for the line.
    os << msg << "\r\n";
    // Send trailer out to end stream to client.
    os << "0\r\n\r\n";
}

/**
 * Obtain the mime type of data based on file extension.
 * 
 * @param path The path from where the file extension is to be determined.
 * 
 * @return The mime type associated with the contents of the file.
 */
std::string getMimeType(const std::string& path) {
    const size_t dotPos = path.rfind('.');
    if (dotPos != std::string::npos) {
        const std::string ext = path.substr(dotPos + 1);
        if (ext == "html") {
            return "text/html";
        } else if (ext == "png") {
            return "image/png";
        } else if (ext == "jpg") {
            return "image/jpeg";
        }
    }
    // In all cases return default mime type.
    return "text/plain";
}

/** Convenience method to split a given string into words.

    This method is just a copy-paste of example code from lecture
    slides.

    \param[in] str The string to be split

    \return A vector of words
 */
std::vector<std::string> split(std::string str) {
    std::istringstream is(str);
    std::string word;
    std::vector<std::string> list;
    while (is >> word) {
        list.push_back(word);
    }
    return list;
}

/** Uses execvp to run the child process.

    This method is a helper method that is used to run the child
    process using the execvp command.  This method is called onlyl on
    the child process from the exec() method in this source file.

    \param[in] argList The list of command-line arguments.  The 1st
    entry is assumed to the command to be executed.
*/
void runChild(std::vector<std::string> argList) {
    // Setup the command-line arguments for execvp.  The following
    // code is a copy-paste from lecture slides.
    std::vector<char*> args;
    for (size_t i = 0; (i < argList.size()); i++) {
        args.push_back(&argList[i][0]);
    }
    // nullptr is very important
    args.push_back(nullptr);
    // Finally run the command in child process
    execvp(args[0], &args[0]);  // Run the command!
    // If control drops here, then the command was not found!
    std::cout << "Command " << argList[0] << " not found!\n";
    // Exit out of child process with error exit code.    
    exit(0);
}

// A method to resolve issues with decimals and shorten the lengths
// of float values to 2 decimal places.
std::string decimals(float number) {
    std::string dec;
    float val = static_cast<int>(number * 100);
    float round = val / 100;
    std::string rounded = std::to_string(round);
    if (round == 0) {
        dec = rounded.substr(0, rounded.find('.'));
    } else {
        dec = rounded.substr(0, rounded.find('.')+ 3);
    }
    std::string copy = dec;
    if (copy.substr(1) == ".00") {
        dec = dec.substr(0);
    }
    return dec;
}

// Opens the /proc/[pid]/stat file on the server. Obtains usertime, systemtime
// and current size of memory by the program and returns a string vector of
// the three obtained values.
std::vector<string> getProc(int pid) {
    std::string file = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream dataFile(file);
    std::string line;
    std::getline(dataFile, line);
    std::istringstream is(line);
    std::vector<std::string> list;
    std::string word;
    int counter = 0;
        while (is >> word) {
            counter++;
            if (counter == 14) {
                float utime = std::stof(word);
                utime = utime / sysconf(_SC_CLK_TCK);
                std::string converted = decimals(utime);
                list.push_back(converted);
            }
            if (counter == 15) {
                float stime = std::stof(word);
                stime = stime / sysconf(_SC_CLK_TCK);
                std::string converted = decimals(stime);
                list.push_back(converted);
            }
            if (counter == 23) {
                long vsize = std::stol(word);
                vsize = vsize / 1000;
                list.push_back(std::to_string(vsize));
            }
        }
    return list;
}

// Prints out html code used in the program to the ostream and chunks it
std::ostream& HTTP(std::ostream& os) {
    std::string text = 
        "<html>\n"
       "  <head>\n"
       "    <script type='text/javascript' "
       "src='https://www.gstatic.com/charts/loader.js'></script>\n"
       "    <script type='text/javascript' src='/draw_chart.js'></script>\n"
       "    <link rel='stylesheet' type='text/css' href='/mystyle.css'>\n"
       "  </head>\n\n"
       "  <body>\n"
       "    <h3>Output from program</h3>\n"
       "    <textarea style='width: 700px; height: 200px'>\n";
    os << std::hex << text.size() << "\r\n";
    os << text << "\r\n";   
    return os;
}

// Generates a vector of data for the table by calling getProc and returning
// a string vector
std::vector<string> rowGen(int pid) { 
    std::vector<std::string> data = getProc(pid);
    return data;
}

// Generates the HTML code for a row of the table using the vector containing
// vectors of string data from the proc file
std::string tableGen(std::vector<std::vector<std::string>> contents) {
    std::string line; 
    int counter = 0;
    
    for (size_t i = 0; i < contents.size(); i++) {
        counter++;
        std::vector<std::string> temp = contents[i];
        line += "       <tr><td>" + std::to_string(counter) + "</td><td>"
        + temp[0] + "</td><td>" + temp[1]+ "</td><td>" + temp[2]+
                "</td></tr>\n";
    }
    return line;
}

// Generates the JSON data for the Google chart using the data from the proc
// file. HTML is then written to a string.
std::string chartGen(std::vector<std::vector<string>> tableData) {
    string chart;
    int counter = 0;
    for (size_t i = 0; i < tableData.size(); i++) {
        counter++;
        std::vector<std::string> temp = tableData[i];
        if (i != tableData.size()-1) {
        chart += "          [" + std::to_string(counter) + ", " +
                temp[0] + ", " + temp[2] + "],\n";
        } else {
            chart += "          [" + std::to_string(counter) +  ", " +
                    temp[0] + ", " + temp[2] + "]\n";
        }
    }
    return chart;
}

// Ends the HTML region, adds the HTML for the table, along with JSON data for
// the Google chart. Chunked data is returned.
void printData(bool genChart, std::ostream& os, 
    std::vector<std::vector<string>> tableData) {
    std::string brackets = "", comma = "", empty = "";
    std::string table = tableGen(tableData);
    if (genChart) {
        brackets = chartGen(tableData);
        comma = ",";
    }
    std::string text = 
            "     </textarea>\n"
            "     <h2>Runtime statistics</h2>\n     <table>\n"
            "       <tr><th>Time (sec)</th><th>User time</th><th>System time"
            "</th><th>Memory (KB)</th></tr>\n"
            + table +
            "     </table>\n"
            "     <div id='chart' style='width: 900px; height: 500px'></div>\n"
            "  </body>\n" "  <script type='text/javascript'>\n"
            "    function getChartData() {\n"
            "      return google.visualization.arrayToDataTable(\n"
            "        [\n          ['Time (sec)', 'CPU Usage', "
            "'Memory Usage']" + comma +"\n" + brackets 
            + "        ]\n      );\n    }\n  </script>\n</html>\n";
    os << std::hex << text.size() << "\r\n";
    os << text << "\r\n"; 
    os << std::hex << empty.size() << "\r\n\r\n";
}

/** Helper method to send the data to client in chunks.
    
    This method is a helper method that is used to send data to the
    client line-by-line.

*/
void sendMoreData(const std::string& mimeType, int pid,
              std::istream& is, std::ostream& os,
        std::vector<std::vector<string>> &table, bool genChart, 
        std::thread &t1) {
    // First write the fixed HTTP header.
    os << "HTTP/1.1 200 OK\r\n"
       << "Content-Type: " << mimeType << "\r\n"        
       << "Transfer-Encoding: chunked\r\n"
       <<"Connection: Close\r\n\r\n";
    // Read line-by line from child-process and write results to
    // client.
    
    std::string line;
    if (pid != -1) {
        HTTP(os);
    }
    while (std::getline(is, line)) {
        // Add required "\n" to terminate the line.
        line += "\n";
        // Add size of line in hex
        os << std::hex << line.size() << "\r\n";
        // Write the actual data for the line.
        os << line << "\r\n";
    }
    // Check if we need to end out exit code
    t1.join();
    if (pid != -1) {
        // Wait for process to finish and get exit code.
        int exitCode = 0;
        waitpid(pid, &exitCode, 0);
        // Create exit code information and send to client.
        line = "\r\nExit code: " + std::to_string(exitCode) + "\r\n";
        os << std::hex << line.size() << "\r\n" << line << "\r\n";
    }
    printData(genChart, os, table);
}

// Method to handle threading in the program. This method obtains data from
// the proc file once every second and pushes it into a vector.
void threaded(int pid, std::vector<std::vector<string>>& tableHTML) {
    int exitCode = 0;
        
        while (waitpid(pid, &exitCode, WNOHANG) == 0) {
            sleep(1);  // sleep for 1 second.
            tableHTML.push_back(rowGen(pid));
            // Record current statistics about the process.
        }
        sleep(1);
}

// Executes code to create a thread to be gather data from the proc file, then
// redirects it to the sendMoreData method.
void exec(std::string cmd, std::string args, std::ostream& os, bool genChart) {
    // Split string into individual command-line arguments.
    std::vector<std::string> cmdArgs = split(args);
    // Add command as the first of cmdArgs as per convention.
    cmdArgs.insert(cmdArgs.begin(), cmd);
    // Setup pipes to obtain inputs from child process
    int pipefd[2];
    pipe(pipefd);
    // Finally fork and exec with parent having more work to do.
    const int pid = fork();
    if (pid == 0) {
        close(pipefd[READ]);        // Close unused end.
        dup2(pipefd[WRITE], 1);     // Tie/redirect std::cout of command
        runChild(cmdArgs);
    } else {
        std::vector<std::vector<string>> tableData;
        std::thread t1(threaded, pid, std::ref(tableData));

        close(pipefd[WRITE]);
        __gnu_cxx::stdio_filebuf<char> fb(pipefd[READ], std::ios::in, 1);
        std::istream is(&fb);
        // Have helper method process the output of child-process
        sendMoreData("text/html", pid, is, os, std::ref(tableData),
                genChart, t1);
    }
}

/**
 * Process HTTP request (from first line & headers) and
 * provide suitable HTTP response back to the client.
 * 
 * @param is The input stream to read data from client.
 * @param os The output stream to send data to client.
 * @param genChart If this flag is true then generate data for chart.
 */
void serveClient(std::istream& is, std::ostream& os, bool genChart) {
    // Read headers from client and print them. This server
    // does not really process client headers
    std::string line;
    // Read the GET request line.
    std::getline(is, line);
    const std::string path = getFilePath(line);
    // Skip/ignore all the HTTP request & headers for now.
    while (std::getline(is, line) && (line != "\r")) {}
    // Check and dispatch the request appropriately
    const std::string cgiPrefix = "cgi-bin/exec?cmd=";
    const int prefixLen         = cgiPrefix.size();
    if (path.substr(0, prefixLen) == cgiPrefix) {
        // Extract the command and parameters for exec.
        const size_t argsPos   = path.find("&args=", prefixLen);
        const std::string cmd  = path.substr(prefixLen, argsPos - prefixLen);
        const std::string args = url_decode(path.substr(argsPos + 6));
        // Now run the command and return result back to client.
        exec(cmd, args, os, genChart);
    } else {
        // Get the file size (if path exists)
        std::ifstream dataFile(path);
        if (!dataFile.good()) {
            // Invalid file/File not found. Return 404 error message.
            send404(os, path);
        } else {
            std::vector<std::vector<string>> a;
            std::thread t1;
            // Send contents of the file to the client.
            sendMoreData(getMimeType(path), -1, dataFile, os,
                    a, genChart, t1);
        }
    }
}

//------------------------------------------------------------------
//  DO  NOT  MODIFY  CODE  BELOW  THIS  LINE
//------------------------------------------------------------------

/** Convenience method to decode HTML/URL encoded strings.

    This method must be used to decode query string parameters
    supplied along with GET request.  This method converts URL encoded
    entities in the from %nn (where 'n' is a hexadecimal digit) to
    corresponding ASCII characters.

    \param[in] str The string to be decoded.  If the string does not
    have any URL encoded characters then this original string is
    returned.  So it is always safe to call this method!

    \return The decoded string.
*/
std::string url_decode(std::string str) {
    // Decode entities in the from "%xx"
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

/*
 * The main method that performs the basic task of accepting connections
 * from the user.
 */
int main(int argc, char** argv) {
    if (argc == 2) {
        // Setup the port number for use by the server
        const int port = std::stoi(argv[1]);
        runServer(port);
    } else if (argc == 4) {
        // Process 1 request from specified file for functional testing
        std::ifstream input(argv[1]);
        std::ofstream output;
        if (argv[2] != std::string("std::cout")) {
            output.open(argv[2]);
        }
        bool genChart = (argv[3] == std::string("true"));
        serveClient(input, (output.is_open() ? output : std::cout), genChart);
    } else {
        std::cerr << "Invalid command-line arguments specified.\n";
    }
    return 0;
}

// End of source code
