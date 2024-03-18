#include "gd32vf103.h"
#include "drivers.h"
#include "lcd.h"

int main(void){
    int ms=0, s=0, key, pKey=-1, c=0, idle=0;
    int lookUpTbl[16]={127,130,0,0,128,0,0,126,129,0,0,0,0,0,0,0};
    int currentBattery = 0;
    int dac=0, speed=-100;
    int adcr, tmpr;
    

    t5omsi();                               // Initialize timer5 1kHz
    colinit();                              // Initialize column toolbox
    l88init();                              // Initialize 8*8 led toolbox
    keyinit();                              // Initialize keyboard toolbox
    Lcd_SetType(LCD_NORMAL);                // or use LCD_INVERTED!
    Lcd_Init();
    LCD_Clear(BLACK);
    LCD_ShowStr(10, 10, "POLL VERSION", WHITE, TRANSPARENT);
    LCD_ShowChar(10, 30, 126, 0, WHITE);

    while (1) {
        idle++;                             // Manage Async events
        LCD_WR_Queue();                   // Manage LCD com queue!

        if (t5expq()) {                     // Manage periodic tasks
            l88row(colset());               // ...8*8LED and Keyboard

            if(currentBattery != 0){
              LCD_ShowChar(10, 30, currentBattery, 0, WHITE);
            }
            
            if ((key=keyscan())>=0) {       // ...Any key pressed?
              currentBattery = lookUpTbl[key];
            }
            l88mem(2,idle>>8);              // ...Performance monitor
            l88mem(3,idle); idle=0;
        }
    }
}