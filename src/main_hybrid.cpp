#include <iostream> 
#include <fstream>          // For IO files 
#include <vector>
#include <string>       
#include <cstdint>          // For uint8_t and uint32_t
#include <chrono>           // For timing
#include <algorithm>        // For std::min and std::max
#include <sstream>
#include <stdexcept>        
#include <mpi.h>            // For MPI
#include <omp.h>            // For OpenMP
#include "threshold.hpp"    

//Function prototypes from image_io
extern bool read_pgm(const std::string& filename, std::vector<uint8_t>& img, int& width, int& height);
extern bool write_pgm(const std::string& filename, const std::vector<uint8_t>& img, int width, int height);
extern void compute_integral(const std::vector<uint8_t>& img, int width, int height, std::vector<uint32_t>& integral);

//For the timing row in CSV file
void log_timing(const std::string& csv_log_file, int width, int height, 
                const ThresholdParams& params, double elapsed_time, int numprocs, int numthreads) 
{
    bool file_exists = false;
    {
        std::ifstream test(csv_log_file);
        if (test.good()) file_exists = true;
    }

    std::ofstream log(csv_log_file, std::ios::app);
    if (!log) {
        std::cerr << "Warning: could not open CSV log file '" << csv_log_file << "'\n";
    } else {
        if (!file_exists) {
            log << "width,height,window_size,C,mpi_processes,omp_threads,time_seconds\n";
        }

        log << width << ","
            << height << ","
            << params.window_size << ","
            << params.C << ","
            << numprocs << ","
            << numthreads << ","
            << elapsed_time << "\n";
    }
}



int main(int argc, char** argv) {
    
    //Initialize MPI with thread support
    int provided_thread_level;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided_thread_level);
    
	
	//Get rank and size
    int rank, numprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	
	//Check threading support
	if (rank == 0 && provided_thread_level < MPI_THREAD_FUNNELED) {
    std::cerr << "MPI implementation does not support required threading level\n";
    MPI_Abort(MPI_COMM_WORLD, 1);
	}

	
    
    //Set OpenMP threads based on environment variable
    int numthreads = 1;
    #ifdef _OPENMP
        numthreads = omp_get_max_threads();
    #endif
    
    //command line arguments, 5 arguments expected
    if (argc != 6) {
        if (rank == 0) {
            std::cerr << "Usage: " << argv[0]
                      << " <input.pgm> <output.pgm> <window_size> <C> <csv_log_file>\n";
            std::cerr << "Note: Number of OpenMP threads is determined by OMP_NUM_THREADS environment variable.\n";
        }
        MPI_Finalize();
        return 1;
    }

    
    std::string input_file;
    std::string output_file;
    ThresholdParams params;
    std::string csv_log_file;

    //Global data buffers shared by all processes
    std::vector<uint8_t> img_global;
    std::vector<uint32_t> integral_global;
    std::vector<uint8_t> out_global;
    int width = 0, height = 0;

    //Rank 0 reads input image and compute integral image
    if (rank == 0) {
        input_file = argv[1];
        output_file = argv[2];
        params.window_size = std::stoi(argv[3]);
        params.C = std::stoi(argv[4]);
        csv_log_file = argv[5];

        if (!read_pgm(input_file, img_global, width, height)) {
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        try {
            //Compute integral image serially
            compute_integral(img_global, width, height, integral_global);
            
            //Rank 0 pre-sizes the global output buffer for MPI_Gatherv
            out_global.resize(static_cast<std::size_t>(width) * height);
            
        } catch (const std::exception& e) {
            std::cerr << "Rank 0 Error in integral: " << e.what() << std::endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }
    
    //Boradcast image dimensions and parameters to all processes/ranks
    MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&params.window_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&params.C, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (width * height == 0) {
        if (rank == 0) std::cerr << "Image is empty or failed to load.\n";
        MPI_Finalize();
        return 1;
    }
    
    //Boradcast complete image and integral image to all processes/ranks
    std::size_t global_size = static_cast<std::size_t>(width) * height;
    if (rank != 0) {
        img_global.resize(global_size);
        integral_global.resize(global_size);
    }
    
    //Broadcast the two large data arrays
    MPI_Bcast(img_global.data(), global_size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    MPI_Bcast(integral_global.data(), global_size, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    
    //Start timing here
    double t_start = MPI_Wtime();
    
    //Perform hybrid MPI+OpenMP adaptive thresholding
    adaptive_threshold_hybrid(img_global, width, height, params, integral_global, 
                              rank, numprocs, out_global);
    
    //End timing here
    double t_end = MPI_Wtime();
    double elapsed = t_end - t_start;
    
    //Rank 0 writes output image and logs timing
    if (rank == 0) {
        if (!write_pgm(output_file, out_global, width, height)) {
            return 1;
        }
        std::cout << "T_hybrid (" << numprocs << " MPI procs, " << numthreads << " OMP threads) = " << elapsed << " s\n";
        log_timing(csv_log_file, width, height, params, elapsed, numprocs, numthreads);
    }

    MPI_Finalize();
    return 0;
}