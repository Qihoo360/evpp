/****************************** Module Header ********************************\
    Name: random.h
 Created: 2012/01/08 8:1:2012 1:37
  Author: weizili

 Purpose: 
\*****************************************************************************/
#ifndef OSLIB_RANDOM_H_
#define OSLIB_RANDOM_H_

namespace osl
{
    // Copy from leveldb http://code.google.com/p/leveldb/source/browse/trunk/util/random.h
    // A very simple random number generator.  Not especially good at
    // generating truly random bits, but good enough for our needs in this
    // package.
    class Random 
    {
    public:
        explicit Random(uint32_t s) : seed_(s & 0x7fffffffu) { }

        uint32_t next() {
            static const uint32_t M = 2147483647L;   // 2^31-1
            static const uint64_t A = 16807;  // bits 14, 8, 7, 5, 2, 1, 0

            // We are computing
            //       seed_ = (seed_ * A) % M,    where M = 2^31-1
            //
            // seed_ must not be zero or M, or else all subsequent computed values
            // will be zero or M respectively.  For all other values, seed_ will end
            // up cycling through every number in [1,M-1]
            uint64_t product = seed_ * A;

            // Compute (product % M) using the fact that ((x << 31) % M) == x.
            seed_ = static_cast<uint32_t>((product >> 31) + (product & M));
            // The first reduction may overflow by 1 bit, so we may need to
            // repeat.  mod == M is not possible; using > allows the faster
            // sign-bit-based test.
            if (seed_ > M) {
                seed_ -= M;
            }

            return seed_;
        }

        // Returns a uniformly distributed value in the range [0..n-1]
        // REQUIRES: n > 0
        uint32_t uniform(int n) { assert( n > 0 ); return next() % n; }

        // Randomly returns true ~"1/n" of the time, and false otherwise.
        // REQUIRES: n > 0
        bool onein(int n) { assert(n > 0 ); return (next() % n) == 0; }

        // Skewed: pick "base" uniformly from range [0,max_log] and then
        // return "base" random bits.  The effect is to pick a number in the
        // range [0,2^max_log-1] with exponential bias towards smaller numbers.
        uint32_t skewed(int max_log) 
        {
            return uniform(1 << uniform(max_log + 1));
        }
    private:
        uint32_t seed_;
    };//end of class Random

    class RandomString {
    public:
        static std::string rand(size_t len = 128) {
            std::string s;
            s.resize(len);
            static Random r(time(NULL));
            const char* end = &s[0] + s.size();
            for (char* p = &s[0]; p < end; ++p)
            {
                *p = r.next() % 255;
            }
            return s;
        }
    };
}

#endif // OSLIB_RANDOM_H_



