# n64_2_usb

MUST USE VERSION 1.6.0 Arduino IDE TO COMPILE PROPERLY
(please refer here: https://www.instructables.com/id/Turn-an-N64-Controller-into-a-USB-Gamepad-using-an/
  please read: "Step 1: Setting Up the Arduino Environment")

tuned the joystick, as none of my n64 controllers were able to reach the max/min values of 127 and -127.
they were only reaching values of around 80 or 90 and -80 or -90. (see: arduino->map)
aslo added a small deadzone to the joystick.
I added a switch function for debugging, and tweaked the output of it.
