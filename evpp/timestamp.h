#pragma once

#include "duration.h"
#include <chrono>


namespace evpp {
class Timestamp {
public:
    Timestamp();
    explicit Timestamp(int64_t nanoseconds);
    explicit Timestamp(const struct timeval& t);

    static Timestamp Now(); // returns the current local time.

    struct timeval TimeVal() const;
    void To(struct timeval* t) const;

    // Unix returns t as a Unix time, the number of seconds elapsed
    // since January 1, 1970 UTC.
    int64_t Unix() const;

    // UnixNano returns t as a Unix time, the number of nanoseconds elapsed
    // since January 1, 1970 UTC. The result is undefined if the Unix time
    // in nanoseconds cannot be represented by an int64.
    int64_t UnixNano() const;

    // UnixNano returns t as a Unix time, the number of microseconds elapsed
    // since January 1, 1970 UTC. The result is undefined if the Unix time
    // in microseconds cannot be represented by an int64.
    int64_t UnixMicro() const;

    void Add(Duration d);

    bool IsEpoch() const;
    bool operator< (const Timestamp& rhs) const;
    bool operator==(const Timestamp& rhs) const;

    Timestamp operator+=(const Duration& rhs);
    Timestamp operator+ (const Duration& rhs) const;
    Timestamp operator-=(const Duration& rhs);
    Timestamp operator- (const Duration& rhs) const;
    Duration  operator- (const Timestamp& rhs) const;

private:
    // ns_ gives the number of nanoseconds elapsed since the Epoch
    // 1970-01-01 00:00:00 +0000 (UTC).
    int64_t ns_;
};
} // namespace evpp

#include "timestamp.inl.h"

