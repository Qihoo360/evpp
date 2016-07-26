#pragma once

#include "func.h"

namespace evldd {

class Server {
public:
    struct Options {
        Options();
    }

public:
    explicit Server(const Options& options);
    ~Server();

    bool Start();
    void Stop();

    void SetMessageCallback(const LddMessgageCallback& cb);

    void NotifyMessage(ChannelBasePtr channel);
private:
    Options             options_;
    LddMessgageCallback msg_fn_;

}

}
