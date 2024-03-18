#include "gd32vf103.h"
#include "drivers.h"
#include "adc.h"
#include "lcd.h"
#include "usart.h"
#include "math.h"
#define EI 1
#define DI 0

//NOTES: Trigonometric functions work!


void rtcInit(void){
   // enable power managemenet unit - perhaps enabled by default
   rcu_periph_clock_enable(RCU_PMU);
   // enable write access to the registers in the backup domain
   pmu_backup_write_enable();
   // enable backup domain
   rcu_periph_clock_enable(RCU_BKPI);
   // reset backup domain registers
   bkp_deinit();
   // set the results of a previous calibration procedure
   // bkp_rtc_calibration_value_set(x);

   // setup RTC
   // enable external low speed XO
   //rcu_osci_on(RCU_LXTAL);
   if (rcu_osci_stab_wait(RCU_HXTAL)) {
     // use external low speed oscillaotr, i.e. 32.768 kHz
     rcu_rtc_clock_config(RCU_RTCSRC_HXTAL_DIV_128);
     rcu_periph_clock_enable(RCU_RTC);
     // wait until shadow registers are synced from the backup domain
     // over the APB bus
     rtc_register_sync_wait();
     // wait until shadow register changes are synced over APB
     // to the backup doamin
     rtc_lwoff_wait();
     // prescale to 1 second
     rtc_prescaler_set(62500 - 1);
     rtc_lwoff_wait();
     rtc_flag_clear(RTC_INT_FLAG_SECOND);
     //rtc_interrupt_enable(RTC_INT_SECOND);
     rtc_lwoff_wait();
   }
}

int main(void){
    int ms=0, s=0, key, pKey=-1, c=0, idle=0, rtc, hh, mm, ss;
    int lookUpTbl[16]={1,4,7,14,2,5,8,0,3,6,9,15,10,11,12,13};
    int hexDigits[2] = {0}, currentDig=0;
    int currentPosition[2] = {10, 0};
    int dac=0, speed=-100;
    int adcr, tmpr;
    //char time[7]={0};

    t5omsi();                               // Initialize timer5 1kHz
    colinit();                              // Initialize column toolbox
    l88init();                              // Initialize 8*8 led toolbox
    keyinit();                              // Initialize keyboard toolbox
    ADC3powerUpInit(1);                     // Initialize ADC0, Ch3
    Lcd_SetType(LCD_NORMAL);//NORMAL);                // or use LCD_INVERTED!
    Lcd_Init();
    LCD_Clear(BLACK);


    while (1) {
  
        LCD_WR_Queue();                     // Manage LCD com queue!
        
        if (t5expq()) {                     // Manage periodic tasks
          l88row(colset());               // ...8*8LED and 

          LCD_ShowNum1(10, 30, 0, 3, WHITE);
        }
    }
}