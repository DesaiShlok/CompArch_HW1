def main():
    # Take CPU frequency once
    cpu_freq_mhz = float(input("Enter CPU frequency in MHz: "))

    print("Enter tick values (type 'q' to quit):")
    while True:
        ticks = input("Ticks: ")
        if ticks.lower() == 'q':
            break
        try:
            ticks = int(ticks)
            time_ns = (ticks * 1000) / cpu_freq_mhz
            print(f"Time: {time_ns:.2f} ns")
        except ValueError:
            print("Invalid input, please enter an integer tick value or 'q' to quit.")

if __name__ == "__main__":
    main()
