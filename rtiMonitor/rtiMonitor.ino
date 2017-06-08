#include <SoftwareSerial.h>

enum display_mode_name { RTI_RGB, RTI_PAL, RTI_NTSC, RTI_OFF };
static const byte display_modes[] = { 0x40, 0x45, 0x4C, 0x46 };
static const byte brightness_levels[] = { 0x20, 0x61, 0x62, 0x23, 0x64, 0x25, 0x26, 0x67, 0x68, 0x29, 0x2A, 0x2C, 0x6B, 0x6D, 0x6E, 0x2F };


SoftwareSerial mySerial(10, 11, true); // RX, TX

const uint8_t RELAY_PIN = 8;
const uint8_t BUTTON_PIN = A5;
const uint8_t REAR_GEAR_PIN = A4;
const uint8_t LED = 13;

void setup() 
{
	pinMode(LED, OUTPUT);
	pinMode(RELAY_PIN, OUTPUT);
	digitalWrite(RELAY_PIN, LOW);

	pinMode(BUTTON_PIN, INPUT);
	pinMode(REAR_GEAR_PIN, INPUT);
	
	Serial.begin(115200);
	Serial.println("Setup");

	mySerial.begin(2400);
	delay(200);
}

const unsigned long send_delay = 50;

bool x = true;

bool bMonitorOn = false;

enum { IDLE = 0, PRESS, WAIT_UNPRESS };
byte ButtomMode = IDLE;
byte RearGearMode = IDLE;

const unsigned long RearGearDelay = 500;
const unsigned long ButtonPressMonitorLiftUpTimeout = 90000;
const unsigned long RearGearMonitorLiftUpTimeout = 20000;

unsigned long MonitorLiftUpMaxTime = RearGearMonitorLiftUpTimeout;

unsigned long MonitorUpTime = 0;
unsigned long RearGearOnTime = 0;

// the loop function runs over and over again until power down or reset
void loop() 
{
	unsigned long Ms = millis();
	if (bMonitorOn)
	{
		if (Ms - MonitorUpTime > MonitorLiftUpMaxTime)
		{
			bMonitorOn = false;
			Serial.println("Time - Mon off");
			digitalWrite(RELAY_PIN, LOW);
		}
	}

	if(bMonitorOn)
	{
		mySerial.write(display_modes[RTI_NTSC]);
		delay(send_delay);
		mySerial.write((byte)0x40);
		delay(send_delay);
		mySerial.write((byte)0x83);
	}
	delay(send_delay);

	x = !x;
	if(x)
		digitalWrite(LED, HIGH);
	else
		digitalWrite(LED, LOW);


	bool bButton = digitalRead(BUTTON_PIN) == LOW;
	bool bRearGear = digitalRead(REAR_GEAR_PIN) == HIGH;

	switch (ButtomMode)
	{
	case IDLE:
		if (bButton)
			ButtomMode = PRESS;
		break;
	case PRESS:
		if (bButton)
		{
			ButtomMode = WAIT_UNPRESS;

			bMonitorOn = !bMonitorOn;
			if (bMonitorOn)
			{
				Serial.println("Button - Mon on");
				MonitorUpTime = Ms;
				MonitorLiftUpMaxTime = ButtonPressMonitorLiftUpTimeout;
				digitalWrite(RELAY_PIN, HIGH);
			}
			else
			{
				Serial.println("Button - Mon off");
				digitalWrite(RELAY_PIN, LOW);
			}
		}
		else
			ButtomMode = IDLE;
		break;
	case WAIT_UNPRESS:
		if (!bButton)
			ButtomMode = IDLE;
		break;
	default:
		break;
	}

	switch (RearGearMode)
	{
	case IDLE:
		if (bRearGear)
		{
			RearGearOnTime = Ms;
			RearGearMode = PRESS;
		}
		break;
	case PRESS:
		if (bRearGear)
		{
			if( Ms - RearGearOnTime > RearGearDelay ) // wait some time to prevent monitor lift up on park to drive switch
			{
				RearGearMode = WAIT_UNPRESS;

				Serial.println("Rear Gear on - Mon on");

				bMonitorOn = true;
				MonitorUpTime = Ms;
				MonitorLiftUpMaxTime = RearGearMonitorLiftUpTimeout;

				digitalWrite(RELAY_PIN, HIGH); // it disconnects rear gear input, and connects camera power permanently
			}
		}
		else
			RearGearMode = IDLE;
		break;
	case WAIT_UNPRESS:
		if( !bRearGear )
		{
			Serial.println("Rear gear off");
			RearGearMode = IDLE;
		}
		break;
	default:
		break;
	}
}
