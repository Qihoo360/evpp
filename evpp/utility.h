#pragma once

#include <string>
#include <sstream>

#ifndef H_CASE_STRING_BIGIN
#define H_CASE_STRING_BIGIN(state) switch(state){
#define H_CASE_STRING(state) case state:return #state;break;
#define H_CASE_STRING_END()  default:return "Unknown";break;}
#endif

namespace evpp {

template< class StringVector,
          class StringType,
          class DelimType>
inline void StringSplit(
    const StringType& str,
    const DelimType& delims,
    unsigned int maxSplits,
    StringVector& ret) {

    if (str.empty()) {
        return;
    }

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
            ret.emplace_back(StringType());
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
