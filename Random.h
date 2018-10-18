#pragma once
#include <limits.h>
#include <random>
#include <memory>

class Random {
//C++11, manual implementation of std::disjunction
//MSVC is buggy and shows the wrong value
#if __cplusplus < 201703L && !defined _HAS_CXX17
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
//C++17
#else
    template <typename T, typename... U>
    using resolvedType = typename std::enable_if<std::disjunction_v<std::is_same<T, U>...>, T>::type;
#endif

public:
    //Provide static seed for predictable results eg. for tests
    Random(unsigned long long seed)
        : _rng(seed)
    {
    }

    Random()
    {
		//Opening random devices is costly
		//Read: https://en.cppreference.com/w/cpp/numeric/random/random_device
        static thread_local std::random_device rd;

		//64 bit seed instead of typicaly used 32bit
		//long size differs from platform to platfrom, so better calculate it on compile time
        constexpr auto ullongHalfSize = sizeof(unsigned long long) / 2;
        auto seed = (static_cast<unsigned long long>(rd()) << ullongHalfSize) | rd();
        _rng.seed(seed);

		//Adds ~5 microseconds to the initialization phase. Uncomment for increased randomness
		////https://codereview.stackexchange.com/questions/109260/seed-stdmt19937-from-stdrandom-device/109266#109266
		////http://www.iro.umontreal.ca/~lecuyer/myftp/papers/lfsr04.pdf
		////"According to this paper [...]  You need to perform about 700000 random number generations (or twists) during/after the initialization phase."
        //_rng.discard(700000);
    }

    double NextDouble()
    {
        std::uniform_real_distribution<double> dis(0.0, 1.0);
        return dis(_rng);
    }

    template <class T = int>
    resolvedType<T, short, int, long, long long, unsigned short, unsigned int, unsigned long, unsigned long long>
    Next(const T min, const T max)
    {
        std::uniform_int_distribution<T> dis(min, max);
        return dis(_rng);
    };

    template <class T = double>
    resolvedType<T, float, double, long double>
    Next(const T min, const T max)
    {
        std::uniform_real_distribution<T> dis(min, max);
        return dis(_rng);
    };

    /*
	    Im getting consistent speed of about 1.000.000.000b/s (0.93Gb/s)
        of filled data with one call on Haswell CPU (not including buffer creation)
		
		Times including buffer creation (5 loops, median)
		old new unsigned char[size] + delete[] = 1404ms
		manually creating unique_ptr<byte[]>(size) and passing it to function = 1461ms
		using NextBytes to create buffer = 1422ms
	*/
    void NextBytes(unsigned char* buffer, const size_t bufferSize)
    {
        constexpr size_t size = sizeof(unsigned long long);
        size_t currentByte = 0;
        while (currentByte < bufferSize) {
            //We dont need to generate new value for every byte, can reuse the calculated bytes
            auto generatedValue = this->Next<unsigned long long>(0, ULLONG_MAX);
            auto amountToCopy = currentByte + size > bufferSize ? bufferSize - currentByte : size;
            memcpy(&(buffer[currentByte]), &generatedValue, amountToCopy);
            currentByte += amountToCopy;
        }
    }

    std::unique_ptr<unsigned char[]> NextBytes(const size_t amount)
    {
        //new instead of std::make_unique for C++11 compatibility
        auto bytes = std::unique_ptr<unsigned char[]>(new unsigned char[amount]);
        this->NextBytes(bytes.get(), amount);
        return bytes;
    }

private:
    //The 64bit version is faster on 64bit architecture and gives more pseudorandomness
    //Not tested on 32bit but it should be slower
    std::mt19937_64 _rng;
};
