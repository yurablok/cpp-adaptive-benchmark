# C++ Adaptive Benchmark

C++11 benchmark that can automatically choose the number of repetitions to
perform for the desired time.

### Basic example:

```cpp
volatile int64_t ci;
volatile double cf;

Benchmark bench;
bench.setColumnsNumber(2);
bench.add("int64_t", 0, [&](uint32_t random) {
    int64_t a = random;
    int64_t b = random << 1;
    ci = a * b;
});
bench.add("int64_t", 1, [&](uint32_t random) {
    int64_t a = random;
    int64_t b = random << 1;
    ci = a / b;
});
bench.add("double", 0, [&](uint32_t random) {
    double a = random;
    double b = random << 1;
    cf = a * b;
});
bench.add("double", 1, [&](uint32_t random) {
    double a = random;
    double b = random << 1;
    cf = a / b;
});
bench.run(5000); // 5s
```

```cpp
Benchmark is running for 4 subjects:
 [0] int64_t... Done.
 [1] int64_t... Done.
 [2] double... Done.
 [3] double... Done.
| Name    | Time |   %   | Time |   %   |
|:--------|-----:|------:|-----:|------:|
| int64_t | 22ns |   110 | 31ns | 119.2 |
| double  | 20ns |   100 | 26ns |   100 |
Benchmark finished in 20s 304ms
```
