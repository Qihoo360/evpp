#pragma once

#include <typeinfo>
#include <algorithm>

namespace evpp {

// A variant type that can hold any other type.
//
// Usage 1 :
//
//    Buffer* buf(new Buffer());
//    Any any(buf);
//    Buffer* b = any_cast<Buffer*>(any);
//    assert(buf == b);
//    delete buf;
//
//
// Usage 2 :
//
//    std::shared_ptr<Buffer> buf(new Buffer());
//    Any any(buf);
//    std::shared_ptr<Buffer> b = any_cast<std::shared_ptr<Buffer>>(any);
//    assert(buf.get() == b.get());
//
//
// Usage 3 :
//
//    std::shared_ptr<Buffer> buf(new Buffer());
//    Any any(buf);
//    std::shared_ptr<Buffer> b = any.Get<std::shared_ptr<Buffer>>();
//    assert(buf.get() == b.get());
//
class Any {
public:
    Any() : content_(nullptr) {}
    ~Any() {
        delete content_;
    }

    template<typename ValueType>
    explicit Any(const ValueType& value)
        : content_(new Holder<ValueType>(value)) {}

    Any(const Any& other)
        : content_(other.content_ ? other.content_->clone() : nullptr) {}

public:
    Any& swap(Any& rhs) {
        std::swap(content_, rhs.content_);
        return*this;
    }

    template<typename ValueType>
    Any& operator=(const ValueType& rhs) {
        Any(rhs).swap(*this);
        return*this;
    }

    Any& operator=(const Any& rhs) {
        Any(rhs).swap(*this);
        return*this;
    }

    bool IsEmpty() const {
        return !content_;
    }

    const std::type_info& GetType() const {
        return content_ ? content_->GetType() : typeid(void);
    }

    template<typename ValueType>
    ValueType operator()() const {
        return Get<ValueType>();
    }

    template<typename ValueType>
    ValueType Get() const {
        if (GetType() == typeid(ValueType)) {
            return static_cast<Any::Holder<ValueType>*>(content_)->held_;
        } else {
            return ValueType();
        }
    }
protected:
    class PlaceHolder {
    public:
        virtual ~PlaceHolder() {}
    public:
        virtual const std::type_info& GetType() const = 0;
        virtual PlaceHolder* clone() const = 0;
    };

    template<typename ValueType>
    class Holder : public PlaceHolder {
    public:
        Holder(const ValueType& value)
            : held_(value) {}

        virtual const std::type_info& GetType() const {
            return typeid(ValueType);
        }

        virtual PlaceHolder* clone() const {
            return new Holder(held_);
        }

        ValueType held_;
    };

protected:
    PlaceHolder* content_;
    template<typename ValueType>
    friend ValueType* any_cast(Any*);
};

template<typename ValueType>
ValueType* any_cast(Any* any) {
    if (any && any->GetType() == typeid(ValueType)) {
        return &(static_cast<Any::Holder<ValueType>*>(any->content_)->held_);
    }

    return nullptr;
}

template<typename ValueType>
const ValueType* any_cast(const Any* any) {
    return any_cast<ValueType>(const_cast<Any*>(any));
}

template<typename ValueType>
ValueType any_cast(const Any& any) {
    const ValueType* result = any_cast<ValueType>(&any);
    assert(result);

    if (!result) {
        return ValueType();
    }

    return *result;
}
}//namespace evpp

