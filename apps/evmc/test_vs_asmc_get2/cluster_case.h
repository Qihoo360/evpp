#ifndef __CLUSTER_CASE_H__
#define __CLUSTER_CASE_H__


#include <map>
#include <string>
#include <sstream>

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

class Timer {
public:
	Timer() {
		begin_ = {0,0};
		end_ = {0,0};
	}
	Timer(const Timer& t) {
		begin_ = t.begin_;
		end_ = t.end_;
	}
	int64_t GetTimestampMs() {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return tv.tv_sec * 1000 + tv.tv_usec / 1000;
	}
	inline void Begin() {
		gettimeofday(&begin_, NULL);
	}

	inline void End() {
		gettimeofday(&end_, NULL);
	}
	static void MsSleep(const int64_t t) {
		struct timeval tv;
		tv.tv_sec = t / 1000;
		tv.tv_usec = (t % 1000) * 1000;
		int err = 0;
		do {
			err = select(0, NULL, NULL, NULL, &tv);
		}while (err < 0 && errno == EINTR);
	}
	inline int64_t Elapsedus() {
		return (end_.tv_sec - begin_.tv_sec) * 1e6 + (end_.tv_usec - begin_.tv_usec);  
	}
          
private:
	struct timeval begin_;
	struct timeval end_;
};
#endif /* __CLUSTER_CASE_H__ */

