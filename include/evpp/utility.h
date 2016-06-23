#include <string>
#include <sstream>

#include "evpp_export.h"

namespace evpp {
    template<class T>
    inline std::string cast(const T& t) {
        std::stringstream ss;
        ss << t;
        return ss.str();
    }
}