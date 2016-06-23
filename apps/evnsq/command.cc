#include "command.h"

namespace evnsq {
    void Command::WriteTo(evpp::Buffer* buf) const {
        buf->Append(name_);
        if (!params_.empty()) {
            auto it = params_.begin();
            auto ite = params_.end();
            for (; it != ite; ++it) {
                buf->AppendInt8(' ');
                buf->Append(*it);
            }
        }
        buf->AppendInt8('\n');
        if (!body_.empty()) {
            buf->AppendInt32(int32_t(body_.size()));
            buf->Append(body_);
        }
    }
}