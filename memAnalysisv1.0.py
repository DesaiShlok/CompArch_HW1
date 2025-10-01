import os
import re
import pandas as pd
import numpy as np
import plotly.graph_objects as go
from plotly.subplots import make_subplots

# --- Configuration ---
OUTPUT_DIR = "memcpy_interactive_plots"
FILE_PREFIX = "UsingMTest"
OUTPUT_FILENAME = "consolidated_memcpy_report.html"
CPU_FREQ = "2100 Mhz" # <-- New: Add your CPU frequency here

def parse_data_files(directory="."):
    """Finds and parses all 'ticks' and 'time' data files."""
    data_records = {}
    file_pattern = re.compile(rf'.*{FILE_PREFIX}_2_(\d+)\.txt')
    print("🔎 Finding and parsing data files...")
    for filename in os.listdir(directory):
        match = file_pattern.match(filename)
        if not match: continue
        exponent = int(match.group(1))
        size_bytes = 2**exponent
        if size_bytes not in data_records:
            data_records[size_bytes] = {'size_bytes': size_bytes, 'exponent': exponent}
        try:
            with open(os.path.join(directory, filename), 'r') as f:
                values = [int(line.strip()) for line in f if line.strip()]
            if "ticks" in filename:
                data_records[size_bytes]['ticks'] = np.array(values)
            elif "time" in filename:
                data_records[size_bytes]['time_ns'] = np.array(values)
        except Exception as e:
            print(f"Could not read or parse {filename}: {e}")
    sorted_records = sorted(data_records.values(), key=lambda x: x['size_bytes'])
    print(f"✅ Found data for {len(sorted_records)} different sizes.")
    return sorted_records

def create_enhanced_summary_plot(all_data):
    """
    Creates a summary plot with mean and a shaded confidence interval for standard deviation.
    """
    df = pd.DataFrame(all_data)
    df['mean_ticks'] = df['ticks'].apply(np.mean)
    df['std_ticks'] = df['ticks'].apply(np.std)
    df['mean_time'] = df['time_ns'].apply(np.mean)
    df['std_time'] = df['time_ns'].apply(np.std)

    fig = make_subplots(rows=1, cols=2, subplot_titles=("Avg. CPU Ticks vs. Size", "Avg. Time vs. Size"))

    # Ticks Plot
    fig.add_trace(go.Scatter(x=df['size_bytes'], y=df['mean_ticks'], mode='lines+markers', name='Mean Ticks'), row=1, col=1)
    fig.add_trace(go.Scatter(x=np.concatenate([df['size_bytes'], df['size_bytes'][::-1]]), y=np.concatenate([df['mean_ticks'] + df['std_ticks'], (df['mean_ticks'] - df['std_ticks'])[::-1]]), fill='toself', fillcolor='rgba(0,100,80,0.2)', line=dict(color='rgba(255,255,255,0)'), hoverinfo="skip", name='±1 std dev'), row=1, col=1)

    # Time Plot
    fig.add_trace(go.Scatter(x=df['size_bytes'], y=df['mean_time'], mode='lines+markers', name='Mean Time', line=dict(color='firebrick')), row=1, col=2)
    fig.add_trace(go.Scatter(x=np.concatenate([df['size_bytes'], df['size_bytes'][::-1]]), y=np.concatenate([df['mean_time'] + df['std_time'], (df['mean_time'] - df['std_time'])[::-1]]), fill='toself', fillcolor='rgba(211,63,63,0.2)', line=dict(color='rgba(255,255,255,0)'), hoverinfo="skip", name='±1 std dev'), row=1, col=2)

    # Annotations
    fig.add_vline(x=32*1024, line_dash="dash", line_color="gray", annotation_text="L1 Cache?", annotation_position="top left", row=1, col=1)
    fig.add_vline(x=256*1024, line_dash="dash", line_color="gray", annotation_text="L2 Cache?", annotation_position="top left", row=1, col=1)
    fig.add_vline(x=8*1024*1024, line_dash="dash", line_color="gray", annotation_text="L3 Cache?", annotation_position="top left", row=1, col=1)
    
    fig.update_layout(title_text='Enhanced Summary: Average Performance & Variability vs. Data Size')
    fig.update_xaxes(type='log', title_text="Data Size (Bytes)")
    fig.update_yaxes(type='log', title_text="Execution Ticks")
    fig.update_yaxes(title_text="Execution Time (ns)", row=1, col=2)
    return fig

def create_violin_comparison_plot(long_df_with_stats):
    """
    Creates a single figure with side-by-side violin plots for both ticks and time,
    with customized hover labels, using pre-calculated stats.
    """
    hovertemplate = ("<b>Value: %{y:.2f}</b><br><br>" + "--- Distribution Stats ---<br>" + "Mean: %{customdata[0]:.2f}<br>" + "Min: %{customdata[1]}<br>" + "Max: %{customdata[2]}<br>" + "Variance: %{customdata[3]:.2f}<br>" + "<extra></extra>")
    fig = make_subplots(rows=2, cols=1, shared_xaxes=True, subplot_titles=("Ticks Distribution", "Time (ns) Distribution"))
    
    # Ticks Plot
    ticks_df = long_df_with_stats[long_df_with_stats['type'] == 'Ticks']
    fig.add_trace(go.Violin(x=ticks_df['size'], y=ticks_df['value'], name='Ticks', box_visible=True, meanline_visible=True, customdata=ticks_df[['mean', 'min', 'max', 'variance']], hovertemplate=hovertemplate), row=1, col=1)
    
    # Time Plot
    time_df = long_df_with_stats[long_df_with_stats['type'] == 'Time (ns)']
    fig.add_trace(go.Violin(x=time_df['size'], y=time_df['value'], name='Time (ns)', box_visible=True, meanline_visible=True, marker_color='darkred', customdata=time_df[['mean', 'min', 'max', 'variance']], hovertemplate=hovertemplate), row=2, col=1)

    fig.update_layout(title_text='Performance Distribution Across All Data Sizes', showlegend=False)
    fig.update_yaxes(title_text="Execution Ticks (Log)", type="log", row=1, col=1)
    fig.update_yaxes(title_text="Execution Time (ns, Log)", type="log", row=2, col=1)
    fig.update_xaxes(title_text="Data Size", row=2, col=1)
    return fig

def create_summary_table_html(stats_df):
    """
    New: Creates a styled HTML table from the summary statistics DataFrame.
    """
    # Pivot the table to a "wide" format which is better for display
    table_df = stats_df.pivot_table(
        index=['size', 'size_bytes'],
        columns='type',
        values=['mean', 'min', 'max', 'variance']
    ).sort_values('size_bytes')

    # Clean up the column names (e.g., from ('mean', 'Ticks') to 'Mean Ticks')
    table_df.columns = [f'{stat.capitalize()} {type_}' for stat, type_ in table_df.columns]
    table_df = table_df.reset_index().drop(columns='size_bytes') # Use 'size' as the main index label
    table_df.rename(columns={'size': 'Data Size'}, inplace=True)
    
    # Convert the DataFrame to an HTML string with styling
    return table_df.to_html(classes='stats-table', index=False, float_format='{:.2f}'.format)

def main():
    """Main function to run the analysis and create a single HTML report."""
    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)
        
    all_data = parse_data_files()
    if not all_data: return

    # --- Centralized Data Processing ---
    # Reshape data into a "long format" for easy plotting
    long_df_list = []
    for record in all_data:
        size_str = f"2^{record['exponent']}"
        if 'ticks' in record:
            for val in record['ticks']: long_df_list.append({'size': size_str, 'size_bytes': record['size_bytes'], 'type': 'Ticks', 'value': val})
        if 'time_ns' in record:
            for val in record['time_ns']: long_df_list.append({'size': size_str, 'size_bytes': record['size_bytes'], 'type': 'Time (ns)', 'value': val})
    long_df = pd.DataFrame(long_df_list)

    # Calculate summary stats once
    stats_df = long_df.groupby(['size', 'size_bytes', 'type']).agg(
        mean=('value', 'mean'), min=('value', 'min'), max=('value', 'max'), variance=('value', 'var')
    ).reset_index()
    long_df_with_stats = pd.merge(long_df, stats_df, on=['size', 'size_bytes', 'type'])

    # --- Report Generation ---
    print("📊 Generating enhanced summary plot...")
    summary_fig = create_enhanced_summary_plot(all_data)

    print("🎻 Generating consolidated violin plot...")
    violin_fig = create_violin_comparison_plot(long_df_with_stats)

    print("📝 Generating summary statistics table...")
    stats_table_html = create_summary_table_html(stats_df)

    # --- Write everything to a single HTML file ---
    report_path = os.path.join(OUTPUT_DIR, OUTPUT_FILENAME)
    print(f"\n✍️ Writing consolidated report to: {report_path}")
    with open(report_path, 'w') as f:
        # Add a header with some simple CSS for the table
        f.write("<html><head><title>Memcpy Performance Analysis</title><style>")
        f.write("body { font-family: sans-serif; } h1, h2 { color: #333; }")
        f.write(".stats-table { border-collapse: collapse; margin: 25px 0; font-size: 0.9em; min-width: 600px; box-shadow: 0 0 20px rgba(0, 0, 0, 0.15); }")
        f.write(".stats-table thead tr { background-color: #009879; color: #ffffff; text-align: left; }")
        f.write(".stats-table th, .stats-table td { padding: 12px 15px; }")
        f.write(".stats-table tbody tr { border-bottom: 1px solid #dddddd; }")
        f.write(".stats-table tbody tr:nth-of-type(even) { background-color: #f3f3f3; }")
        f.write(".stats-table tbody tr:last-of-type { border-bottom: 2px solid #009879; }")
        f.write("</style></head><body>")
        
        # New heading with CPU info
        f.write(f"<h1>Memcpy Performance Analysis Report</h1>")
        f.write(f"<p><i>Measurements were done at CPU frequency: <b>{CPU_FREQ}</b></i></p>")
        
        f.write("<hr><h2 style='font-family: sans-serif;'>Summary Trend with Confidence Interval</h2>")
        f.write(summary_fig.to_html(full_html=False, include_plotlyjs='cdn'))
        
        f.write("<hr><h2 style='font-family: sans-serif;'>Distribution Comparison (Violin Plots)</h2>")
        f.write(violin_fig.to_html(full_html=False, include_plotlyjs='cdn'))
        
        # New table section
        f.write("<hr><h2 style='font-family: sans-serif;'>Summary Statistics</h2>")
        f.write(stats_table_html)
        
        f.write("</body></html>")
        
    print("\n🎉 Analysis complete!")

if __name__ == '__main__':
    main()