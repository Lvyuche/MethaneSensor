# MethaneSensor
Repo for methane sensor project

Slides: https://uscedu-my.sharepoint.com/:f:/r/personal/haoyuxie_usc_edu/Documents/Research/IoT%20DR?csf=1&web=1&e=mYp3R2

Welcome to the MethaneSensor wiki!

# Current methane señor project progress:

## 1. Sensor calibration

> successfully mapping from 5 seconds AO to stable AO, ensuring shorten the heating time from 20 seconds to 5 seconds. successfully mapping from stable MQ4 AO to nano methane gas concentration, ensuring that AO is readable in PPM unit.

### Current Problem

(a) The MQ4 sensor is too unstable that it sometimes changes its characters based on unknown reason. for example, the basic AO in fresh air should be around 0.11, but sometimes the AO in fresh air will be around 0.9. 

Our solution: eliminate the abnormal data. After cooling down the system, restart the system and capture data.

(b) The 1 second slope doesn't work because the AO is too unstable that it has offset after averaging and sampling. And the slope for each 1 second is same even in different methane gas concentration.

## 2. Lithium-ion capacitor experiemnt

> running the MQ4 xDot gas sensor system for 35 minutes with a fully charged Lithium-ion capacitor solar system from 4:30pm to 5:05pm without turning MQ4 on/off periodically.

### Current Problem

(a) MCP1460 doesn't work based on the circuit provided by datasheet, can't boost voltage successfully. 

(b) EN pin only control whether boost voltage or not, it won't power MQ4 on/off.

### Next step: 
(a) replace the 250F supercar with the 1100F 4V cda supercap.

(b) If the system runs out of battery during the day, use larger solar panel and extend the MQ4 power-off time. If the system runs out of battery during the night, add one more super capacitor.

(c) Solve the MCP1460 boost problem by rebuilding the MCP1460 circuit.


# Details about the mapping experiments

## Data Collection

Topology: PC ---- xDot ----- MQ4-1, MQ4-2

* MQ4-1: continuously generate AO without powered off.

* MQ4-2: turn on for 5 seconds and generate AO, then turn it off for 25 seconds.

* xDot: read AO from MQ4-1 and MQ4-2 and print AO.

* PC: running read_meta_data.py to monitor and capture AO from the USB serial port(debug console) and save the data.

* Expectation: collect correct data for mapping process. 

> what's the correct data?

> data when there is no apparent offset between each AO in only fresh air. It's easier to build an accurate mapping model by only capturing AO measured in 0 - 5000 ppm methane gas concentration because of the limitation of MQ4.


## Linear Regression Mapping

* Preprocess the data to delete the abnormal data.

* Use linear regression model to map the AO (turn on 5 second and turn off 25 seconds) to stable AO.

* Draw picture to compare the original AO and the AO after mapping.

* Test the linear regression model with real time AO.

* Expectation: shorten the measurement time by mapping AO(turn on 5 seconds) to AO(stable) with high accuracy. 

# General problems & solutions

* Programming xDot: “PyOCD deploy failed, deploying using mass storage device” OR “The interface firmware FAILED to reset/halt the target MCU”

<img width="350" alt="image" src="https://github.com/Lvyuche/MethaneSensor/assets/112199186/3cda11b6-6e72-45dd-8c73-3d257139224f">

<img width="350" alt="image" src="https://github.com/Lvyuche/MethaneSensor/assets/112199186/5e692c3c-1c4d-4254-81fb-93006c78733c">

Solution: Hold the “RESET” button during “drag & drop”. It’s useless to update the DAPLink firmware but don’t use DAPLink version 0254 ([0254_K20DX_XDOT_L151_0X8000.BIN](https://daplink.io/firmware/0254_k20dx_xdot_l151_0x8000.bin)) because it doesn’t support xDot

<img width="323" alt="image" src="https://github.com/Lvyuche/MethaneSensor/assets/112199186/f77338f5-b10b-4e2c-ae5f-e8a4c92e1def">


* Programming xDot: encounter legacy dependency problem while compiling iiot.cpp

Solution: Update the dependency library

<img width="350" alt="image" src="https://github.com/Lvyuche/MethaneSensor/assets/112199186/8aaecd6a-5eac-4e5b-98b1-8bc63cfd08be">

* LoraWan connection failed due to the wrong validation

Solution: We can't solve it by cooling down, adding antenna, or changing xDot, so we believe changing the gate would solve the problem because we haven't encountered this problem before using this gateway. Changing the password for LoraWan in iiot.cpp and setup in gateway also works but that's not the solution we want.

* Don't change the resistor of MQ4 after starting collecting AO or mapping process.

* Wash Nano sensor with fresh air for 5 minutes after each turning on.






