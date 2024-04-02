import pandas as pd
import matplotlib.pyplot as plt

# Load the CSV file
file_path = 'src/5500_sensor_data_4.0_ao.csv'  # Make sure this path is correct based on your file's location
data = pd.read_csv(file_path)

# Filter out rows where '|Diff|' is greater than 500
print(len(data))
data = data[data['|Diff|'] <= 3000]
print(len(data))

# Filter out rows where MQ-4 > 10000
print(len(data))
data = data[data['Nano'] <= 10000]
print(len(data))

# Filter out rows where 'Nano' column is 0 and diff >= 500
print(len(data))
# data = data[(data['Nano'] != 0) | (data['|Diff|'] <= 100)]
data = data[(data['Nano'] != 0)]
data = data[(data['MQ-4'] != 0)]
print(len(data))

# Prepare the data for plotting
x = data.index * 2  # Row number as x-axis, with each row representing a 2-second interval
y1 = data['MQ-4']  # MQ-4 sensor data
y2 = data['Nano']  # Nano sensor data
y3 = data['|Diff|']  # Absolute difference
# y3 = data['|Diff|']  # Absolute difference


# x = x[:500]
# y1 = y1[:500]
# y2 = y2[:500]
# y3 = y3[:500]

# Plotting the data
plt.figure(figsize=(14, 8))
plt.plot(x, y1, label='MQ-4', marker='o', markersize=5) # linestyle='-'
plt.plot(x, y2, label='Nano', marker='x', markersize=5)
plt.plot(x, y3, label='|Diff|', marker='^', markersize=5)

# Adding title and labels
plt.title('Sensor Data Visualization')
plt.xlabel('Time (seconds)')
plt.ylabel('Sensor Values')
plt.legend()
plt.grid(True)

# Display the plot
plt.show()
