/* Simple logger from "Robert S. Barnes" https://stackoverflow.com/a/6168353 */
#include "logger.h"

#include <iostream>
#include <string>

LogIt::LogIt(loglevel_e _loglevel) {
    _buffer << _loglevel << " :"
            << std::string(_loglevel > logDEBUG ? (_loglevel - logDEBUG) * 4 : 1, ' ');
}

LogIt::~LogIt() {
    _buffer << std::endl;
    // This is atomic according to the POSIX standard
    // http://www.gnu.org/s/libc/manual/html_node/Streams-and-Threads.html
    std::cerr << _buffer.str();
}
