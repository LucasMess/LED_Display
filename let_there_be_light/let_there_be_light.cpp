#include <Adafruit_NeoPixel.h>
// Equalizer chip control pins.
const int PIN_STROBE = 7;
const int PIN_RESET = 8;
// Where the data from the equalizer is sent to.
const int PIN_INPUT = 0;
// Sends data to lights.
const int PIN_LIGHTS = 6;
// Number of LEDs per channel of the equalizer.
const int LEDS_PER_CHANNEL = 8;
// Number of channel bands in the equalizer chip.
const int CHANNEL_COUNT = 7;

const int MAX_BRIGHTNESS = 100;

enum State{
	Idle,
	Normal,
	BaseDrop,
};

State CurrentState = Normal;

float channelSignal[CHANNEL_COUNT];
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(56, PIN_LIGHTS, NEO_GRB + NEO_KHZ800);

const int SAMPLE_SIZE = 10;
// Used to store the previous values of the LED signals for comparison.
char previousValues[SAMPLE_SIZE][CHANNEL_COUNT];	
int currentSample = 0;
int sumValues[CHANNEL_COUNT];
int averageValues[CHANNEL_COUNT];


void setup() {

	pinMode(PIN_STROBE, OUTPUT);
	pinMode(PIN_RESET, OUTPUT);
	pinMode(PIN_INPUT, INPUT);
	pinMode(PIN_LIGHTS, OUTPUT);
	Serial.begin(9600);
	pixels.begin();
	pixels.setBrightness(100);


	digitalWrite(PIN_RESET, HIGH);
	delay(1);
	digitalWrite(PIN_RESET, LOW);
}

void loop() {
	getChannelValues();
	recalculateAverage();

	checkIfIdle();
	//if (CurrentState != Idle)
	//	checkIfBaseDrop();

	switch (CurrentState){
		case Idle:
			idleMode();
			break;
		case Normal: 
			normalMode();
			break;
		case BaseDrop:
			baseDropMode();
			break;
	}
}

// Color the LEDS with random colors.
void idleMode(){
	int pixelsOfChannel[LEDS_PER_CHANNEL];
	for (int i = 0; i < CHANNEL_COUNT; i++){
		getPixelsOfChannel(i, pixelsOfChannel);
		for (int p = 0; p < LEDS_PER_CHANNEL; p++){
			int randR = rand() % 255;
			int randG = rand() % 255;
			int randB = rand() % 255;
			int randBright = rand() % MAX_BRIGHTNESS;
			pixels.setPixelColor(pixelsOfChannel[p], pixels.Color(randR, randG, randB));
			//pixels.setBrightness(randBright);
	  	}
		Serial.print("- ");
	}
	Serial.print(" IDLE MODE");
	Serial.println();  
	pixels.show();
	delay(50);
}

int cyclesSinceAllZero = 0;
void checkIfIdle(){
	CurrentState = Normal;

	for (int i = 0; i < CHANNEL_COUNT; i++){
		if (averageValues[i] != 0){
			cyclesSinceAllZero = 0;
			return;
		}
	}
	cyclesSinceAllZero++;
	if (cyclesSinceAllZero > 5){
		CurrentState = Idle;
		Serial.println(" No input? ");
		cyclesSinceAllZero = 100;
	}



}

// Activate base drop mode if all requirements are met.
void checkIfBaseDrop(){
/*
	If the signal of all the channels follows this format:
		channelx >= avrChannelx * (2 - x * .25)
	then it is very likely that this is a base beat.
*/
	float recentBeatFactor = 1;

	// This might help with very fast beats, it will make it more difficult for a beat to occur right
	// after another.
	if (CurrentState == BaseDrop){
		return;
		recentBeatFactor = 3;
	}

	bool beat = true;
	//Serial.print("Comp: ");
	for (int i = 0; i < CHANNEL_COUNT; i++){
		//Serial.print(channelSignal[i]);
		int val = averageValues[i] * (2 - i * .5) * recentBeatFactor;
		//Serial.print(" -> ");
		//Serial.print(val);
		//Serial.print(" ");
		if (channelSignal[i] / 1024.0 * LEDS_PER_CHANNEL >= val ){
			continue;
		}
		else {
			beat = false;
			break;
		}
	}
	//Serial.println();

	// Activate base drop mode
	if (beat){
		baseDropInit();
	}
}

void recalculateAverage(){
	Serial.print(" Averages: ");
	for (int i = 0; i < CHANNEL_COUNT; i++){
		averageValues[i] = sumValues[i] / SAMPLE_SIZE;
		Serial.print(averageValues[i]);
		Serial.print(" ");
	}
	Serial.println();
}

int baseDropLevel = 8;
void baseDropMode(){
	// 8 LEDS per channel!!!
	int pixelsOfChannel[LEDS_PER_CHANNEL];
	for (int i = 0; i < CHANNEL_COUNT; i++){
		getPixelsOfChannel(i, pixelsOfChannel);
		for (int p = 0; p < LEDS_PER_CHANNEL; p++){
			pixels.setPixelColor(pixelsOfChannel[p], pixels.Color(0,0,255));
	  		pixels.setBrightness(MAX_BRIGHTNESS * baseDropLevel / MAX_BRIGHTNESS);
	  	}
		Serial.print(baseDropLevel);
		Serial.print(" ");
	}
	Serial.print(" BASE DROP MODE");
	Serial.println();  
	baseDropLevel--;
	if (baseDropLevel == 0)
		CurrentState = Normal;
	pixels.show();
	delay(50);
}

void normalMode(){
  // 8 LEDS per channel!!!
	int pixelsOfChannel[LEDS_PER_CHANNEL];
	for (int i = 0; i < CHANNEL_COUNT; i++){
	// Assuming that the values from the equalizer are from 0 to 1024.
		float signalStrength = (channelSignal[i]/1023.0) * LEDS_PER_CHANNEL;
		getPixelsOfChannel(i, pixelsOfChannel);
		for (int p = 0; p < LEDS_PER_CHANNEL; p++){
			if (p < ceil(signalStrength))
				pixels.setPixelColor(pixelsOfChannel[p], getChannelColor(i));
	  		else // Make pixel display no color above the signal level.
	  			pixels.setPixelColor(pixelsOfChannel[p], pixels.Color(0, 0, 0));
	  		pixels.setBrightness(MAX_BRIGHTNESS);
		}
		Serial.print((int)signalStrength);
		Serial.print(" ");
	}  
	Serial.println();
	pixels.show();
	delay(75);
}

// Retrieve values for each channel and adds the new one to the total sum for the average calculation.
void getChannelValues(){
	for (int i = 0; i < CHANNEL_COUNT; i++){
		digitalWrite(PIN_STROBE, HIGH);
		delay(1);
		digitalWrite(PIN_STROBE, LOW);
		delay(1);
		channelSignal[i] = analogRead(PIN_INPUT);
		// Remove old value from the sum.
		sumValues[i] -= (int)previousValues[currentSample][i];
		previousValues[currentSample][i] = (char)(channelSignal[i] / 1024.0 * LEDS_PER_CHANNEL);
		// Add new value to sum.
		sumValues[i] += (int)previousValues[currentSample][i];
	}
	// Increment sample for the next sample batch.
	currentSample++;
	currentSample %= SAMPLE_SIZE;
}

// Returns the indices of the lights of the given channel in ascending order, so
// the first element is the bottom most index.
void getPixelsOfChannel(int channel, int lights[]){
	int even = channel % 2;
	int indices[LEDS_PER_CHANNEL];
	int p = 0;
	if (even){
		for (int i = channel * LEDS_PER_CHANNEL; i < channel * LEDS_PER_CHANNEL + LEDS_PER_CHANNEL; i++){
			lights[p] = i;
			p++;
		}
	}
	else{
		for (int i = channel * LEDS_PER_CHANNEL + LEDS_PER_CHANNEL - 1; i >= channel * LEDS_PER_CHANNEL; i--){
			lights[p] = i;
			p++;
		}
	}
}

uint32_t getChannelColor(int channel){
	switch(channel){
		case 0:
		return pixels.Color(139,0,255);
		case 1:
		return pixels.Color(255,0,0);
		case 2:
		return pixels.Color(255,127,0);
		case 3:
		return pixels.Color(255,255,0);
		case 4:
		return pixels.Color(0,255,0);
		case 5:
		return pixels.Color(0,255,255);
		case 6:
		return pixels.Color(0,0,255); 
		case 7:
		return pixels.Color(75, 0, 130); 
	}
}

void baseDropInit(){
	uint32_t color = pixels.Color(0,0,255);
	CurrentState = BaseDrop;
	baseDropLevel = 8;

	for (int i = 0; i < 60; i++){
		pixels.setPixelColor(i, color);
	}

	Serial.println("NEW BEAT");
	pixels.show();
	delay(50);
}


