#include "dot_util.h"
#include "RadioEvent.h"
// #include "DigitalOut.h"
// #include "mbed.h"

#define SAMPCNT 1
#define READBUF 70

#define CHANNEL_PLAN CP_US915
/////////////////////////////////////////////////////////////
// * these options must match the settings on your gateway //
// * edit their values to match your configuration         //
// * frequency sub band is only relevant for the 915 bands //
// * either the network name and passphrase can be used or //
//     the network ID (8 bytes) and KEY (16 bytes)         //
/////////////////////////////////////////////////////////////
static std::string network_name = "mgasproject";
static std::string network_passphrase = "mgasproject";
//static uint8_t network_id[] = { 0x6C, 0x4E, 0xEF, 0x66, 0xF4, 0x79, 0x86, 0xA6 };
//static uint8_t network_key[] = { 0x1F, 0x33, 0xA1, 0x70, 0xA5, 0xF1, 0xFD, 0xA0, 0xAB, 0x69, 0x7A, 0xAE, 0x2B, 0x95, 0x91, 0x6B };
static uint8_t frequency_sub_band = 7;
static lora::NetworkType network_type = lora::PUBLIC_LORAWAN;
static uint8_t join_delay = 5;
static uint8_t ack = 0;
static bool adr = true;

uint8_t cmd_meas[] = {0x61, 0x00, 0x01, 0x00, 0x00, 0x00, 0x57, 0x93, 0x02};
uint8_t cmd_data[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xed, 0x76};
uint8_t read_buf[READBUF];

// sleep variables
uint8_t boot=1;
uint8_t pcycle=0xff;

std::vector<uint8_t> device_id;
int d_id_size;

mDot* dot = NULL;
lora::ChannelPlan* plan = NULL;

mbed::UnbufferedSerial pc(USBTX, USBRX);
BufferedSerial pUART(UART1_TX, UART1_RX);

// Alert LED instance
DigitalOut led(LED1);

// Blinking rate in milliseconds
#define BLINKING_RATE     1000ms

// Assuming the MQ-4 sensor's AO pin is connected to pin A0 (analog input) (wake up PIN)
AnalogIn mq4Sensor(PA_0);
DigitalOut transistor(GPIO2);
// Read the analog value (0.0 to 1.0) from the sensor's AO pin
float sensorValue;
int R0 = 5570;

// Define model coefficients and intercept
double a = -9.62136052e-05;
double b = 2.44096051e+00;
double c = -3453.797143035675;

// Function to apply the mapping
double mapping(double mq4_reading) {
    // Apply the polynomial mapping
    double nano_reading = a * mq4_reading * mq4_reading + b * mq4_reading + c;
    return nano_reading;
}

void measurement () {
    // Convert the reading to a more meaningful value (e.g., ppm)
    // This conversion requires calibration with known methane concentrations
    // For demonstration, we'll just output the raw analog value
    sensorValue = mq4Sensor.read();
    std::printf("%f,", sensorValue); // MQ-4 sensor value (raw)
    float sensor_volt = sensorValue * (5.0); //Convert analog values to voltage
    float RS_gas = ((5.0 - sensor_volt) - 1.0) * 1000 / sensor_volt; //Get value of RS in a gas
    float ratio = RS_gas / R0;   // Get ratio RS_gas/RS_air
    double ppm = 1000*pow((ratio),-2.95);

    ppm = mapping(ppm);

    // std::printf("%f,", ratio); // MQ-4 sensor value (ratio)
    std::printf("%f,", ppm); // MQ-4 sensor value (ppm)
    led = !led;
    // vcc = vcc == 1 ? 0: 1;
    ThisThread::sleep_for(BLINKING_RATE);
}

// Put xDot to sleep
void dot_sleep(uint32_t sec) {
    // Put IO to sleep
	sleep_save_io();
	sleep_configure_io();
    // go to sleep sec seconds and wake using the RTC alarm
    dot->sleep(sec, mDot::RTC_ALARM, false);

	// restore the GPIO state.
	sleep_restore_io();    
}

// Initialize sensor
bool sensor_start() {
    int tries = 5;
    bool success = false;
    int rcnt = 0;
    while ((tries>0)&&(!success)) {
        pUART.write(cmd_meas, sizeof(cmd_meas));
        read_buf[0] = 0xff; read_buf[1] = 0xff;       
        rcnt = pUART.read(read_buf, sizeof(read_buf));

        if ((rcnt>2)&&(read_buf[0]==0x61)&&(read_buf[1]==0)&&(!((read_buf[6]==0x57)&&(read_buf[7]==0x93)&&(read_buf[8]==0x02)))) {
            logInfo("[DONE] Valid response from Sensor.");
            success = true;
        } else {
            logInfo("[WAIT] No valid response from Sensor.");
            //ThisThread::sleep_for(500);
            dot_sleep(1);
        }

        tries--;
    }
    return success;
}

// Read data from Sensor
bool sensor_read(std::vector<uint8_t> *tx_data) {
    int tries = 5;
    bool success = false;
    int rcnt;
    uint8_t ccycle;

    // Attempt to READ sensor "tries" times
    tries = 5;
    success = false;

    while ((tries>0)&&(!success)) {
        pUART.write(cmd_data, sizeof(cmd_data));
        read_buf[0] = 0xff; read_buf[1] = 0xff;
        rcnt=pUART.read(read_buf, sizeof(read_buf));
        if (rcnt>6) {
            ccycle = read_buf[6];

            if ((read_buf[0]==0x01)&&(read_buf[1]==0)&&(ccycle!=pcycle)) {
                logInfo("[DONE] New data from Sensor.");
                pcycle=ccycle;
                success = true;
                tx_data[0].push_back('x');
                for (int j=6; j<rcnt; j=j+4) for (int i=j+3; i>=j; i--) {
                    tx_data[0].push_back(read_buf[i]);
                }
            } else {
                logInfo("[WAIT] Duplicate data from Sensor.");
                //ThisThread::sleep_for(500);
                dot_sleep(1);
            }
        } else {
            logInfo("[WAIT] No response from Sensor.");
            //ThisThread::sleep_for(500);
            dot_sleep(1);
        }
        tries--;
    }
    return success;
}

template <typename T>
T fromBigEndian(const uint8_t* data) {
    T result = 0;
    for(size_t i = 0; i < sizeof(T); ++i) {
        result |= static_cast<T>(data[i]) << ((sizeof(T) - 1 - i) * 8);
    }
    return result;
}

struct SensorData {
    uint8_t x;
    uint32_t cycleCount;
    uint32_t id;
    float conc;
    float temp;
    float pressure;
    float relHum;
    float absHum;
};

// Function to reverse the byte order of any type
template <typename T>
void reverseEndian(T& value) {
    auto ptr = reinterpret_cast<uint8_t*>(&value);
    std::reverse(ptr, ptr + sizeof(T));
}

// Assumes system uses IEEE 754 floating-point format
float fromBigEndianFloat(const uint8_t* data) {
    float value;
    std::memcpy(&value, data, sizeof(value));
    reverseEndian(value); // Adjust for endianess
    return value;
}

// Updated SensorData extraction function
bool extractSensorData(const std::vector<uint8_t>& tx_data, SensorData& data) {

    data.x = tx_data[0];
    data.cycleCount = fromBigEndian<uint32_t>(tx_data.data() + 1);
    data.conc = fromBigEndianFloat(tx_data.data() + 5);
    data.id = fromBigEndian<uint32_t>(tx_data.data() + 9);
    // data.conc = fromBigEndianFloat(tx_data.data() + 9);
    data.temp = fromBigEndianFloat(tx_data.data() + 13);
    data.pressure = fromBigEndianFloat(tx_data.data() + 17);
    data.relHum = fromBigEndianFloat(tx_data.data() + 21);
    data.absHum = fromBigEndianFloat(tx_data.data() + 25);

    return true;
}

// The main routine
int main() {
    // TX data vector
    std::vector<uint8_t> tx_data;    

    // Custom event handler for automatically displaying RX data
    RadioEvent events;

    // PC debug console
    pc.baud(9600);
    mts::MTSLog::setLogLevel(mts::MTSLog::TRACE_LEVEL);
    // Set up UART
    pUART.set_blocking(false);
    pUART.set_baud(38400);
    pUART.set_format(8,BufferedSerial::None,1);

    // Create channel plan
    plan = create_channel_plan();
    assert(plan);

    // Create MultiTech xDot instance
    dot = mDot::getInstance(plan);
    assert(dot);

    // attach the custom events handler
    dot->setEvents(&events);

    std::printf("%d.0,-1.0,-1.0\n", R0);

	while (1) {
		// Check if starting afresh
		if (boot) {
            boot=0;
			// wait 3 second after POWER up
			//ThisThread::sleep_for(3000);
			dot_sleep(3);

			// RESET xDot
			// reset_xdot();

			// Initialize Methane Sensor
			if (! sensor_start()) {
				logInfo("[FAIL] Sensor failed to initialize 5 times.");
				// send_err_packet(0xff);
			}
		}

		if (! boot) {// Attempt to read from the sensor
            if (! sensor_read(&tx_data)) {
                logInfo("[FAIL] Sensor failed to produce data 5 Times.");
                // send_err_packet(0xff);
            }

            if (! boot) {
                // Send valid data then go to deep sleep
                // if (!dot->getNetworkJoinStatus()) join_network();
                // send_data(tx_data);
                SensorData sensorData;
                measurement();


                // for (size_t i = 0; i < tx_data.size(); ++i) {
                //     // Print current element as a hexadecimal number, followed by a new line
                //     printf("%d: %02X\n", i, tx_data[i]);
                // }

                if (extractSensorData(tx_data, sensorData)) {
                    // Print extracted data using printf
                    // printf("Data extracted successfully:\n");
                    // printf("x: %u\n", sensorData.x);
                    // printf("Cycle Count: %u\n", sensorData.cycleCount);
                    // printf("ID: %u\n", sensorData.id);
                    printf("%f\n", sensorData.conc); // Concentration
                    // printf("Temperature: %f\n", sensorData.temp);
                    // printf("Pressure: %f\n", sensorData.pressure);
                    // printf("Relative Humidity: %f\n", sensorData.relHum);
                    // printf("Absolute Humidity: %f\n", sensorData.absHum);
                } else {
                    // printf("Failed to extract data\n");
                }
                
                tx_data.clear();
                dot_sleep(2);
            }
        }
	}

    return 0;
}