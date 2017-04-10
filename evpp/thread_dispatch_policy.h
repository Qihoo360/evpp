#pragma once

namespace evpp {
class ThreadDispatchPolicy {
public:
    enum Policy {
        kRoundRobin,
        kIPAddressHashing,
    };

    ThreadDispatchPolicy() : policy_(kRoundRobin) {}

    void SetThreadDispatchPolicy(Policy v) {
        policy_ = v;
    }

    bool IsRoundRobin() const {
        return policy_ == kRoundRobin;
    }
protected:
    Policy policy_;
};
}