.section .text

.equ GPIOA,  0x40010800     // GPIOA Base address.
.equ GPIOB,  0x40010C00     // GPIOB Base address.
.equ CTR0,   0x00           // Port control register 0 offset
.equ ISTAT,  0x08           // Port input status register
.equ OCTL,   0x0C           // Port output control register
.equ BOP,    0x10           // Port bit operate register offset
.equ BC,     0x14           // Port bit clear register offset
.equ GPIO_AM, 0x0           // Mode: Input, Analog Mode
.equ GPIO_50, 0x3           // Mode: Output, max speed 50MHz
.equ GPIO_FI, 0x4           // Conf: (Input) Floating input
.equ GPIO_UD, 0x8           // Conf: (Input) Pull-up/-down
.equ GPIO_PP, 0x0           // Conf: (Output) Push-pull
.equ GPIO_OD, 0x4           // Conf: (Output) Open-drain


//Configs a GPIO-module
//a0 - Which GPIO-module
//a1 - Input mode
//a2 - Mode
//a3 - Targeted pin
gpioconf:
    mv t0, a0 //Loads GPIOX-module
    mv t4, a3 //t4 is targeted pin

    li t1, 0x8 //Is it CTR0 or CTR1
    blt t4, t1, 1f //If pin is less than 8 skip
    addi t0, t0, 4 //Moves to next control register because first one is at t0 with offset 0
    addi t4, t4, -8 //Moves t4 back to the beginning of the next control register
1:  
    slli t4, t4, 2 //Multiply with 4 to move to use for reference for correct spot
    li t1, 0xF //0x0000 000F
    sll t1, t1, t4 //Ex: If t4 = 1 then 1 * 4 = 4 move 4: 0x0000 00F0
    not t1, t1 //Ex: 0x0000 00F0 -> FFFF FF0F (our mask)

    //Create config mode
    mv t2, a1 //Move input mode into t2. Ex: 0x0100
    or t2, t2, a2 //Ex: t2 = 0x4, a2 = 0x0, Result = 0x4
    sll t2, t2, t4 //Moves to right spot: Ex: 0x0000 0004 -> [t4 = 1] -> 0x0000 0040

    //Load to CTR0
    lw t3, CTR0(t0) //Because we changed the base address this will point to either 1 or 0
    and t3, t3, t1 //Erases previous config
    or t3, t3, t2 //Loads new config
    sw t3, CTR0(t0) //Saves new config

    ret //Returns


//Turns on and off pins
//a0 = GPIOX
//a1 = pattern
//a2 = pins
gpiobop:
    //t1 is the originall pattern, t2 is the opposite pattern 16 bits offset
    //the bits that t1 turns on are also turned of in t2 and vice versa
    and t1, a1, a2
    xor t2, a2, t1
    slli t2, t2, 16
    or t1, t1, t2
    sw t1, BOP(a0)    

    ret


//a0 = GPIO-module & return value
gpioreadin:
    //Get input from ISTAT and load it into a0
    lh a0, ISTAT(a0)
    ret

//a0 = GPIO-module and return value
gpioreadout:
    //Get input from OCTL and load it into a0
    lh a0, OCTL(a0) 
    ret

.equ RCU,    0x40021000
.equ APB1EN, 0x1C
.equ T5EN,   0x10
.equ APB2EN, 0x18
.equ PAEN,   0x4
.equ PBEN,   0x8


//Turns on GPIO-Modules
//a0 = GPIO-Modules to be turned on
rcu:
    li t0, RCU //Loads base address of RCU
    lw t1, APB2EN(t0) //Loads current config
    or t1, t1, a0 //Updates config
    sw t1, APB2EN(t0) //Saves config    
    ret

.global main
main:  
    
//Init 
    li a0, PBEN + PAEN //Ready to start modules
    call rcu    

    li a0, GPIOB
    li a1, GPIO_PP //Push-pull
    li a2, GPIO_50 //With max 50Mhz
    li a3, 0x0
    li a4, 0x3 //For loop
1:
    call gpioconf
    addi a3, a3, 1
    bne a3, a4, 1b

    li a0, GPIOA
    li a1, GPIO_FI //High-Z input
    li a2, GPIO_AM //Analog input
    li a3, 0x5
    li a4, 0x9
1:
    call gpioconf
    addi a3, a3, 1
    bne a3, a4, 1b

//Superloop
    
super: 
    
    la t0, code
    li t4, 0x4 //Iterable
    li t3, 0x4 //Amount of correct choices

    li a1, 0x0 //Pattern

lock:
    li a0, GPIOB
    li a2, 0x7
    call gpiobop


1:  //To line 144: Waits for switch A5 to be switched ON
    li a0, GPIOA
    call gpioreadin //We read the input register
    srli t1, a0, 5 //We move the switches down
    andi t1, t1, 0x8 //We mask out switch A5
    beq t1, zero, 1b //As long as A5 is not on keep checking


2:  //To line 152: Waits for switch A5 to be switched OFF
    li a0, GPIOA
    call gpioreadin //We read the input register
    srli t1, a0, 5 //We move the switches down
    andi t1, t1, 0x8 //We mask out switch A5
    bne t1, zero, 2b //As long as A5 is on keep checking

    li a0, GPIOA
    call gpioreadin //Gets the current ISTAT config
    srli t1, a0, 5 //Shifts it so that the bits are in the right spot
    andi t1, t1, 0x7 //Masks out all of the uninteresting bits
    lb t2, 0(t0) //Loads the current byte from t0
    bne t1, t2, 1f //Checks if the switches and the byte are the same
    addi t3, t3, -1 //Decreases t3 for every correct input (if this doesnt hit 0 then it went wrong somewhere)

1:
    addi t0, t0, 1 //Moves to the next byte
    addi t4, t4, -1 //Decreases the iterable
    addi a1, a1, 1 //Ready to show next number
    bne t4, zero, lock //If all four have not been inputted... continue

    bne t3, zero, super //if something was wrong, reset.

1:  //Stuck showing ON-ON-ON forever
    li a0, GPIOB
    li a1, 0x7
    li a2, 0x7
    call gpiobop
    j 1b //[Halt]



.data
code:   
    .byte 0x1, 0x5, 0x3, 0x2

.end
