#include <iostream>
#include <chrono>
#include <vector>

int main() {
    constexpr size_t iterations = 100000; // 반복 횟수
    constexpr size_t allocation_size = 500; // 메모리 요청 크기

    using clock = std::chrono::steady_clock;

    // new 연산 시간 측정
    std::vector<double> new_times;
    for (size_t i = 0; i < iterations; ++i) {
        auto start = clock::now();
        char* ptr = new char[allocation_size];
        auto end = clock::now();
        delete[] ptr;

        new_times.push_back(std::chrono::duration<double, std::nano>(end - start).count());
    }

    // delete 연산 시간 측정
    std::vector<double> delete_times;
    for (size_t i = 0; i < iterations; ++i) {
        char* ptr = new char[allocation_size];
        auto start = clock::now();
        delete[] ptr;
        auto end = clock::now();

        delete_times.push_back(std::chrono::duration<double, std::nano>(end - start).count());
    }

    // 평균 계산
    auto average = [](const std::vector<double>& times) {
        double sum = 0;
        for (auto time : times) {
            sum += time;
        }
        return sum / times.size();
        };

    std::cout << "Average new time: " << average(new_times) << " ns\n";
    std::cout << "Average delete time: " << average(delete_times) << " ns\n";

    return 0;
}
