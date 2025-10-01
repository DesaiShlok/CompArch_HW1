import matplotlib.pyplot as plt
import numpy as np
import os

def analyze_and_plot_timings(filepath='outNs.txt'):
    """
    Reads memcpy timing data from a file, calculates statistics,
    identifies outliers, and plots the results.

    Args:
        filepath (str): The path to the input file containing timing data.
    """
    # --- 1. Read and Parse the Data ---
    if not os.path.exists(filepath):
        print(f"Error: The file '{filepath}' was not found.")
        # Create a dummy file for demonstration if it doesn't exist.
        print("Creating a sample 'outNs.txt' file for demonstration.")
        with open(filepath, 'w') as f:
            sample_data = np.random.normal(loc=100, scale=5, size=500).astype(int)
            # Add some outliers
            sample_data = np.append(sample_data, [200, 250, 30, 280])
            np.random.shuffle(sample_data)
            f.write('\n'.join(map(str, sample_data)))
    
    try:
        with open(filepath, 'r') as f:
            # Read each line, strip whitespace, and convert to integer
            timings = [int(line.strip()) for line in f if line.strip()]
        if not timings:
            print("Error: The file is empty or contains no valid numbers.")
            return
    except ValueError:
        print(f"Error: The file '{filepath}' contains non-numeric data.")
        return
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        return

    # --- 2. Perform Statistical Analysis ---
    timings_np = np.array(timings)
    
    # Calculate mean and median
    mean_time = np.mean(timings_np)
    median_time = np.median(timings_np)

    # Identify outliers using the Interquartile Range (IQR) method
    q1, q3 = np.percentile(timings_np, [25, 75])
    iqr = q3 - q1
    lower_bound = q1 - (1.5 * iqr)
    upper_bound = q3 + (1.5 * iqr)

    # Filter out the outliers
    outliers = timings_np[(timings_np < lower_bound) | (timings_np > upper_bound)]
    outlier_indices = np.where((timings_np < lower_bound) | (timings_np > upper_bound))[0]

    print("--- Timing Analysis Results ---")
    print(f"Total Readings: {len(timings_np)}")
    print(f"Mean Time:      {mean_time:.2f} ns")
    print(f"Median Time:    {median_time:.2f} ns")
    print(f"Identified Outliers ({len(outliers)}): {outliers}")
    print("-----------------------------")

    # --- 3. Plot the Data ---
    plt.style.use('seaborn-v0_8-whitegrid')
    fig, ax = plt.subplots(figsize=(12, 7))

    # Plot all data points
    # Using a smaller, semi-transparent marker to see data density
    ax.scatter(range(len(timings_np)), timings_np, label='Data Points', color='royalblue', alpha=0.6, s=10)

    # Highlight outliers
    if len(outliers) > 0:
        ax.scatter(outlier_indices, outliers, color='red', marker='x', s=50, label='Outliers', zorder=5)

    # Plot mean and median lines
    ax.axhline(mean_time, color='darkorange', linestyle='--', linewidth=2, label=f'Mean: {mean_time:.2f} ns')
    ax.axhline(median_time, color='green', linestyle=':', linewidth=2, label=f'Median: {median_time:.2f} ns')

    # --- 4. Customize the Plot ---
    ax.set_title('Memory Copy times for 64B', fontsize=16)
    ax.set_xlabel('Sample Number', fontsize=12)
    ax.set_ylabel('Time (nanoseconds)', fontsize=12)
    ax.legend(fontsize=10)
    ax.grid(True, which='both', linestyle='-', linewidth=0.5)
    
    # Set a sensible y-axis limit to focus on the bulk of the data, but ensure outliers are visible
    if len(timings_np) > len(outliers):
        non_outliers = timings_np[(timings_np >= lower_bound) & (timings_np <= upper_bound)]
        if len(non_outliers) > 0:
            y_min = non_outliers.min() * 0.95
            y_max = non_outliers.max() * 1.05
            ax.set_ylim([min(y_min, timings_np.min()*0.95), max(y_max, timings_np.max()*1.05)])
            
    plt.tight_layout()
    plt.show()


if __name__ == '__main__':
    # The script will look for 'outNs.txt' in the same directory it is run from.
    # Make sure your data file is placed there.
    analyze_and_plot_timings('outNs.txt')
