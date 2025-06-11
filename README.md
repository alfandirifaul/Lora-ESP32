**ESP32 LoRa Button Transmitter with SX1278**
This project implements an ESP32-based LoRa transmitter that sends messages when a button is pressed, using the SX1278 LoRa module.

**Features**
1. Send LoRa packets with incrementing message numbers
2. Robust button debouncing for reliable single-press detection
3. Configurable frequency for different regions
4. Simple serial monitoring for debugging

**Hardware Requirements**
1. ESP32 development board
2. LoRa SX1278 module (RFM95W)
3. Push button
4. Breadboard and jumper wires

**GPIO Connections for LoRa**
ESP32 Pin	LoRa SX1278 Pin	Button Connection
GPIO 5	NSS (CS)	
GPIO 14	RST	
GPIO 2	DIO0	
GPIO 18	SCK	
GPIO 19	MISO	
GPIO 23	MOSI	
3.3V VCC	
GND	 GND	

**Additional GPIO Connections (Transmitter)**
GPIO 4		Button
Note: The button uses ESP32's internal pull-up resistor. When not pressed, GPIO 4 reads HIGH. When pressed, it connects to GND and reads LOW.

**Additional GPIO Connections (Receiver)**
GPIO 12		LED
