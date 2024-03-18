#include "gd32vf103.h"
#include "drivers.h"
#include "lcd.h"
#include "dac.h"
#include "pwm.h"
#include "adc.h"
#include "eclicw.h"

typedef struct point{
  int x, y;
}Point;

void initPoint(int x, int y, Point *p){
  p->x = x;
  p->y = y;
}

void destroyPoint(Point *p){
  free(p);
}

void drawPoint(Point *p){
  LCD_DrawPoint(p->x, p->y, WHITE);
}

void movePoint(Point *p, int *amountOfPoints){
  LCD_DrawPoint(p->x, p->y, BLACK);

  p->x++;

  drawPoint(p);

}


int main(void){
    int ms=0, s=0, key, pKey=-1, c=0, idle=0;
    int lookUpTbl[16]={10,1000,1000,1000,100,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000};
    int dac=0, speed=-100;
    int adcr, tmpr;
    
    Point points[170];
    int amountOfPoints=-1, currentPoint=0, yOffset=0, currentSpeed=10;

    for (int i = 0; i < 170; i++)
    {
      initPoint(-1, 40, &points[i]);
    }
    
  
    int currentX = 0;

    int lastAdcr = -1;

    t5omsi();                               // Initialize timer5 1kHz
    colinit();                              // Initialize column toolbox
    l88init();                              // Initialize 8*8 led toolbox
    keyinit();                              // Initialize keyboard toolbox
    Lcd_SetType(LCD_NORMAL);                // or use LCD_INVERTED!
    Lcd_Init();
    ADC3powerUpInit(1);    
    LCD_Clear(BLACK);

    


    //LCD_ShowStr(10, 10, "POLL VERSION", WHITE, TRANSPARENT);

    while (1) {
        idle++;                             // Manage Async events
        LCD_WR_Queue();                   // Manage LCD com queue!

        

        if (adc_flag_get(ADC0,ADC_FLAG_EOC)==SET) { // ...ADC done?
          if (adc_flag_get(ADC0,ADC_FLAG_EOIC)==SET) { //...ch3 or ch16?
            tmpr = adc_inserted_data_read(ADC0, ADC_INSERTED_CHANNEL_0);
            //l88mem(6,((0x680-tmpr)/5)+25);
            adc_flag_clear(ADC0, ADC_FLAG_EOC);
            adc_flag_clear(ADC0, ADC_FLAG_EOIC);
          } else {

            //adcr is the read value from the potentiometer transformed from analog to digital signal (between 0 and 4080)
            adcr = adc_regular_data_read(ADC0); // ......get data
            
            lastAdcr = map(adcr, 0, 0, 4079, 79);
            

            adc_flag_clear(ADC0, ADC_FLAG_EOC); // ......clear IF
          }
        }

        if (t5expq()) {                     // Manage periodic tasks
            l88row(colset());               // ...8*8LED and Keyboard
            ms++;

           
            if(ms >= currentSpeed){

              amountOfPoints++;

              if(amountOfPoints > 161){
                amountOfPoints--;
              }
              
              
              for (int i = 0; i < amountOfPoints; i++)
              {
                if(points[i].x < 170){
                  movePoint(&points[i], &amountOfPoints);

                }
                
              }

              points[currentPoint].y = lastAdcr;
              points[currentPoint].x = -1;


              currentPoint++;

              if(currentPoint > 160){
                currentPoint = 0;
              }

              ms = 0;

              
            }

            if((key=keyscan()) >= 0){
              currentSpeed = lookUpTbl[key];
            }

            adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL); //Trigger another ADC conversion!
        }
    }
}