#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>

using Clock = std::chrono::high_resolution_clock;

struct SpaceTracker {
    size_t currentBytes = 0;
    size_t peakBytes = 0;

    void add(size_t bytes) {
        currentBytes += bytes;
        peakBytes = std::max(peakBytes, currentBytes);
    }

    void remove(size_t bytes) {
        if (bytes > currentBytes) {
            currentBytes = 0;
        } else {
            currentBytes -= bytes;
        }
    }
};

struct SortMetrics {
    std::string name;
    std::string theoreticalTime;
    std::string theoreticalSpace;

    long long comparisons = 0;
    long long movesOrSwaps = 0;
    double timeMs = 0.0;
    size_t peakAuxBytes = 0;
};

void bubbleSort(std::vector<int>& arr, SortMetrics& m) {
    const int n = static_cast<int>(arr.size());
    for (int i = 0; i < n - 1; ++i) {
        bool swapped = false;
        for (int j = 0; j < n - i - 1; ++j) {
            ++m.comparisons;
            if (arr[j] > arr[j + 1]) {
                std::swap(arr[j], arr[j + 1]);
                ++m.movesOrSwaps;
                swapped = true;
            }
        }
        if (!swapped) {
            break;
        }
    }
}

void selectionSort(std::vector<int>& arr, SortMetrics& m) {
    const int n = static_cast<int>(arr.size());
    for (int i = 0; i < n - 1; ++i) {
        int minIdx = i;
        for (int j = i + 1; j < n; ++j) {
            ++m.comparisons;
            if (arr[j] < arr[minIdx]) {
                minIdx = j;
            }
        }
        if (minIdx != i) {
            std::swap(arr[i], arr[minIdx]);
            ++m.movesOrSwaps;
        }
    }
}

void insertionSort(std::vector<int>& arr, SortMetrics& m) {
    const int n = static_cast<int>(arr.size());
    for (int i = 1; i < n; ++i) {
        int key = arr[i];
        int j = i - 1;

        while (j >= 0) {
            ++m.comparisons;
            if (arr[j] > key) {
                arr[j + 1] = arr[j];
                ++m.movesOrSwaps;
                --j;
            } else {
                break;
            }
        }

        arr[j + 1] = key;
        ++m.movesOrSwaps;
    }
}

void mergeRange(std::vector<int>& arr, int left, int mid, int right, SortMetrics& m, SpaceTracker& st) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    std::vector<int> L(n1);
    std::vector<int> R(n2);
    st.add((static_cast<size_t>(n1) + static_cast<size_t>(n2)) * sizeof(int));

    for (int i = 0; i < n1; ++i) {
        L[i] = arr[left + i];
        ++m.movesOrSwaps;
    }
    for (int j = 0; j < n2; ++j) {
        R[j] = arr[mid + 1 + j];
        ++m.movesOrSwaps;
    }

    int i = 0;
    int j = 0;
    int k = left;

    while (i < n1 && j < n2) {
        ++m.comparisons;
        if (L[i] <= R[j]) {
            arr[k++] = L[i++];
        } else {
            arr[k++] = R[j++];
        }
        ++m.movesOrSwaps;
    }

    while (i < n1) {
        arr[k++] = L[i++];
        ++m.movesOrSwaps;
    }

    while (j < n2) {
        arr[k++] = R[j++];
        ++m.movesOrSwaps;
    }

    st.remove((static_cast<size_t>(n1) + static_cast<size_t>(n2)) * sizeof(int));
}

void mergeSortRec(std::vector<int>& arr, int left, int right, SortMetrics& m, SpaceTracker& st, int depth, int& maxDepth) {
    maxDepth = std::max(maxDepth, depth);
    if (left >= right) {
        return;
    }
    int mid = left + (right - left) / 2;
    mergeSortRec(arr, left, mid, m, st, depth + 1, maxDepth);
    mergeSortRec(arr, mid + 1, right, m, st, depth + 1, maxDepth);
    mergeRange(arr, left, mid, right, m, st);
}

void mergeSort(std::vector<int>& arr, SortMetrics& m) {
    if (arr.empty()) {
        return;
    }

    SpaceTracker st;
    int maxDepth = 0;
    mergeSortRec(arr, 0, static_cast<int>(arr.size()) - 1, m, st, 1, maxDepth);

    // 粗估遞迴堆疊空間（每層約 64 bytes）
    size_t recursiveStackBytes = static_cast<size_t>(maxDepth) * 64;
    m.peakAuxBytes = std::max(st.peakBytes, recursiveStackBytes);
}

int partitionLomuto(std::vector<int>& arr, int low, int high, SortMetrics& m) {
    int pivot = arr[high];
    int i = low - 1;

    for (int j = low; j < high; ++j) {
        ++m.comparisons;
        if (arr[j] <= pivot) {
            ++i;
            std::swap(arr[i], arr[j]);
            ++m.movesOrSwaps;
        }
    }
    std::swap(arr[i + 1], arr[high]);
    ++m.movesOrSwaps;
    return i + 1;
}

void quickSortRec(std::vector<int>& arr, int low, int high, SortMetrics& m, int depth, int& maxDepth) {
    if (low >= high) {
        return;
    }

    maxDepth = std::max(maxDepth, depth);
    int p = partitionLomuto(arr, low, high, m);
    quickSortRec(arr, low, p - 1, m, depth + 1, maxDepth);
    quickSortRec(arr, p + 1, high, m, depth + 1, maxDepth);
}

void quickSort(std::vector<int>& arr, SortMetrics& m) {
    if (arr.empty()) {
        return;
    }

    int maxDepth = 1;
    quickSortRec(arr, 0, static_cast<int>(arr.size()) - 1, m, 1, maxDepth);

    // 粗估遞迴堆疊空間（每層約 64 bytes）
    m.peakAuxBytes = static_cast<size_t>(maxDepth) * 64;
}

void printDivider(int w1, int w2, int w3, int w4, int w5) {
    std::cout << '+' << std::string(w1 + 2, '-')
              << '+' << std::string(w2 + 2, '-')
              << '+' << std::string(w3 + 2, '-')
              << '+' << std::string(w4 + 2, '-')
              << '+' << std::string(w5 + 2, '-')
              << "+\n";
}

void printAsciiBars(const std::vector<SortMetrics>& results) {
    const int maxWidth = 50;

    double maxTime = 0.0;
    size_t maxMem = 0;
    for (const auto& r : results) {
        maxTime = std::max(maxTime, r.timeMs);
        maxMem = std::max(maxMem, r.peakAuxBytes);
    }

    std::cout << "\n=== Time Visualization (ms) ===\n";
    for (const auto& r : results) {
        int len = (maxTime == 0.0) ? 0 : static_cast<int>(std::round((r.timeMs / maxTime) * maxWidth));
        std::cout << std::left << std::setw(14) << r.name << " | "
                  << std::string(len, '#') << " "
                  << std::fixed << std::setprecision(3) << r.timeMs << " ms\n";
    }

    std::cout << "\n=== Space Visualization (KB) ===\n";
    for (const auto& r : results) {
        int len = (maxMem == 0) ? 0 : static_cast<int>(std::round((static_cast<double>(r.peakAuxBytes) / static_cast<double>(maxMem)) * maxWidth));
        double kb = static_cast<double>(r.peakAuxBytes) / 1024.0;
        std::cout << std::left << std::setw(14) << r.name << " | "
                  << std::string(len, '*') << " "
                  << std::fixed << std::setprecision(3) << kb << " KB\n";
    }
}

void writeCsv(const std::string& path, const std::vector<SortMetrics>& results, int n, int seed) {
    std::ofstream out(path);
    out << "n,seed,algorithm,time_ms,comparisons,moves_or_swaps,peak_aux_bytes,theoretical_time,theoretical_space\n";
    for (const auto& r : results) {
        out << n << ','
            << seed << ','
            << '"' << r.name << '"' << ','
            << std::fixed << std::setprecision(6) << r.timeMs << ','
            << r.comparisons << ','
            << r.movesOrSwaps << ','
            << r.peakAuxBytes << ','
            << '"' << r.theoreticalTime << '"' << ','
            << '"' << r.theoreticalSpace << '"'
            << '\n';
    }
}

template <typename SortFn>
SortMetrics runSort(const std::vector<int>& base, const std::string& name, const std::string& tTime, const std::string& tSpace, SortFn fn) {
    SortMetrics m;
    m.name = name;
    m.theoreticalTime = tTime;
    m.theoreticalSpace = tSpace;

    std::vector<int> data = base;

    auto start = Clock::now();
    fn(data, m);
    auto end = Clock::now();

    m.timeMs = std::chrono::duration<double, std::milli>(end - start).count();

    if (m.peakAuxBytes == 0) {
        // O(1) 類演算法用 1 byte 表示近似常數輔助空間，避免圖表全為 0。
        m.peakAuxBytes = 1;
    }

    return m;
}

int main() {
    int n = 0;
    int seed = 42;

    std::cout << "===== Input Guide =====\n";
    std::cout << "Step 1) Enter n (data size), e.g. 1000\n";
    std::cout << "Step 2) Enter seed (integer), e.g. 42\n\n";

    std::cout << "Input n (recommended 10 ~ 50000): " << std::flush;
    std::cin >> n;
    if (!std::cin || n <= 0) {
        std::cerr << "Error: n must be a positive integer.\n";
        return 1;
    }

    std::cout << "Input seed (default 42, integer): " << std::flush;
    std::cin >> seed;
    if (!std::cin) {
        std::cerr << "Error: seed must be an integer.\n";
        return 1;
    }

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(1, 1000000);

    std::vector<int> base;
    base.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        base.push_back(dist(rng));
    }

    std::vector<SortMetrics> results;
    results.push_back(runSort(base, "Bubble", "Best O(n), Avg/Worst O(n^2)", "O(1)", bubbleSort));
    results.push_back(runSort(base, "Selection", "Best/Avg/Worst O(n^2)", "O(1)", selectionSort));
    results.push_back(runSort(base, "Merge", "Best/Avg/Worst O(n log n)", "O(n)", mergeSort));
    results.push_back(runSort(base, "Quick", "Avg O(n log n), Worst O(n^2)", "Avg O(log n)", quickSort));
    results.push_back(runSort(base, "Insertion", "Best O(n), Avg/Worst O(n^2)", "O(1)", insertionSort));

    const int w1 = 12;
    const int w2 = 14;
    const int w3 = 12;
    const int w4 = 14;
    const int w5 = 46;

    std::cout << "\n===== Sorting Complexity Test Result =====\n";
    std::cout << "n = " << n << ", seed = " << seed << "\n\n";

    printDivider(w1, w2, w3, w4, w5);
    std::cout << "| " << std::left << std::setw(w1) << "Algorithm"
              << " | " << std::setw(w2) << "Time(ms)"
              << " | " << std::setw(w3) << "Comp"
              << " | " << std::setw(w4) << "Aux Space(B)"
              << " | " << std::setw(w5) << "Theoretical"
              << " |\n";
    printDivider(w1, w2, w3, w4, w5);

    for (const auto& r : results) {
        std::string theory = r.theoreticalTime + " / " + r.theoreticalSpace;
        std::cout << "| " << std::left << std::setw(w1) << r.name
                  << " | " << std::setw(w2) << std::fixed << std::setprecision(3) << r.timeMs
                  << " | " << std::setw(w3) << r.comparisons
                  << " | " << std::setw(w4) << r.peakAuxBytes
                  << " | " << std::setw(w5) << theory
                  << " |\n";
    }

    printDivider(w1, w2, w3, w4, w5);

    printAsciiBars(results);

    const std::string csvPath = "results.csv";
    writeCsv(csvPath, results, n, seed);

    std::cout << "\nCSV exported: " << csvPath << "\n";
    std::cout << "You can open results.csv in Excel / Google Sheets for charts.\n";
    std::cout << "\nPress Enter to exit...";

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();

    return 0;
}
