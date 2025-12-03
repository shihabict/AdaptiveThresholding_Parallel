#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <chrono>
#include <algorithm>

// ---------- Helper: index 2D -> 1D ----------
static inline std::size_t idx(int r, int c, int width) {
    return static_cast<std::size_t>(r) * width + c;
}

// ---------- PGM Reader (P2 or P5, 8-bit) ----------
bool read_pgm(const std::string& filename,
              std::vector<uint8_t>& img,
              int& width, int& height)
{
    width = 0;
    height = 0;
    img.clear();

    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "Error: cannot open input file '" << filename << "'\n";
        return false;
    }

    auto read_next_token = [&](std::string& token)->bool {
        token.clear();
        char ch;
        // Skip whitespace and comments
        while (in.get(ch)) {
            if (ch == '#') {
                std::string dummy;
                std::getline(in, dummy); // skip comment line
                continue;
            }
            if (!std::isspace(static_cast<unsigned char>(ch))) {
                token.push_back(ch);
                break;
            }
        }
        if (token.empty() && !in) return false;
        // Read rest of token
        while (in.get(ch)) {
            if (std::isspace(static_cast<unsigned char>(ch)) || ch == '#') {
                if (ch == '#') {
                    std::string dummy;
                    std::getline(in, dummy); // skip rest of comment
                }
                break;
            }
            token.push_back(ch);
        }
        return !token.empty();
    };

    std::string magic;
    if (!read_next_token(magic)) {
        std::cerr << "Error: failed to read magic number\n";
        return false;
    }

    if (magic != "P2" && magic != "P5") {
        std::cerr << "Error: unsupported PGM format '" << magic
                  << "' (only P2, P5 allowed)\n";
        return false;
    }

    std::string token;
    if (!read_next_token(token)) {
        std::cerr << "Error: failed to read width\n";
        return false;
    }
    width = std::stoi(token);

    if (!read_next_token(token)) {
        std::cerr << "Error: failed to read height\n";
        return false;
    }
    height = std::stoi(token);

    if (!read_next_token(token)) {
        std::cerr << "Error: failed to read maxval\n";
        return false;
    }
    int maxval = std::stoi(token);
    if (maxval <= 0 || maxval > 255) {
        std::cerr << "Error: unsupported maxval " << maxval
                  << " (must be 1..255)\n";
        return false;
    }

    std::size_t num_pixels = static_cast<std::size_t>(width) * height;
    img.resize(num_pixels);

    if (magic == "P5") {
        // Binary: go to start of pixel data
        in.read(reinterpret_cast<char*>(img.data()),
                static_cast<std::streamsize>(num_pixels));
        if (!in) {
            std::cerr << "Error: failed to read binary pixel data\n";
            return false;
        }
    } else {
        // ASCII P2
        for (std::size_t i = 0; i < num_pixels; ++i) {
            if (!read_next_token(token)) {
                std::cerr << "Error: failed to read pixel " << i << "\n";
                return false;
            }
            int val = std::stoi(token);
            if (val < 0) val = 0;
            if (val > maxval) val = maxval;
            img[i] = static_cast<uint8_t>(val);
        }
    }

    return true;
}

// ---------- PGM Writer (P5, binary, 8-bit) ----------
bool write_pgm(const std::string& filename,
               const std::vector<uint8_t>& img,
               int width, int height)
{
    std::size_t expected = static_cast<std::size_t>(width) * height;
    if (img.size() != expected) {
        std::cerr << "Error: image size mismatch in write_pgm\n";
        return false;
    }

    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        std::cerr << "Error: cannot open output file '" << filename << "'\n";
        return false;
    }

    out << "P5\n" << width << " " << height << "\n255\n";
    out.write(reinterpret_cast<const char*>(img.data()),
              static_cast<std::streamsize>(img.size()));

    if (!out) {
        std::cerr << "Error: failed to write pixel data\n";
        return false;
    }

    return true;
}

// ---------- Integral Image ----------
void compute_integral(const std::vector<uint8_t>& img,
                      int width, int height,
                      std::vector<uint32_t>& integral)
{
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

// ---------- Get sum of window using integral image ----------
uint64_t get_window_sum(const std::vector<uint32_t>& integral,
                        int width, int height,
                        int r0, int c0, int r1, int c1)
{
    uint64_t A = integral[idx(r1, c1, width)];
    uint64_t B = (r0 > 0) ? integral[idx(r0 - 1, c1, width)] : 0;
    uint64_t C = (c0 > 0) ? integral[idx(r1, c0 - 1, width)] : 0;
    uint64_t D = (r0 > 0 && c0 > 0) ? integral[idx(r0 - 1, c0 - 1, width)] : 0;
    return A - B - C + D;
}

// ---------- Adaptive Threshold (Serial) ----------
void adaptive_threshold_serial(const std::vector<uint8_t>& img,
                               int width, int height,
                               int window_size, int C,
                               const std::vector<uint32_t>& integral,
                               std::vector<uint8_t>& out)
{
    if (window_size <= 1 || window_size % 2 == 0) {
        std::cerr << "Error: window_size must be odd and > 1\n";
        return;
    }

    out.resize(static_cast<std::size_t>(width) * height);

    int radius = window_size / 2;

    for (int r = 0; r < height; ++r) {
        int r0 = std::max(0, r - radius);
        int r1 = std::min(height - 1, r + radius);

        for (int c = 0; c < width; ++c) {
            int c0 = std::max(0, c - radius);
            int c1 = std::min(width - 1, c + radius);

            int win_h = r1 - r0 + 1;
            int win_w = c1 - c0 + 1;
            int area  = win_h * win_w;

            uint64_t sum = get_window_sum(integral, width, height, r0, c0, r1, c1);
            double mean = static_cast<double>(sum) / static_cast<double>(area);
            double thresh = mean - static_cast<double>(C);

            uint8_t pix = img[idx(r, c, width)];
            uint8_t val = (static_cast<double>(pix) > thresh) ? 255 : 0;

            out[idx(r, c, width)] = val;
        }
    }
}

// ---------- main ----------
int main(int argc, char** argv) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0]
                  << " <input.pgm> <output.pgm> <window_size> <C> <csv_log_file>\n";
        std::cerr << "Example: " << argv[0]
                  << " input.pgm output.pgm 31 10 serial_results.csv\n";
        return 1;
    }

    std::string input_file  = argv[1];
    std::string output_file = argv[2];
    int window_size = std::stoi(argv[3]);
    int C           = std::stoi(argv[4]);
    std::string csv_log_file = argv[5];

    std::vector<uint8_t> img;
    int width = 0, height = 0;

    if (!read_pgm(input_file, img, width, height)) {
        return 1;
    }

    std::vector<uint32_t> integral;
    compute_integral(img, width, height, integral);

    std::vector<uint8_t> out;

    auto t0 = std::chrono::high_resolution_clock::now();
    adaptive_threshold_serial(img, width, height, window_size, C, integral, out);
    auto t1 = std::chrono::high_resolution_clock::now();

    double elapsed = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "T_serial = " << elapsed << " s\n";

    if (!write_pgm(output_file, out, width, height)) {
        return 1;
    }

    // ---- Write CSV log (append), add header if file is new ----
    bool file_exists = false;
    {
        std::ifstream test(csv_log_file);
        if (test.good()) file_exists = true;
    }

    std::ofstream log(csv_log_file, std::ios::app);
    if (!log) {
        std::cerr << "Warning: could not open CSV log file '" << csv_log_file << "'\n";
    } else {
        // If file did not exist, write header first
        if (!file_exists) {
            log << "width,height,window_size,C,time_seconds\n";
        }

        // Append row
        log << width << ","
            << height << ","
            << window_size << ","
            << C << ","
            << elapsed << "\n";
    }

    return 0;
}
