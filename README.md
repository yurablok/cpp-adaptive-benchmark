# C++ Adaptive Benchmark

C++11 benchmark that can automatically choose the number of repetitions to
perform for the desired time.

The adaptive part of the benchmark follows a three-stage approach:
1. Rough measurement: Runs `minimumRepetitions` to find an initial average.
2. Clarifying measurement: If the testee is too fast, it performs an additional
   clarifying iteration.
3. Main measurement: Scales the iterations to fill the requested `timePerTestee_s`.

### Basic example:

```cpp
Benchmark benchmark;
benchmark.setColumnsNumber(2);
benchmark.add("int64_t throughput", 0, [&](uint32_t random, uint32_t) -> uint32_t {
    int64_t a = random;
    int64_t b = random;
    return a * b;
});
benchmark.add("int64_t throughput", 1, [&](uint32_t random, uint32_t) -> uint32_t {
    int64_t a = random;
    int64_t b = random;
    return a / b;
});
benchmark.add("int64_t latency", 0, [&](uint32_t random, uint32_t previous) -> uint32_t {
    int64_t a = random;
    int64_t b = previous | 1;
    return a * b;
});
benchmark.add("int64_t latency", 1, [&](uint32_t random, uint32_t previous) -> uint32_t {
    int64_t a = random;
    int64_t b = previous | 1;
    return a / b;
});
benchmark.add("double throughput", 0, [&](uint32_t random, uint32_t) -> uint32_t {
    double a = random;
    double b = random;
    return a * b;
});
benchmark.add("double throughput", 1, [&](uint32_t random, uint32_t) -> uint32_t {
    double a = random;
    double b = random;
    return a / b;
});
benchmark.add("double latency", 0, [&](uint32_t random, uint32_t previous) -> uint32_t {
    double a = random;
    double b = previous | 1;
    return a * b;
});
benchmark.add("double latency", 1, [&](uint32_t random, uint32_t previous) -> uint32_t {
    double a = random;
    double b = previous | 1;
    return a / b;
});
benchmark.run(5); // 5s per testee
```

```cpp
Benchmark is running for 8 subjects:
 [0] int64_t throughput... Done in 5s 206ms
 [1] int64_t throughput... Done in 5s 565ms
 [2] int64_t latency... Done in 5s 176ms
 [3] int64_t latency... Done in 5s 051ms
 [4] double throughput... Done in 5s 631ms
 [5] double throughput... Done in 5s 217ms
 [6] double latency... Done in 5s 079ms
 [7] double latency... Done in 5s 154ms

Minimum time:
| Name               |      Time |   %   |      Time |   %   |
|:-------------------|----------:|------:|----------:|------:|
| int64_t throughput |     786ps |   100 |     673ps |   100 |
| int64_t latency    | 2ns 672ps | 339.9 | 3ns 690ps | 548.2 |
| double throughput  |     898ps | 114.2 | 1ns 011ps | 150.2 |
| double latency     | 5ns 345ps |   680 | 8ns 018ps |  1191 |

Maximum time:
| Name               |       Time |   %   |       Time |   %   |
|:-------------------|-----------:|------:|-----------:|------:|
| int64_t throughput |  6ns 367ps | 113.5 |  2ns 186ps |   100 |
| int64_t latency    | 15ns 195ps | 270.9 |  6ns 208ps | 283.9 |
| double throughput  |  5ns 608ps |   100 |  5ns 949ps | 272.1 |
| double latency     | 29ns 079ps | 518.5 | 33ns 711ps |  1542 |

Average time:
| Name               |      Time |   %   |      Time |   %   |
|:-------------------|----------:|------:|----------:|------:|
| int64_t throughput |     843ps |   100 |     867ps |   100 |
| int64_t latency    | 2ns 770ps | 328.5 | 3ns 746ps |   432 |
| double throughput  | 1ns 036ps | 122.8 | 1ns 070ps | 123.4 |
| double latency     | 5ns 444ps | 645.7 | 8ns 267ps | 953.5 |

Benchmark finished in 41s 727ms
```
