# C++ Adaptive Benchmark

C++11 benchmark that can automatically choose the number of repetitions to
perform for the desired time.

### Basic example:

```cpp
Benchmark benchmark;
benchmark.setColumnsNumber(2);
benchmark.add("int64_t", 0, [&](uint32_t random) -> uint32_t {
    int64_t a = random;
    int64_t b = random;
    return a * b;
});
benchmark.add("int64_t", 1, [&](uint32_t random) -> uint32_t {
    int64_t a = random;
    int64_t b = random;
    return a / b;
});
benchmark.add("double", 0, [&](uint32_t random) -> uint32_t {
    double a = random;
    double b = random;
    return a * b;
});
benchmark.add("double", 1, [&](uint32_t random) -> uint32_t {
    double a = random;
    double b = random;
    return a / b;
});
benchmark.run(5); // 5s
```

```cpp
Benchmark is running for 4 subjects:
 [0] int64_t... Done in 5s 066ms
 [1] int64_t... Done in 5s 065ms
 [2] double... Done in 5s 047ms
 [3] double... Done in 5s 039ms

Minimum time:
| Name    |      Time |   %   |      Time |   %   |
|:--------|----------:|------:|----------:|------:|
| int64_t |     835ps |   100 |     835ps |   100 |
| double  | 1ns 003ps | 120.1 | 1ns 011ps |   121 |

Maximum time:
| Name    |      Time |   %   |      Time |   %   |
|:--------|----------:|------:|----------:|------:|
| int64_t | 1ns 263ps |   100 | 1ns 137ps |   100 |
| double  | 1ns 287ps | 101.9 | 1ns 624ps | 142.8 |

Average time:
| Name    |      Time |   %   |      Time |   %   |
|:--------|----------:|------:|----------:|------:|
| int64_t |     843ps |   100 |     846ps |   100 |
| double  | 1ns 017ps | 120.6 | 1ns 031ps | 121.8 |

Benchmark finished in 20s 047ms
```
