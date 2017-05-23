evmc
---

`evmc` is a nonblocking async C++ client library for memcached and membase. It is based on [evpp](https://github.com/Qihoo360/evpp) which is a modern C++ network library.

### Status

This library is currently used in production which sends more than 300 billions requests every day.

### Features

1. Support single `memcached` instance
2. Support a cluster of `memcached` which are configurated by vbucket, like `membase` or `couchbase`
3. Support failover and load balance

### Dependencies

- libhashkit
- [evpp](https://github.com/nsqio/nsq)
- [rapidjson](https://github.com/nsqio/nsq)
- [libevent](https://github.com/libevent/libevent)
- [glog](https://github.com/google/glog)
