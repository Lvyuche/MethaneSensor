import serial
import re

import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import PolynomialFeatures
from sklearn.linear_model import LinearRegression
import matplotlib.pyplot as plt

# Load the CSV file and preprocess
file_path = 'sensor/5500_sensor_data_4.0.csv'
data = pd.read_csv(file_path)

# data = data[data['|Diff|'] <= 3000]
data = data[data['Nano'] <= 10000]
# data = data[(data['Nano'] != 0) | (data['|Diff|'] <= 500)]
data = data[(data['MQ-4'] != 0)]
data = data[(data['Nano'] != 0)]

# Prepare the data
X = data['AO'].values.reshape(-1, 1)
y = data['Nano'].values
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Create polynomial features
degree = 2
poly_features = PolynomialFeatures(degree=degree)
X_poly_train = poly_features.fit_transform(X_train)

# Train the model
model = LinearRegression()
model.fit(X_poly_train, y_train)

# Predict and plot
X_poly = poly_features.transform(X)
y_pred = model.predict(X_poly)

def mapping (mq4_reading):
    c, b, a = model.coef_
    intercept = model.intercept_
    nano_reading = a * mq4_reading * mq4_reading + b * mq4_reading + intercept

    return nano_reading

# Serial port configuration
serial_port = '/dev/tty.usbmodem1202'  # Change to your serial port
baud_rate = 9600
R0 = 5500

# Directory where the CSV file will be saved
csv_file_directory = 'src/'

# Adjusted regular expression pattern to match and capture three floating-point numbers
data_pattern = re.compile(r'^(-?\d+\.\d+),(-?\d+\.\d+)$')

def save_sensor_data():
    with serial.Serial(serial_port, baud_rate, timeout=1) as ser:
        print("Listening for sensor data...")
        file_opened = False
        
        try:
            while True:
                line = ser.readline().decode('utf-8').strip()  # Read and decode the line
                match = data_pattern.match(line)
                if match:  # Check if the line matches the pattern
                    if not file_opened:
                        first_number = match.group(1)  # Extract the first number for file name
                        csv_file_path = f'{csv_file_directory}{R0}_sensor_data_4.0_ao.csv'
                        file = open(csv_file_path, 'a')  # Open the CSV file for writing
                        print(f"Saving sensor data to {csv_file_path}...")
                        # Adjusted CSV header with new columns
                        file.write("AO,MQ-4,Nano,Diff,|Diff|\n")
                        file_opened = True
                    else:

                        # Extract and process the numbers
                        ao, nano = match.groups()
                        ao = float(ao)
                        # sensor_volt = ao * (3.54)
                        # RS_gas = ((3.54 - sensor_volt) - 1.0) * 1000 / sensor_volt
                        # ratio = RS_gas / R0
                        # ppm = 1000*pow((ratio),-2.95)

                        ppm = mapping(ao)
                        mq4_int = int(float(ppm))  # Convert to integer (keep integer part)
                        nano_int = int(float(nano))  # Convert to integer (keep integer part)
                        diff = mq4_int - nano_int
                        abs_diff = abs(diff)
                        
                        # Construct the new line with added columns
                        new_line = f"{ao},{mq4_int},{nano_int},{diff},{abs_diff}\n"
                        
                        print(f"Data: {new_line}")  # Print the modified data to console
                        file.write(new_line)  # Write the new line to the CSV file
                        file.flush()  # Ensure data is written to file immediately
        except KeyboardInterrupt:
            print("Stopped by user.")
        finally:
            if file_opened:
                file.close()  # Make sure to close the file if it was opened

if __name__ == "__main__":
    save_sensor_data()
