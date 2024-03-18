#include "gd32vf103.h"
#include "drivers.h"
#include "adc.h"
#include "lcd.h"
#include "usart.h"
#define EI 1
#define DI 0

typedef struct timer{
  uint8_t hour, minute, second;
}Timer;



void initTimer(uint8_t h, uint8_t m, uint8_t s, Timer time){
  time.hour = h;
  time.minute = m;
  time.second = s;
}

void updateTimer(Timer time){

 

  if(time.second >= 60){

    time.minute++;

    if(time.minute >= 60){
      time.minute = 0;
      time.hour++;

      if(time.hour >= 24){
        time.hour = 0;
      }
    }

    time.second=0;

    int totalTime=0;
    totalTime += (time.hour * 10000) + (time.minute * 100) + (time.second);
    rtc_counter_set(totalTime);
    
  }
}


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

void addToMessage(int *message, int *currentPos, char *newChar, int charAmount)
{

  for (int i = 0; i < charAmount; i++)
  {
    message[(*currentPos)++] = newChar[i];
  }
  
}


void addToMessage2(int *message, int *currentPos, int *newChar, int charAmount)
{

  for (int i = 0; i < charAmount; i++)
  {
    message[(*currentPos)++] = newChar[i];
  }
  
}

void addToMessage2Dig(int *message, int *currentPos, int newChar)
{
  int ten = (newChar / 10);
  int one = (newChar - (ten * 10));

  message[(*currentPos)++] = ten + 0x30;
  message[(*currentPos)++] = one + 0x30;
  
  
}

//TODO: Move old messages up when adding new one show SET after you have set the timer
//TODO: You should probably it over via message, so set message to "SET\n"
int main(void){
    int ms=0, s=0, key, pKey=-1, c=0, idle=0, rtc, hh, mm, ss;
    int lookUpTbl[16]={1,4,7,14,2,5,8,0,3,6,9,15,10,11,12,13};

    int setTime[6] = {0}, currentTime=5;

    Timer timer;
    initTimer(0, 0, 0, timer);

    int testing[15] = {0}, cTest=0;

    int isSet=0;

    int currentPos=0, currentMessage=0, currentRecieve=0;

    int result[6] = {0};
    
    int message[17], savedMessage[2][16], currSave=0, hasSaved=1, currSaveTot=0, currSaveInst=0;
    message[17] = '\n';

    int analysis[3], currentAnalysis=0, startAnalysis=0;
    

    int currentX=0;
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

  
    // addToMessage(message, &currentPos, "ID-000", 6);


    rtcInit();                              // Initialize RTC
    
    u0init(EI);                             // Initialize USART0 toolbox
    

    eclic_global_interrupt_enable();        // !!! INTERRUPT ENABLED !!!

    while (1) {
        idle++;                             // Manage Async events
        LCD_WR_Queue();                     // Manage LCD com queue!
        //u0_TX_Queue();                      // Manage U(S)ART TX Queue!

        if (adc_flag_get(ADC0,ADC_FLAG_EOC)==SET) { // ...ADC done?
          if (adc_flag_get(ADC0,ADC_FLAG_EOIC)==SET) { //...ch3 or ch16?
            tmpr = adc_inserted_data_read(ADC0, ADC_INSERTED_CHANNEL_0);
            // usart_data_transmit(USART0, ((0x680-tmpr)/5)+25); // USRAT0 TX!
            adc_flag_clear(ADC0, ADC_FLAG_EOC);
            adc_flag_clear(ADC0, ADC_FLAG_EOIC);
          } else {
            adcr = adc_regular_data_read(ADC0); // ......get data
            // usart_data_transmit(USART0, adcr);

            adc_flag_clear(ADC0, ADC_FLAG_EOC); // ......clear IF
            adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
          }
        }
        


        if(usart_flag_get(USART0, USART_FLAG_RBNE)){

          
          
          if(usart_data_receive(USART0) != '\0'){

            if(currentRecieve == 12 || currentRecieve == 6){
              currentX+=8;
            }


            currentRecieve++;
            LCD_ShowChar((currentX+=8), 50, usart_data_receive(USART0), OPAQUE, WHITE);
   

          }else{
            
            
            int x=0,y=30,j=0;

            for (int i = 0; i <= currSaveTot; i++)
            {
              while(savedMessage[i][j]!='\0')
              {
                if(j == 6 || j == 12){
                  x+=8;
                }
                if(savedMessage[i][j]!='\n')
                  LCD_ShowChar((x+=8), y, savedMessage[i][j], OPAQUE, WHITE);
                j+=1;
              }
              x=j=0;
              y-=20;

            }

            if(currSaveTot < 1)
              currSaveTot++;
            


     
            
           


            
          }

 
        }

        

      
        if (t5expq()) {                     // Manage periodic tasks
            l88row(colset());               // ...8*8LED and Keyboard
            

            if(isSet==0){
              
              if((key=keyscan()) >= 0){
                
                setTime[currentTime--] = lookUpTbl[key];
                
                if(currentTime >= -1){
                  
                  usart_data_transmit(USART0, lookUpTbl[key] + 0x30);

                  int lookKey[1] = {lookUpTbl[key] + 0x30};
                  addToMessage2(message, &currentPos, lookKey, 1);

                }

                if(lookUpTbl[key] == 13 && currentTime <= 0){
                  int totalTime=0, j=1;
                  for (int i = 0; i < 6; i++)
                  {
                    if(i < 2){
                      timer.second += (setTime[i] * j);
                      j*=10;
                    }
                    else if(i < 4){
                      if(i == 2){
                        j = 1;
                      }
                      
                      timer.minute += (setTime[i] * j);
                      j*=10;

                    }
                    else{
                      if(i == 4){
                        j = 1;
                      }

                      timer.hour += (setTime[i] * j);
                      j*=10;

                    }
                  }
                  totalTime += (timer.hour * 10000) + (timer.minute * 100) + (timer.second);


                  rtc_counter_set(totalTime);
                  addToMessage(message, &currentPos, "SET\n\0", 5);
                  currentX = 0;
                  currentRecieve = 0;
                  isSet=1;
              
                  LCD_Clear(BLACK);
                }
                
              }

              continue;
            }

            timer.hour = rtc_counter_get() / 10000;
            timer.minute = (rtc_counter_get() / 100) - (timer.hour * 100);
            timer.second = rtc_counter_get() - (timer.hour * 10000) - (timer.minute * 100);

            if((key=keyscan()) >= 0){

              LCD_Clear(BLACK);
              

              int temp=0;
              if(savedMessage[0] != NULL)
                addToMessage2(savedMessage[1], &temp, savedMessage[0], 16);
              temp=0;
              addToMessage2(savedMessage[0], &temp, message, 16);              
      
              
              
              currentMessage = 0;
              currentPos = 0;
              currentX = 0;       
              cTest=0;       
              currentRecieve = 0;

              currentAnalysis=0;
              startAnalysis=1;

              addToMessage2Dig(message, &currentPos, timer.hour);
              addToMessage2Dig(message, &currentPos, timer.minute);
              addToMessage2Dig(message, &currentPos, timer.second);

              switch(lookUpTbl[key]){
                case 10:

                  // int result[6] = {0};
                  result[0] = 'I';
                  result[1] = 'D';

                  result[2] = '+'; //temp will most likely be positive
                  if(((0x680-tmpr)/5)+25 < 0){
                    result[2] = '-';
                  }

            
                  result[3] = '0';

                  int temp = ((0x680-tmpr)/5)+25;

                  result[4] = ((temp / 10) % 10) + 0x30;
                  result[5] = (temp % 10) + 0x30;
                   
                  addToMessage2(message, &currentPos, result, 6);
                  addToMessage(message, &currentPos, "OK!", 3);
                  
                break;
                case 11:
                  addToMessage(message, &currentPos, "ID-000", 6);
                  addToMessage(message, &currentPos, "OK!", 3);
                break;
                case 12:
                  addToMessage(message, &currentPos, "ID*000", 6);
                  
                  addToMessage(message, &currentPos, "ERR", 3);
                break;
              }


              message[15] = '\0';
              message[16] = '\n';


              
              

            }

         
            
            

            
            if(usart_flag_get(USART0, USART_FLAG_TBE) && message[currentMessage] != '\n'){
              usart_data_transmit(USART0, message[currentMessage++]);
            }
          
            


               

            
            updateTimer(timer);           
            adc_software_trigger_enable(ADC0, //Trigger another ADC conversion!
                                        ADC_REGULAR_CHANNEL);
        }
    }
}