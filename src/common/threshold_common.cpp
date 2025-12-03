#include "threshold.hpp"

#include <algorithm>  // std::max, std::min
#include <iostream>   // optional for debug
#include <stdexcept>

// Helper to index 2D (r, c) in a 1D row-major array
static inline std::size_t idx(int r, int c, int width) {
    return static_cast<std::size_t>(r) * width + c;
}

// Compute integral image:
// integral(r, c) = sum of img(0..r, 0..c)
void compute_integral(const std::vector<uint8_t>& img,
                      int width, int height,
                      std::vector<uint32_t>& integral)
{
    if (width <= 0 || height <= 0) {
        throw std::runtime_error("compute_integral: invalid width/height");
    }
    if (static_cast<int>(img.size()) != width * height) {
        throw std::runtime_error("compute_integral: img size mismatch");
    }

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

// Get sum of pixels in window [r0..r1] x [c0..c1] using integral image
// Assumes 0 <= r0 <= r1 < height, 0 <= c0 <= c1 < width
static inline uint64_t get_window_sum(const std::vector<uint32_t>& integral,
                                      int width, int height,
                                      int r0, int c0, int r1, int c1)
{
    // Use 64-bit for safety in accumulation
    uint64_t A = integral[idx(r1, c1, width)];
    uint64_t B = (r0 > 0) ? integral[idx(r0 - 1, c1, width)] : 0;
    uint64_t C = (c0 > 0) ? integral[idx(r1, c0 - 1, width)] : 0;
    uint64_t D = (r0 > 0 && c0 > 0) ? integral[idx(r0 - 1, c0 - 1, width)] : 0;

    return A - B - C + D;
}

void adaptive_threshold_serial(const std::vector<uint8_t>& img,
                               int width, int height,
                               const ThresholdParams& params,
                               const std::vector<uint32_t>& integral,
                               std::vector<uint8_t>& out)
{
    if (width <= 0 || height <= 0) {
        throw std::runtime_error("adaptive_threshold_serial: invalid width/height");
    }
    if (static_cast<int>(img.size()) != width * height) {
        throw std::runtime_error("adaptive_threshold_serial: img size mismatch");
    }
    if (static_cast<int>(integral.size()) != width * height) {
        throw std::runtime_error("adaptive_threshold_serial: integral size mismatch");
    }
    if (params.window_size <= 1 || params.window_size % 2 == 0) {
        throw std::runtime_error("adaptive_threshold_serial: window_size must be odd and > 1");
    }

    out.resize(static_cast<std::size_t>(width) * height);

    int w = params.window_size;
    int radius = w / 2;  // e.g., w=31 -> radius=15

    for (int r = 0; r < height; ++r) {
        // Compute vertical window bounds (clamped)
        int r0 = std::max(0, r - radius);
        int r1 = std::min(height - 1, r + radius);

        for (int c = 0; c < width; ++c) {
            // Compute horizontal window bounds (clamped)
            int c0 = std::max(0, c - radius);
            int c1 = std::min(width - 1, c + radius);

            int win_h = r1 - r0 + 1;
            int win_w = c1 - c0 + 1;
            int area = win_h * win_w;

            uint64_t sum = get_window_sum(integral, width, height, r0, c0, r1, c1);
            double mean = static_cast<double>(sum) / static_cast<double>(area);

            double thresh = mean - static_cast<double>(params.C);

            uint8_t pix = img[idx(r, c, width)];
            uint8_t val = (static_cast<double>(pix) > thresh) ? 255 : 0;

            out[idx(r, c, width)] = val;
        }
    }
}
