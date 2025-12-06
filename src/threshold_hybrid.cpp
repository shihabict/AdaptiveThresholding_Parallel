#include "threshold.hpp"
#include <mpi.h>            //For MPI
#include <algorithm>
#include <iostream>
#include <cmath>  //For std::ceil and std::floor
#include <omp.h> //For OpenMP

//To make 2D (r, c) in a 1D row-major array
static inline std::size_t idx(int r, int c, int width) {
    return static_cast<std::size_t>(r) * width + c;
}

//Get sum of pixel values in the rectangular window using the integral image
static inline uint64_t get_window_sum(const std::vector<uint32_t>& integral,
                                      int width,
                                      int r0, int c0, int r1, int c1)
{
    //Integral image values at the corners of the window with formula A-B-C+D
    uint64_t A = integral[idx(r1, c1, width)];
    uint64_t B = (r0 > 0) ? integral[idx(r0 - 1, c1, width)] : 0;
    uint64_t C = (c0 > 0) ? integral[idx(r1, c0 - 1, width)] : 0;
    uint64_t D = (r0 > 0 && c0 > 0) ? integral[idx(r0 - 1, c0 - 1, width)] : 0;

    return A - B - C + D;
}



//Hybrid MPI+OpenMP adaptive thresholding function
void adaptive_threshold_hybrid(const std::vector<uint8_t>& img_global,
                               int width, int height,
                               const ThresholdParams& params,
                               const std::vector<uint32_t>& integral_global,
                               int rank, int numprocs,
                               std::vector<uint8_t>& out_global)
{
   
    
    
    
    //load balancing
    int base_rows = height / numprocs;
    int remainder = height % numprocs;
    
    int rows_per_proc;
    if (rank < remainder) {
        rows_per_proc = base_rows + 1;
    } else {
        rows_per_proc = base_rows;
    }

    int start_row = base_rows * rank + std::min(rank, remainder);
    int end_row = start_row + rows_per_proc; 
    
    int local_size = rows_per_proc * width; 
    std::vector<uint8_t> out_local(local_size);
    
    //Computation using OpenMP within each MPI process
    int w = params.window_size;
    int radius = w / 2;

    




    //Computation using OpenMP within each MPI process
    //data shared with this process and threads work on different rows
    #pragma omp parallel for default(none) shared(img_global, integral_global, out_local, params, width, height, radius, end_row, start_row) schedule(static)
    for (int r_global = start_row; r_global < end_row; ++r_global) {
        
        //compute vertical window bounds which are clamped
        int r0 = std::max(0, r_global - radius);
        int r1 = std::min(height - 1, r_global + radius);

        for (int c = 0; c < width; ++c) {
            //compute horizontal window bounds which are clamped
            int c0 = std::max(0, c - radius);
            int c1 = std::min(width - 1, c + radius);

            int win_h = r1 - r0 + 1;
            int win_w = c1 - c0 + 1;
            int area = win_h * win_w;
            
            //calculation of sum, mean, threshold, and output pixel value            
            uint64_t sum = get_window_sum(integral_global, width, r0, c0, r1, c1);
            
            double mean = static_cast<double>(sum) / static_cast<double>(area);
            double thresh = mean - static_cast<double>(params.C);

            uint8_t pix = img_global[idx(r_global, c, width)];
            uint8_t val = (static_cast<double>(pix) > thresh) ? 255 : 0;
            
            //store the result in local output array
            int r_local = r_global - start_row;
            out_local[idx(r_local, c, width)] = val;
        }
    } //openMP parallel region ends here

    
    //now communication done to gather all local outputs to global output on Rank 0 using MPI_Gatherv
    std::vector<int> recv_counts(numprocs);
    std::vector<int> displacements(numprocs);
    int current_displacement = 0;
    
    if (rank == 0) {
        for (int p = 0; p < numprocs; ++p) {
            int p_rows;
            if (p < remainder) {
                p_rows = base_rows + 1;
            } else {
                p_rows = base_rows;
            }
            recv_counts[p] = p_rows * width;
            displacements[p] = current_displacement;
            current_displacement += recv_counts[p];
        }
    }
    
    

    //Gather all local output chunks in global output buffer on Rank 0
    MPI_Gatherv(out_local.data(), 
                local_size, 
                MPI_UNSIGNED_CHAR,
                out_global.data(), //only the root rank
                recv_counts.data(), 
                displacements.data(),
                MPI_UNSIGNED_CHAR, 
                0, // Root Rank
                MPI_COMM_WORLD);
}