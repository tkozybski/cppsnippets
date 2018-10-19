#pragma once
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <limits.h>
#include <random>

class Random {

//C++11, manual implementation of std::disjunction
//MSVC is buggy and shows the wrong value
#if __cpluscplus >= 201103L || (defined _HAS_CXX17 && _HAS_CXX17 == 1)
    template <typename T, typename... U>
    using resolvedType = typename std::enable_if<std::disjunction_v<std::is_same<T, U>...>, T>::type;
#else
    template <class...>
    struct disjunction : std::false_type {
    };
    template <class B1>
    struct disjunction<B1> : B1 {
    };
    template <class B1, class... Bn>
    struct disjunction<B1, Bn...>
        : std::conditional_t<bool(B1::value), B1, disjunction<Bn...>> {
    };
    template <typename T, typename... U>
    using resolvedType = typename std::enable_if<disjunction<std::is_same<T, U>...>::value, T>::type;

#endif

public:
    //Provide static seed for predictable results eg. for tests
    Random(uint64_t seed1, uint64_t seed2, uint64_t seed3, uint64_t seed4)
    {
        _rng.Seed(seed1, seed2, seed3, seed4);
    }

    Random()
    {
        //Opening random devices is costly
        //Read: https://en.cppreference.com/w/cpp/numeric/random/random_device
        static thread_local std::random_device rd;

        //Two sources of seed makes the randomness better
        //MinGW fails on providing random_device, so it's a "solution"
        auto epochTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        auto seed1 = (static_cast<uint64_t>(rd()) << 32) | (epochTime & 0xffffffff);
        auto seed2 = (static_cast<uint64_t>(rd()) << 32) | (rd() & 0xffffffff);
        auto seed3 = (static_cast<uint64_t>(rd()) << 32) | (epochTime & 0xffffffff);
        auto seed4 = (static_cast<uint64_t>(rd()) << 32) | (rd() & 0xffffffff);

        _rng.Seed(seed1, seed2, seed3, seed4);
    }

    double NextDouble()
    {
        thread_local static std::uniform_real_distribution<double> dist(0.0, 1.0);
        //std::uniform_real_distribution<double> dis(0.0, 1.0);
        return dist(_rng);
    }

    template <class T = int>
    resolvedType<T, short, int, long, long long, unsigned short, unsigned int, unsigned long, unsigned long long>
    Next(const T min, const T max)
    {
        using param_type = typename std::uniform_int_distribution<T>::param_type;
        thread_local static std::uniform_int_distribution<T> dist;
        return dist(_rng, param_type{ min, max });
    }

    template <class T = double>
    resolvedType<T, float, double, long double>
    Next(const T min, const T max)
    {
        using param_type = typename std::uniform_real_distribution<T>::param_type;
        thread_local static std::uniform_real_distribution<T> dist;
        return dist(_rng, param_type{ min, max });
    }

    /*
	    Im getting consistent speed of ~2.000.000.000b/s (1.86Gb/s)
        of filled data with one call on Haswell CPU (not including buffer creation)
	*/

    void NextBytes(unsigned char* buffer, const size_t bufferSize)
    {
        constexpr size_t size = sizeof(uint64_t);
        size_t currentByte = 0;
        while (currentByte < bufferSize) {
            //We dont need to generate new value for every byte, can reuse the calculated bytes
            auto generatedValue = this->Next<uint64_t>(0, ULLONG_MAX);
            auto amountToCopy = currentByte + size > bufferSize ? bufferSize - currentByte : size;
            memcpy(&(buffer[currentByte]), &generatedValue, amountToCopy);
            currentByte += amountToCopy;
        }
    }

private:
    //Not tested on 32bit but it should be slower

    //class Xor128Bit {
    //public:
    //    using result_type = uint64_t;
    //    void Seed(result_type seed1, result_type seed2)
    //    {
    //        u = seed1;
    //        w = seed2;
    //        for (int i = 0; i < 256; ++i)
    //            (*this)();
    //    }

    //    static inline uint64_t rotl(const uint64_t x, int k)
    //    {
    //        return (x << k) | (x >> (64 - k));
    //    }

    //    //Xorshift128+
    //    //result_type operator()()
    //    //{
    //    //    uint64_t x = u;
    //    //    const uint64_t y = w;
    //    //    u = y;
    //    //    x ^= x << 23;
    //    //    w = x ^ y ^ (x >> 17) ^ (y >> 26);
    //    //    return w + y;
    //    //}

    //    ////Xoroshiro128+
    //    //result_type operator()()
    //    //{
    //    //    const uint64_t s0 = u;
    //    //    uint64_t s1 = w;
    //    //    const uint64_t result = s0 + s1;

    //    //    s1 ^= s0;
    //    //    u = rotl(s0, 24) ^ s1 ^ (s1 << 16); // a, b
    //    //    w = rotl(s1, 37); // c

    //    //    return result;
    //    //}

    //    //Xoroshiro128**
    //    result_type operator()()
    //    {
    //        const uint64_t s0 = u;
    //        uint64_t s1 = w;
    //        const uint64_t result = rotl(s0 * 5, 7) * 9;

    //        s1 ^= s0;
    //        u = rotl(s0, 24) ^ s1 ^ (s1 << 16); // a, b
    //        w = rotl(s1, 37); // c

    //        return result;
    //    }

    //    static constexpr result_type(min)()
    //    {
    //        return 0;
    //    }

    //    static constexpr result_type(max)()
    //    {
    //        return ULLONG_MAX;
    //    }

    //private:
    //    uint64_t u, w;
    //} _rng;

    class Xor256Bit {
    public:
        using result_type = uint64_t;
        void Seed(result_type seed1, result_type seed2, result_type seed3, result_type seed4)
        {
            u = seed1;
            w = seed2;
			x = seed3;
            y = seed4;

            for (int i = 0; i < 256; ++i)
                (*this)();
        }

        static inline uint64_t rotl(const uint64_t x, int k)
        {
            return (x << k) | (x >> (64 - k));
        }

        //Xoroshiro256+
        //result_type operator()()
        //{
        //    const uint64_t result_plus = u + y;
        //    const uint64_t t = w << 17;
        //    x ^= u;
        //    y ^= w;
        //    w ^= x;
        //    u ^= y;

        //    x ^= t;
        //    y = rotl(y, 45);

        //    return result_plus;
        //}

        //Xoroshiro256**
        result_type operator()()
        {
            const uint64_t result_starstar = rotl(w * 5, 7) * 9;
            const uint64_t t = w << 17;

            x ^= u;
            y ^= w;
            w ^= x;
            u ^= y;

            x ^= t;

            y = rotl(y, 45);

            return result_starstar;
        }

        static constexpr result_type(min)()
        {
            return 0;
        }

        static constexpr result_type(max)()
        {
            return ULLONG_MAX;
        }

    private:
        uint64_t u, w, x, y;
    } _rng;
};
