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
// The amount of time between reads of channel data.
const int EQ_SWITCH_DELAY = 20;

enum State{
	Normal,
	BaseDrop
	
};

State CurrentState = Normal;

float channelSignal[CHANNEL_COUNT];
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(60, PIN_LIGHTS, NEO_GRB + NEO_KHZ800);

void setup() {
	
	pinMode(PIN_STROBE, OUTPUT);
	pinMode(PIN_RESET, OUTPUT);
	pinMode(PIN_INPUT, INPUT);
	pinMode(PIN_LIGHTS, OUTPUT);
	Serial.begin(9600);
	pixels.begin();
	pixels.setBrightness(60);


	digitalWrite(PIN_RESET, HIGH);
	delay(1);
	digitalWrite(PIN_RESET, LOW);
}

void loop() {
	getChannelValues();

	if (channelSignal[0] > 950){
		baseDropInit();
		return;
	}

	
	switch (CurrentState){
		case Normal: 
		normalMode();
		break;
		case BaseDrop:
		baseDropMode();
		break;
	}
}

int baseDropLevel = 8;
void baseDropMode(){
  // 8 LEDS per channel!!!
	int pixelsOfChannel[LEDS_PER_CHANNEL];
	for (int i = 0; i < CHANNEL_COUNT; i++){
		getPixelsOfChannel(i, pixelsOfChannel);
		
		for (int p = 0; p < LEDS_PER_CHANNEL; p++){
			if (p <= baseDropLevel)
				pixels.setPixelColor(pixelsOfChannel[p], getChannelColor(i));
	  else // Make pixel display no color above the signal level.
	  	pixels.setPixelColor(pixelsOfChannel[p], pixels.Color(0, 0, 0));
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
		float signalStrength = (channelSignal[i]/1024.0) * LEDS_PER_CHANNEL;
		getPixelsOfChannel(i, pixelsOfChannel);
		
		for (int p = 0; p < LEDS_PER_CHANNEL; p++){
			if (p <= ceil(signalStrength))
				pixels.setPixelColor(pixelsOfChannel[p], getChannelColor(i));
	  else // Make pixel display no color above the signal level.
	  	pixels.setPixelColor(pixelsOfChannel[p], pixels.Color(0, 0, 0));
	}
	Serial.print((int)signalStrength);
	Serial.print(" ");
	
}  
Serial.println();
pixels.show();
delay(50);
}

void getChannelValues(){
  // Retrieve values for each channel.
	for (int i = 0; i < CHANNEL_COUNT; i++){
		digitalWrite(PIN_STROBE, HIGH);
		delay(1);
		digitalWrite(PIN_STROBE, LOW);
		delay(1);
		channelSignal[i] = analogRead(PIN_INPUT);

	}
}

// Returns the indices of the lights of the given channel in ascending order, so
// the first element is the bottom most index.
void getPixelsOfChannel(int channel, int lights[]){
	int even = channel % 2;
	int indices[LEDS_PER_CHANNEL];
	int p = 0;
	if (!even){
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
		return pixels.Color(255,255,255);
		case 1:
		return pixels.Color(0,255,0);
		case 2:
		return pixels.Color(255,0,0);
		case 3:
		return pixels.Color(0,0,255);
		case 4:
		return pixels.Color(255,0,255);
		case 5:
		return pixels.Color(0,255,255);
		case 6:
		return pixels.Color(255,255,0); 
		case 7:
		return pixels.Color(125, 40, 40); 
	}
}

void baseDropInit(){
	uint32_t color = pixels.Color(0,0,255);
	CurrentState = BaseDrop;
	baseDropLevel = 8;

	for (int i = 0; i < 60; i++){
		pixels.setPixelColor(i, color);
	}
	
	Serial.println();
	pixels.show();
	delay(50);
}


