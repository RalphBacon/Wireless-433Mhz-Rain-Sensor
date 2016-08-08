#include "Arduino.h"
#include <CapacitiveSensor.h>
#include <VirtualWire.h>
#include <Dht11.h>

// Transmitter data stream GPIO
#define txPin 9

// Print out debugging messages to Serial Monitor?
bool isDebug = false;
bool isXmitError = false;

// Rain Sensor
#define rainDigital 4
#define rainAnalog 7

// Transmitter
#define TxPin 9

// Temperature & Humidity DHT11
#define weatherPin 8

// LED indicators
#define blueLED 12	// touch on
#define greenLED 11	// power

// Light Dependent Resistor (LDR)
#define LDRpin 3
#define darknessLevel 500

// Beeper pin
#define beepPin 7

// How often to run the loop (send the data)
#define waitFor 2000

// Door switch pin (for interrupt routine). Magnet holds switch open.
#define doorPin 3
volatile bool doorIsOpen = true;
unsigned long oldMillis = 0;

// Attach wire, foil, touch pad etc to pin 2
CapacitiveSensor sense = CapacitiveSensor(5, 2);
#define touchToggleLevel 200
bool isActive = true;

// Temperature & humidity
static Dht11 dht11(weatherPin);

// ----------------------------------------------------------------------------
// Simple structure (not a class) for data transmission
// ----------------------------------------------------------------------------
struct WeatherData {
	int rainA = 0;
	int rainD = 0;
	int darkLevel = 0;
	int currTemp = 0;
	int currHumid = 0;bool isError = false;bool isTimeout = false;
} weatherData;

// ----------------------------------------------------------------------------
// One size fits all Serial Monitor debugging messages
// ----------------------------------------------------------------------------
template<typename T>
void debugPrint(T printMe, bool newLine = false) {
	if (isDebug) {
		if (newLine) {
			Serial.println(printMe);
		}
		else {
			Serial.print(printMe);
		}
		Serial.flush();
	}
}

// ----------------------------------------------------------------------------
// SETUP     SETUP     SETUP    SETUP    SETUP    SETUP    SETUP    SETUP
// ----------------------------------------------------------------------------
void setup() {

	// Debugging window
	Serial.begin(9600);

	// Rain digital input, trigger level set by pot on board
	pinMode(rainDigital, INPUT);

	// Touch Sensor: No autocalibrate use first line, for 10 seconds use second line
	//sense.set_CS_AutocaL_Millis(0xFFFFFFFF);
	sense.set_CS_AutocaL_Millis(10000);

	// Virtual Wire transmission setup
	vw_set_ptt_inverted(true);
	vw_setup(2000); // baud rate, lower for more reliability
	vw_set_tx_pin(txPin);

	// LED indicators. Green LED is power pin, also flashes if temp conversion detected
	pinMode(blueLED, OUTPUT);
	pinMode(greenLED, OUTPUT);
	digitalWrite(blueLED, HIGH);
	digitalWrite(greenLED, HIGH);

	// Attach interrupt to door Pin
	pinMode(doorPin, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(doorPin), getDoorState, CHANGE);

	// All done
	Serial.println("Setup completed successfully.");
}

// ----------------------------------------------------------------------------
// LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP
// ----------------------------------------------------------------------------
void loop() {

	// Define our data type to hold all the values
	// If we had not included an object field name in our struct definition we would do this here:
	// struct WeatherData weatherData;

	// If a temperature conversion error is detected flash the green LED
	if (isXmitError) digitalWrite(greenLED, !digitalRead(greenLED));

	// Check whether touch pad has been touched - toggle on/off
	if (isActive) {
		// Rain Sensor
		detectRain(&weatherData.rainA, &weatherData.rainD);

		// Check for door open state
		if (doorIsOpen) entryBeep();

		// Darkness
		weatherData.darkLevel = darkness();

		// Check for door open state
		if (doorIsOpen) entryBeep();

		// Temperature & Humidity %
		getTemperatureHumidity(&weatherData.currTemp, &weatherData.currHumid, &weatherData.isError,
				&weatherData.isTimeout);

		// Check for door open state
		if (doorIsOpen) entryBeep();

		// Transmit all this data
		xmitData(&weatherData);

		// Check for door open state
		if (doorIsOpen) entryBeep();

		// Let the receiver deal with the data received so far
		doDelay(waitFor);
	}
	else {
		// routine takes about 250ms normally so add that to required delay here
		doDelay(waitFor + 250);
	}

	// if debug is not switched on then things run too fast so put in a delay here
	// Serial.print takes a long time (relatively speaking) to execute @ 9600 bps
	if (!isDebug) {
		doDelay(500);
	}
	else {
		// Reset door open flag (set by ISR) to stop the beeping
		if (doorIsOpen) entryBeep();
	}

	/* With the delays above we can control this loop to only happen once every
	 * two seconds. This is plenty fast enough (especially as the HX711 can only
	 * be interrogated every second or so (temp and humidity conversion apparently
	 * are not quick!) and we don't need the data any more often than that. After
	 * all, we have to process this data in the yet-to-be-designed receiver.
	 */
}

// ----------------------------------------------------------------------------
// Transmit all data. Concatenate all data to be transmitted into a single string.
// The data is being passed in here as a pointer to the actual memory location
// ----------------------------------------------------------------------------
void xmitData(struct WeatherData *weatherData) {

	// Initialise buffer for both final transmission and intermediate value
	char TxBuffer[100];
	char dataBuffer[10];

	// Set to all null characters using 'memset'
	memset(TxBuffer, '\0', sizeof TxBuffer);
	memset(dataBuffer, '\0', sizeof dataBuffer);

	// Rain Analog (0=wet to 1023=dry) in WeatherData.rainA
	// Convert the integer value to a string. Access the underlying
	// value by using the container name (weatherData) and the member
	// eg weatherData -> rainA
	TxBuffer[0] = 'R';
	itoa(weatherData->rainA, dataBuffer, 10);

	// Concatenate the two arrays for transmission
	strcat(TxBuffer, dataBuffer);

	// Temperature (delimit all strings with null char)
	strcat(TxBuffer, "C\0");
	memset(dataBuffer, '\0', sizeof dataBuffer);
	itoa(weatherData->currTemp, dataBuffer, 10);
	strcat(TxBuffer, dataBuffer);

	// Rain Digital
	strcat(TxBuffer, "D\0");
	memset(dataBuffer, '\0', sizeof dataBuffer);
	itoa(weatherData->rainD, dataBuffer, 10);
	strcat(TxBuffer, dataBuffer);

	// Humidity %
	strcat(TxBuffer, "H\0");
	memset(dataBuffer, '\0', sizeof dataBuffer);
	itoa(weatherData->currHumid, dataBuffer, 10);
	strcat(TxBuffer, dataBuffer);

	// Whether darkness has fallen over Middle Earth
	strcat(TxBuffer, "K\0");
	memset(dataBuffer, '\0', sizeof dataBuffer);
	itoa(weatherData->darkLevel, dataBuffer, 10);
	strcat(TxBuffer, dataBuffer);

	// Error & Timeout on temperature/humidity conversion
	strcat(TxBuffer, "E\0");
	memset(dataBuffer, '\0', sizeof dataBuffer);
	itoa(weatherData->isError, dataBuffer, 10);
	strcat(TxBuffer, dataBuffer);
	memset(dataBuffer, '\0', sizeof dataBuffer);
	itoa(weatherData->isTimeout, dataBuffer, 10);
	strcat(TxBuffer, dataBuffer);

	// Debugging: this is what we're transmitting
	for (unsigned int i = 0; i < sizeof TxBuffer; i++) {
		if (TxBuffer[i] != 0) {
			debugPrint(TxBuffer[i]);
			debugPrint(",");
		}
	}

	// Transmit the data and wait until buffer is clear. We must first
	// cast (=convert) the TxBuffer to the required data type
	vw_send((unsigned char *) TxBuffer, strlen(TxBuffer));
	vw_wait_tx();
}

// ----------------------------------------------------------------------------
// Get the temperature, humidity and whether there were any problems doing so.
// Note all parameters are passed as a pointer reference so we can UPDATE them
// so the calling method can see the new value.
// ----------------------------------------------------------------------------
void getTemperatureHumidity(int *tempC, int *humid, bool *isError, bool *isTimeout) {

	switch (dht11.read()) {
		case Dht11::OK:
			debugPrint("Humidity (%): ");
			*humid = dht11.getHumidity();
			debugPrint(*humid, true);

			debugPrint("Temperature (C): ");
			*tempC = dht11.getTemperature();
			debugPrint(*tempC, true);
			break;

		case Dht11::ERROR_CHECKSUM:
			debugPrint("Checksum error", true);
			*isError = true;
			isXmitError = true;
			break;

		case Dht11::ERROR_TIMEOUT:
			debugPrint("Timeout error", true);
			*isTimeout = true;
			isXmitError = true;
			break;

		default:
			debugPrint("Unknown error");
			*isError = true;
			break;
	}
}

// ----------------------------------------------------------------------------
// Detects rain on sensor, capacitive (0=under water to 1023=dry) and digital (1=not wet, 0=wet)
// ----------------------------------------------------------------------------
void detectRain(int *rainA, int *rainD) {
	*rainA = analogRead(rainAnalog);
	*rainD = digitalRead(rainDigital);

	debugPrint("Analog Rain: ");
	debugPrint(*rainA, true);

	debugPrint("Digital Rain: ");
	debugPrint(*rainD, true);
}

// ----------------------------------------------------------------------------
// Determines whether we are in a night-time situation
// ----------------------------------------------------------------------------
int darkness() {
	int darkness = analogRead(LDRpin);
	debugPrint("Darkness level: ");
	debugPrint(darkness, true);
	return darkness;
}

// ----------------------------------------------------------------------------
// Detect whether the touch pad has been touched - will control on/off function
// ----------------------------------------------------------------------------
void doTouch() {
	// Take several samples
	long signal = sense.capacitiveSensor(70);

	// Master ON-OFF switch (controlled by pretty cat collar touch pad)
	if (signal > touchToggleLevel) {
		isActive = !isActive;
		digitalWrite(blueLED, isActive ? HIGH : LOW);

		debugPrint("Touch level: ");
		debugPrint(signal, true);

		// Do not proceed until user has removed finger from sensor
		while (sense.capacitiveSensor(70) > touchToggleLevel)
			delay(250);
	}
}

// ----------------------------------------------------------------------------
// To improve responsiveness, do the touch control whilst delaying overall progress
// So we split the delay into chunks of 100ms at a time and check the door each
// iteration. Improves responsiveness greatly.
// ----------------------------------------------------------------------------
void doDelay(int millis) {
	for (int cnt = 0; 100 * cnt < millis; cnt++) {
		if (doorIsOpen) {
			entryBeep();
		}
		else {
			delay(100);
		}
		doTouch();
	}
}

// ----------------------------------------------------------------------------
// Emit a double beep to alert user to switch unit on/off as required
// ----------------------------------------------------------------------------
void entryBeep() {
	tone(beepPin, 1000, 50);
	delay(100);
	tone(beepPin, 1500, 50);
	doorIsOpen = false;
}

// ----------------------------------------------------------------------------
// Interrupt (ISR) routine to detect door opening
// ----------------------------------------------------------------------------
void getDoorState() {
	// This routine is automatically triggered when the door is opened: the magnet
	// no longer holds the reed switch open so it closes bringing door pin LOW or
	// it holds it open bringing door pin HIGH

	// Don't do this interrupt more than every 1/4 second as it probably switch
	// bounce causing the interrupt to fire!
	if (millis() > oldMillis + 250) {
		oldMillis = millis();
		if (!doorIsOpen) doorIsOpen = true;
	}
}
