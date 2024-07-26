import pandas as pd
import matplotlib.pyplot as plt

# Load the data from the file
file_name = 'pwm_led_breath'
file_path = f'data/{file_name}.csv'
data = pd.read_csv(file_path, sep=';', header=None, names=['Time', 'Brightness'])

# Plotting the FadeIn sequence
plt.figure(figsize=(10, 6))
plt.plot(data['Time'], data['Brightness'], marker='o', linestyle='-', color='blue')
plt.title(f'{file_name} Sequence')
plt.xlabel('Time (ms)')
plt.ylabel('Brightness Level, MAX_DUTY_CYCLE = 1000')
plt.grid(True)

# Save the plot as a PNG file
plt.savefig(f'img/{file_name}.png')

# Optionally, you can still call plt.show() and it will work fine when you run the script in an environment that supports it
plt.show()

