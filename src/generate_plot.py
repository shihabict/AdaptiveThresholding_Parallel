import pandas as pd
import matplotlib.pyplot as plt

def main(data_path):
    # Change this path if needed
    csv_path = data_path

    df = pd.read_csv(csv_path)

    # Compute total cores
    df["cores"] = df["mpi_processes"] * df["omp_threads"]

    # Find baseline time: MPI=1, OMP=1
    base = df[(df["mpi_processes"] == 1) & (df["omp_threads"] == 1)]
    if base.empty:
        raise ValueError("Baseline row not found (mpi_processes=1 and omp_threads=1).")
    T1 = float(base.iloc[0]["time_seconds"])

    # Compute speedup
    df["speedup"] = T1 / df["time_seconds"]

    # Build configuration label (one per row)
    df["config"] = df.apply(
        lambda r: f"MPI={int(r.mpi_processes)}, OMP={int(r.omp_threads)}, Cores={int(r.cores)}",
        axis=1
    )

    # Keep the CSV row order (or sort if you want consistent ordering)
    # If you prefer sorting by MPI then OMP, uncomment the next line:
    df = df.sort_values(["mpi_processes", "omp_threads"]).reset_index(drop=True)

    x = range(len(df))

    plt.figure(figsize=(14, 5))
    plt.plot(x, df["speedup"], marker="o")

    plt.ylabel("Speedup (T1 / Tp)")
    title = "Speedup vs Configuration"
    # Add basic experiment tag (optional)
    if {"width", "height"}.issubset(df.columns):
        title += f" ({int(df.iloc[0]['width'])}×{int(df.iloc[0]['height'])})"
    plt.title(title)

    plt.xticks(list(x), df["config"], rotation=45, ha="right")
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(f"speedup_vs_configuration_{data_path.split('/')[1].split('.')[0].split('_')[-1]}.png", dpi=200)

    print("Saved: speedup_vs_configuration.png")

if __name__ == "__main__":
    data_path = "results/timing_matrix_small.csv"
    main(data_path)
