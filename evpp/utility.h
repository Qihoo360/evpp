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


    template< class StringVector,
        class StringType,
        class DelimType>
        inline void StringSplit(
            const StringType& str,
            const DelimType& delims,
            unsigned int maxSplits,
            StringVector& ret) {
        unsigned int numSplits = 0;

        // Use STL methods
        size_t start, pos;
        start = 0;

        do {
            pos = str.find_first_of(delims, start);

            if (pos == start) {
                ret.push_back(StringType());
                start = pos + 1;
            } else if (pos == StringType::npos || (maxSplits && numSplits + 1 == maxSplits)) {
                // Copy the rest of the string
                ret.push_back(StringType());
                *(ret.rbegin()) = StringType(str.data() + start, str.size() - start);
                break;
            } else {
                // Copy up to delimiter
                //ret.push_back( str.substr( start, pos - start ) );
                ret.push_back(StringType());
                *(ret.rbegin()) = StringType(str.data() + start, pos - start);
                start = pos + 1;
            }

            ++numSplits;

        } while (pos != StringType::npos);
    }
}