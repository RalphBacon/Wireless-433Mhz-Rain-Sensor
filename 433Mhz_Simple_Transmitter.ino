#include "Arduino.h"
#include "VirtualWire.h"

void setup()
{
	Serial.begin(9600);

	vw_set_ptt_inverted(true);
	vw_setup(2000);
	vw_set_tx_pin(9);
}


void loop()
{
	// Get data value
	int data = analogRead(A0);
	Serial.println(data);

	//  Concatenate all data to be transmitted
	char TxBuffer[5];
	char dataBuffer[4];

	memset (TxBuffer, '\0', sizeof TxBuffer);
	memset (dataBuffer, '\0', sizeof dataBuffer);

	TxBuffer[0] = 'R';
	itoa(data, dataBuffer, 10);

	// Concatenate the two arrays for transmission
	strcat(TxBuffer, dataBuffer);

	vw_send((unsigned char *) TxBuffer, strlen(TxBuffer));
	vw_wait_tx();

	delay(1000);
}
