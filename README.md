# AdaptiveThresholding_Parallel


# Parallel Adaptive Thresholding — Phase 1 (Serial Version)

This phase implements the **serial baseline** of adaptive thresholding for image segmentation.  
It provides the ground-truth performance that later phases (OpenMP, MPI, Hybrid MPI+OpenMP) will be compared against.

---

## 🎯 Phase 1 Objective

- Implement a correct **serial adaptive thresholding** algorithm  
- Use a **summed-area table (integral image)** for fast local mean computation  
- Process grayscale **PGM** images for simplicity and HPC compatibility  
- Log image size and runtime into a **CSV file**  
- Establish the performance baseline needed for speedup and efficiency analysis  

---

## 🧪 How to Build and Run

### 1. Compile the program

```bash
g++ -O2 -std=c++17 src/serial/main_serial.cpp -o adaptive_serial
```

## Convert JPG → PGM
```bash
python sec/convert_to_pgm.py input_img.jpg converted_input.pgm
```
## Run the serial adaptive thresholding
``` bash
./adaptive_serial input.pgm output.pgm window_size C log.csv

```

## CSV Output Format
``` bash
width,height,window_size,C,time_seconds
640,480,31,10,0.034552
1024,768,31,10,0.112139
2048,2048,31,10,0.765901
```

## Convert PGM → JPG
```bash
python sec/convert_to_jpg.py input_img.pgm converted_input.jpg
```