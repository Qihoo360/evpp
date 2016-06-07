#include "test_common.h"

#include "evpp/buffer.h"

using evpp::Buffer;
using std::string;

TEST_UNIT(testBufferAppendRead) {
    Buffer buf;
    H_TEST_EQUAL(buf.length(), 0);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend);

    const string str(200, 'x');
    buf.Append(str);
    H_TEST_EQUAL(buf.length(), str.size());
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize - str.size());
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend);

    const string str2 = buf.NextString(50);
    H_TEST_EQUAL(str2.size(), 50);
    H_TEST_EQUAL(buf.length(), str.size() - str2.size());
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize - str.size());
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend + str2.size());
    H_TEST_EQUAL(str2, string(50, 'x'));

    buf.Append(str);
    H_TEST_EQUAL(buf.length(), 2 * str.size() - str2.size());
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize - 2 * str.size());
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend + str2.size());

    const string str3 = buf.NextAllString();
    H_TEST_EQUAL(str3.size(), 350);
    H_TEST_EQUAL(buf.length(), 0);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend);
    H_TEST_EQUAL(str3, string(350, 'x'));
}

TEST_UNIT(testBufferGrow) {
    Buffer buf;
    buf.Append(string(400, 'y'));
    H_TEST_EQUAL(buf.length(), 400);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize - 400);

    buf.Retrieve(50);
    H_TEST_EQUAL(buf.length(), 350);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize - 400);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend + 50);

    buf.Append(string(1000, 'z'));
    H_TEST_EQUAL(buf.length(), 1350);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend);
    H_TEST_ASSERT(buf.WritableBytes() >= 0);

    buf.Reset();
    H_TEST_EQUAL(buf.length(), 0);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend);
    H_TEST_ASSERT(buf.WritableBytes() >= Buffer::kInitialSize * 2);
}

TEST_UNIT(testBufferInsideGrow) {
    Buffer buf;
    buf.Append(string(800, 'y'));
    H_TEST_EQUAL(buf.length(), 800);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize - 800);

    buf.Retrieve(500);
    H_TEST_EQUAL(buf.length(), 300);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize - 800);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend + 500);

    buf.Append(string(300, 'z'));
    H_TEST_EQUAL(buf.length(), 600);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize - 600);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend);
}

TEST_UNIT(testBufferShrink) {
    Buffer buf;
    buf.Append(string(2000, 'y'));
    H_TEST_EQUAL(buf.length(), 2000);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend);
    H_TEST_ASSERT(buf.WritableBytes() >= 0);

    buf.Retrieve(1500);
    H_TEST_EQUAL(buf.length(), 500);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend + 1500);
    H_TEST_ASSERT(buf.WritableBytes() >= 0);

    buf.Shrink(0);
    H_TEST_EQUAL(buf.length(), 500);
    H_TEST_EQUAL(buf.WritableBytes(), 0);
    H_TEST_EQUAL(buf.NextAllString(), string(500, 'y'));
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend);
}

TEST_UNIT(testBufferPrepend) {
    Buffer buf;
    buf.Append(string(200, 'y'));
    H_TEST_EQUAL(buf.length(), 200);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize - 200);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend);

    int x = 0;
    buf.Prepend(&x, sizeof x);
    H_TEST_EQUAL(buf.length(), 204);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize - 200);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend - 4);
}

TEST_UNIT(testBufferReadInt) {
    Buffer buf;
    buf.Append("HTTP");

    H_TEST_EQUAL(buf.length(), 4);
    H_TEST_EQUAL(buf.PeekInt8(), 'H');
    int top16 = buf.PeekInt16();
    H_TEST_EQUAL(top16, 'H' * 256 + 'T');
    H_TEST_EQUAL(buf.PeekInt32(), top16 * 65536 + 'T' * 256 + 'P');

    H_TEST_EQUAL(buf.ReadInt8(), 'H');
    H_TEST_EQUAL(buf.ReadInt16(), 'T' * 256 + 'T');
    H_TEST_EQUAL(buf.ReadInt8(), 'P');
    H_TEST_EQUAL(buf.length(), 0);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize);

    buf.AppendInt8(-1);
    buf.AppendInt16(-2);
    buf.AppendInt32(-3);
    H_TEST_EQUAL(buf.length(), 7);
    H_TEST_EQUAL(buf.ReadInt8(), -1);
    H_TEST_EQUAL(buf.ReadInt16(), -2);
    H_TEST_EQUAL(buf.ReadInt32(), -3);
    H_TEST_EQUAL(buf.length(), 0);
}

TEST_UNIT(testBufferFindEOL) {
    Buffer buf;
    buf.Append(string(100000, 'x'));
    const char* null = NULL;
    H_TEST_EQUAL(buf.FindEOL(), null);
    H_TEST_EQUAL(buf.FindEOL(buf.data() + 90000), null);
}


TEST_UNIT(testBufferTruncate) {
    Buffer buf;
    buf.Append("HTTP");

    H_TEST_EQUAL(buf.length(), 4);
    buf.Truncate(3);
    H_TEST_EQUAL(buf.length(), 3);
    buf.Truncate(2);
    H_TEST_EQUAL(buf.length(), 2);
    buf.Truncate(1);
    H_TEST_EQUAL(buf.length(), 1);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize - 1);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend);
    buf.Truncate(0);
    H_TEST_EQUAL(buf.length(), 0);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend);

    buf.Append("HTTP");
    buf.Reset();
    H_TEST_EQUAL(buf.length(), 0);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend);

    buf.Append("HTTP");
    buf.Truncate(Buffer::kInitialSize + 1000);
    H_TEST_EQUAL(buf.length(), 4);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize - 4);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend);

    buf.Append("HTTPS");
    H_TEST_EQUAL(buf.length(), 9);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize - 9);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend);

    buf.Next(4);
    H_TEST_EQUAL(buf.length(), 5);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize - 9);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend + 4);
    buf.Truncate(5);
    H_TEST_EQUAL(buf.length(), 5);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize - 9);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend + 4);
    buf.Truncate(6);
    H_TEST_EQUAL(buf.length(), 5);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize - 9);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend + 4);
    buf.Truncate(4);
    H_TEST_EQUAL(buf.length(), 4);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize - 8);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend + 4);
}


TEST_UNIT(testBufferReserve) {
    Buffer buf;
    buf.Append("HTTP");
    H_TEST_EQUAL(buf.length(), 4);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize - 4);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend);

    buf.Reserve(100);
    H_TEST_EQUAL(buf.length(), 4);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize - 4);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend);


    buf.Reserve(Buffer::kInitialSize);
    H_TEST_EQUAL(buf.length(), 4);
    H_TEST_EQUAL(buf.WritableBytes(), Buffer::kInitialSize - 4);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend);

    buf.Reserve(2 * Buffer::kInitialSize);
    H_TEST_EQUAL(buf.length(), 4);
    H_TEST_ASSERT(buf.WritableBytes() >= 2 * Buffer::kInitialSize - 4);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend);

    buf.Append("HTTPS");
    H_TEST_EQUAL(buf.length(), 9);
    H_TEST_ASSERT(buf.WritableBytes() >= 2 * Buffer::kInitialSize - 9);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend);

    buf.Next(4);
    H_TEST_EQUAL(buf.length(), 5);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend + 4);

    buf.Reserve(8 * Buffer::kInitialSize);
    H_TEST_EQUAL(buf.length(), 5);
    H_TEST_ASSERT(buf.WritableBytes() >= 8 * Buffer::kInitialSize - 5);
    H_TEST_EQUAL(buf.PrependableBytes(), Buffer::kCheapPrepend);
}


//#include <boost/asio/ip/address.hpp>
//boost::asio::ip::address