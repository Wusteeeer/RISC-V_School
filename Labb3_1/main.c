#include "gd32vf103.h"
#include "drivers.h"
#include "dac.h"
#include "pwm.h"

//Stages:
//Check button press
//D pressed ? Show on LED
//# pressed ? erase everything
//* pressed ? delete last element
//none of the above (a.k.a number pressed) ? add number and move previous ones to the next power of 10
//move up to the next stage and set total

int main(void){
    int key, idle=0, hundreds=0, tens=0, ones=0, stage=0, total=0;
    int lookUpTbl[16]={1,4,7,14,2,5,8,0,3,6,9,15,10,11,12,13};

    t5omsi();                               // Initialize timer5 1kHz
    colinit();                              // Initialize column toolbox
    l88init();                              // Initialize 8*8 led toolbox
    keyinit();                              // Initialize keyboard toolbox
    T1powerUpInitPWM(0x1);     
    T1setPWMch0(0);  //Start with LED turned off  

    while (1) {
        idle++;                             // Manage Async events
 
        if (t5expq()) {                     // Manage periodic tasks



          l88row(colset());               // ...8*8LED and Keyboard
  
            if ((key=keyscan())>=0) {       // ...Any key pressed?
              
              if(lookUpTbl[key] == 13) //D on keypad
              {

                //Doesnt seem to be dropping performance too much (yippie)
                int mappedTotal = map(total, 0, 0, 100, 15999); //Calling map function to map total between 0 and 15000 instead of 0 and 100
                T1setPWMch0(mappedTotal);   //Setting brightness of A0 LED
                continue;
              }
              

              if(lookUpTbl[key] == 15){ //15 is delete
                
                //reset all values
                ones = 0;
                tens = 0;
                hundreds = 0;
                total = 0;
                stage = 0;

                //Wipe the 8*8 display
                for (int i = 0; i < 4; i++)
                {
                  l88mem(i, 0x0);
                }
                
                //Dont do anything else (so we dont add 15)
                continue;
              }

              if(lookUpTbl[key] == 14) //This is erase
              {
                
                //Checks which stage we are on (the previous stage that is)
                if(stage - 1 == 2){
                  tens = hundreds / 10;
                  l88mem(1, tens);
                  l88mem(0, 0x0);
                  hundreds = 0;
                }
                else if(stage - 1 == 1){
                  ones = tens / 10;
                  l88mem(2, ones);
                  l88mem(1, 0x0);
                  tens = 0;
                }

                stage--;

                total = ones + tens + hundreds;
                l88mem(3, total);
                continue; //So we dont add 14
              }

              if(stage == 1){
                //Moves tens up one space
                tens = ones*10;
                l88mem(1, tens);
              }
              else if(stage == 2){
                //Moves hundreds up one space...
                hundreds = tens*10;
                l88mem(0, hundreds);

                //...then moves tens up one space
                tens = ones*10;
                l88mem(1, tens);
              }

              ones=lookUpTbl[key];
              l88mem(2,ones);
              stage++;

              total = 0;
              total = total + ones;
              total = total + tens;
              total = total + hundreds;
              l88mem(3, total);


            }

            #pragma region fps
            l88mem(6,idle>>8);              // ...Performance monitor
            l88mem(7,idle); idle=0;
            #pragma endregion
            

            
        }
    }
}