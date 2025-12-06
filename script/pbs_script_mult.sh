#!/bin/bash
# Hybrid performance comparison script for Picocluster
# Run this via:  run_script_hyb pbs_Script.sh
# When prompted:
#   - Enter MPI Processes: 4
#   - Enter Threads per MPI Process: 4

set -e  # exit on first error

# Go to submission directory (for Slurm)
if [ -n "$SLURM_SUBMIT_DIR" ]; then
    cd "$SLURM_SUBMIT_DIR"
fi

# --------- Modules (adapt if your cluster uses different names) ---------
module load gcc
module load openmpi
module load python

# --------- User settings ---------
EXE="./main_hybrid"

INPUT_JPG="highway_4k.jpg"   # your original JPG (change if needed)
INPUT_PGM="input.pgm"       # PGM used by C++ code

WIN_SIZE=15
C_VAL=5

# --------- Prepare input PGM (once) ---------
if [ ! -f "$INPUT_PGM" ]; then
    echo "Converting $INPUT_JPG -> $INPUT_PGM ..."
    python3 convert_to_pgm.py "$INPUT_JPG" "$INPUT_PGM"
fi

# --------- Summary CSV ---------
SUMMARY="timing_summary.csv"
echo "mode,width,height,window_size,C,mpi_processes,omp_threads,time_seconds" > "$SUMMARY"

# Helper function to append a run's timing (skipping header)
append_timing () {
    local mode_name="$1"
    local timing_file="$2"

    # timing file has header: width,height,window_size,C,mpi_processes,omp_threads,time_seconds
    # we prepend the mode name as the first column
    tail -n +2 "$timing_file" | while IFS= read -r line; do
        echo "$mode_name,$line" >> "$SUMMARY"
    done
}

echo "======================================================"
echo " 1) SERIAL BASELINE (no MPI, 1 thread)"
echo "======================================================"

export OMP_NUM_THREADS=1
OUT_PGM_SERIAL="output_serial.pgm"
TIME_SERIAL="timing_serial.csv"

# Run without mpirun for pure serial baseline
"$EXE" "$INPUT_PGM" "$OUT_PGM_SERIAL" "$WIN_SIZE" "$C_VAL" "$TIME_SERIAL"
append_timing "serial_1core" "$TIME_SERIAL"

echo "======================================================"
echo " 2) OPENMP ONLY (MPI=1, OMP=4)"
echo "======================================================"

export OMP_NUM_THREADS=4
OUT_PGM_OMP="output_omp4.pgm"
TIME_OMP="timing_omp4.csv"

mpirun -np 1 "$EXE" "$INPUT_PGM" "$OUT_PGM_OMP" "$WIN_SIZE" "$C_VAL" "$TIME_OMP"
append_timing "omp_1x4" "$TIME_OMP"

echo "======================================================"
echo " 3) MPI ONLY (MPI=4, OMP=1)"
echo "======================================================"

export OMP_NUM_THREADS=1
OUT_PGM_MPI="output_mpi4.pgm"
TIME_MPI="timing_mpi4.csv"

mpirun -np 4 "$EXE" "$INPUT_PGM" "$OUT_PGM_MPI" "$WIN_SIZE" "$C_VAL" "$TIME_MPI"
append_timing "mpi_4x1" "$TIME_MPI"

echo "======================================================"
echo " 4) HYBRID (MPI=4, OMP=4)"
echo "======================================================"

export OMP_NUM_THREADS=4
OUT_PGM_HYB="output_hybrid4x4.pgm"
TIME_HYB="timing_hybrid4x4.csv"

mpirun -np 4 "$EXE" "$INPUT_PGM" "$OUT_PGM_HYB" "$WIN_SIZE" "$C_VAL" "$TIME_HYB"
append_timing "hybrid_4x4" "$TIME_HYB"

echo "======================================================"
echo "All runs finished."
echo "Summary timings saved in: $SUMMARY"
echo "Output images:"
echo "  Serial : $OUT_PGM_SERIAL"
echo "  OpenMP : $OUT_PGM_OMP"
echo "  MPI    : $OUT_PGM_MPI"
echo "  Hybrid : $OUT_PGM_HYB"
echo "======================================================"
