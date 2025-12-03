#ifndef THRESHOLD_HPP
#define THRESHOLD_HPP

#include <vector>
#include <cstdint>

// Parameters for adaptive thresholding
struct ThresholdParams {
    int window_size;  // e.g. 31 (must be odd, > 1)
    int C;            // constant subtracted from local mean (e.g. 10)
};

// Compute integral image (summed-area table) from an 8-bit grayscale image.
//
// img: input image, size = width * height, row-major
// integral: output integral image, size = width * height, row-major
//
// integral(r, c) = sum_{0..r, 0..c} img(y, x)
// (prefix sum over rows and columns)
void compute_integral(const std::vector<uint8_t>& img,
                      int width, int height,
                      std::vector<uint32_t>& integral);

// Serial adaptive mean thresholding using precomputed integral image.
//
// img:        input image, size = width * height
// integral:   integral image (from compute_integral)
// params:     window_size (odd) and C offset
// out:        output binary image (0 or 255), resized to width * height
void adaptive_threshold_serial(const std::vector<uint8_t>& img,
                               int width, int height,
                               const ThresholdParams& params,
                               const std::vector<uint32_t>& integral,
                               std::vector<uint8_t>& out);

#endif // THRESHOLD_HPP
