#pragma config FOSC = HSMP      // Externi oscilator
#pragma config FCMEN = ON       // Fail-Safe Clock
#pragma config WDTEN = OFF      // Watchdog Timer OFF

#include "lcd.h"
#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define _XTAL_FREQ 32e6
#define SETDUTY(x) CCPR1L = x

#define LED1 LATDbits.LATD2
#define LED2 LATDbits.LATD3
#define LED3 LATCbits.LATC4
#define LED4 LATDbits.LATD4
#define LED5 LATDbits.LATD5
#define LED6 LATDbits.LATD6

#define BTN1 PORTCbits.RC0
#define BTN2 PORTAbits.RA4
#define BTN3 PORTAbits.RA3
#define BTN4 PORTAbits.RA2

#define DAC_SS LATB3            // DAC slave select pin
#define DAC_CH1 0b10110000      // kanal 1/B

void uart_init(void);
unsigned int adc_read(char channel);
void spi_init(void);
void SPIWrite(uint8_t channel ,uint8_t data);
void adc_init(void);
void putch(unsigned char data);
void driveLED(char in);
void IT_init(void);
void GPIO_init(void);
void PWM_init(void);
void T1_init();

typedef struct {
char data[100];
char full;
unsigned int length;
} message;

volatile message message1;
volatile enum game {GAME_START, GAME_STOP, GAME_CONTINUE, NEXT_LEVEL, GAME_OFF} game_state;
volatile unsigned int delay = 62500;
volatile unsigned char led_stateHRA = 0;
volatile signed static w = -1;

    // <editor-fold defaultstate="collapsed" desc="SINTABLE">
    const unsigned char tabulka[128] =
    {
        0x80,0x86,0x8c,0x92,0x98,0x9e,0xa5,0xaa,0xb0,0xb6,0xbc,0xc1,0xc6,0xcb,0xd0,0xd5,
        0xda,0xde,0xe2,0xe6,0xea,0xed,0xf0,0xf3,0xf5,0xf8,0xfa,0xfb,0xfd,0xfe,0xfe,0xff,
        0xff,0xff,0xfe,0xfe,0xfd,0xfb,0xfa,0xf8,0xf5,0xf3,0xf0,0xed,0xea,0xe6,0xe2,0xde,
        0xda,0xd5,0xd0,0xcb,0xc6,0xc1,0xbc,0xb6,0xb0,0xaa,0xa5,0x9e,0x98,0x92,0x8c,0x86,
        0x80,0x79,0x73,0x6d,0x67,0x61,0x5a,0x55,0x4f,0x49,0x43,0x3e,0x39,0x34,0x2f,0x2a,
        0x25,0x21,0x1d,0x19,0x15,0x12,0xf,0xc,0xa,0x7,0x5,0x4,0x2,0x1,0x1,0x0,
        0x0,0x0,0x1,0x1,0x2,0x4,0x5,0x7,0xa,0xc,0xf,0x12,0x15,0x19,0x1d,0x21,
        0x25,0x2a,0x2f,0x34,0x39,0x3e,0x43,0x49,0x4f,0x55,0x5a,0x61,0x67,0x6d,0x73,0x79,
    };
    // </editor-fold>
   
void __interrupt() UART_TMR1_ISR(void)
{
    static unsigned char RX_i = 0;
    char zprava;
   
    if(RC1IE && RC1IF)
    {
        zprava = RCREG1;
        if(zprava == '\n')
        {
            message1.length = RX_i;
            RX_i = 0;
            message1.full = 1;
        }
        else
        {
            message1.data[RX_i] = zprava;
            RX_i++;
        }
    }
   
    if (TX1IE && TX1IF)
    {
        TXREG1 = message1.data[message1.length -1];
        if(message1.length == 1)
        {
            TX1IE = 0;
        }
        else
        {
            message1.length--;
        }
    }
    
    if(TMR1IF && TMR1IE && (game_state == GAME_CONTINUE))
    {
        static unsigned char i = 0;
        unsigned int tmr_over = 0xFFFF - delay;
        led_stateHRA |= (1 << w);
        driveLED(led_stateHRA);
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
            led_stateHRA = 0;
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
   
    GPIO_init();
    LCD_Init();
    uart_init();
    adc_init();
    spi_init();
    IT_init();
    T1_init();
   
   char menu_names[7][5] = {
        "GPIO",
        "UART",
        "PWM",
        "ADC",
        "DAC",
        "HRA",
        "   "
    };
   
   char cursor[1][3] = {
       "->",
   };
   
    char text1M [17];
    char text2M [17];
    char textGPIO [17];
    char textUART [17];
    char textPWM [17];
    char textADC [17];
    char textDAC [17];
    unsigned int potHRA;
    char text1[17];
    char text2[17];
    char textSTART[17];
    char textLEVEL [17];
    char textSTOP1 [17];
    char textSTOP2 [17];
    unsigned int rand_num;
    unsigned int level = 1;
               
    game_state = GAME_START;
    char position = 0;
    char menu_position = 6;
   
    while(1)
    {
       
    switch(menu_position)
    {
        case 0:
           
            LCD_Clear();
            sprintf(textGPIO,"Binarni citac          ");
            LCD_ShowString(1,textGPIO);
           
                LED1 = 1;
                LED2 = 1;
                LED3 = 1;
                LED4 = 1;
                LED5 = 1;
                LED6 = 1;

                unsigned char led_state = 0;

                while(1){
                    if(BTN1){
                    __delay_ms(80);
                    if(BTN1){
                        if(led_state == 0b11111111){
                            led_state = 0;
                        }
                    led_state++;
                    driveLED(led_state);
                    }
                    }
                if(BTN4)
                {
                    __delay_ms(80);
                    if(BTN4)
                    {
                        menu_position = 6;
                        break;
                    }
                }

                }

            break;
       
        case 1:
           
            LCD_Clear();
            sprintf(textUART,"zprava naopak          ");
            LCD_ShowString(1,textUART);
           
            while(1){
                if(message1.full)
                {
                    message1.full = 0;
                    TX1IE = 1;
                }
                if(BTN4)
                {
                    __delay_ms(80);
                    if(BTN4)
                    {
                        menu_position = 6;
                        break;
                    }
                }
            }
           
            break;
           
        case 2:
           
            LCD_Clear();
            sprintf(textPWM,"PWM pomoci POT          ");
            LCD_ShowString(1,textPWM);
           
            #pragma config PLLCFG = ON      // 4X PLL
           
            PWM_init();

            // ADC pro potenciometr                
            ADCON2bits.ADFM = 0;            //left justified
            ADCON0bits.CHS = 5;             // kanal AN5

             while (1)
            {
                GODONE = 1;                 // spustim konverzi
                while(GODONE){};            // cekam na konverzi

                if(ADRESH > 130)
                {
                LATC1 = 1;
                SETDUTY(255-ADRESH);
                }
                else if(ADRESH < 125)
                {
                LATC1 = 0;
                SETDUTY(255-ADRESH);
                }
                else if ((ADRESH > 125)&&(ADRESH < 130))
                {
                SETDUTY(0);
                }
                if(BTN4)
                {
                    __delay_ms(80);
                    if(BTN4)
                    {
                        menu_position = 6;
                        break;
                    }
                }
            }
               
           
            break;
           
        case 3:      
           
                LCD_Clear();
                sprintf(textADC,"nasobeni pot          ");
                LCD_ShowString(1,textADC);
               
                unsigned int potenciometer1;
                unsigned int potenciometer2;
                unsigned long value_out;
               
                while(1)
                {        
                ADCON0bits.CHS = 5;
                GODONE = 1;
                while(GODONE);
                potenciometer2 = (ADRESH << 8) | ADRESL;      // cteni vysledku

                ADCON0bits.CHS = 4;
                ADCON2bits.ADFM = 0;

                GODONE = 1;
                while(GODONE);
                potenciometer1 = ADRESH;

                __delay_ms(50);

                value_out = (unsigned long)potenciometer1*(unsigned long)potenciometer2; //vystup je long i vstupy musim pretypovat na long
                printf("%lu\n",value_out);

                ADCON2bits.ADFM = 1;
               
                if(BTN4)
                {
                    __delay_ms(80);
                    if(BTN4)
                    {
                        menu_position = 6;
                        break;
                    }
                }
                }
           
            break;
           
        case 4:
           
                LCD_Clear();
                sprintf(textDAC,"orezany sinus          ");
                LCD_ShowString(1,textDAC);
               
                unsigned int pot1, pot2;
                unsigned int sin_final;
                uint8_t  j = 0;

                /* hlavni smycka */
                while(1){
                    pot1 = adc_read(4);
                    pot1 = 255 - pot1/8;
                    pot2 = adc_read(5);
                    pot2 = pot2/8;

                    if((tabulka[j] > pot1) && (tabulka[j] >= 128))
                    {
                        SPIWrite(DAC_CH1, pot1);
                        j++;
                    }
                    else if((tabulka[j] < pot2) && (tabulka[j] < 128))
                    {
                        SPIWrite(DAC_CH1, pot2);
                        j++;
                    }
                    else
                    {
                    SPIWrite(DAC_CH1,tabulka[j++]);
                    }
                    if(j == 128) j=0;

                    sin_final = adc_read(13);
                    printf("%u\r",sin_final);
                    __delay_ms(1);
                   
                if(BTN4)
                {
                    __delay_ms(80);
                    if(BTN4)
                    {
                        menu_position = 6;
                        break;
                    }
                }
                }
            break;
           
        case 5:
                            
                ADCON0bits.CHS = 5; 
                
                while(1)
                {
                switch(game_state)
                {
                    case GAME_START:
                        
                        sprintf(textSTART,"Nova hra:BTN1                   ");
                        LCD_ShowString(1, textSTART);
                        if(BTN1)
                        {
                            rand_num = (rand()+25) % 1023;
                            game_state = GAME_CONTINUE;
                        }
                        else if(BTN4)
                        {
                            __delay_ms(80);
                            if(BTN4)
                            {
                                menu_position = 6;
                                game_state = GAME_OFF;
                            }
                        }
                        break;

                    case GAME_STOP:

                        delay = 62500;
                        sprintf(textSTOP1,"GAME OVER                  ");
                        LCD_ShowString(1, textSTOP1);
                        sprintf(textSTOP2,"level: %u              ", level);
                        LCD_ShowString(2,textSTOP2);
                        
                        __delay_ms(2000);
                        level = 1;
                        menu_position = 6;
                        game_state = GAME_OFF;

                        break;

                    case GAME_CONTINUE:

                        GODONE = 1;
                        while(GODONE);
                        potHRA = (unsigned int)(ADRESH << 8) | (unsigned int)ADRESL;

                        sprintf(text2, "otoc na:%u              ", rand_num);
                        LCD_ShowString(1, text2);

                        sprintf(text1, "aktualni:%u                 ", potHRA);
                        LCD_ShowString(2, text1);

                        if(potHRA == rand_num)
                        {
                            for(long k=1; k<200000; k++);
                            if(potHRA == rand_num)
                            {
                                game_state = NEXT_LEVEL;
                            }
                        }
                        else if(BTN4)
                        {
                            __delay_ms(80);
                            if(BTN4)
                            {
                                menu_position = 6;
                                game_state = GAME_OFF;
                            }
                        }

                        break;  

                    case NEXT_LEVEL:
                        sprintf(textLEVEL, "Next level: BTN1          ");
                        LCD_ShowString(1, textLEVEL);
                        if(BTN1)
                        {
                            level++;
                            w = -1;
                            led_stateHRA = 0;
                            delay = delay/2;
                            rand_num = rand() % 1023;
                            game_state = GAME_CONTINUE;
                        }
                        else if(BTN4)
                        {
                            __delay_ms(80);
                            if(BTN4)
                            {
                                menu_position = 6;
                                game_state = GAME_OFF;
                            }
                        }

                        break;
                    }
                break;
                    }
            break;
           
        case 6:
                #pragma config PLLCFG = ON      // 4X PLL
                sprintf(text1M, "%s%s           ",cursor, (menu_names+5*position));
                LCD_ShowString(1, text1M);

                sprintf(text2M, "%s           ",(menu_names+5*position+5));
                LCD_ShowString(2, text2M);
                
                game_state = GAME_START;
                
                if(BTN2 && !(position == 5))
                {
                    __delay_ms(80);
                    if(BTN2)
                    {
                        position ++;
                    }
                }
                else if(BTN1 && !(position == 0))
                {
                    __delay_ms(80);
                    if(BTN1)
                    {
                    position --;
                    }
                }
                else if(BTN3)
                {
                    __delay_ms(80);
                    if(BTN3)
                    {
                        menu_position = position;
                    }
                }
            break;
    }
    }
    return;
}
void driveLED(char in){
        LATD2 = ~(in & 1);             //LED0
        LATD3 = ~(in & 2 ? 1 : 0);     //LED1
        LATC4 = ~(in & 4 ? 1 : 0);     //LED2
        LATD4 = ~(in & 8 ? 1 : 0);     //LED3
        LATD5 = ~(in & 16 ? 1 : 0);    //LED4
        LATD6 = ~(in & 32 ? 1 : 0);    //LED5
}

/* funkce zapisu SPI funkce zapisuje dva bajty za sebou */
void SPIWrite(uint8_t channel ,uint8_t data){
   
    uint8_t msb, lsb;
    DAC_SS = 0;                         // slave select
   
    msb = (channel | (data>>4));        // prvni bajt
    lsb = (data<<4) & 0xFF;             // druhy bajt

    SSPBUF = msb;                       // zapis do bufferu
    while(!SSP1IF);                     // pockat nez SPI posle prvni bajt
    SSP1IF = 0;

    SSPBUF = lsb;                       // zapis do bufferu
    while(!SSP1IF);                     // pockat nez SPI posle druhy bajt
    SSP1IF = 0;    
   
    DAC_SS = 1;                         // vypnout slave select
}

void spi_init(void){
   
    /* vyber pinu jako vystupy */
    TRISCbits.TRISC3 = 0;   //SCK
    TRISCbits.TRISC5 = 0;   //SDO
    TRISBbits.TRISB3 = 0;   //SS
 
    LATBbits.LATB3 = 1;         // DAC SS off
   
    SSP1CON1bits.SSPM = 0b0010; // SPI clock fosc/64
    SSP1CON1bits.CKP = 0;       //nastaveni SPI modu
    SSP1STATbits.CKE = 1;       //nastaveni SPI modu
    SSP1CON1bits.SSPEN = 1;     // SPI zapnuto
}

void adc_init(void){    
    ANSELAbits.ANSA5 = 1;
    ANSELEbits.ANSE0 = 1;
    ANSELBbits.ANSB5 = 1;
   
    TRISAbits.RA5 = 1;
    TRISEbits.RE0 = 1;
    TRISBbits.RB5 = 1;
   
    ADCON2bits.ADFM = 1;            //right justified
    ADCON2bits.ADCS = 0b110;        //Fosc/64
    ADCON2bits.ACQT = 0b110;        //16
    ADCON0bits.ADON = 1;            //ADC zapnout
}

unsigned int adc_read(char channel){
    unsigned int value;
    ADCON0bits.CHS = channel;
   
    GODONE = 1;
    while(GODONE);
    value = (unsigned int)(ADRESH << 8) | (unsigned int)ADRESL;
    return value;
}

void uart_init(void){
    ANSELCbits.ANSC6 = 0;
    ANSELCbits.ANSC7 = 0;
    TRISCbits.TRISC6 = 0;       // TX pin jako vstup
    TRISCbits.TRISC7 = 1;       // RX pin jako vstup

/*baudrate*/
    SPBRG1 = 25;                // (32_000_000 / (64 * 19200)) - 1
    RCSTA1bits.SPEN = 1;        // zapnuti UART
    TXSTA1bits.SYNC = 0;        // nastaveni asynchroniho modu
    TXSTA1bits.TXEN = 1;        // zapnuti TX
    RCSTA1bits.CREN = 1;        // zapnuti RX    
}

void IT_init(void)
{
    RC1IE = 1;              // zap  preruseni od RCREG
    PEIE = 1;               // preruseni od periferii    
    GIE = 1;                // globalni preruseni
    TMR1IE = 1;                     // povoleni preruseni pro TMR1
}

void putch(unsigned char data){
    while(!TX1IF);
    TXREG1 = data;
}

void GPIO_init(void)
{
    TRISDbits.TRISD2 = 0;
    TRISDbits.TRISD3 = 0;
    TRISCbits.TRISC4 = 0;
    TRISDbits.TRISD4 = 0;
    TRISDbits.TRISD5 = 0;
    TRISDbits.TRISD6 = 0;
   
    ANSELAbits.ANSA3 = 0;
    ANSELAbits.ANSA2 = 0;
   
    TRISCbits.RC0 = 1;
    TRISAbits.RA4 = 1;
    TRISAbits.RA3 = 1;
    TRISAbits.RA2 = 1;
}

void PWM_init(void){

    TRISCbits.RC2 = 1;              //vypnu pred konfiguraci pak zapnu

    CCPTMRS0bits.C1TSEL = 0b00;     // Timer 2
    PR2 = 199;                      // f = 10kHz
    CCP1CONbits.P1M = 0b00;         // PWM single output
    CCP1CONbits.CCP1M = 0b1100;     // PWM single (Capture/Compare/PWM)
    CCPR1L = 0;                     // strida 0%    
    T2CONbits.T2CKPS = 0b00;        // 1:1 Prescaler
    TMR2IF = 0;                     // nastavi se az pretece timer
    TMR2ON = 1;                     // staci zapnout defaultne je nastaven jak chceme
    while(!TMR2IF){};               // cekam az jednou pretece

    TRISCbits.RC2 = 0;              // nastavim jako vystup pin P1A
    TRISCbits.RC1 = 0;
}

void T1_init(){
    T1CONbits.TMR1CS = 0b01;        // zdroj casovace 1
    T1CONbits.T1CKPS = 0b11;        // nastaveni delicky
    TMR1ON = 1;
    TMR1IF = 0;                     // smazani priznaku (pro jistotu
}
//RB5 - AN13
//analog out1 - DAC CH1




