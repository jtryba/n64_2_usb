/**
 * Gamecube controller to Nintendo 64 adapter
 * by Andrew Brown
 * Rewritten for N64 to HID by Peter Den Hartog
 * Then Rewritten once again for USB Gamepad compliance by Bob Jones
 * And once more by jtryba
 */

/**
 * To use, hook up the following to the Arduino Leonardo:
 * Digital I/O 2: N64 serial line
 * All appropriate grounding and power lines
 */
 
#include "crc_table.h"
#include "N64_Controller.h"

#define JOY_DEAD 5
#define JOY_MAX 80

// Joystick State
JoyState_t JoySt;

N64_Controller N64Controller;

unsigned int debugPin = 10;

bool debug_enable = false;

void setup()
{
  pinMode(debugPin, INPUT_PULLUP);
  delay(10);
  if (digitalRead(debugPin) == LOW)
  {
    debug_enable = true;

    Serial.begin(9600);
  
    //Needed for Leonardo
    while(!Serial);
  }

  N64Controller.Initialize();
  
  JoySt.XAxis = 127;
  JoySt.YAxis = 127;
  JoySt.Buttons = 0;
}

void loop()
{
  N64Controller.Update();
  
  signed int x = N64Controller.GetStick_x();
  
  if (x > -JOY_DEAD && x < JOY_DEAD)
  {
    x = 0;
  }
  else
  {
    x = constrain(x, -JOY_MAX, JOY_MAX);
  }
  
  x = map(x, -JOY_MAX, JOY_MAX, -127, 127);
  
  signed int y = N64Controller.GetStick_y();
  
  if (y > -JOY_DEAD && y < JOY_DEAD)
  {
    y = 0;
  }
  else
  {
    y = constrain(y, -JOY_MAX, JOY_MAX);
  }
  
  y = map(y, -JOY_MAX, JOY_MAX, -127, 127);
  
  // invert y axis
  if (y > 0)
  {
    y = 0 - y;
  }
  else if (y < 0)
  {
    int y2 = 0 + -y;
    y = y2;
  }
  
  JoySt.XAxis = 127 + x;
  
  JoySt.YAxis = 127 + y;
  
  JoySt.Buttons = N64Controller.Getbuttons();

  if (debug_enable)
  {
    char buf[32];
    memset(buf, 0, 32);
    sprintf(buf, "0x%d%d%d%d%d%d%d%d%d%d%d%d%d%d",
      bitRead(JoySt.Buttons, 13),
      bitRead(JoySt.Buttons, 12),
      bitRead(JoySt.Buttons, 11),
      bitRead(JoySt.Buttons, 10),
      bitRead(JoySt.Buttons, 9),
      bitRead(JoySt.Buttons, 8),
      bitRead(JoySt.Buttons, 7),
      bitRead(JoySt.Buttons, 6),
      bitRead(JoySt.Buttons, 5),
      bitRead(JoySt.Buttons, 4),
      bitRead(JoySt.Buttons, 3),
      bitRead(JoySt.Buttons, 2),
      bitRead(JoySt.Buttons, 1),
      bitRead(JoySt.Buttons, 0)
    );
    Serial.print(buf);
    Serial.print(' ');
    Serial.print(x, DEC);
    Serial.print(' ');
    Serial.println(y, DEC);
    //N64Controller.print_N64_status(true);
  }
  
  Joystick.setState(&JoySt);
  delay(10);
}
