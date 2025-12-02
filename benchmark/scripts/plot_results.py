import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os

# Set style for paper-grade plots
sns.set_theme(style="whitegrid")
plt.rcParams.update({'font.size': 12})

RESULTS_FILE = "benchmark_results.csv"

def main():
    if not os.path.exists(RESULTS_FILE):
        print(f"Error: {RESULTS_FILE} not found. Run 'make run' first.")
        return

    df = pd.read_csv(RESULTS_FILE)

    # Create separate plots for Exact and Approx because scales differ vastly
    algorithms = df['Algorithm'].unique()

    for algo in algorithms:
        subset = df[df['Algorithm'] == algo]
        
        plt.figure(figsize=(10, 6))
        
        # Plot Time vs SizeG (assuming SizeH scales with SizeG in our runner)
        sns.lineplot(data=subset, x="SizeG", y="TimeMS", marker="o", linewidth=2.5)
        
        plt.title(f"Performance Analysis: {algo.upper()} Algorithm")
        plt.xlabel("Graph Size |V(G)|")
        plt.ylabel("Execution Time (ms)")
        
        # Log scale for Exact usually makes sense due to exponential nature
        if algo == "exact":
            plt.yscale("log")
            plt.ylabel("Execution Time (ms) - Log Scale")
        
        plt.tight_layout()
        output_file = f"results_{algo}.png"
        plt.savefig(output_file, dpi=300)
        print(f"Saved plot to {output_file}")

if __name__ == "__main__":
    main()
