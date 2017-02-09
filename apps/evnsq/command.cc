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

    if (body_.empty()) {
        return;
    }

    if (body_.size() == 1) {
        const std::string& d = body_[0];
        buf->AppendInt32(int32_t(d.size()));
        buf->Append(d);
    } else {
        assert(body_.size() > 1);
        assert(name_ == "MPUB");
        int32_t mpub_body_size = 4;

        for (size_t i = 0; i < body_.size(); ++i) {
            mpub_body_size += 4;
            mpub_body_size += body_[i].size();
        }

        buf->AppendInt32(mpub_body_size);
        buf->AppendInt32(int32_t(body_.size()));

        for (size_t i = 0; i < body_.size(); ++i) {
            const std::string& msg = body_[i];
            buf->AppendInt32(int32_t(msg.size()));
            buf->Append(msg);
        }
    }
}
}
