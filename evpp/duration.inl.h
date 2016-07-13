#pragma once

namespace evpp {
inline Duration::Duration()
    : ns_(0) {}

inline Duration::Duration(const struct timeval& t)
    : ns_(t.tv_sec * kSecond + t.tv_usec * kMicrosecond) {}

inline Duration::Duration(int64_t nanoseconds)
    : ns_(nanoseconds) {}

inline Duration::Duration(int nanoseconds)
    : ns_(nanoseconds) {}

inline Duration::Duration(double seconds)
    : ns_((int64_t)(seconds * kSecond)) {}

inline int64_t Duration::Nanoseconds() const {
    return ns_;
}

inline double Duration::Seconds() const {
    return double(ns_) / kSecond;
}

inline double Duration::Milliseconds() const {
    return double(ns_) / kMillisecond;
}

inline double Duration::Microseconds() const {
    return double(ns_) / kMicrosecond;
}

inline double Duration::Minutes() const {
    return double(ns_) / kMinute;
}

inline double Duration::Hours() const {
    return double(ns_) / kHour;
}

inline bool Duration::IsZero() const {
    return ns_ == 0;
}

inline struct timeval Duration::TimeVal() const {
    struct timeval t;
    To(&t);
    return t;
}

inline void Duration::To(struct timeval* t) const {
    t->tv_sec = (long)(ns_ / kSecond);
    t->tv_usec = (long)(ns_ % kSecond) / (long)kMicrosecond;
}

inline bool Duration::operator<(const Duration& rhs) const {
    return ns_ < rhs.ns_;
}

inline bool Duration::operator<=(const Duration& rhs) const {
    return ns_ <= rhs.ns_;
}

inline bool Duration::operator>(const Duration& rhs) const {
    return ns_ > rhs.ns_;
}

inline bool Duration::operator>=(const Duration& rhs) const {
    return ns_ >= rhs.ns_;
}

inline bool Duration::operator==(const Duration& rhs) const {
    return ns_ == rhs.ns_;
}

inline Duration Duration::operator+=(const Duration& rhs) {
    ns_ += rhs.ns_;
    return *this;
}

inline Duration Duration::operator-=(const Duration& rhs) {
    ns_ -= rhs.ns_;
    return *this;
}

inline Duration Duration::operator*=(int n) {
    ns_ *= n;
    return *this;
}

inline Duration Duration::operator/=(int n) {
    ns_ /= n;
    return *this;
}
} // namespace evpp

