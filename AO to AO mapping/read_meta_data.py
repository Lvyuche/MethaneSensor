import serial
import re

# Serial port configuration
serial_port = '/dev/tty.usbmodem11202'  # Change to your serial port
baud_rate = 9600

# Regular expression pattern to match two floating-point numbers
data_pattern = re.compile(r'^\s*(-?\d*\.?\d+)\s*,\s*(-?\d*\.?\d+)\s*$')

def save_sensor_data():
    with serial.Serial(serial_port, baud_rate, timeout=1) as ser:
        print("Listening for sensor data...")
        file_opened = False

        try:
            while True:
                line = ser.readline().decode('utf-8').strip()  # Read and decode the line
                print(f"Read line: {line}")  # Debug print the raw line
                match = data_pattern.match(line)
                if match:  # Check if the line matches the pattern
                    ao, ao_real = match.groups()
                    print(f"Matched AO: {ao}, AO_Real: {ao_real}")  # Print matched data
                    
                    if not file_opened:
                        csv_file_path = 'meta_data/5_1.csv'
                        file = open(csv_file_path, 'a')  # Open the CSV file for appending
                        print(f"Saving sensor data to {csv_file_path}...")
                        file.write("AO,AO_Real\n")  # Write header if it's the first data entry
                        file_opened = True

                    file.write(f"{ao},{ao_real}\n")  # Write the new line to the CSV file
                    file.flush()  # Ensure data is written to file immediately
                else:
                    print("No match found.")  # Debug for no match

        except KeyboardInterrupt:
            print("Stopped by user.")
        except Exception as e:
            print(f"Error: {e}")  # Print any exceptions that occur
        finally:
            if file_opened:
                file.close()  # Make sure to close the file if it was opened
            print("File closed and script stopped.")

if __name__ == "__main__":
    save_sensor_data()
