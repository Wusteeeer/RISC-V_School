#include "gd32vf103.h"
#include "drivers.h"
#include "adc.h"
#include "lcd.h"
#include "usart.h"
#define EI 1
#define DI 0

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
    //rtcInit();                              // Initialize RTC
    //rtc_counter_set(3600+60+1);
    u0init(EI);                             // Initialize USART0 toolbox

    eclic_global_interrupt_enable();        // !!! INTERRUPT ENABLED !!!

    while (1) {
        idle++;                             // Manage Async events
        LCD_WR_Queue();                     // Manage LCD com queue!
        //u0_TX_Queue();                      // Manage U(S)ART TX Queue!
        if (adc_flag_get(ADC0,ADC_FLAG_EOC)==SET) { // ...ADC done?
          if (adc_flag_get(ADC0,ADC_FLAG_EOIC)==SET) { //...ch3 or ch16?
            tmpr = adc_inserted_data_read(ADC0, ADC_INSERTED_CHANNEL_0);
            //l88mem(6,((0x680-tmpr)/5)+25);
            // usart_data_transmit(USART0, ((0x680-tmpr)/5)+25); // USRAT0 TX!
            adc_flag_clear(ADC0, ADC_FLAG_EOC);
            adc_flag_clear(ADC0, ADC_FLAG_EOIC);
          } else {
            adcr = adc_regular_data_read(ADC0); // ......get data

            adc_flag_clear(ADC0, ADC_FLAG_EOC); // ......clear IF
          }
        }
        if (usart_flag_get(USART0,USART_FLAG_RBNE)){ // USART0 RX?
          // l88mem(6,usart_data_receive(USART0)); // Yes: Retrive & display!

          if(usart_data_receive(USART0) == 127){
            LCD_DeleteChar(&currentPosition[0], &currentPosition[1], BLACK); 

          }else{

            LCD_ShowChar(currentPosition[0],currentPosition[1],usart_data_receive(USART0), OPAQUE, WHITE);
            currentPosition[0] += 10;

            if(currentPosition[0] >= 150){
              currentPosition[1] += 20;
              currentPosition[0] = 10;
            }

          }
          
        }

        if (t5expq()) {                     // Manage periodic tasks
            l88row(colset());               // ...8*8LED and Keyboard
            ms++;                           // ...One second heart beat
            if (ms==1000){
              ms=0;
              l88mem(0,s++);
              //LCD_ShowStr(10, 30, digits[s%10], WHITE, OPAQUE);
              //LCD_ShowChar(10, 50, 0x7E, OPAQUE, WHITE);
            }
            if ((key=keyscan())>=0) {       // ...Any key pressed?
              hexDigits[currentDig] = lookUpTbl[key];

              if(currentDig == 1){

                uint16_t total = hexDigits[0] + hexDigits[1];
                
                if(total == 127){
                  
                  usart_data_transmit(USART0, 127);
                }
                else if(total > 31){

                  usart_data_transmit(USART0, total);


                }
                
                
                

              }

              if(currentDig == 0){
                hexDigits[0] *= 16;
              }

              currentDig++;

              currentDig %= 2;
            }
            l88mem(2,idle>>8);              // ...Performance monitor
            l88mem(3,idle); idle=0;
            adc_software_trigger_enable(ADC0, //Trigger another ADC conversion!
                                        ADC_REGULAR_CHANNEL);
        }
    }
}