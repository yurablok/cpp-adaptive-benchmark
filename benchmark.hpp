// C++ Adaptive Benchmark
//
// C++11 benchmark that can automatically choose the number of repetitions to
// perform for the desired time.
//
// Author: Yurii Blok
// License: BSL-1.0
// https://github.com/yurablok/cpp-adaptive-benchmark
// History:
// v0.4   2025-12-18    Added previous value into callbacks to simplify some test cases.
// v0.3   2023-02-04    Added picosecond accuracy and min, max, avg statistics.
// v0.2   2023-01-19    Added nanosecond accuracy on Windows.
// v0.1   2022-04-21    First release.

#pragma once
#include <cassert>
#include <chrono>
#include <vector>
#include <string>
#include <functional>
#include <iomanip>
#include <iostream>



class Benchmark {
public:
    // number: 1..10
    void setColumnsNumber(const uint8_t number);

    // column: 0..number-1
    void add(std::string name, const uint8_t column,
        std::function<uint32_t(uint32_t random, uint32_t previous)> testee);

    void run(const uint32_t timePerTestee_s = 5, const uint32_t minimumRepetitions = 500);

    static int64_t getSteadyTickStd_ns() noexcept;
    static int64_t getSteadyTick_ns() noexcept;

    // Input: 0..106 days in picoseconds
    // Output: 3..11 symbols
    //   d h m s ms us ns ps
    static std::string makeDurationString(const int64_t duration_ps);

    Benchmark();

    class lcg32 {
    public:
        lcg32() = default;
        lcg32(const uint32_t seed_) noexcept {
            seed(seed_);
        }
        void seed(const uint32_t seed_) noexcept {
            //m_x = seed_ == 0 ? 1 : seed_;
            m_x = seed_ | (1 << 31);
        }
        uint32_t operator()() noexcept {
            // 68.23 % this  vs  63.21 % minstd
            m_x = (UINT64_C(1260864976) * m_x + UINT64_C(1379216869)) % UINT32_MAX;
            return m_x;
        }
    private:
        uint32_t m_x = 1;
    };

private:
    static std::string toString(const uint64_t value, const uint8_t width);

    struct TesteeMeta {
        std::function<uint32_t(uint32_t random, uint32_t previous)> function;
        int64_t minimum_ps = 0;
        int64_t average_ps = 0;
        int64_t maximum_ps = 0;
    };
    std::vector<std::pair<std::string, std::vector<TesteeMeta>>> m_testees;
    struct ColumnMeta {
        int64_t minTime_ps = INT64_MAX;
        int64_t maxTime_ps = INT64_MAX;
        int64_t avgTime_ps = INT64_MAX;
        uint32_t minTimeStrLength = sizeof("Time") - 1;
        uint32_t maxTimeStrLength = sizeof("Time") - 1;
        uint32_t avgTimeStrLength = sizeof("Time") - 1;
    };
    std::vector<ColumnMeta> m_columns;
    uint32_t m_maxNameLength = sizeof("Name") - 1;

# ifdef _WIN32
#  ifdef _M_ARM64
    static uint64_t s_Hz;
#  else
    static std::array<uint64_t, sizeof(uintptr_t) * 8> s_Hz; // MAXIMUM_PROC_PER_GROUP: 32 | 64
#  endif // _M_ARM64
# endif // _WIN32
};



#ifdef _WIN32
namespace winapi {
extern "C" {
# ifdef _M_ARM64
    extern int64_t _ReadStatusReg(int32_t code);
# else
    extern uint64_t __stdcall __rdtscp(uint32_t* processorIdx);
#  ifndef WINADVAPI
    extern int32_t __stdcall RegOpenKeyExA(uint32_t* key, const char* subkey,
        uint32_t options, uint32_t rights, uint32_t** result);
    extern int32_t __stdcall RegQueryValueExA(uint32_t* key, const char* name,
        uint32_t* reserved, uint32_t* type, uint8_t* data, uint32_t* size);
    extern int32_t __stdcall RegCloseKey(uint32_t* key);
#  endif // WINADVAPI
# endif // _M_ARM64
} // extern "C"
} // namespace winapi
#endif // _WIN32

void Benchmark::setColumnsNumber(const uint8_t number) {
    assert(number >= 1);
    assert(number <= 10);
    m_columns.resize(number);
}

void Benchmark::add(std::string name, const uint8_t column,
        std::function<uint32_t(uint32_t random, uint32_t previous)> testee) {
    assert(!name.empty());
    assert(column < m_columns.size());
    assert(testee);
    m_maxNameLength = std::max(uint32_t(name.size()), m_maxNameLength);

    std::vector<TesteeMeta>* vec = nullptr;
    for (auto& it : m_testees) {
        if (it.first == name) {
            vec = &it.second;
            break;
        }
    }
    if (vec == nullptr) {
        m_testees.emplace_back();
        m_testees.back().first = std::move(name);
        vec = &m_testees.back().second;
    }
    vec->resize(m_columns.size());
    auto& meta = vec->at(column);
    meta.function  = std::move(testee);
}

void Benchmark::run(const uint32_t timePerTestee_s, const uint32_t minimumRepetitions) {
    assert(timePerTestee_s > 0);
    assert(minimumRepetitions >= 10);
    const int64_t benchmarkBegin_ns = getSteadyTickStd_ns();
    std::cout << "Benchmark is running for "
        << m_testees.size() * m_columns.size() << " subjects:\n";
    lcg32 rng;
    rng.seed(uint32_t(benchmarkBegin_ns));
    const int64_t timePerTestee_ns = int64_t(timePerTestee_s) * 1000000000;
    const int64_t totalTime_ns = timePerTestee_ns * m_testees.size();

    int64_t testeeIdx = 0;
    uint32_t doNotOptimize = 0;
    for (auto& itVec : m_testees) {
        uint8_t columnIdx = 0;
        for (auto& testee : itVec.second) {
            const int64_t benchmarkTesteeBegin_ns = getSteadyTickStd_ns();
            std::cout << " [" << testeeIdx++ << "] " << itVec.first << "... ";
            if (!testee.function) {
                std::cout << "Noop." << std::endl;
                continue;
            }
            std::cout.flush();

            testee.minimum_ps = INT64_MAX;
            testee.maximum_ps = 0;
            testee.average_ps = 0;
            int64_t sum_ns = 0;
            // Rough measurement
            for (uint32_t i = 0; i < minimumRepetitions; ++i) {
                const uint32_t random = rng();
                const int64_t begin_ns = getSteadyTick_ns();

                doNotOptimize += testee.function(random, doNotOptimize);

                const int64_t end_ns = getSteadyTick_ns();
                const int64_t diff_ns = end_ns - begin_ns;
                if (diff_ns <= 1) {
                    continue;
                }
                sum_ns += diff_ns;
                testee.minimum_ps = std::min(testee.minimum_ps, diff_ns * 1000);
                testee.maximum_ps = std::max(testee.maximum_ps, diff_ns * 1000);
            }
            testee.average_ps = (sum_ns / minimumRepetitions) * 1000;
#         ifdef DEBUG_ADAPTIVE_BENCHMARK
            std::cout
                << "\n min=" << makeDurationString(testee.minimum_ps)
                << " max=" << makeDurationString(testee.maximum_ps)
                << " avg=" << makeDurationString(testee.average_ps);
#         endif

            constexpr int64_t minDesiredTime_ps = INT64_C(5000000000); // 5 ms
            constexpr int64_t minClarifyingTime_ps = INT64_C(500000000000); // 500 ms
            uint32_t n = 0;
            if (testee.average_ps < minDesiredTime_ps) {
                n = uint32_t(minDesiredTime_ps / testee.average_ps);
                constexpr uint32_t reps = minClarifyingTime_ps / minDesiredTime_ps;
                testee.minimum_ps = INT64_MAX;
                testee.maximum_ps = 0;
                testee.average_ps = 0;
                sum_ns = 0;
                const int64_t clarifyingBegin_ps = getSteadyTick_ns() * 1000;
                // Clarifying measurement
                for (uint32_t i = 0; i < reps; ++i) {
                    const uint32_t random = rng();
                    const int64_t begin_ns = getSteadyTick_ns();

                    for (uint32_t j = 0; j < n; ++j) {
                        doNotOptimize += testee.function(random, doNotOptimize);
                    }

                    const int64_t end_ns = getSteadyTick_ns();
                    const int64_t diff_ns = end_ns - begin_ns;
                    if (diff_ns <= 1) {
                        continue;
                    }
                    sum_ns += diff_ns;
                    testee.minimum_ps = std::min(testee.minimum_ps, (diff_ns * 1000) / n);
                    testee.maximum_ps = std::max(testee.maximum_ps, (diff_ns * 1000) / n);
                }
                const int64_t clarifyingEnd_ps = getSteadyTick_ns() * 1000;
                testee.average_ps = (sum_ns * 1000) / reps;
                testee.average_ps /= n;
#             ifdef DEBUG_ADAPTIVE_BENCHMARK
                std::cout << "\n clarifying="
                    << makeDurationString(clarifyingEnd_ps - clarifyingBegin_ps);
#             endif

                n = uint32_t(minDesiredTime_ps / testee.average_ps);
                testee.minimum_ps = INT64_MAX;
                testee.maximum_ps = 0;
                testee.average_ps = 0;
                sum_ns = 0;
                const int64_t clarifying2Begin_ps = getSteadyTick_ns() * 1000;
                // Clarifying measurement
                for (uint32_t i = 0; i < reps; ++i) {
                    const uint32_t random = rng();
                    const int64_t begin_ns = getSteadyTick_ns();

                    for (uint32_t j = 0; j < n; ++j) {
                        doNotOptimize += testee.function(random, doNotOptimize);
                    }

                    const int64_t end_ns = getSteadyTick_ns();
                    const int64_t diff_ns = end_ns - begin_ns;
                    if (diff_ns <= 1) {
                        continue;
                    }
                    sum_ns += diff_ns;
                    testee.minimum_ps = std::min(testee.minimum_ps, (diff_ns * 1000) / n);
                    testee.maximum_ps = std::max(testee.maximum_ps, (diff_ns * 1000) / n);
                }
                const int64_t clarifying2End_ps = getSteadyTick_ns() * 1000;
                testee.average_ps = (sum_ns * 1000) / reps;
                testee.average_ps /= n;
#             ifdef DEBUG_ADAPTIVE_BENCHMARK
                std::cout << "\n clarifying="
                    << makeDurationString(clarifying2End_ps - clarifying2Begin_ps);
#             endif
            }
#         ifdef DEBUG_ADAPTIVE_BENCHMARK
            std::cout
                << "\n n=" << n
                << " min=" << makeDurationString(testee.minimum_ps)
                << " max=" << makeDurationString(testee.maximum_ps)
                << " avg=" << makeDurationString(testee.average_ps);
#         endif

            const int64_t lastTick_ns = benchmarkTesteeBegin_ns + timePerTestee_ns;
            const int64_t remainingTime_ns = lastTick_ns - getSteadyTickStd_ns();
            uint64_t repetitions = 0;
            if (remainingTime_ns > 0) {
                repetitions = (remainingTime_ns * 1000) / testee.average_ps;
                n = uint32_t(minDesiredTime_ps / testee.average_ps);
                if (n > 0) {
                    repetitions /= n;
                    if (repetitions > 0) {
                        sum_ns = 0;
                    }
                }
            }

            // Main measurement
            if (n == 0) {
                for (uint64_t i = 0; i < repetitions; ++i) {
                    const uint32_t random = rng();
                    const int64_t begin_ns = getSteadyTick_ns();

                    doNotOptimize += testee.function(random, doNotOptimize);

                    const int64_t end_ns = getSteadyTick_ns();
                    const int64_t diff_ns = end_ns - begin_ns;
                    if (diff_ns <= 1) {
                        continue;
                    }
                    sum_ns += diff_ns;
                    testee.minimum_ps = std::min(testee.minimum_ps, diff_ns * 1000);
                    testee.maximum_ps = std::max(testee.maximum_ps, diff_ns * 1000);
                }
                testee.average_ps = sum_ns / (minimumRepetitions + repetitions) * 1000;
            }
            else if (repetitions > 0) {
                for (uint64_t i = 0; i < repetitions; ++i) {
                    const uint32_t random = rng();
                    const int64_t begin_ns = getSteadyTick_ns();

                    for (uint32_t j = 0; j < n; ++j) {
                        doNotOptimize += testee.function(random, doNotOptimize);
                    }

                    const int64_t end_ns = getSteadyTick_ns();
                    const int64_t diff_ns = end_ns - begin_ns;
                    if (diff_ns <= 1) {
                        continue;
                    }
                    sum_ns += diff_ns;
                    testee.minimum_ps = std::min(testee.minimum_ps, (diff_ns * 1000) / n);
                    testee.maximum_ps = std::max(testee.maximum_ps, (diff_ns * 1000) / n);
                }
                testee.average_ps = (sum_ns * 1000) / repetitions;
                testee.average_ps /= n;
            }
#         ifdef DEBUG_ADAPTIVE_BENCHMARK
            std::cout
                << "\n n=" << n << " r=" << repetitions
                << " min=" << makeDurationString(testee.minimum_ps)
                << " max=" << makeDurationString(testee.maximum_ps)
                << " avg=" << makeDurationString(testee.average_ps) << "\n";
#         endif

            std::cout << "Done in " << makeDurationString(
                    (getSteadyTickStd_ns() - benchmarkTesteeBegin_ns) * 1009)
                << (doNotOptimize ? " " : "  ") << std::endl;

            auto& column = m_columns[columnIdx++];
            column.minTime_ps = std::min(testee.minimum_ps, column.minTime_ps);
            column.minTimeStrLength = std::max(column.minTimeStrLength,
                uint32_t(makeDurationString(testee.minimum_ps).size()));
            column.maxTime_ps = std::min(testee.maximum_ps, column.maxTime_ps);
            column.maxTimeStrLength = std::max(column.maxTimeStrLength,
                uint32_t(makeDurationString(testee.maximum_ps).size()));
            column.avgTime_ps = std::min(testee.average_ps, column.avgTime_ps);
            column.avgTimeStrLength = std::max(column.avgTimeStrLength,
                uint32_t(makeDurationString(testee.average_ps).size()));
        }
    }
    
    // | Name | Time | % | Time | % |
    // |:-----|-----:|--:|-----:|--:|
    // | name | 123s |4.5| 678s |9.0|
    const auto print = [&](const uint8_t mode) { // 0 - min, 1 - max, 2 - avg
        std::cout << "| " << std::setw(m_maxNameLength) << std::setfill(' ') << std::left
            << "Name" << " |";
        for (size_t columnIdx = 0; columnIdx < m_columns.size(); ++columnIdx) {
            const auto& column = m_columns[columnIdx];
            uint32_t timeStrLength = 0;
            switch (mode) {
            case 0: timeStrLength = column.minTimeStrLength; break;
            case 1: timeStrLength = column.maxTimeStrLength; break;
            case 2: timeStrLength = column.avgTimeStrLength; break;
            } //                                                                     100.0
            std::cout << std::setw(timeStrLength + 1) << std::right << "Time" << " |   %   |";
        }
        std::cout << "\n|:" << std::setw(m_maxNameLength + 1) << std::setfill('-') << "-"
            << "|";
        for (size_t columnIdx = 0; columnIdx < m_columns.size(); ++columnIdx) {
            const auto& column = m_columns[columnIdx];
            uint32_t timeStrLength = 0;
            switch (mode) {
            case 0: timeStrLength = column.minTimeStrLength; break;
            case 1: timeStrLength = column.maxTimeStrLength; break;
            case 2: timeStrLength = column.avgTimeStrLength; break;
            } //                                                                  100.0
            std::cout << std::setw(timeStrLength + 1) << std::right << "-" << ":|------:|";
        }
        std::cout << "\n";
        for (const auto& itVec : m_testees) {
            std::cout << "| " << std::setw(m_maxNameLength) << std::setfill(' ')
                << std::left << itVec.first << " |";
            for (size_t columnIdx = 0; columnIdx < itVec.second.size(); ++columnIdx) {
                const auto& testee = itVec.second[columnIdx];
                const auto& column = m_columns[columnIdx];
                int64_t testeeTime_ps = 0;
                int64_t time_ps = 0;
                uint32_t timeStrLength = 0;
                switch (mode) {
                case 0:
                    testeeTime_ps = testee.minimum_ps;
                    time_ps = column.minTime_ps;
                    timeStrLength = column.minTimeStrLength;
                    break;
                case 1:
                    testeeTime_ps = testee.maximum_ps;
                    time_ps = column.maxTime_ps;
                    timeStrLength = column.maxTimeStrLength;
                    break;
                case 2:
                    testeeTime_ps = testee.average_ps;
                    time_ps = column.avgTime_ps;
                    timeStrLength = column.avgTimeStrLength;
                    break;
                }
                float perc = 0.1f * float((testeeTime_ps * 1000) / std::max(time_ps, INT64_C(1)));
                if (perc >= 1000.0f) {
                    perc = float(uint32_t(perc));
                }
                std::cout << std::setw(timeStrLength + 1) << std::right
                    << makeDurationString(testeeTime_ps)
                    << " | " << std::setw(5) << perc << " |";
            }
            std::cout << "\n";
        }
    };
    std::cout << "\nMinimum time:\n";
    print(0);
    std::cout << "\nMaximum time:\n";
    print(1);
    std::cout << "\nAverage time:\n";
    print(2);
    std::cout << "\nBenchmark finished in " << makeDurationString(
        (getSteadyTickStd_ns() - benchmarkBegin_ns) * 1000) << std::endl;
}

int64_t Benchmark::getSteadyTickStd_ns() noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}
int64_t Benchmark::getSteadyTick_ns() noexcept {
#ifdef _WIN32
# ifdef _M_ARM64
#  ifndef ARM64_SYSREG
#   define ARM64_SYSREG(op0, op1, crn, crm, op2) \
        ( ((op0 & 1) << 14) | \
          ((op1 & 7) << 11) | \
          ((crn & 15) << 7) | \
          ((crm & 15) << 3) | \
          ((op2 & 7) << 0) )
#  endif
#  ifndef ARM64_CNTVCT_EL0
    // Counter-timer Virtual Count register
    // https://developer.arm.com/documentation/ddi0601/2022-12/AArch64-Registers/CNTVCT-EL0--Counter-timer-Virtual-Count-register?lang=en
    constexpr int32_t ARM64_CNTVCT_EL0 = ARM64_SYSREG(3, 3, 14, 0, 2);
#  endif
    const uint64_t tsc = static_cast<uint64_t>(winapi::_ReadStatusReg(ARM64_CNTVCT_EL0));
    const uint64_t Hz = s_Hz;
# else
    uint32_t processorIdx = 0;
    const uint64_t tsc = winapi::__rdtscp(&processorIdx);
    const uint64_t Hz = s_Hz[processorIdx];
# endif
    //NOTE: glibc:
    // This computation should be stable until
    // we get machines with about 16GHz frequency.
    const uint64_t s = (tsc / Hz) * UINT64_C(1000000000);
    const uint64_t ns = ((tsc % Hz) * UINT64_C(1000000000)) / Hz;
    return s + ns;
#else
    return getSteadyTickStd_ns();
#endif // _WIN32
}

// Input: 0..106 days in picoseconds
// Output: 3..11 symbols
//   d h m s ms us ns ps
std::string Benchmark::makeDurationString(const int64_t duration_ps) {
    std::string result;
    const auto duration = std::chrono::nanoseconds(duration_ps / 1000);
    // ___ps
    if (duration_ps <= 999) {
        result += std::to_string(duration_ps);
        result += "ps";
    }
    // ___ns
    else if (duration <= std::chrono::nanoseconds(999)) {
        const auto div = std::div(duration_ps, int64_t(1000));
        const int64_t ns = div.quot;
        const int64_t ps = div.rem;
        result += std::to_string(ns);
        result += "ns ";
        result += toString(ps, 3);
        result += "ps";
    }
    // ___us ___ns
    else if (duration <= std::chrono::microseconds(999)) {
        const auto div = std::div(duration.count(), int64_t(1000));
        const int64_t us = div.quot;
        const int64_t ns = div.rem;
        result += std::to_string(us);
        result += "us ";
        result += toString(ns, 3);
        result += "ns";
    }
    // ___ms ___us
    else if (duration <= std::chrono::milliseconds(999)) {
        const auto div = std::div(
            std::chrono::duration_cast<std::chrono::microseconds>(duration).count(),
            int64_t(1000)
        );
        const int64_t ms = div.quot;
        const int64_t us = div.rem;
        result += std::to_string(ms);
        result += "ms ";
        result += toString(us, 3);
        result += "us";
    }
    // __s ___ms
    else if (duration <= std::chrono::seconds(59)) {
        const auto div = std::div(
            std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(),
            int64_t(1000)
        );
        const int64_t s = div.quot;
        const int64_t ms = div.rem;
        result += std::to_string(s);
        result += "s ";
        result += toString(ms, 3);
        result += "ms";
    }
    // __m __s
    else if (duration <= std::chrono::minutes(59)) {
        const auto div = std::div(
            std::chrono::duration_cast<std::chrono::seconds>(duration).count(),
            int64_t(60)
        );
        const int64_t m = div.quot;
        const int64_t s = div.rem;
        result += std::to_string(m);
        result += "m ";
        result += toString(s, 2);
        result += "s";
    }
    // __h __m
    else if (duration <= std::chrono::hours(23)) {
        const auto div = std::div(
            int32_t(std::chrono::duration_cast<std::chrono::minutes>(duration).count()),
            int32_t(60)
        );
        const int32_t h = div.quot;
        const int32_t m = div.rem;
        result += std::to_string(h);
        result += "h ";
        result += toString(m, 2);
        result += "m";
    }
    // ____d __h
    else {
        const auto div = std::div(
            int32_t(std::chrono::duration_cast<std::chrono::hours>(duration).count()),
            int32_t(24)
        );
        const int32_t d = div.quot;
        const int32_t h = div.rem;
        result += std::to_string(d);
        result += "d ";
        result += toString(h, 2);
        result += "h";
    }
    return result;
}

Benchmark::Benchmark() {
#ifdef _WIN32
# ifdef _M_ARM64
#  ifndef ARM64_CNTFRQ_EL0
    // Counter-timer Frequency register
    // https://developer.arm.com/documentation/ddi0601/2022-12/AArch64-Registers/CNTFRQ-EL0--Counter-timer-Frequency-register?lang=en
    constexpr int32_t ARM64_CNTFRQ_EL0 = ARM64_SYSREG(3, 3, 14, 0, 0);
#  endif
    s_Hz = static_cast<uint64_t>(winapi::_ReadStatusReg(ARM64_CNTFRQ_EL0));
# else
    std::fill(s_Hz.begin(), s_Hz.end(), 1);
    const std::string path = "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\";
    for (size_t idx = 0; idx < s_Hz.size(); ++idx) {
        std::string name = path;
        name += std::to_string(idx);
#     ifndef HKEY_LOCAL_MACHINE
        constexpr uint32_t HKEY_LOCAL_MACHINE = 0x80000002;
#     endif
#     ifndef KEY_READ
        constexpr uint32_t KEY_READ = 131097;
#     endif
#     ifndef WINADVAPI
        uint32_t* key = nullptr;
        if (winapi::RegOpenKeyExA(reinterpret_cast<uint32_t*>(HKEY_LOCAL_MACHINE),
                name.c_str(), 0, KEY_READ, &key) != 0) {
            break;
        }
        uint32_t MHz = 0;
        uint32_t size = sizeof(MHz);
        if (winapi::RegQueryValueExA(key, "~MHz", nullptr, nullptr,
                reinterpret_cast<uint8_t*>(&MHz), &size) != 0) {
            winapi::RegCloseKey(key);
            break;
        }
        s_Hz[idx] = uint64_t(MHz) * 1000000;
        winapi::RegCloseKey(key);
#     else
        HKEY key = nullptr;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                name.c_str(), 0, KEY_READ, &key) != 0) {
            break;
        }
        DWORD MHz = 0;
        DWORD size = sizeof(MHz);
        if (RegQueryValueExA(key, "~MHz", nullptr, nullptr,
                reinterpret_cast<LPBYTE>(&MHz), &size) != 0) {
            RegCloseKey(key);
            break;
        }
        s_Hz[idx] = uint64_t(MHz) * 1000000;
        RegCloseKey(key);
#     endif // WINADVAPI
    }
# endif
#endif // _WIN32
}

std::string Benchmark::toString(const uint64_t value, const uint8_t width) {
    std::string result = std::to_string(value);
    if (result.size() < width) {
        result = std::string(width - result.size(), '0') + result;
    }
    return result;
}

#ifdef _WIN32
# ifdef _M_ARM64
uint64_t Benchmark::s_Hz = 0;
# else
decltype(Benchmark::s_Hz) Benchmark::s_Hz;
# endif // _M_ARM64
#endif // _WIN32
