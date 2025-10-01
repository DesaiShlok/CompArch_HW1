import os
import re
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns

def analyze_benchmark_data(directory='.'):
    """
    Finds, parses, and analyzes benchmark data files in a given directory.

    Args:
        directory (str): The path to the directory containing the data files.
                         Defaults to the current directory.
    """
    # Regex to parse filenames like 'timeUsingCalcTimes_2_6.txt'
    # It captures the function name ('CalcTimes') and the power of 2 ('6')
    file_pattern = re.compile(r"timeUsing(\w+)_2_(\d+)\.txt")

    # Dictionary to hold the collected data, grouped by function name
    # e.g., {'CalcTimes': {6: [t1, t2, ...], 7: [t1, t2, ...]}, ...}
    collected_data = {}

    print(f"Searching for data files in '{os.path.abspath(directory)}'...")

    # --- 1. Data Collection ---
    for filename in os.listdir(directory):
        match = file_pattern.match(filename)
        if match:
            function_name = match.group(1)
            power_of_2 = int(match.group(2))
            
            # Initialize dictionary for a new function if not seen before
            if function_name not in collected_data:
                collected_data[function_name] = {}

            try:
                filepath = os.path.join(directory, filename)
                with open(filepath, 'r') as f:
                    # Read all lines, convert to int, and filter out empty lines
                    times_ns = [int(line.strip()) for line in f if line.strip()]
                    if times_ns:
                        collected_data[function_name][power_of_2] = times_ns
                        print(f"  - Loaded {len(times_ns)} samples from {filename}")
                    else:
                        print(f"  - Warning: {filename} is empty.")

            except (IOError, ValueError) as e:
                print(f"  - Error reading or parsing {filename}: {e}")

    if not collected_data:
        print("\nNo data files found matching the pattern 'timeUsing<FunctionName>_2_<Power>.txt'.")
        print("Please make sure the script is in the same directory as your data files.")
        return

    # --- 2. Statistical Analysis ---
    # Dictionary to hold the calculated statistics for each dataset
    analysis_results = {}
    
    for function_name, data_by_power in collected_data.items():
        analysis_results[function_name] = {}
        for power_of_2, times_ns in data_by_power.items():
            analysis_results[function_name][power_of_2] = {
                'mean': np.mean(times_ns),
                'median': np.median(times_ns),
                'min': np.min(times_ns),
                'max': np.max(times_ns),
                'std_dev': np.std(times_ns),
                'count': len(times_ns)
            }

    # --- 3. Print Summary Table ---
    print("\n--- Benchmark Analysis Summary ---")
    for function_name in sorted(analysis_results.keys()):
        print(f"\nFunction: {function_name}")
        print("-" * 65)
        print(f"{'Data Size (2^N)':<18} {'Mean (ns)':<12} {'Median (ns)':<12} {'Std Dev (ns)':<15} {'Samples':<8}")
        print("-" * 65)

        # Sort by power of 2 for clean output
        sorted_powers = sorted(analysis_results[function_name].keys())
        
        for power_of_2 in sorted_powers:
            stats = analysis_results[function_name][power_of_2]
            size_str = f"2^{power_of_2}"
            mean_str = f"{stats['mean']:.2f}"
            median_str = f"{stats['median']:.2f}"
            std_dev_str = f"{stats['std_dev']:.2f}"
            count_str = str(stats['count'])
            print(f"{size_str:<18} {mean_str:<12} {median_str:<12} {std_dev_str:<15} {count_str:<8}")
        print("-" * 65)
        
    # --- 4. Generate and Save Individual Distribution Plots ---
    plots_dir = 'plots'
    os.makedirs(plots_dir, exist_ok=True)
    print(f"\nSaving individual distribution plots to '{plots_dir}/' folder...")

    for function_name, data_by_power in collected_data.items():
        for power_of_2, times_ns in data_by_power.items():
            # Create a new figure for each individual plot
            fig_ind, ax_ind = plt.subplots(figsize=(10, 6))
            
            # Use Seaborn for a combined boxplot (for stats/outliers) and stripplot (to see all samples)
            sns.boxplot(data=times_ns, ax=ax_ind, color='skyblue')
            sns.stripplot(data=times_ns, ax=ax_ind, color='black', size=4, jitter=0.2)

            ax_ind.set_title(f"Execution Time Distribution for {function_name} (Size: 2^{power_of_2})", fontsize=16)
            ax_ind.set_ylabel("Execution Time (nanoseconds)", fontsize=12)
            ax_ind.set_xlabel("Data Samples", fontsize=12)
            ax_ind.tick_params(axis='x', which='both', bottom=False, top=False, labelbottom=False) # Hide x-axis labels

            # Construct filename and save the plot
            plot_filename = os.path.join(plots_dir, f"{function_name}_2_{power_of_2}_distribution.png")
            plt.savefig(plot_filename)
            plt.close(fig_ind) # Close the figure to free memory and prevent it from being displayed
            print(f"  - Saved: {plot_filename}")


    # --- 5. Plotting and Showing Main Summary Result ---
    plt.style.use('seaborn-v0_8-whitegrid')
    fig, ax = plt.subplots(figsize=(12, 7))

    for function_name in sorted(analysis_results.keys()):
        # Sort data by power of 2 to ensure lines are drawn correctly
        sorted_powers = sorted(analysis_results[function_name].keys())
        
        x_values = [p for p in sorted_powers]
        y_values_mean = [analysis_results[function_name][p]['mean'] for p in sorted_powers]
        
        # Plot the mean execution time
        ax.plot(x_values, y_values_mean, marker='o', linestyle='-', label=f"{function_name} (Mean)")

    ax.set_title('Code Execution Time vs. Data Size', fontsize=16)
    ax.set_xlabel('Data Size (N in 2^N bytes)', fontsize=12)
    ax.set_ylabel('Average Execution Time (nanoseconds)', fontsize=12)
    
    # Use a logarithmic scale for the y-axis if times vary greatly
    ax.set_yscale('log')
    ax.legend()
    ax.grid(True, which="both", ls="--")
    
    # Set x-axis ticks to be integers
    if collected_data:
        all_powers = [p for d in collected_data.values() for p in d.keys()]
        if all_powers:
            max_power = max(all_powers)
            min_power = min(all_powers)
            ax.set_xticks(range(min_power, max_power + 1))
    
    plt.tight_layout()
    print("\nDisplaying summary plot. Close the plot window to exit.")
    plt.show()


if __name__ == "__main__":
    # Run the analysis in the current directory where the script is located.
    analyze_benchmark_data()

