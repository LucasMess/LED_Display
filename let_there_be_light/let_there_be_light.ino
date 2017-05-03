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
// The brightess of the pixels.
const int MAX_BRIGHTNESS = 255;

enum State{
	Idle,
	Normal,
	Beat,
};

State CurrentState = Normal;

float channelSignal[CHANNEL_COUNT];

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(56, PIN_LIGHTS, NEO_GRB + NEO_KHZ800);

// The number of samples that is used to calculate the average signal.
const int SAMPLE_SIZE = 10;
// Used to store the previous values of the LED signals for comparison.
char previousValues[SAMPLE_SIZE][CHANNEL_COUNT];	
int currentSample = 0;
int sumValues[CHANNEL_COUNT];
float averageValues[CHANNEL_COUNT];
float totalAverageVolume = 0;

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
	if (CurrentState != Idle)
		checkIfBaseDrop();

	switch (CurrentState){
		case Idle:
			idleMode();
			break;
		case Normal: 
			normalMode();
			break;
		case Beat:
			beatMode();
			break;
	}
}

// Color the LEDS with random colors.
void idleMode(){
	int pixelsOfChannel[LEDS_PER_CHANNEL];
	for (int i = 0; i < CHANNEL_COUNT; i++){
		getPixelsOfChannel(i, pixelsOfChannel);
		for (int p = 0; p < LEDS_PER_CHANNEL; p++){
			pixels.setPixelColor(pixelsOfChannel[p], getRandomColor());
	  	}
		Serial.print("- ");
	}
	Serial.print(" IDLE MODE");
	Serial.println();  
	pixels.show();
	delay(50);
}

int cyclesSinceAllZero = 0;
// Puts the light show in idle mode if there is no signal detected.
void checkIfIdle(){
	for (int i = 0; i < CHANNEL_COUNT; i++){
		if (averageValues[i] != 0){
			cyclesSinceAllZero = 0;
			if (CurrentState == Idle)
				CurrentState = Normal;
			return;
		}
	}
	cyclesSinceAllZero++;
	if (cyclesSinceAllZero > 50){
		CurrentState = Idle;
		Serial.println(" No input? ");
	}
}

float beatThreshold = 3.0;
int maxTimeSinceLastBeat = 300;
int timeSinceLastBeat = 0;
// Activate base drop mode if all requirements are met.
void checkIfBaseDrop(){

	bool foundBeat = false;
	for (int i = 0; i < CHANNEL_COUNT; i++){
		int val = 8;
		int signal = ceil(channelSignal[i] / 1023.0 * LEDS_PER_CHANNEL); 
		if (signal == val && averageValues[i] < beatThreshold){
			foundBeat = true;
			break;
		}
	}

	// Activate base drop mode
	float changeInThreshold = .1;
	if (foundBeat){
		Serial.print("timeSinceLastBeat: ");
		Serial.print(timeSinceLastBeat);
		Serial.print(" beatThreshold: ");
		Serial.print(beatThreshold);
		Serial.print(";");
		if (timeSinceLastBeat < 20){
			beatThreshold -= changeInThreshold;
		}
		else
		{
			beatThreshold += changeInThreshold  * 3;
		}
		timeSinceLastBeat = 0;
		beatModeInit();
	}
	else{
		timeSinceLastBeat++;
		if (timeSinceLastBeat > maxTimeSinceLastBeat){
			beatThreshold += changeInThreshold * 3;
			timeSinceLastBeat = 0;
		}
	}
}

void recalculateAverage(){
	//Serial.print(" Averages: ");
	float sum = 0;
	for (int i = 0; i < CHANNEL_COUNT; i++){
		averageValues[i] = sumValues[i] / SAMPLE_SIZE;
		sum += averageValues[i];
		//Serial.print(averageValues[i]);
		//Serial.print(" ");
	}
	totalAverageVolume = sum / CHANNEL_COUNT;
	if (totalAverageVolume < .01)
		totalAverageVolume = .01;
	//Serial.println();
}

int baseDropLevel = 8;
uint32_t beatColor;
// The light show that is displayed for beats.
void beatMode(){
	// 8 LEDS per channel!!!
	int pixelsOfChannel[LEDS_PER_CHANNEL];
	for (int i = 0; i < CHANNEL_COUNT; i++){
		getPixelsOfChannel(i, pixelsOfChannel);
		float signalStrength = (channelSignal[i]/1023.0) * LEDS_PER_CHANNEL;	
		for (int p = 0; p < LEDS_PER_CHANNEL; p++){
			if (p < baseDropLevel || p < ceil(signalStrength))
				pixels.setPixelColor(pixelsOfChannel[p], beatColor);
	  		else // Make pixel display no color above the signal level.
	  			pixels.setPixelColor(pixelsOfChannel[p], pixels.Color(0, 0, 0));
	  	}
		Serial.print(baseDropLevel);
		Serial.print(" ");
	}
	Serial.print(" BASE DROP MODE");
	Serial.println();  
	baseDropLevel-= 2;
	if (baseDropLevel == 0)
		CurrentState = Normal;
	pixels.show();
	delay(0);
}

// The light show that is displayed normally.
void normalMode(){
  // 8 LEDS per channel!!!
	int pixelsOfChannel[LEDS_PER_CHANNEL];
	for (int i = 0; i < CHANNEL_COUNT; i++){
	// Assuming that the values from the equalizer are from 0 to 1024.
		float signalStrength = (channelSignal[i]/1023.0) * LEDS_PER_CHANNEL;
		//signalStrength = 1.5/totalAverageVolume * signalStrength;
		getPixelsOfChannel(i, pixelsOfChannel);
		for (int p = 0; p < LEDS_PER_CHANNEL; p++){
			if (p < ceil(signalStrength))
				pixels.setPixelColor(pixelsOfChannel[p], getChannelColor(i));
	  		else // Make pixel display no color above the signal level.
	  			pixels.setPixelColor(pixelsOfChannel[p], pixels.Color(0, 0, 0));
		}
		Serial.print((int)signalStrength);
		Serial.print(" ");
	}  

	Serial.println();
	pixels.show();
	delay(10);
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

// Gets the predetermined color of each channel.
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

uint32_t getRandomChannelColor(){
	int randChannel = rand() % 7;
	return getChannelColor(randChannel);
}

// Gets random color for pixel.
uint32_t getRandomColor(){
	int contrast = 8;
	int randR = (255/contrast) * (rand() % contrast) - 1;
	int randG = (255/contrast) * (rand() % contrast) - 1;
	int randB = (255/contrast) * (rand() % contrast) - 1;
	return pixels.Color(randR, randG, randB);
}

// Initializes beat mode
void beatModeInit(){
	beatColor = getRandomChannelColor();
	CurrentState = Beat;
	baseDropLevel = 8;

	for (int i = 0; i < 60; i++){
		pixels.setPixelColor(i, beatColor);
	}

	Serial.println("NEW BEAT");
	pixels.show();
}


