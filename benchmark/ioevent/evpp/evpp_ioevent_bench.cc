// Modified from muduo/examples/pingpong/bench.cc

// Benchmark inspired by libevent/test/bench.c
// See also: http://libev.schmorp.de/bench.html

#include <evpp/exp.h>
#include <evpp/event_loop.h>
#include <evpp/fd_channel.h>
#include <evpp/libevent_headers.h>

#include <getopt.h>

#ifndef _WIN32
#include <stdio.h>
#include <sys/resource.h>
#include <sys/socket.h>
#endif

#include "../../../examples/echo/tcpecho/winmain-inl.h"

using namespace evpp;

std::vector<int> g_pipes;
int numPipes;
int numActive;
int numWrites;
EventLoop* g_loop;
std::vector<FdChannel*> g_channels;

int g_reads, g_writes, g_fired;
int g_total_reads = 0;

void readCallback(int fd, int idx) {
    g_total_reads++;
    char ch = 0;
    g_reads += static_cast<int>(::recv(fd, &ch, sizeof(ch), 0));
    if (g_writes > 0) {
        int widx = idx + 1;
        if (widx >= numPipes) {
            widx -= numPipes;
        }
        ::send(g_pipes[2 * widx + 1], &ch, 1, 0);
        g_writes--;
        g_fired++;
    }
    if (g_fired == g_reads) {
        g_loop->Stop();
    }
}

std::pair<int, int> runOnce() {
    Timestamp beforeInit(Timestamp::Now());
    int space = numPipes / numActive;
    space *= 2;
    char ch = 'm';
    for (int i = 0; i < numActive; ++i) {
        ::send(g_pipes[i * space + 1], &ch, 1, 0);
    }

    g_fired = numActive;
    g_reads = 0;
    g_writes = numWrites;
    Timestamp beforeLoop(Timestamp::Now());
    g_loop->Run();

    Timestamp end(Timestamp::Now());

    int iterTime = static_cast<int>(end.UnixMicro() - beforeInit.UnixMicro());
    int loopTime = static_cast<int>(end.UnixMicro() - beforeLoop.UnixMicro());
    return std::make_pair(iterTime, loopTime);
}

int main(int argc, char* argv[]) {
    numPipes = 100;
    numActive = 1;
    numWrites = 100;
    int c;
    while ((c = getopt(argc, argv, "n:a:w:")) != -1) {
        switch (c) {
        case 'n':
            numPipes = atoi(optarg);
            break;
        case 'a':
            numActive = atoi(optarg);
            break;
        case 'w':
            numWrites = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Illegal argument \"%c\"\n", c);
            return 1;
        }
    }

#ifndef _WIN32
    //struct rlimit rl;
    //rl.rlim_cur = rl.rlim_max = numPipes * 2 + 50;
    //if (::setrlimit(RLIMIT_NOFILE, &rl) == -1) {
    //    perror("setrlimit");
    //    //return 1;  // comment out this line if under valgrind
    //}
#endif

    g_pipes.resize(2 * numPipes);
    for (int i = 0; i < numPipes; ++i) {
        if (evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, &g_pipes[i * 2]) == -1) {
            perror("pipe");
        }

        if (evutil_make_socket_nonblocking(g_pipes[i*2]) < 0 ||
            evutil_make_socket_nonblocking(g_pipes[i*2 + 1]) < 0) {
        }

    }

    EventLoop loop;
    g_loop = &loop;

    for (int i = 0; i < numPipes; ++i) {
        FdChannel* channel = new FdChannel(&loop, g_pipes[i * 2], true, false);
        channel->SetReadCallback(std::bind(readCallback, std::placeholders::_1, channel->fd(), i));
        channel->AttachToLoop();
        g_channels.push_back(channel);
    }

    std::vector<std::pair<int, int>> costs;
    for (int i = 0; i < 25; ++i) {
        std::pair<int, int> t = runOnce();
        printf("%8d %8d g_reads=%d g_fired=%d g_total_reads=%d\n", t.first, t.second, g_reads, g_fired, g_total_reads);
        costs.push_back(t);
    }

    int sum1 = 0, sum2 = 0;
    for (auto t : costs) {
        sum1 += t.first;
        sum2 += t.second;
    }
    printf("%s Average : %8d %8d\n", argv[0], sum1 / int(costs.size()), sum2 / int(costs.size()));

    for (auto it = g_channels.begin();
         it != g_channels.end(); ++it) {
        (*it)->DisableAllEvent();
        (*it)->Close();
        delete *it;
    }
}

