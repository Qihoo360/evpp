// Modeled after the time.Duration of Golang project.
#pragma once

#include "evpp/inner_pre.h"

namespace evpp {

// A Duration represents the elapsed time between two instants
// as an int64 nanosecond count. The representation limits the
// largest representable duration to approximately 290 years.
class EVPP_EXPORT Duration {
public:
    static const int64_t kNanosecond; // = 1LL
    static const int64_t kMicrosecond;// = 1000
    static const int64_t kMillisecond;// = 1000 * kMicrosecond
    static const int64_t kSecond; // = 1000 * kMillisecond
    static const int64_t kMinute; // = 60 * kSecond
    static const int64_t kHour;   // = 60 * kMinute
public:
    Duration();
    explicit Duration(const struct timeval& t);
    explicit Duration(int64_t nanoseconds);
    explicit Duration(int nanoseconds);
    explicit Duration(double seconds);

    // Nanoseconds returns the duration as an integer nanosecond count.
    int64_t Nanoseconds() const;

    // These methods return double because the dominant
    // use case is for printing a floating point number like 1.5s, and
    // a truncation to integer would make them not useful in those cases.

    // Seconds returns the duration as a floating point number of seconds.
    double Seconds() const;

    double Milliseconds() const;
    double Microseconds() const;
    double Minutes() const;
    double Hours() const;

    struct timeval TimeVal() const;
    void To(struct timeval* t) const;

    bool IsZero() const;
    bool operator< (const Duration& rhs) const;
    bool operator<=(const Duration& rhs) const;
    bool operator> (const Duration& rhs) const;
    bool operator>=(const Duration& rhs) const;
    bool operator==(const Duration& rhs) const;

    Duration operator+=(const Duration& rhs);
    Duration operator-=(const Duration& rhs);
    Duration operator*=(int ns);
    Duration operator/=(int ns);

private:
    int64_t ns_; // nanoseconds
};
} // namespace evpp

#include "duration.inl.h"

