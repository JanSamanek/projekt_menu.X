#pragma config FOSC = HSMP      // Externi oscilator
#pragma config PLLCFG = ON      // 4X PLL
#pragma config FCMEN = ON       // Fail-Safe Clock
#pragma config WDTEN = OFF      // Watchdog Timer OFF

#include <xc.h>
#include "lcd.h"
#include <stdio.h>

#define BTN1 PORTCbits.RC0
#define _XTAL_FREQ 32E6

volatile enum game {GAME_START, GAME_STOP, GAME_CONTINUE, NEXT_LEVEL} game_state;
volatile unsigned int delay = 62500;
volatile unsigned char led_state = 0;
volatile signed static w = -1;

void driveLED(char in);

void __interrupt() T1_ISR_HANDLER(void){
    if(TMR1IF && TMR1IE && (game_state == GAME_CONTINUE))
    {
        static unsigned char i = 0;
        unsigned int tmr_over = 0xFFFF - delay;
        led_state |= (1 << w);
        driveLED(led_state);
        if(i>100)
        {
            w++;
            i=0;
        }
        i++;
        TMR1 = tmr_over;
        TMR1IF = 0;
       
        if(w == 6)
        {
            led_state = 0;
            w = -1;
            game_state = GAME_STOP;
        }
    }
    else
    {
        TMR1IF = 0;
    }
}


void main(void) {
   
    TRISCbits.TRISC0 = 1;
    TRISDbits.RD2 = 0;
    TRISDbits.RD3 = 0;
    TRISCbits.RC4 = 0;
    TRISDbits.RD4 = 0;
    TRISDbits.RD5 = 0;    
    TRISDbits.RD6 = 0;
   
    LATDbits.LATD2 = 0;
    LATDbits.LATD3 = 0;
    LATCbits.LATC4 = 0;
    LATDbits.LATD4 = 0;
    LATDbits.LATD5 = 0;
    LATDbits.LATD6 = 0;
   
    LCD_Init();
   
    ANSELE |= (1<<0);               //AN5
    ADCON2bits.ADFM = 1;            //right justified
    ADCON2bits.ADCS = 0b110;        //Fosc/64
    ADCON2bits.ACQT = 0b110;        //16
    ADCON0bits.ADON = 1;            //ADC zapnout
    ADCON0bits.CHS = 5;
   
    T1CONbits.TMR1CS = 0b01;        // zdroj casovace 1
    T1CONbits.T1CKPS = 0b11;        // nastaveni delicky                                            
    PEIE = 1;                       // povoleni preruseni od periferii
    GIE = 1;                        // globalni povoleni preruseni
    TMR1IE = 1;                     // povoleni preruseni pro TMR1
    TMR1IF = 0;                     // smazani priznaku (pro jistotu)
    TMR1ON = 1;                     // spusteni TMR1
   
    unsigned int pot;
    char text1[17];
    char text2[17];
    char textSTART[17];
    char textLEVEL [17];
    char textSTOP1 [17];
    char textSTOP2 [17];
    unsigned int rand_num;
               
    game_state = GAME_START;
               
    while(1)
    {
        switch(game_state)
        {
            case GAME_START:
               
                sprintf(textSTART,"Nova hra:BTN1                   ");
                LCD_ShowString(1, textSTART);
                if(BTN1)
                {
                    rand_num = 10;
                    game_state = GAME_CONTINUE;
                }
                break;
           
            case GAME_STOP:
               
                delay = 62500;
                sprintf(textSTOP1,"GAME OVER                  ");
                LCD_ShowString(1, textSTOP1);
                sprintf(textSTOP2,"Nova hra: BTN1              ");
                LCD_ShowString(2,textSTOP2);
                if(BTN1)
                {
                    rand_num = rand() % 1023;
                    game_state = GAME_CONTINUE;
                }
               
                break;
           
            case GAME_CONTINUE:
                               
                GODONE = 1;
                while(GODONE);
                pot = (unsigned int)(ADRESH << 8) | (unsigned int)ADRESL;
       
                sprintf(text2, "otoc na:%u              ", rand_num);
                LCD_ShowString(1, text2);
       
                sprintf(text1, "aktualni:%u                 ", pot);
                LCD_ShowString(2, text1);
               
                if(pot == rand_num)
                {
                    for(long k=1; k<200000; k++);
                    if(pot == rand_num)
                    {
                        game_state = NEXT_LEVEL;
                    }
                }
               
                break;  
               
            case NEXT_LEVEL:
                sprintf(textLEVEL, "Next level: BTN1          ");
                LCD_ShowString(1, textLEVEL);
                if(BTN1)
                {
                    w = -1;
                    led_state = 0;
                    delay = delay/2;
                    rand_num = rand() % 1023;
                    game_state = GAME_CONTINUE;
                }
               
                break;
        }
    }
    return;
}

void driveLED(char in){
        LATD2 = in & 1;             //LED0
        LATD3 = in & 2 ? 1 : 0;     //LED1
        LATC4 = in & 4 ? 1 : 0;     //LED2
        LATD4 = in & 8 ? 1 : 0;     //LED3
        LATD5 = in & 16 ? 1 : 0;    //LED4
        LATD6 = in & 32 ? 1 : 0;    //LED5
}
