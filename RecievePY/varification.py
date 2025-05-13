import argparse
import pandas as pd
import matplotlib.pyplot as plt

def plot_compare(original_csv, interp_csv, channel='ax'):
    """
    Plot original and interpolated IMU data for the specified channel
    in two vertically stacked, linked plots for zoom/pan.
    """
    # Load the data
    orig_df = pd.read_csv(original_csv, sep=",")
    interp_df = pd.read_csv(interp_csv, sep=",")

    # Time axes
    t_orig = orig_df['t_rel']
    t_interp = interp_df['t_sec']

    # Create linked subplots
    fig, (ax1, ax2) = plt.subplots(2, 1, sharex=True, figsize=(10, 6))
    ax1.plot(t_orig, orig_df[channel], label='Original')
    ax1.set_ylabel(channel)
    ax1.set_title('Original IMU Data')
    ax1.grid(True)

    ax2.plot(t_interp, interp_df[channel], label='Interpolated')
    ax2.set_ylabel(channel)
    ax2.set_title('Interpolated IMU Data')
    ax2.set_xlabel('Time (s)')
    ax2.grid(True)

    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Compare original vs. interpolated IMU data")
    parser.add_argument("--original_csv", required=True, help="Path to the original IMU CSV")
    parser.add_argument("--interp_csv", required=True, help="Path to the interpolated IMU CSV")
    parser.add_argument("--channel", default="ax", help="IMU channel to plot (e.g., ax, ay, az, gx, gy, gz)")
    args = parser.parse_args()

    plot_compare(args.original_csv, args.interp_csv, args.channel)