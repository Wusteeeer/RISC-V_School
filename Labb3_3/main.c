#include "gd32vf103.h"
#include "drivers.h"
#include "dac.h"
#include "pwm.h"
#include "adc.h"
#include "eclicw.h"
#include "stdlib.h"
#define MAXCARS 5

//TODO: ALLT ÄR BASICALLY KLART! Testa en massa och se om du kan förbättra bilarna på något sätt

typedef struct car{
    
  int pos;
  int speed;
  int row;
  int moveTime;


}Car;

Car *createCar(int p, int s, int row){
  
  Car *c = malloc(sizeof(struct car));

  c->pos = p;
  c->speed = s;
  c->row = row;
  c->moveTime = 0;

  return c;

}




void moveCar(Car *c){
  c->row = c->row + c->speed; //Shift it one step to the left (sll) by the speed
}

void resetCar(Car *c, int newPos){
  c->moveTime = 0;
  c->speed = 0x1;

  c->pos = 0x1 << newPos;
  

  c->row = 0x0;
}


void mt40k_init(void (*pISR)(void)){
   eclicw_enable(CLIC_INT_TMR, 1, 1, pISR);
   // Set the timer expier (compar) value to 675 (27Mhz/40kHz).
   *( volatile uint64_t * )( TIMER_CTRL_ADDR + TIMER_MTIMECMP ) = 675;
   // Reset the timer value to zero.
   *( volatile uint64_t * )( TIMER_CTRL_ADDR + TIMER_MTIME ) = 0;
}
/*
void lp(void){
    static int x=0;

    // Be aware of possible spirous int updating mtimecmp...
    // LSW = -1; MSW = update; LSW = update, in this case safe.
    *( volatile uint64_t * )( TIMER_CTRL_ADDR + TIMER_MTIMECMP )+=675;

    // Calcylate next output value...
    DAC0set(x++%2?4000:0);
    l88mem(7,x);
}
*/

void lp(void){
    static int x=0, xp=0, y=0, yp=0;
    static int g=0b0010001100000000; //0.1367287359(<<16)
    static int p=0b1011100111111110; //0.7265425280(<<16)

    // Be aware of possible spirous int updating mtimecmp...
    // LSW = -1; MSW = update; LSW = update, in this case safe.
    *( volatile uint64_t * )( TIMER_CTRL_ADDR + TIMER_MTIMECMP )+=675;

    // Calcylate next output value...
    xp=x; yp=y;
    x = ((adc_regular_data_read(ADC0)-2048))*g;
    y = (x+xp)+(int)(((int64_t)yp*(int64_t)p)>>16);
    DAC0set((y>>16)+2048);              // ...and present it to hw-driver.

    adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);


    
}

void initGameOver(uint8_t *over){
  *over = 1;
}

int main(void){

    srand(42);

    int playerPos=0, speedFlag=0;
    int carUniTime = 500;
    int score=0, scoreFlag=0;
    int spawnTime = 0, resetTime=0, speedTime=0;

    int adcr, tmpr;

    uint8_t gameOver = 0;

    Car *cars[100];
    int amountOfCars = 0;

    t5omsi();                               // Initialize timer5 1kHz
    colinit();                              // Initialize column toolbox
    l88init();                              // Initialize 8*8 led toolbox
    keyinit();                              // Initialize keyboard toolbox
    ADC3powerUpInit(1);                     // Initialize ADC0, Ch3 & Ch16

    while (1) {


      if(gameOver == 1){
        if (t5expq()){
          l88mem(1, score);
          l88mem(0, score >> 16);
          l88row(colset());
          for (int i = 0; i < 8; i++)
          {
            l88mem(i, 0x0);
          }
        }

        continue;
      }

      


        if (adc_flag_get(ADC0,ADC_FLAG_EOC)==SET) { // ...ADC done?
          if (adc_flag_get(ADC0,ADC_FLAG_EOIC)==SET) { //...ch3 or ch16?
            tmpr = adc_inserted_data_read(ADC0, ADC_INSERTED_CHANNEL_0);
            //l88mem(6,((0x680-tmpr)/5)+25);
            adc_flag_clear(ADC0, ADC_FLAG_EOC);
            adc_flag_clear(ADC0, ADC_FLAG_EOIC);
          } else {

            //adcr is the read value from the potentiometer transformed from analog to digital signal (between 0 and 4080)
            adcr = adc_regular_data_read(ADC0); // ......get data
            adcr = adcr >> 9; //Divide it down to only allow values between 0..7

            int val = 0x80 >> adcr; //Move from the left by the transformed signal to make wheel feel correct

            playerPos = val;
           
            l88mem(6, val); //Show the player

            adc_flag_clear(ADC0, ADC_FLAG_EOC); // ......clear IF
          }
        }

        for (int i = 0; i < amountOfCars; i++)
        {


          int val = cars[i]->pos | rowGet(cars[i]->row); //Allow for multiple thing to be drawn on the same row

          if(cars[i]->pos == playerPos && cars[i]->row == 6){ //If a car and the player are at the same location ? Game over
            
            initGameOver(&gameOver);
            continue;
          }


          if(cars[i]->row >= 7){ //If a car is below the player ? give point
            if(scoreFlag==0){
              score++;
              
              scoreFlag=1;

            }
          }

          l88mem(cars[i]->row, val); //Load cars into memory

      
        }

        if (t5expq()) {   // Manage periodic tasks
          
          l88row(colset());               // ...8*8LED and Keyboard
          for (int i = 0; i < 8; i++)
          {
            l88mem(i, 0x0); //Erased everything after every draw to not leave remnants
          }


          if(spawnTime >= carUniTime * 2){ //Scale the spawning according to how fast the cars are
            
            int newPos = rand() % 8; //Get a random offset

            if(amountOfCars <= MAXCARS){ //As long as the amount of cars has not exceeded the max amount of cars
              cars[amountOfCars] = createCar(0x1 << newPos, 0x1, 0x0); //Create a new car
              amountOfCars++; 
            }else{
              
              for (int i = 0; i < amountOfCars; i++)
              {
                if(cars[i]->row >= 8){
                  resetCar(cars[i], newPos); //Move car to new position
                }
               
              }
              
            }
           
            spawnTime = 0; //Reset time
            scoreFlag = 0; //Reset score flag to allow for more scoring

          }

          spawnTime++; //Increase spawn timer
          speedTime++; //Increase speed timer

          for (int i = 0; i < amountOfCars; i++)
          {
            cars[i]->moveTime++;

            if(cars[i]->moveTime >= carUniTime){
              moveCar(cars[i]); //Move car after move timer has exceeded carunitime
              cars[i]->moveTime = 0; //Reset move timer
          
            }

  
          }

          //Increase speed of cars
          if(speedTime >= 3000){
            if(carUniTime >= 75){
              carUniTime -= 20;
            }
            speedTime=0;
          }

          adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL); //Trigger another ADC conversion!
          

        }
    }
}