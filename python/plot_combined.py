import pandas as pd
import matplotlib.pyplot as plt

# Load the data from the files
animation_base = 'pulse'
file_name_quadratic = f'{animation_base}_quadratic'
file_name_exponential = f'{animation_base}_exp'
file_name_sine = f'{animation_base}_sine'
file_name_sine_opt = f'{animation_base}_sine_opt'

file_path_quadratic = f'data/{file_name_quadratic}.csv'
file_path_exponential = f'data/{file_name_exponential}.csv'
file_path_sine = f'data/{file_name_sine}.csv'
file_path_sine_opt = f'data/{file_name_sine_opt}.csv'

# Read CSV data
# data_quadratic = pd.read_csv(file_path_quadratic, sep=';', header=None, names=['Time', 'Brightness'])
# data_exponential = pd.read_csv(file_path_exponential, sep=';', header=None, names=['Time', 'Brightness'])
data_sine = pd.read_csv(file_path_sine, sep=';', header=None, names=['Time', 'Brightness'])
data_sine_opt = pd.read_csv(file_path_sine_opt, sep=';', header=None, names=['Time', 'Brightness'])

# Plot all the data on the same plot with different labels and colors
plt.figure(figsize=(10, 6))  # Optional: Set the figure size
# plt.plot(data_quadratic['Time'], data_quadratic['Brightness'], label='Quadratic', color='red')
# plt.plot(data_exponential['Time'], data_exponential['Brightness'], label='Exponential', color='green')
plt.plot(data_sine['Time'], data_sine['Brightness'], label='Sine', color='blue')
plt.plot(data_sine_opt['Time'], data_sine_opt['Brightness'], label='Sine Optimized', color='purple')

#define USE_LED_ANIMATION_QUADRATIC (0)
#define USE_LED_ANIMATION_EXPONENTIAL (0)
#define USE_LED_ANIMATION_SINE (0)
#define USE_LED_ANIMATION_SINE_APPROX (1)

# Adding labels, title, and legend
plt.xlabel('Time (ms)')
plt.ylabel('Brightness MAX(1000)')
plt.title(f'Waveform Comparison for {animation_base.capitalize()} Animation')
plt.legend()

# Save the plot as a PNG file
plt.savefig(f'img/comparison/{animation_base}_comparison.png')

# Optionally, display the plot
plt.show()
