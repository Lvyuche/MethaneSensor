#include "dot_util.h"
#include "RadioEvent.h"
#include <cstdio>

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
using ThisThread::sleep_for;

static std::string network_name = "MTCDT-20314492";
static std::string network_passphrase = "MTCDT-20314492";
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

// Alert LED instance
DigitalOut led(LED1);
DigitalOut transistor(GPIO2);
#define BLINKING_RATE 20000

// Assuming the MQ-4 sensor's AO pin is connected to pin A0 (analog input) (wake up PIN)
AnalogIn mq4Sensor_real(PA_0); // wake white
AnalogIn mq4Sensor_fast(PB_14);


void measurement_real () {
    // Convert the reading to a more meaningful value (e.g., ppm)
    // This conversion requires calibration with known methane concentrations
    // For demonstration, we'll just output the raw analog value
    float sensorValue = 0;
    for(int i = 0; i < 10; i++) sensorValue += mq4Sensor_real.read();
    sensorValue = sensorValue / 100.0;
    std::printf("%f\n", sensorValue);
}

void measurement_fast () {
    // Convert the reading to a more meaningful value (e.g., ppm)
    // This conversion requires calibration with known methane concentrations
    // For demonstration, we'll just output the raw analog value
    float sensorValue = 0;
    for(int i = 0; i < 10; i++) sensorValue += mq4Sensor_fast.read();
    sensorValue = sensorValue / 100.0;
    std::printf("%f, ", sensorValue);
}

void measurement () {
    transistor = 1;  // Assuming this enables the sensor
    led = 1;
    ThisThread::sleep_for(std::chrono::milliseconds(100)); // Wait for sensor stabilization

    for (int i = 0; i < 100; i++) {
        measurement_fast();
        measurement_real();
        ThisThread::sleep_for(std::chrono::milliseconds(50));  // Wait for 50 ms
    }
    led = 0;
    transistor = 0;  // Disable the sensor

    ThisThread::sleep_for(std::chrono::milliseconds(100)); // Wait for sensor stabilization
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

// RESET xdot
void reset_xdot() {
// start from a well-known state
logInfo("defaulting Dot configuration");
dot->resetConfig();
dot->resetNetworkSession();

// make sure library logging is turned on
dot->setLogLevel(mts::MTSLog::INFO_LEVEL);

// update configuration if necessary
if (dot->getJoinMode() != mDot::OTA) {
    logInfo("changing network join mode to OTA");
    if (dot->setJoinMode(mDot::OTA) != mDot::MDOT_OK) {
        logError("failed to set network join mode to OTA");
    }
}

// To preserve session over power-off or reset enable this flag
//dot->setPreserveSession(false);
update_ota_config_name_phrase(network_name, network_passphrase, frequency_sub_band, network_type, ack);
update_network_link_check_config(3, 5);

// enable or disable Adaptive Data Rate
dot->setAdr(adr);

// Configure the join delay
dot->setJoinDelay(join_delay);

// save changes to configuration
logInfo("saving configuration");
if (!dot->saveConfig()) {
    logError("failed to save configuration");
}

// display configuration
display_config();
}



// The main routine
int main() {
    // TX data vector
    std::vector<uint8_t> tx_data;    

    // Custom event handler for automatically displaying RX data
    RadioEvent events;

    // Create channel plan
    plan = create_channel_plan();
    assert(plan);

    // Create MultiTech xDot instance
    dot = mDot::getInstance(plan);
    assert(dot);

    // attach the custom events handler
    dot->setEvents(&events);
    while (1) {
        measurement();
        // DR deep sleep 4 seconds
        tx_data.clear();

        for (int i = 0; i < 400; i++) {
        measurement_fast();
        measurement_real();
        ThisThread::sleep_for(std::chrono::milliseconds(10));  // Wait for 10 ms
        }

        // dot_sleep(4);
        // ThisThread::sleep_for(4000);
    }
    return 0;
}
