#ifndef EVMC_OBJECTPOOL_HPP_
#define EVMC_OBJECTPOOL_HPP_
#include <memory>
#include <vector>
#include <functional>
#include <iostream>

namespace evmc {
template <class T, class Child>
class ObjectPool
{
public:
	using DeleterType = std::function<void(T*)>;
	ObjectPool(int chunknum = 1024) {
		try {
			for (int i = 0; i < chunknum; ++i) {
				pool_.push_back(std::unique_ptr<T>(new Child()));
			}
		} catch(std::exception& e) {
			LOG_WARN << "init objectPool failed:" << e.what();
		}
	}
/*
    std::unique_ptr<T, DeleterType> get()
    {
		std::lock_guard<std::mutex> lck(mutex_);
        if (pool_.empty()) {
			std::unique_ptr<T, DeleterType> ptr(new Child(), [this](T* t) {
					mutex_.lock();
					pool_.push_back(std::unique_ptr<T>(t));
					mutex_.unlock();
					});
			return std::move(ptr);
		}

        std::unique_ptr<T, DeleterType> ptr(pool_.back().release(), [this](T* t) {
				mutex_.lock();
				pool_.push_back(std::unique_ptr<T>(t));
				mutex_.unlock();
        });

        pool_.pop_back();
        return std::move(ptr);
    }*/

	std::shared_ptr<T> get_shared() {
		std::lock_guard<std::mutex> lck(mutex_);
		if (pool_.empty()) {
			return std::shared_ptr<T>(new Child(), [this](T* t) {  
					mutex_.lock();
					pool_.push_back(std::unique_ptr<T>(t));  
					mutex_.unlock();
					}); 
		}
		auto pin = std::unique_ptr<T>(std::move(pool_.back()));
		pool_.pop_back();
		return std::shared_ptr<T>(pin.release(), [this](T* t) {  
				mutex_.lock();
				pool_.push_back(std::unique_ptr<T>(t));  
				mutex_.unlock();
				}); 
	}
private:
	std::vector<std::unique_ptr<T> > pool_;
	std::mutex mutex_;
};

}
#endif
