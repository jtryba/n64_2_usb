//N64_Controller.cpp

#include "N64_Controller.h"

N64_Controller::N64_Controller() {}

N64_Controller::~N64_Controller() {}

void N64_Controller::Initialize()
{
    
  // Communication with gamecube controller on this pin
  // Don't remove these lines, we don't want to push +5V to the controller
  digitalWrite(N64_PIN, LOW);  
  pinMode(N64_PIN, INPUT);


  // Initialize the gamecube controller by sending it a null byte.
  // This is unnecessary for a standard controller, but is required for the
  // Wavebird.
  unsigned char initialize = 0x00;
  noInterrupts();
  Send(&initialize, 1);

  // Stupid routine to wait for the gamecube controller to stop
  // sending its response. We don't care what it is, but we
  // can't start asking for status if it's still responding
  for (byte ii=0; ii < 64; ii++) {
      // make sure the line is idle for 64 iterations, should
      // be plenty.
      if (!N64_QUERY) ii = 0;
  }

  // Query for the gamecube controller's status. We do this
  // to get the 0 point for the control stick.
  unsigned char command[] = {0x01};
  Send(command, 1);
  // read in data and dump it to N64_raw_dump
  Get();
  interrupts();
  translate_raw_data(); 

  delay(25);
  Update();

  stick_x_offset = CurrentStatus.stick_x;
  stick_y_offset = CurrentStatus.stick_y;
}

void N64_Controller::Update()
{
    // Command to send to the gamecube
    // The last bit is rumble, flip it to rumble
    // yes this does need to be inside the loop, the
    // array gets mutilated when it goes through N64_send
    unsigned char command[] = {0x01};

    // don't want interrupts getting in the way
    noInterrupts();
    // send those 3 bytes
    Send(command, 1);
    // read in data and dump it to N64_raw_dump
    Get();
    // end of time sensitive code
    interrupts();

    // translate the data in N64_raw_dump to something useful
    translate_raw_data(); 
}

void N64_Controller::translate_raw_data()
{
    // The get_N64_status function sloppily dumps its data 1 bit per byte
    // into the get_status_extended char array. It's our job to go through
    // that and put each piece neatly into the struct N64_status
    int ii;
    memset(&CurrentStatus, 0, sizeof(N64_status));
    // line 1
    // bits: A, B, Z, Start, Dup, Ddown, Dleft, Dright
    for (ii = 0; ii < 8; ii++) {
        CurrentStatus.data1 |= Raw_Dump[ii] ? (0x80 >> ii) : 0;
    }
    // line 2
    // bits: 0, 0, L, R, Cup, Cdown, Cleft, Cright
    for (ii = 0; ii < 8; ii++) {
        CurrentStatus.data2 |= Raw_Dump[8+ii] ? (0x80 >> ii) : 0;
    }
    // line 3
    // bits: joystick x value
    // These are 8 bit values centered at 0x80 (128)
    for (ii = 0; ii < 8; ii++) {
        CurrentStatus.stick_x |= Raw_Dump[16+ii] ? (0x80 >> ii) : 0;
    }
    
    for (ii = 0; ii < 8; ii++) {
        CurrentStatus.stick_y |= Raw_Dump[24+ii] ? (0x80 >> ii) : 0;
    }

    CurrentStatus.stick_x -= stick_x_offset;
    CurrentStatus.stick_y -= stick_y_offset;
}

/**
 * This sends the given byte sequence to the controller
 * length must be at least 1
 * Oh, it destroys the buffer passed in as it writes it
 */
void N64_Controller::Send(unsigned char *buffer, char length)
{
    // Send these bytes
    char bits;
    
    bool bit;

    // This routine is very carefully timed by examining the assembly output.
    // Do not change any statements, it could throw the timings off
    //
    // We get 16 cycles per microsecond, which should be plenty, but we need to
    // be conservative. Most assembly ops take 1 cycle, but a few take 2
    //
    // I use manually constructed for-loops out of gotos so I have more control
    // over the outputted assembly. I can insert nops where it was impossible
    // with a for loop
    
    asm volatile (";Starting outer for loop");
outer_loop:
    {
        asm volatile (";Starting inner for loop");
        bits=8;
inner_loop:
        {
            // Starting a bit, set the line low
            asm volatile (";Setting line to low");
            N64_LOW; // 1 op, 2 cycles

            asm volatile (";branching");
            if (*buffer >> 7) {
                asm volatile (";Bit is a 1");
                // 1 bit
                // remain low for 1us, then go high for 3us
                // nop block 1
                asm volatile ("nop\nnop\nnop\nnop\nnop\n");
                
                asm volatile (";Setting line to high");
                N64_HIGH;

                // nop block 2
                // we'll wait only 2us to sync up with both conditions
                // at the bottom of the if statement
                asm volatile ("nop\nnop\nnop\nnop\nnop\n"  
                              "nop\nnop\nnop\nnop\nnop\n"  
                              "nop\nnop\nnop\nnop\nnop\n"  
                              "nop\nnop\nnop\nnop\nnop\n"  
                              "nop\nnop\nnop\nnop\nnop\n"  
                              "nop\nnop\nnop\nnop\nnop\n"  
                              );

            } else {
                asm volatile (";Bit is a 0");
                // 0 bit
                // remain low for 3us, then go high for 1us
                // nop block 3
                asm volatile ("nop\nnop\nnop\nnop\nnop\n"  
                              "nop\nnop\nnop\nnop\nnop\n"  
                              "nop\nnop\nnop\nnop\nnop\n"  
                              "nop\nnop\nnop\nnop\nnop\n"  
                              "nop\nnop\nnop\nnop\nnop\n"  
                              "nop\nnop\nnop\nnop\nnop\n"  
                              "nop\nnop\nnop\nnop\nnop\n"  
                              "nop\n");

                asm volatile (";Setting line to high");
                N64_HIGH;

                // wait for 1us
                asm volatile ("; end of conditional branch, need to wait 1us more before next bit");
                
            }
            // end of the if, the line is high and needs to remain
            // high for exactly 16 more cycles, regardless of the previous
            // branch path

            asm volatile (";finishing inner loop body");
            --bits;
            if (bits != 0) {
                // nop block 4
                // this block is why a for loop was impossible
                asm volatile ("nop\nnop\nnop\nnop\nnop\n"  
                              "nop\nnop\nnop\nnop\n");
                // rotate bits
                asm volatile (";rotating out bits");
                *buffer <<= 1;

                goto inner_loop;
            } // fall out of inner loop
        }
        asm volatile (";continuing outer loop");
        // In this case: the inner loop exits and the outer loop iterates,
        // there are /exactly/ 16 cycles taken up by the necessary operations.
        // So no nops are needed here (that was lucky!)
        --length;
        if (length != 0) {
            ++buffer;
            goto outer_loop;
        } // fall out of outer loop
    }

    // send a single stop (1) bit
    // nop block 5
    asm volatile ("nop\nnop\nnop\nnop\n");
    N64_LOW;
    // wait 1 us, 16 cycles, then raise the line 
    // 16-2=14
    // nop block 6
    asm volatile ("nop\nnop\nnop\nnop\nnop\n"
                  "nop\nnop\nnop\nnop\nnop\n"  
                  "nop\nnop\nnop\nnop\n");
    N64_HIGH;

}

void N64_Controller::Get()
{
    // listen for the expected 8 bytes of data back from the controller and
    // blast it out to the N64_raw_dump array, one bit per byte for extra speed.
    // Afterwards, call translate_raw_data() to interpret the raw data and pack
    // it into the N64_status struct.
    asm volatile (";Starting to listen");
    unsigned char timeout;
    char bitcount = 32;
    char *bitbin = Raw_Dump;

    // Again, using gotos here to make the assembly more predictable and
    // optimization easier (please don't kill me)
read_loop:
    timeout = 0x3f;
    // wait for line to go low
    while (N64_QUERY) {
        if (!--timeout)
            return;
    }
    // wait approx 2us and poll the line
    asm volatile (
                  "nop\nnop\nnop\nnop\nnop\n"  
                  "nop\nnop\nnop\nnop\nnop\n"  
                  "nop\nnop\nnop\nnop\nnop\n"  
                  "nop\nnop\nnop\nnop\nnop\n"  
                  "nop\nnop\nnop\nnop\nnop\n"  
                  "nop\nnop\nnop\nnop\nnop\n"  
            );
    *bitbin = N64_QUERY;
    ++bitbin;
    --bitcount;
    if (bitcount == 0)
        return;

    // wait for line to go high again
    // it may already be high, so this should just drop through
    timeout = 0x3f;
    while (!N64_QUERY) {
        if (!--timeout)
            return;
    }
    goto read_loop;
}

char N64_Controller::GetStick_x() { return CurrentStatus.stick_x; }

char N64_Controller::GetStick_y() { return CurrentStatus.stick_y; }

uint16_t N64_Controller::Getbuttons() { return ((uint16_t)CurrentStatus.data1 << 6) + CurrentStatus.data2; }

void N64_Controller::print_N64_status(bool Verbose)
{
  if(Verbose == false)
  {
     for (byte ii = 0; ii < 16; ii++) 
     {
         Serial.print(Raw_Dump[ii] == N64_PIN_REGISTER, DEC);
     }
     Serial.print(' ');
     Serial.print(CurrentStatus.stick_x, DEC);
     Serial.print(' ');
     Serial.print(CurrentStatus.stick_y, DEC);
     Serial.print(" \n");
  }
  else
  {
    // bits: A, B, Z, Start, Dup, Ddown, Dleft, Dright
    // bits: 0, 0, L, R, Cup, Cdown, Cleft, Cright
    Serial.println();
    Serial.print("Start: ");
    Serial.println(CurrentStatus.data1 & 16 ? 1:0);

    Serial.print("Z:     ");
    Serial.println(CurrentStatus.data1 & 32 ? 1:0);

    Serial.print("B:     ");
    Serial.println(CurrentStatus.data1 & 64 ? 1:0);

    Serial.print("A:     ");
    Serial.println(CurrentStatus.data1 & 128 ? 1:0);

    Serial.print("L:     ");
    Serial.println(CurrentStatus.data2 & 32 ? 1:0);
    Serial.print("R:     ");
    Serial.println(CurrentStatus.data2 & 16 ? 1:0);

    Serial.print("Cup:   ");
    Serial.println(CurrentStatus.data2 & 0x08 ? 1:0);
    Serial.print("Cdown: ");
    Serial.println(CurrentStatus.data2 & 0x04 ? 1:0);
    Serial.print("Cright:");
    Serial.println(CurrentStatus.data2 & 0x01 ? 1:0);
    Serial.print("Cleft: ");
    Serial.println(CurrentStatus.data2 & 0x02 ? 1:0);
    
    Serial.print("Dup:   ");
    Serial.println(CurrentStatus.data1 & 0x08 ? 1:0);
    Serial.print("Ddown: ");
    Serial.println(CurrentStatus.data1 & 0x04 ? 1:0);
    Serial.print("Dright:");
    Serial.println(CurrentStatus.data1 & 0x01 ? 1:0);
    Serial.print("Dleft: ");
    Serial.println(CurrentStatus.data1 & 0x02 ? 1:0);

    Serial.print("Stick X:");
    Serial.println(CurrentStatus.stick_x, DEC);
    Serial.print("Stick Y:");
    Serial.println(CurrentStatus.stick_y, DEC);
  }
}
