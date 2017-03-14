

#include <evpp/exp.h>
#include <evpp/event_loop.h>
#include <evpp/fd_channel.h>
#include <evpp/libevent_headers.h>
#include <evpp/libevent_watcher.h>

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
std::vector<std::shared_ptr<PipeEventWatcher>> g_pipe_event_watchers;

int g_reads, g_writes, g_fired;

void ReadCallbackOfFdChannel(int fd, int idx) {
    char ch = 'm';

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

void ReadCallbackOfPipeEventWatcher(int idx) {
    g_reads++;
    if (g_writes > 0) {
        int widx = idx + 1;
        if (widx >= numPipes) {
            widx -= numPipes;
        }
        g_pipe_event_watchers[widx]->Notify();
        g_writes--;
        g_fired++;
    }

    if (g_fired == g_reads) {
        g_loop->Stop();
    }
}

std::pair<int, int> FdChannelRunOnce() {
    Timestamp beforeInit(Timestamp::Now());

    //int space = numPipes / numActive;
    //space *= 2;
    for (int i = 0; i < numActive; ++i) {
        ::send(g_pipes[i * 2 + 1], "m", 1, 0);
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

std::pair<int, int> PipeEventWatcherRunOnce() {
    Timestamp beforeInit(Timestamp::Now());
    for (int i = 0; i < numActive; ++i) {
        int fd = g_pipe_event_watchers[i]->wfd();
        ::send(fd, "m", 1, 0);
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
    numPipes = 1000;
    numActive = 10;
    numWrites = 1000;

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
        g_channels.push_back(channel);
        channel->SetReadCallback(std::bind(ReadCallbackOfFdChannel, std::placeholders::_1, channel->fd(), i));
        channel->AttachToLoop();

        auto f = std::bind(&ReadCallbackOfPipeEventWatcher, i);
        std::shared_ptr<PipeEventWatcher> w(new PipeEventWatcher(g_loop, f));
        w->Init();
        w->AsyncWait();
        g_pipe_event_watchers.push_back(w);
    }

    std::vector<std::pair<int, int>> fd_channel_cost;
    std::vector<std::pair<int, int>> pipe_event_watcher_cost;
    for (int i = 0; i < 25; ++i) {
        std::pair<int, int> t = FdChannelRunOnce();
        printf("       FdChannelRunOnce %8d %8d\n", t.first, t.second);
        fd_channel_cost.push_back(t);
        t = PipeEventWatcherRunOnce();
        printf("PipeEventWatcherRunOnce %8d %8d\n", t.first, t.second);
        pipe_event_watcher_cost.push_back(t);
    }

    int sum1 = 0, sum2 = 0;
    for (auto t : fd_channel_cost) {
        sum1 += t.first;
        sum2 += t.second;
    }
    printf("%s        FdChannelRunOnce Average : %8d %8d\n", argv[0], sum1 / int(fd_channel_cost.size()), sum2 / int(fd_channel_cost.size()));

    sum1 = 0, sum2 = 0;
    for (auto t : pipe_event_watcher_cost) {
        sum1 += t.first;
        sum2 += t.second;
    }
    printf("%s PipeEventWatcherRunOnce Average : %8d %8d\n", argv[0], sum1 / int(pipe_event_watcher_cost.size()), sum2 / int(pipe_event_watcher_cost.size()));

    for (auto it = g_channels.begin();
         it != g_channels.end(); ++it) {
        (*it)->DisableAllEvent();
        (*it)->Close();
        delete *it;
    }

    return 0;
}


/*
   ./build-release/bin/benchmark_fd_channel_vs_pipe_event_watcher        FdChannelRunOnce Average :     1920     1912
   ./build-release/bin/benchmark_fd_channel_vs_pipe_event_watcher PipeEventWatcherRunOnce Average :     1138     1130
 */
