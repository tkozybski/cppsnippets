# cppsnippets
Random stuff I wanna share

```cpp
#include "Random.h"

#include <stdio.h>
#include <iostream>
#include <memory>

int main(int argc, char **argv)
{
    Random rnd;
    auto randomInt = rnd.Next(0, 100);
    double randomDouble = rnd.Next(0.0, 100.0);
    auto randomFloat = rnd.Next<float>(0, 100);

    std::cout << "Int: " << randomInt << std::endl;
    std::cout << "Double: " << randomDouble << std::endl;
    std::cout << "Float: " << randomFloat << std::endl;
    std::cout << "NextDouble: " << rnd.NextDouble() << std::endl;

    constexpr int bufferSize = 100;
    auto buffer = std::make_unique<unsigned char[]>(bufferSize);
    rnd.NextBytes(buffer.get(), bufferSize);
    for (auto i = 0; i < bufferSize; i++ )
    {
        printf("%X,", buffer[i]);
    }
	return 0;
}
```

Results
>Int: 17
>
>Double: 45.417
>
>Float: 2.0576
>
>NextDouble: 0.965276
>
>A,35,9,36,6,9,8C,F8,9F,49,E6,79,10,D0,7,3F,9C,33,82,15,9,F0,DB,2C,59,DD,4F,4B,7F,61,AD,71,81,31,44,A6,15,6F,13,53,E,27,B2,71,46,95,93,61,EC,81,B9,E8,FF,12,46,59,15,EA,81,4C,36,2B,44,4C,85,DC,3C,D4,35,
68,2D,57,46,EB,BE,2F,2D,E7,FD,29,76,E1,31,3C,F4,CE,5E,57,F3,34,18,49,8B,F5,71,E5,27,1B,F0,63,

