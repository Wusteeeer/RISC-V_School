#include "gd32vf103.h"
#include "drivers.h"
#include "dac.h"
#include "pwm.h"
#include "adc.h"
#include "eclicw.h"

int main(void){
    int key, idle=0, ms=0;
    int lookUpTbl[16]={1,4,7,14,2,5,8,0,3,6,9,15,10,11,12,13};
    int adcr, tmpr;

    t5omsi();                               // Initialize timer5 1kHz
    colinit();                              // Initialize column toolbox
    l88init();                              // Initialize 8*8 led toolbox
    keyinit();                              // Initialize keyboard toolbox
    T1powerUpInitPWM(0x1);     
    ADC3powerUpInit(1);    

    while (1) {
        idle++;    
        
        if (adc_flag_get(ADC0,ADC_FLAG_EOC)==SET) { // ...ADC done?
          if (adc_flag_get(ADC0,ADC_FLAG_EOIC)==SET) { //...ch3 or ch16?
            tmpr = adc_inserted_data_read(ADC0, ADC_INSERTED_CHANNEL_0);
            //l88mem(6,((0x680-tmpr)/5)+25);
            adc_flag_clear(ADC0, ADC_FLAG_EOC);
            adc_flag_clear(ADC0, ADC_FLAG_EOIC);
          } else {

            //adcr is the read value from the potentiometer transformed from analog to digital signal (between 0 and 4080)
            adcr = adc_regular_data_read(ADC0); // ......get data
            


            adc_flag_clear(ADC0, ADC_FLAG_EOC); // ......clear IF
          }
        }


 
        if (t5expq()) {                     // Manage periodic tasks
          
          T1setPWMch0(map(adcr, 0, 0, 4080, 4000));


          l88row(colset());               // ...8*8LED and Keyboard

        
          // T1setPWMmotorB(map(adcr, 0, 0, 4079, 100));
            

          #pragma region fps
          l88mem(6,idle>>8);              // ...Performance monitor
          l88mem(7,idle); idle=0;
          #pragma endregion
            

          adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL); //Trigger another ADC conversion!
        }
    }
}