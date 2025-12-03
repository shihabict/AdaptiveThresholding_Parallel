#include "image_io.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {

// Helper: skip comments and whitespace, then read next token
bool read_next_token(std::istream& is, std::string& token) {
    token.clear();
    char ch;

    // Skip whitespace and comments
    while (is.get(ch)) {
        if (ch == '#') {
            // Skip the rest of the comment line
            std::string dummy;
            std::getline(is, dummy);
            continue;
        }
        if (!std::isspace(static_cast<unsigned char>(ch))) {
            // Found start of token
            token.push_back(ch);
            break;
        }
    }

    if (token.empty() && !is) {
        return false; // no token found
    }

    // Read the rest of the token
    while (is.get(ch)) {
        if (std::isspace(static_cast<unsigned char>(ch)) || ch == '#') {
            if (ch == '#') {
                std::string dummy;
                std::getline(is, dummy); // consume comment line
            }
            break;
        }
        token.push_back(ch);
    }

    return !token.empty();
}

} // namespace


bool read_pgm(const std::string& filename,
              std::vector<uint8_t>& img,
              int& width, int& height)
{
    width  = 0;
    height = 0;
    img.clear();

    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "read_pgm: failed to open file '" << filename << "'\n";
        return false;
    }

    // Read magic number (P2 or P5)
    std::string magic;
    if (!read_next_token(in, magic)) {
        std::cerr << "read_pgm: failed to read magic number\n";
        return false;
    }

    if (magic != "P2" && magic != "P5") {
        std::cerr << "read_pgm: unsupported PGM format (expected P2 or P5, got '"
                  << magic << "')\n";
        return false;
    }

    // Read width
    std::string token;
    if (!read_next_token(in, token)) {
        std::cerr << "read_pgm: failed to read width\n";
        return false;
    }
    width = std::stoi(token);

    // Read height
    if (!read_next_token(in, token)) {
        std::cerr << "read_pgm: failed to read height\n";
        return false;
    }
    height = std::stoi(token);

    // Read max value
    if (!read_next_token(in, token)) {
        std::cerr << "read_pgm: failed to read maxval\n";
        return false;
    }
    int maxval = std::stoi(token);
    if (maxval <= 0 || maxval > 255) {
        std::cerr << "read_pgm: unsupported maxval = " << maxval
                  << " (only 1..255 supported)\n";
        return false;
    }

    const std::size_t num_pixels = static_cast<std::size_t>(width) * height;
    img.resize(num_pixels);

    if (magic == "P5") {
        // Binary format: next bytes are pixel data
        // Ensure we are at the start of pixel data (skip a single whitespace)
        in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        in.read(reinterpret_cast<char*>(img.data()),
                static_cast<std::streamsize>(num_pixels));
        if (!in) {
            std::cerr << "read_pgm: failed to read binary pixel data\n";
            return false;
        }
    } else {
        // ASCII format (P2): read integers as text
        for (std::size_t i = 0; i < num_pixels; ++i) {
            if (!read_next_token(in, token)) {
                std::cerr << "read_pgm: failed to read pixel " << i << "\n";
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


bool write_pgm(const std::string& filename,
               const std::vector<uint8_t>& img,
               int width, int height)
{
    const std::size_t expected = static_cast<std::size_t>(width) * height;
    if (img.size() != expected) {
        std::cerr << "write_pgm: image size (" << img.size()
                  << ") does not match width*height (" << expected << ")\n";
        return false;
    }

    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        std::cerr << "write_pgm: failed to open file '" << filename << "'\n";
        return false;
    }

    // We write P5 (binary) format with maxval = 255
    out << "P5\n" << width << " " << height << "\n255\n";

    out.write(reinterpret_cast<const char*>(img.data()),
              static_cast<std::streamsize>(img.size()));

    if (!out) {
        std::cerr << "write_pgm: failed to write pixel data to '" << filename << "'\n";
        return false;
    }

    return true;
}
