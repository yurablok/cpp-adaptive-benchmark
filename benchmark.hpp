// C++ Adaptive Benchmark
//
// C++11 benchmark that can automatically choose the number of repetitions to
// perform for the desired time.
//
// Author: Yurii Blok
// License: BSL-1.0
// https://github.com/yurablok/cpp-adaptive-benchmark
// History:
// v0.1 2022-Apr-21     First release.

#pragma once
#include <cassert>
#include <chrono>
#include <vector>
#include <string>
#include <random>
#include <functional>
#include <iomanip>
#include <iostream>


class Benchmark {
public:
    // number: 1..10
    void setColumnsNumber(const uint8_t number) {
        assert(number >= 1);
        assert(number <= 10);
        m_columns.resize(number);
    }
    void add(std::string name, const uint8_t column, std::function<void(uint32_t random)> testee) {
        assert(!name.empty());
        assert(column < m_columns.size());
        assert(testee);
        m_maxNameLength = std::max(static_cast<uint32_t>(name.size()), m_maxNameLength);

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
    void run(const uint64_t timePerTestee_ms = 5000, const uint64_t minimumRepetitions = 10000) {
        const int64_t benchmarkBegin_ns = getSteadyTick_ns();
        std::cout << "Benchmark is running for "
            << m_testees.size() * m_columns.size() << " subjects:\n";
        std::minstd_rand rng(benchmarkBegin_ns);
        const int64_t timePerTestee_ns = timePerTestee_ms * 1000000;
        const int64_t totalTime_ns = timePerTestee_ns * m_testees.size();

        int64_t testeeIdx = 0;
        for (auto& itVec : m_testees) {
            uint8_t columnIdx = 0;
            for (auto& testee : itVec.second) {
                std::cout << " [" << testeeIdx++ << "] " << itVec.first << "... ";
                if (!testee.function) {
                    std::cout << "Noop." << std::endl;
                    continue;
                }
                std::cout.flush();

                testee.time_ns = INT64_MAX;
                int64_t timeAverage_ns = 0;
                for (uint64_t i = 0; i < minimumRepetitions; ++i) {
                    const uint32_t random = rng();
                    const int64_t begin_ns = getSteadyTick_ns();
                    testee.function(random);
                    const int64_t end_ns = getSteadyTick_ns();
                    timeAverage_ns += end_ns - begin_ns;
                    testee.time_ns = std::min(end_ns - begin_ns, testee.time_ns);
                }
                timeAverage_ns /= minimumRepetitions;
                timeAverage_ns = std::max(timeAverage_ns, static_cast<int64_t>(100));

                const int64_t lastTick_ns = benchmarkBegin_ns + (testeeIdx + 1) * timePerTestee_ns;
                const int64_t remainingTime_ns = lastTick_ns - getSteadyTick_ns();
                uint64_t exceededNTimes = 0;
                if (remainingTime_ns < 0) {
                    exceededNTimes = (timePerTestee_ns - remainingTime_ns) / timePerTestee_ns + 1;
                }

                const uint64_t repetitions = remainingTime_ns / timeAverage_ns;
                for (uint64_t i = 0; i < repetitions; ++i) {
                    const uint32_t random = rng();
                    const int64_t begin_ns = getSteadyTick_ns();
                    testee.function(random);
                    const int64_t end_ns = getSteadyTick_ns();
                    testee.time_ns = std::min(end_ns - begin_ns, testee.time_ns);
                }

                if (exceededNTimes > 0) {
                    std::cout << "Warning: the test took ~" << exceededNTimes
                        << " times longer than expected." << std::endl;
                }
                else {
                    std::cout << "Done." << std::endl;
                }
                auto& column = m_columns[columnIdx++];
                column.minTime_ns = std::min(testee.time_ns, column.minTime_ns);
                column.maxTimeLength = std::max(column.maxTimeLength,
                    static_cast<uint32_t>(makeDurationString(testee.time_ns).size()));
            }
        }
        // | Name | Time | % | Time | % |
        // |:-----|-----:|--:|-----:|--:|
        // | name | 123s |4.5| 678s |9.0|
        std::cout << "| " << std::setw(m_maxNameLength) << std::setfill(' ') << std::left
            << "Name" << " |";
        for (size_t columnIdx = 0; columnIdx < m_columns.size(); ++columnIdx) {
            const auto& column = m_columns[columnIdx]; //                                   100.0
            std::cout << std::setw(column.maxTimeLength + 1) << std::right << "Time" << " |   %   |";
        }
        std::cout << "\n|:" << std::setw(m_maxNameLength + 1) << std::setfill('-') << "-"
            << "|";
        for (size_t columnIdx = 0; columnIdx < m_columns.size(); ++columnIdx) {
            const auto& column = m_columns[columnIdx]; //                                100.0
            std::cout << std::setw(column.maxTimeLength + 1) << std::right << "-" << ":|------:|";
        }
        std::cout << "\n";
        for (const auto& itVec : m_testees) {
            std::cout << "| " << std::setw(m_maxNameLength) << std::setfill(' ')
                << std::left << itVec.first << " |";
            for (size_t columnIdx = 0; columnIdx < itVec.second.size(); ++columnIdx) {
                const auto& testee = itVec.second[columnIdx];
                const auto& column = m_columns[columnIdx];
                const float perc = 0.1f *
                    static_cast<float>((testee.time_ns * 1000) / column.minTime_ns);
                std::cout << std::setw(column.maxTimeLength + 1) << std::right
                    << makeDurationString(testee.time_ns) << " | " << std::setw(5) << perc << " |";
            }
            std::cout << "\n";
        }
        std::cout << "Benchmark finished in " << makeDurationString(
            getSteadyTick_ns() - benchmarkBegin_ns) << std::endl;
    }

    static int64_t getSteadyTick_ns() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }
    // Output: 3..11 symbols
    static std::string makeDurationString(const int64_t duration_ns) {
        std::string result;
        const auto duration = std::chrono::nanoseconds(duration_ns);
        // ___ns
        if (duration <= std::chrono::nanoseconds(999)) {
            result += std::to_string(duration.count());
            result += "ns";
        }
        // ___us ___ns
        else if (duration <= std::chrono::microseconds(999)) {
            const auto div = std::div(
                duration.count(),
                static_cast<int64_t>(1000)
            );
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
                static_cast<int64_t>(1000)
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
                static_cast<int64_t>(1000)
            );
            const int64_t s = div.quot;
            const int64_t ms = div.rem;
            result += std::to_string(s);
            result += "s ";
            result += toString(ms, 3);
            result += "ms";
        }
        // __m __s
        else if (duration < std::chrono::minutes(59)) {
            const auto div = std::div(
                std::chrono::duration_cast<std::chrono::seconds>(duration).count(),
                static_cast<int64_t>(60)
            );
            const int64_t m = div.quot;
            const int64_t s = div.rem;
            result += std::to_string(m);
            result += "m ";
            result += toString(s, 2);
            result += "s";
        }
        // __h __m
        else if (duration < std::chrono::hours(23)) {
            const auto div = std::div(
                static_cast<int32_t>(std::chrono::duration_cast<std::chrono::minutes>(duration).count()),
                static_cast<int32_t>(60)
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
                static_cast<int32_t>(std::chrono::duration_cast<std::chrono::hours>(duration).count()),
                static_cast<int32_t>(24)
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

private:
    static std::string toString(const uint64_t value, const uint8_t width) {
        int64_t temp = value;
        size_t count = 0;
        while (temp != 0) {
            temp /= 10;
            ++count;
        }
        std::string result;
        if (count < width) {
            result.resize(width - count, '0');
        }
        if (value != 0) {
            result += std::to_string(value);
        }
        return result;
    }

    struct TesteeMeta {
        std::function<void(uint32_t random)> function;
        int64_t time_ns = 0;
    };
    std::vector<std::pair<std::string, std::vector<TesteeMeta>>> m_testees;
    struct ColumnMeta {
        int64_t minTime_ns = INT64_MAX;
        uint32_t maxTimeLength = sizeof("Time") - 1;
    };
    std::vector<ColumnMeta> m_columns;
    uint32_t m_maxNameLength = sizeof("Name") - 1;
};
