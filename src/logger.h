/* Simple logger from "Robert S. Barnes" https://stackoverflow.com/a/6168353 */
#pragma once

#include <sstream>

enum loglevel_e { logERROR, logWARNING, logINFO, logDEBUG };

// project-wide log level. NOTE when this is changed, do make -B
#ifdef NDEBUG
constexpr loglevel_e loglevel = logERROR;
#else
constexpr loglevel_e loglevel = logDEBUG;
#endif

class logIt {
   public:
    logIt(loglevel_e _loglevel = logERROR);

    ~logIt();

    template <typename T>
    logIt& operator<<(T const& value) {
        _buffer << value;
        return *this;
    }

   private:
    std::ostringstream _buffer;
};

#define LOG(level) \
if constexpr (level > loglevel) ; \
else logIt(level)
