# Hybrid MPI+OpenMP Parallel Adaptive Thresholding

Parallel Adaptive Thresholding — Hybrid MPI + OpenMP Version

This project implements a hybrid parallel adaptive thresholding algorithm using both MPI (Message Passing Interface) for distributed-memory parallelism and OpenMP for shared-memory multithreading.

It extends the serial version by distributing image rows across multiple nodes while using multiple CPU cores within each node — a common design pattern in high-performance computing (HPC).

## 🎯 Project Objective

- Implement adaptive image thresholding with local mean estimation
- Accelerate computation using Hybrid MPI + OpenMP parallelism
- Use integral images (summed-area tables) for O(1) window-sum operations
- Process grayscale PGM images for efficiency and HPC compatibility
- Execute experiments on the Picocluster ARM HPC system
- Compare performance across:
  - Serial
  - OpenMP-only
  - MPI-only
  - Hybrid MPI+OpenMP
- Log performance metrics to CSV for speedup analysis

## 🖼️ Algorithm Summary

Adaptive thresholding computes for each pixel:

```
mean = sum(window) / area
threshold = mean - C
pixel_out = 255 if pixel > threshold else 0
```

Where:

- `window_size` (e.g., 15×15) defines the local region
- `C` is a constant offset
- Local sums use an integral image for constant-time lookup

This produces a binarized image robust to non-uniform lighting.

## 📂 Project Structure

```
Project_Hybrid/
│
├── main_hybrid.cpp               # MPI control, OpenMP setup, timing, I/O
├── threshold_hybrid.cpp          # Hybrid thresholding kernel
├── threshold_common.cpp          # Integral image + shared utilities
├── threshold.hpp                 # Parameter definitions
├── image_io.cpp                  # PGM read/write implementation
│
├── convert_to_pgm.py             # Convert JPG/PNG → PGM
├── convert_to_jpg.py             # Convert PGM → JPG
│
├── pbs_Script.sh                 # Picocluster batch job script
│
├── sample_outputs/               # Example threshold results
│
└── README.md
```

## 🧪 Performance Summary (4K Image Test)

| Mode | MPI | OMP | Total Cores | Time (s) | Speedup |
|------|-----|-----|-------------|----------|---------|
| serial_1core | 1 | 1 | 1 | 0.4598 | 1.00× |
| omp_1x4 | 1 | 4 | 4 | 0.1318 | 3.49× |
| mpi_4x1 | 4 | 1 | 4 | 1.0099 | 0.45× |
| hybrid_4x4 | 4 | 4 | 16 | 0.9174 | 0.50× |

### 📝 Interpretation

- **OpenMP-only** performs best (shared memory, no communication overhead).
- **MPI-only** and **Hybrid** versions are slower because:
  - 4K image + integral table ≈ 60 MB broadcast/gather per run
  - Gigabit network → communication dominates
  - Computation on ARM Cortex A72 nodes is relatively light
- These results highlight an important HPC reality: For moderately sized images, intra-node parallelism outperforms inter-node MPI due to communication costs.

## ▶️ How to Build and Run

### 1️⃣ Compile the Hybrid Program

```bash
mpic++ -Ofast -fopenmp \
    main_hybrid.cpp threshold_hybrid.cpp threshold_common.cpp image_io.cpp \
    -o adaptive_hybrid
```

### 2️⃣ Convert an Input Image to PGM

```bash
python3 convert_to_pgm.py input.jpg input.pgm
```

### 3️⃣ Run with MPI + OpenMP

```bash
export OMP_NUM_THREADS=4
mpirun -np 4 ./adaptive_hybrid input.pgm output.pgm 15 5 timing.csv
```

**Arguments:**

- `input.pgm` — Input grayscale image
- `output.pgm` — Output binary image
- `15` — Window size
- `5` — Constant C
- `timing.csv` — Output log file

## 🧵 PBS Script for Picocluster

Submit using the helper script:

```bash
run_script_hyb pbs_Script.sh
```

The script:

- Converts image → PGM
- Runs serial, OpenMP, MPI, and Hybrid modes
- Saves timing results in a summary CSV
- Outputs thresholded images for comparison

## 📜 Requirements

- C++17 compiler with OpenMP (GCC recommended)
- OpenMPI or MPICH
- Python 3 with Pillow for image conversion
- PGM image format

## 🧠 References

- Adaptive thresholding using integral images (Viola–Jones framework)
- Hybrid MPI+OpenMP programming patterns
- Distributed-memory scaling and communication bottlenecks

## 👩‍💻 Author

**Sanjoy Dev** and **MD Shihab Uddin**  
Parallel Programming — Hybrid Adaptive Thresholding  
Department of Computer Science  
University of Alabama in Huntsville
