# C++ Adaptive Benchmark

C++11 benchmark that can automatically choose the number of repetitions to
perform for the desired time.

To achieve picosecond precision, the timing measurement is performed over multiple
executions of a testee. For this purpose, an internal loop is used with an adaptive
number of repetitions that adjusts to a target execution time.

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
 [0] int64_t throughput... Done in 5s 000ms
 [1] int64_t throughput... Done in 5s 000ms
 [2] int64_t latency... Done in 5s 000ms
 [3] int64_t latency... Done in 5s 000ms
 [4] double throughput... Done in 5s 000ms
 [5] double throughput... Done in 5s 000ms
 [6] double latency... Done in 5s 000ms
 [7] double latency... Done in 5s 000ms

Minimum time:
| Name               |      Time |   %   |      Time |   %   |
|:-------------------|----------:|------:|----------:|------:|
| int64_t throughput | 1ns 001ps | 112.5 |     834ps |   100 |
| int64_t latency    | 2ns 670ps | 300.3 | 3ns 670ps |   440 |
| double throughput  |     889ps |   100 | 1ns 473ps | 176.6 |
| double latency     | 5ns 337ps | 600.3 | 8ns 004ps | 959.7 |

Average time:
| Name               |      Time |   %   |      Time |   %   |
|:-------------------|----------:|------:|----------:|------:|
| int64_t throughput | 1ns 009ps | 108.8 |     840ps |   100 |
| int64_t latency    | 2ns 700ps | 291.2 | 3ns 773ps | 449.1 |
| double throughput  |     927ps |   100 | 1ns 522ps | 181.1 |
| double latency     | 5ns 370ps | 579.2 | 8ns 060ps | 959.5 |

Maximum time:
| Name               |       Time |   %   |       Time |   %   |
|:-------------------|-----------:|------:|-----------:|------:|
| int64_t throughput |  2ns 019ps |   100 |  2ns 313ps |   100 |
| int64_t latency    | 12ns 884ps | 638.1 | 10ns 371ps | 448.3 |
| double throughput  |  2ns 248ps | 111.3 |  3ns 461ps | 149.6 |
| double latency     |  9ns 746ps | 482.7 | 23ns 183ps |  1002 |

Benchmark finished in 40s 022ms
```
