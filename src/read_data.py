import serial
import re

# Serial port configuration
serial_port = '/dev/tty.usbmodem11202'  # Change to your serial port
baud_rate = 9600

# Directory where the CSV file will be saved
csv_file_directory = './IoT DR/sensor data/'

# Adjusted regular expression pattern to match and capture three floating-point numbers
data_pattern = re.compile(r'^(-?\d+\.\d+),(-?\d+\.\d+),(-?\d+\.\d+)$')

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
                        csv_file_path = f'{csv_file_directory}{first_number}_sensor_data.csv'
                        file = open(csv_file_path, 'w')  # Open the CSV file for writing
                        print(f"Saving sensor data to {csv_file_path}...")
                        # Adjusted CSV header with new columns
                        file.write("AO,MQ-4,Nano,Diff,|Diff|\n")
                        file_opened = True
                    else:
                        # Extract and process the numbers
                        ao, mq4, nano = match.groups()
                        mq4_int = int(float(mq4))  # Convert to integer (keep integer part)
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
