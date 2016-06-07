#pragma once

#include <string.h>
#include <assert.h>
#include <string>

namespace evpp {
    namespace base {
        class Slice {
        public:
            typedef char value_type;

        public:
            // Create an empty slice.
            Slice() : data_(""), size_(0) {}

            // Create a slice that refers to d[0,n-1].
            Slice(const char* d, size_t n) : data_(d), size_(n) {}

            // Create a slice that refers to the contents of "s"
            Slice(const std::string& s) : data_(s.data()), size_(s.size()) {}

            // Create a slice that refers to s[0,strlen(s)-1]
            Slice(const char* s) : data_(s), size_(strlen(s)) {}

            // Return a pointer to the beginning of the referenced data
            const char* data() const { return data_; }

            // Return the length (in bytes) of the referenced data
            size_t size() const { return size_; }

            // Return true if the length of the referenced data is zero
            bool empty() const { return size_ == 0; }

            // Return the ith byte in the referenced data.
            // REQUIRES: n < size()
            char operator[](size_t n) const {
                assert(n < size());
                return data_[n];
            }

            // Change this slice to refer to an empty array
            void clear() { data_ = ""; size_ = 0; }

            // Drop the first "n" bytes from this slice.
            void remove_prefix(size_t n) {
                assert(n <= size());
                data_ += n;
                size_ -= n;
            }

            // Return a string that contains the copy of the referenced data.
            std::string toString() const { return std::string(data_, size_); }

            // Three-way comparison.  Returns value:
            //   <  0 if "*this" <  "b",
            //   == 0 if "*this" == "b",
            //   >  0 if "*this" >  "b"
            int compare(const Slice& b) const;

        private:
            const char* data_;
            size_t size_;
        };

        typedef Slice slice;

        //---------------------------------------------------------
        //typedef Map<Slice, Slice> SliceSliceMap;
        //---------------------------------------------------------

        inline bool operator==(const Slice& x, const Slice& y) {
            return ((x.size() == y.size()) &&
                (memcmp(x.data(), y.data(), x.size()) == 0));
        }

        inline bool operator!=(const Slice& x, const Slice& y) {
            return !(x == y);
        }

        inline bool operator<(const Slice& x, const Slice& y) {
            return x.compare(y) < 0;
        }


        inline int Slice::compare(const Slice& b) const {
            const size_t min_len = (size_ < b.size_) ? size_ : b.size_;
            int r = memcmp(data_, b.data_, min_len);
            if (r == 0) {
                if (size_ < b.size_) r = -1;
                else if (size_ > b.size_) r = +1;
            }
            return r;
        }
    }
}  // namespace evpp
