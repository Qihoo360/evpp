// Modified from muduo/examples/pingpong/bench.cc

// Benchmark inspired by libevent/test/bench.c
// See also: http://libev.schmorp.de/bench.html

#include <evpp/event_loop.h>
#include <evpp/fd_channel.h>
#include <evpp/timestamp.h>
#include <evpp/event_watcher.h>

#include <getopt.h>

#ifndef _WIN32
#include <stdio.h>
#include <sys/resource.h>
#include <sys/socket.h>
#endif

#include "../../../examples/echo/tcpecho/winmain-inl.h"

using namespace evpp;

typedef std::shared_ptr<PipeEventWatcher> PipeEventWatcherPtr;
int numPipes;
int numActive;
int numWrites;
EventLoop* g_loop;
std::vector<PipeEventWatcherPtr> g_pipes;

int g_reads, g_writes, g_fired;
int g_total_reads = 0;

void ReadCallback(int idx) {
    g_total_reads++;
    g_reads++;
    if (g_writes > 0) {
        int widx = idx + 1;
        if (widx >= numPipes) {
            widx -= numPipes;
        }
        g_pipes[widx]->Notify();
        g_writes--;
        g_fired++;
    }

    if (g_fired == g_reads) {
        g_loop->Stop();
    }
}

std::pair<int, int> runOnce() {
    Timestamp beforeInit(Timestamp::Now());
    for (int i = 0; i < numActive; ++i) {
        g_pipes[i]->Notify();
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
            fprintf(stderr, "Usage : %s -n <pipes-number> -a <active-pipes-number> -w <write-num>\n", argv[0]);
            return 1;
        }
    }

#ifndef _WIN32
    //struct rlimit rl;
    //rl.rlim_cur = rl.rlim_max = numPipes * 2 + 5000;
    //if (::setrlimit(RLIMIT_NOFILE, &rl) == -1) {
    //    perror("setrlimit");
    //    //return 1;  // comment out this line if under valgrind
    //}
#endif

    EventLoop loop;
    g_loop = &loop;

    for (int i = 0; i < numPipes; ++i) {
        auto f = std::bind(&ReadCallback, i);
        PipeEventWatcherPtr w(new PipeEventWatcher(g_loop, f));
        w->Init();
        w->AsyncWait();
        g_pipes.push_back(w);
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

    for (auto pipe : g_pipes) {
        pipe->Cancel();
    }
    g_pipes.clear();
}

