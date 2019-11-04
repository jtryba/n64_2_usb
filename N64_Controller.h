//N64_Controller.h

#ifndef N64_Controller_H
#define N64_Controller_H

#include <Arduino.h>

#define N64_PIN 2
#define N64_PIN_REGISTER 0x02
#define N64_PIN_DIR DDRD
// these two macros set arduino pin 2 to input or output, which with an
// external 1K pull-up resistor to the 3.3V rail, is like pulling it high or
// low.  These operations translate to 1 op code, which takes 2 cycles
#define N64_HIGH DDRD &= ~N64_PIN_REGISTER
#define N64_LOW DDRD |= N64_PIN_REGISTER
#define N64_QUERY (PIND & N64_PIN_REGISTER)


// 8 bytes of data that we get from the controller
struct N64_status {
    //bits: A, B, Z, Start, Dup, Ddown, Dleft, Dright
    unsigned char data1;
    // bits: 0, 0, L, R, Cup, Cdown, Cleft, Cright
    unsigned char data2;
    char stick_x;
    char stick_y;
};

class N64_Controller
{
public:
    N64_Controller();
    ~N64_Controller();
    void Initialize();
    void Update();
    char GetStick_x();
    char GetStick_y();
    uint16_t Getbuttons();
        
    void print_N64_status(bool Verbose);

private:
    void Send(unsigned char *buffer, char length);
    void Get();
    void translate_raw_data();
    char Raw_Dump[33]; // 1 received bit per byte
    char stick_x_offset;
    char stick_y_offset;
    N64_status CurrentStatus;
};


#endif //N64_CONTROLLER_H
