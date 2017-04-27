#include <Adafruit_NeoPixel.h>
// Equalizer chip control pins.
const int PIN_STROBE = 2;
const int PIN_RESET = 3;
// Where the data from the equalizer is sent to.
const int PIN_INPUT = 0;
// Sends data to lights.
const int PIN_LIGHTS = 6;
// Number of channel bands in the equalizer chip.
const int CHANNEL_COUNT = 7;
// The amount of time between reads of channel data.
const int EQ_SWITCH_DELAY = 20;


int channelSignal[CHANNEL_COUNT];
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(60, PIN_LIGHTS, NEO_GRB + NEO_KHZ800);

void setup() {
 
  pinMode(PIN_STROBE, OUTPUT);
  pinMode(PIN_RESET, OUTPUT);
  pinMode(PIN_INPUT, INPUT);
  pinMode(PIN_LIGHTS, OUTPUT);
  Serial.begin(9600);
  pixels.begin();

}

void loop() {
  getChannelValues();
  colorize();
}

void colorize(){
  // 8 LEDS per channel!!!
  // Channel 1
  for (int i = 0; i < 8; i++){
    int start = i * 8;
    // Assuming that the values from the equalizer are from 0 to 1024.
    int signalStrength = (channelSignal[i]/(float)1024) * 8;
    //int signalStrength = 4;
    for (int p = start; p < start + signalStrength; p++){
      pixels.setPixelColor(p, getChannelColor(i));
    }
    Serial.print(signalStrength);
    Serial.print(" ");
    
  }  
  Serial.println();
  pixels.show();
  delay(500);
}

void getChannelValues(){

  digitalWrite(PIN_RESET, HIGH);
  delay(1);
  digitalWrite(PIN_RESET, LOW);
  
  // Retrieve values for each channel.
  for (int i = 0; i < CHANNEL_COUNT; i++){
    // Data sheet recommends 18 microseconds.
    int strobeLen = 1;
    digitalWrite(PIN_STROBE, LOW);
    delay(strobeLen);
    channelSignal[i] = analogRead(  PIN_INPUT);    
    digitalWrite(PIN_STROBE, HIGH);
    delay(strobeLen);
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

