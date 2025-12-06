#include "threshold.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
// reads the next token from the PGM file, skipping comments and whitespace
bool read_next_token(std::istream& is, std::string& token) {
    token.clear();
    char ch;
    // skip whitespace and comments
    while (is.get(ch)) {
        if (ch == '#') {    //skip comment line
            std::string dummy;
            std::getline(is, dummy);
            continue;
        }
        if (!std::isspace(static_cast<unsigned char>(ch))) {
            token.push_back(ch);    // first character of the token
            break;
        }
    }
    if (token.empty() && !is) return false;
    // read the rest of the token
    while (is.get(ch)) {
        if (std::isspace(static_cast<unsigned char>(ch)) || ch == '#') {
            if (ch == '#') {
                std::string dummy;
                std::getline(is, dummy);
            }
            break;
        }
        token.push_back(ch);
    }
    return !token.empty();
}
} // namespace for internal helpers

// Reads a PGM file (P2 or P5) into img vector, setting width and height
//Outputs pixel array, width and height
bool read_pgm(const std::string& filename, std::vector<uint8_t>& img, int& width, int& height) {
    width = 0; height = 0; img.clear();
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "Error: cannot open input file '" << filename << "'\n";
        return false;
    }
    // read magic number
    std::string magic;
    if (!read_next_token(in, magic)) return false;
    if (magic != "P2" && magic != "P5") return false;

    // read width, height, maxval
    std::string token;
    if (!read_next_token(in, token)) return false;
    width = std::stoi(token);
    if (!read_next_token(in, token)) return false;
    height = std::stoi(token);
    if (!read_next_token(in, token)) return false;
    int maxval = std::stoi(token);

    //allocate storage for pixel data
    std::size_t num_pixels = static_cast<std::size_t>(width) * height;
    img.resize(num_pixels);

    // read pixel data
    if (magic == "P5") {
        in.ignore(1); // skip single whitespace
        in.read(reinterpret_cast<char*>(img.data()), num_pixels);
    } else {
        //ASCII format, reads pixel by pixel
        for (std::size_t i = 0; i < num_pixels; ++i) {
            if (!read_next_token(in, token)) return false;
            img[i] = static_cast<uint8_t>(std::stoi(token));
        }
    }
    return true;
}

// Writes a PGM file (P5) from img vector, with given width and height
bool write_pgm(const std::string& filename, const std::vector<uint8_t>& img, int width, int height) {
    std::ofstream out(filename, std::ios::binary);
    if (!out) return false;
    // write header
    out << "P5\n" << width << " " << height << "\n255\n";
    // write pixel data in bytes
    out.write(reinterpret_cast<const char*>(img.data()), img.size());
    return true;
}