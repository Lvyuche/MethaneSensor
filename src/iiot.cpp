#include "dot_util.h"
#include "RadioEvent.h"

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

// Send ERROR packet then reboot
void send_err_packet(uint32_t err_code) {
    std::vector<uint8_t> tx_data;
    tx_data.push_back(err_code);

    // Send TX data to the gateway
    if (!dot->getNetworkJoinStatus()) join_network();
    send_data(tx_data);

    // RESET everything after 10 sec of deepsleep
    boot=1;
    dot_sleep(10);
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

	while (1) {
		// Check if starting afresh
		if (boot) {
            boot=0;
			// wait 3 second after POWER up
			//ThisThread::sleep_for(3000);
			dot_sleep(3);

			// RESET xDot
			reset_xdot();

			// Initialize Methane Sensor
			if (! sensor_start()) {
				logInfo("[FAIL] Sensor failed to initialize 5 times.");
				send_err_packet(0xff);
			}
		}

		if (! boot) {// Attempt to read from the sensor
            if (! sensor_read(&tx_data)) {
                logInfo("[FAIL] Sensor failed to produce data 5 Times.");
                send_err_packet(0xff);
            }

            if (! boot) {
                // Send valid data then go to deep sleep
                if (!dot->getNetworkJoinStatus()) join_network();
                send_data(tx_data);
                tx_data.clear();
                dot_sleep(2);
            }
        }
	}

    return 0;
}

