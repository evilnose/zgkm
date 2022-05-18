/* Simple logger from "Robert S. Barnes" https://stackoverflow.com/a/6168353 */
#include "logger.h"

#include <iostream>
#include <string>

LogIt::LogIt(loglevel_e _loglevel) {
    std::string level_str;
    switch (_loglevel) {
        case 0:
            level_str = "ERROR";
            break;
        case 1:
            level_str = "WARNING";
            break;
        case 2:
            level_str = "INFO";
            break;
        case 3:
            level_str = "DEBUG";
            break;
        default:
            level_str = "DEBUG*";
            break;
    }
    _buffer << level_str << ":"
            << std::string(_loglevel > logDEBUG ? (_loglevel - logDEBUG) * 4 : 1, ' ');
}

LogIt::~LogIt() {
    // This is atomic according to the POSIX standard
    // http://www.gnu.org/s/libc/manual/html_node/Streams-and-Threads.html
    std::cerr << _buffer.str();
    std::cerr << std::endl;
}
