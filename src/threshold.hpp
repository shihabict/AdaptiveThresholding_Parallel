#include <vector>
#include <cstdint>
#include <string>
#include <stdexcept>

//Structure to hold thresholding parameters
struct ThresholdParams {
    int window_size;
    int C; //this is the constant to subtract from mean
};

//Function prototypes
void compute_integral(const std::vector<uint8_t>& img,
                      int width, int height,
                      std::vector<uint32_t>& integral);

//this is the hybrid MPI+OpenMP version
void adaptive_threshold_hybrid(const std::vector<uint8_t>& img_global,
                               int width, int height,
                               const ThresholdParams& params,
                               const std::vector<uint32_t>& integral_global,
                               int rank, int numprocs,
                               std::vector<uint8_t>& out_global);


//this is the serial version
void adaptive_threshold_serial(const std::vector<uint8_t>& img,
                               int width, int height,
                               const ThresholdParams& params,
                               const std::vector<uint32_t>& integral,
                               std::vector<uint8_t>& out);