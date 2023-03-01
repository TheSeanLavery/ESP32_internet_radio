
#include <Arduino.h>
#include <driver/dac.h>
#include "Audio.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "AiEsp32RotaryEncoder.h"


/*
connecting Rotary encoder
Rotary encoder side    MICROCONTROLLER side  
-------------------    ---------------------------------------------------------------------
CLK (A pin)            any microcontroler intput pin with interrupt -> in this example pin 32
DT (B pin)             any microcontroler intput pin with interrupt -> in this example pin 21
SW (button pin)        any microcontroler intput pin with interrupt -> in this example pin 25
GND - to microcontroler GND
VCC                    microcontroler VCC (then set ROTARY_ENCODER_VCC_PIN -1) 
***OR in case VCC pin is not free you can cheat and connect:***
VCC                    any microcontroler output pin - but set also ROTARY_ENCODER_VCC_PIN 25 
                        in this example pin 25
*/

#define ROTARY_ENCODER_A_PIN 23
#define ROTARY_ENCODER_B_PIN 19
#define ROTARY_ENCODER_BUTTON_PIN 18
#define ROTARY_ENCODER_VCC_PIN -1 /* 27 put -1 of Rotary encoder Vcc is connected directly to 3,3V; else you can use declared output pin for powering rotary encoder */


#define ROTARY_ENCODER_STEPS 4

//instead of changing here, rather change numbers above
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);


Adafruit_SSD1306 display(-1);



Audio audio_ = Audio(false); // Use external DAC


  // Content in audio buffer (provided by esp32-audioI2S library)
uint32_t audioBufferFilled_ = 0;

// Size of audio buffer (provided by esp32-audioI2S library)
uint32_t audioBufferSize_ = 0;

// Current station index
uint8_t stationIndex_ = 0;

// Flag to indicate the user wants to changed the station
bool stationChanged_ = true;

// Flag to indicate that audio is muted after tuning to a new station
bool stationChangedMute_ = true;

// Name of the current station as provided by the stream header data
String stationStr_ = "";

// Flag indicating the station name has changed
bool stationUpdatedFlag_ = false;

// Flag indicating that the connection to a host could not be established
bool connectionError_ = false;



int timeConnect_ = 0;

//char array of stations
const char* stations[] = {
  "http://stream.rockantenne.de/rockantenne/stream/mp3",
  "http://138.201.83.14:8010/stream/1/",
  "https://kexp-mp3-128.streamguys1.com/kexp128.mp3"
};
const char* ssid     = "ASUS_38_2G";
const char* password = "whatdoyouthinkwouldbegood";
const char* url      = "http://stream.rockantenne.de/rockantenne/stream/mp3";

//potentiometer pin
const int potPin = 17;

//rotary encoder pins (CLK 34, DT 33, SW 35) 
const int clkPin = 34;
const int dtPin = 33;
const int swPin = 35;

const int MAX_STAION = 2;
const int MIN_STAION = 0;

void rotary_onButtonClick()
{
	static unsigned long lastTimePressed = 0;
	//ignore multiple press in that time milliseconds
	if (millis() - lastTimePressed < 500)
	{
		return;
	}
	lastTimePressed = millis();
	Serial.print("button pressed ");
	Serial.print(millis());
	Serial.println(" milliseconds after restart");
}

int encoderPrevValue = 0;
int encoderValue = 0;

void rotary_loop()
{
	//dont print anything unless value changed
	if (rotaryEncoder.encoderChanged())
	{

		encoderValue = rotaryEncoder.readEncoder();
		Serial.print("Value: ");
		Serial.println(encoderValue);

		if(encoderValue > encoderPrevValue)
		{
			stationChanged_ = true;
			stationIndex_++;
		}
		else if(encoderValue < encoderPrevValue)
		{
			Serial.println("Rotated left");
			stationChanged_ = true;
			stationIndex_--;
		}
		else
		{
			Serial.println("Not rotated");
		}
		//change station

		if(stationIndex_ > MAX_STAION)
		{
			stationIndex_ = MIN_STAION;
		}
		else if(stationIndex_ < MIN_STAION)
		{
			stationIndex_ = MAX_STAION;
		}
		encoderPrevValue = encoderValue;

	}
	if (rotaryEncoder.isEncoderButtonClicked())
	{
		rotary_onButtonClick();
	}
}

void IRAM_ATTR readEncoderISR()
{
	rotaryEncoder.readEncoder_ISR();
}





// Create audio object (using I2S DAC)

void displayDemo()   
{            
	// initialize with the I2C addr 0x3C
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  

	// Clear the buffer.
	display.clearDisplay();

	// Display Text
	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.setCursor(0,28);
	display.println("Hello world!");
	display.display();
	return;
	delay(2000);
	display.clearDisplay();

	// Display Inverted Text
	display.setTextColor(BLACK, WHITE); // 'inverted' text
	display.setCursor(0,28);
	display.println("Hello world!");
	display.display();
	delay(2000);
	display.clearDisplay();

	// Changing Font Size
	display.setTextColor(WHITE);
	display.setCursor(0,24);
	display.setTextSize(2);
	display.println("Hello!");
	display.display();
	delay(2000);
	display.clearDisplay();

	// Display Numbers
	display.setTextSize(1);
	display.setCursor(0,28);
	display.println(123456789);
	display.display();
	delay(2000);
	display.clearDisplay();

	// Specifying Base For Numbers
	display.setCursor(0,28);
	display.print("0x"); display.print(0xFF, HEX); 
	display.print("(HEX) = ");
	display.print(0xFF, DEC);
	display.println("(DEC)"); 
	display.display();
	delay(2000);
	display.clearDisplay();

	// Display ASCII Characters
	display.setCursor(0,24);
	display.setTextSize(2);
	display.write(3);
	display.display();
	delay(2000);
	display.clearDisplay();

	// Scroll full screen
	display.setCursor(0,0);
	display.setTextSize(1);
	display.println("Full");
	display.println("screen");
	display.println("scrolling!");
	display.display();
	display.startscrollright(0x00, 0x07);
	delay(2000);
	display.stopscroll();
	delay(1000);
	display.startscrollleft(0x00, 0x07);
	delay(2000);
	display.stopscroll();
	delay(1000);    
	display.startscrolldiagright(0x00, 0x07);
	delay(2000);
	display.startscrolldiagleft(0x00, 0x07);
	delay(2000);
	display.stopscroll();
	display.clearDisplay();

	// Scroll part of the screen
	display.setCursor(0,0);
	display.setTextSize(1);
	display.println("Scroll");
	display.println("some part");
	display.println("of the screen.");
	display.display();
	display.startscrollright(0x00, 0x00);
}



void audioProcessing(void *p) {
    while (true) {
       // Let 'esp32-audioI2S' library process the web radio stream data
        audio_.loop();

        audioBufferFilled_ = audio_.inBufferFilled(); // Update used buffer capacity
        
        vTaskDelay(1 / portTICK_PERIOD_MS); // Let other tasks execute
    }
}

bool buttonBeingPressed = false;
bool lastButtonState = false;

void printStatusToScreen(String status) {
	display.clearDisplay();
	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.setCursor(0,0);
	display.println(status);
	display.display();
}
//list of int to hold button states
int buttonStates[3] = {0,0,0};
void audio_showstation(const char *info){
    Serial.print("station     ");Serial.println(info);
	printStatusToScreen(info);

}

void connectoURLStream(char* url){
 audio_.setPinout(27, 32, 25);

    audio_.stopSong();

    // Establish HTTP connection to requested stream URL
    const char *streamUrl = url;

    bool success = audio_.connecttohost( streamUrl );
	
    if (success) {
        stationChanged_ = false; // Clear flag
        connectionError_ = false; // Clear in case a connection error occured before

        timeConnect_ = millis(); // Store time in order to detect stream errors after connecting
		printStatusToScreen("Connected to stream");
    }
    else {
        stationChanged_ = false; // Clear flag
        connectionError_ = true; // Raise connection error flag
        Serial.println("error connecting to stream");
		printStatusToScreen("error connecting to stream");
    }

    // Update buffer state variables
    audioBufferFilled_ = audio_.inBufferFilled(); // 0 after connecting
    audioBufferSize_ = audio_.inBufferFree() + audioBufferFilled_;

    //should be playing

}


void checkInput() {

}

void setup() {

  displayDemo();
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
	

  Serial.println("WiFi connected");
  Serial.println("IP address: ");

  Serial.println(WiFi.localIP());

   // Start the audio processing task
    xTaskCreate(audioProcessing, "Audio processing task", 4096, nullptr, configMAX_PRIORITIES - 1, nullptr);

   connectoURLStream((char*)url);



   //we must initialize rotary encoder
	rotaryEncoder.begin();
	rotaryEncoder.setup(readEncoderISR);
	//set boundaries and if values should cycle or not
	//in this example we will set possible values between 0 and 1000;
	bool circleValues = false;
	rotaryEncoder.setBoundaries(0, 1000, circleValues); //minValue, maxValue, circleValues true|false (when max go to min and vice versa)

	/*Rotary acceleration introduced 25.2.2021.
   * in case range to select is huge, for example - select a value between 0 and 1000 and we want 785
   * without accelerateion you need long time to get to that number
   * Using acceleration, faster you turn, faster will the value raise.
   * For fine tuning slow down.
   */
	//rotaryEncoder.disableAcceleration(); //acceleration is now enabled by default - disable if you dont need it
	rotaryEncoder.setAcceleration(250); //or set the value - larger number = more accelearation; 0 or 1 means disabled acceleration

    
}


void loop() {


  if (connectionError_) {
       Serial.println("Connection error");

        vTaskDelay(200 / portTICK_PERIOD_MS); // Wait until next cycle
    }
    
      checkInput();
	  rotary_loop();
	  if(stationChanged_){
		
		//  connectoURLStream((char*)stations[stationIndex_]);
		  printf("Station changed to %s", stationIndex_);
	  }
      vTaskDelay(20 / portTICK_PERIOD_MS); // Wait until next cycle
    

}