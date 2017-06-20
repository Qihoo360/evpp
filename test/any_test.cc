#include "test_common.h"

#include <evpp/any.h>
#include <evpp/buffer.h>

TEST_UNIT(testAny1) {
    evpp::Buffer* buf(new evpp::Buffer());
    evpp::Any any(buf);

    evpp::Buffer* b1 = evpp::any_cast<evpp::Buffer*>(any);
    H_TEST_ASSERT(buf == b1);

    delete buf;
}

TEST_UNIT(testAny2) {
    std::shared_ptr<evpp::Buffer> buf(new evpp::Buffer());
    evpp::Any any(buf);

    std::shared_ptr<evpp::Buffer> b1 = evpp::any_cast<std::shared_ptr<evpp::Buffer>>(any);
    H_TEST_ASSERT(buf.get() == b1.get());
}

TEST_UNIT(testAny3) {
    std::shared_ptr<evpp::Buffer> buf(new evpp::Buffer());
    evpp::Any any(buf);

    std::shared_ptr<evpp::Buffer> b1 = any.Get<std::shared_ptr<evpp::Buffer>>();
    H_TEST_ASSERT(buf.get() == b1.get());
}

