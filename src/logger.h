/* Simple logger from "Robert S. Barnes" https://stackoverflow.com/a/6168353 */
#pragma once

#include <sstream>

enum loglevel_e { logERROR, logWARNING, logINFO, logDEBUG };
using EndlType = std::ostringstream& (*)(std::ostringstream&);

// for simple endl
typedef std::basic_ostream<char, std::char_traits<char> > CerrType;
typedef CerrType& (*StandardEndLine)(CerrType&);

// project-wide log level. NOTE when this is changed, do make -B
#ifdef NDEBUG
constexpr loglevel_e loglevel = logERROR;
#else
constexpr loglevel_e loglevel = logDEBUG;
#endif

class LogIt {
   public:
    LogIt(loglevel_e _loglevel);

    ~LogIt();

    template <typename T>
    LogIt& operator<<(const T & value) {
        _buffer << value;
        return *this;
    }

    LogIt& operator<<(StandardEndLine endl_) {
        _buffer << endl_;
        return *this;
    }

   private:
    std::ostringstream _buffer;
};

#define LOG(level) \
if constexpr (level > loglevel) ; \
else LogIt(level)
