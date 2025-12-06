#include "threshold.hpp"
#include <algorithm>
#include <iostream>

//To make 2D (r, c) in a 1D row-major array
static inline std::size_t idx(int r, int c, int width) {
    return static_cast<std::size_t>(r) * width + c;
}

//Compute the integral image from the input image
void compute_integral(const std::vector<uint8_t>& img, int width, int height, std::vector<uint32_t>& integral) {
    integral.assign(static_cast<std::size_t>(width) * height, 0);
    for (int r = 0; r < height; ++r) {
        uint32_t row_sum = 0;
        for (int c = 0; c < width; ++c) {
            row_sum += img[idx(r, c, width)];
            uint32_t above = (r > 0) ? integral[idx(r - 1, c, width)] : 0;
            integral[idx(r, c, width)] = row_sum + above;
        }
    }
}

//Get sum of pixel values in the rectangular window using the integral image
static inline uint64_t get_window_sum(const std::vector<uint32_t>& integral, int width, int r0, int c0, int r1, int c1) {
    uint64_t A = integral[idx(r1, c1, width)];
    uint64_t B = (r0 > 0) ? integral[idx(r0 - 1, c1, width)] : 0;
    uint64_t C = (c0 > 0) ? integral[idx(r1, c0 - 1, width)] : 0;
    uint64_t D = (r0 > 0 && c0 > 0) ? integral[idx(r0 - 1, c0 - 1, width)] : 0;
    return A - B - C + D;
}


//Serial adaptive thresholding function
void adaptive_threshold_serial(const std::vector<uint8_t>& img, int width, int height,
                               const ThresholdParams& params, const std::vector<uint32_t>& integral,
                               std::vector<uint8_t>& out) {
    out.resize(static_cast<std::size_t>(width) * height);
    int radius = params.window_size / 2;
    for (int r = 0; r < height; ++r) {
        int r0 = std::max(0, r - radius);
        int r1 = std::min(height - 1, r + radius);
        for (int c = 0; c < width; ++c) {
            int c0 = std::max(0, c - radius);
            int c1 = std::min(width - 1, c + radius);
            uint64_t sum = get_window_sum(integral, width, r0, c0, r1, c1);
            double mean = static_cast<double>(sum) / ((r1 - r0 + 1) * (c1 - c0 + 1));
            out[idx(r, c, width)] = (img[idx(r, c, width)] > (mean - params.C)) ? 255 : 0;
        }
    }
}