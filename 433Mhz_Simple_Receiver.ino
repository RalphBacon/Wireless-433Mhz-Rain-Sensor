#include "Arduino.h"
#include <VirtualWire.h>

char data[10] = { };
unsigned int dataCnt = 0;

void setup() {
	vw_set_ptt_inverted(true); // Required for DR3100
	vw_set_rx_pin(2);
	vw_setup(2000);  // Bits per sec
	Serial.begin(9600);

	vw_rx_start();       // Start the receiver PLL running
}

template<typename T, size_t N>
void zero(T (&myArray)[N]) {
	for (unsigned int cnt = 0; cnt < sizeof myArray; cnt++) {
		myArray[cnt] = 0;
	}
}

void loop() {
	//zero(data);
	//dataCnt = 0;

	uint8_t buf[VW_MAX_MESSAGE_LEN];
	uint8_t buflen = VW_MAX_MESSAGE_LEN;

	while (vw_get_message(buf, &buflen) && dataCnt < 10) {

		//char readData = swSerial.read();
		//Serial.print(readData);
		//data[dataCnt++] = readData;

		for (int i = 0; i < buflen; i++) {

			// Data is in HEX so convert to ASCII
			char hexData = buf[i];
			//char asciiData = hex2Ascii(hexData[0]) * 16) + hex2Ascii(hexData[1];
			Serial.print(hexData);
		}
		Serial.println("");
	}


//Serial.println(data);

	delay(100);
}

// Convert to ASCII
char hex2Ascii(char hexData) {
	if (hexData > '0' && hexData <= '9') {
		return hexData - '0';
	}

	if (hexData > 'a' && hexData <= 'f') {
		return hexData - 'a';
	}
	return 10 + hexData - 'A';
}
