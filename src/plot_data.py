import pandas as pd
import matplotlib.pyplot as plt

# Load the CSV file
file_path = 'src/5550.0_sensor_data.csv'
data = pd.read_csv(file_path)

# Prepare the data for plotting
x = data.index  # Row number as x-axis
y1 = data['MQ-4']  # MQ-4 sensor data
y2 = data['Nano']  # Nano sensor data
y3 = data['|Diff|']  # Absolute difference

# Plotting the data
plt.figure(figsize=(14, 8))
plt.plot(x, y1, label='MQ-4', marker='o', linestyle='-', markersize=5)
plt.plot(x, y2, label='Nano', marker='x', linestyle='-', markersize=5)
plt.plot(x, y3, label='|Diff|', marker='^', linestyle='-', markersize=5)

# Adding title and labels
plt.title('Sensor Data Visualization')
plt.xlabel('Row Number')
plt.ylabel('Sensor Values')
plt.legend()
plt.grid(True)

# Display the plot
plt.show()
