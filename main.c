/************** ECE2049 DEMO CODE ******************/
/**************  13 March 2019   ******************/
/***************************************************/


#include <msp430.h>
#include <stdio.h>
#include "peripherals.h"

long unsigned int timer_cnt=0;
long unsigned int prev_time=0;
char tdir = 1;
int SongNote = 0;
uint8_t led;
int flag = 0;

void runtimerA2(void);
void stoptimerA2(int reset);
__interrupt void TimerA2_ISR (void);
//void updateLCD(char currentDisplay[], char string[]);

void configUserLED(char inbits);
void configUserButtons(void);
uint8_t getState(void); //uint8_t

typedef enum {
    UPDATE = 0,
    EDIT_MONTH = 1,
    EDIT_DAY = 2,
    EDIT_HOURS = 3,
    EDIT_MINS = 4,
    EDIT_SEC = 5,
} GAME_STATE;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer
    _BIS_SR(GIE); //enables interrupts
    runtimerA2(); //start timer

    configDisplay();
    configKeypad();
    configUserButtons();

    unsigned char currKey = getKey();
    uint8_t currButton = 0;
    unsigned char currKeyint = getKey();


    GAME_STATE my_state = EDIT_MONTH;
    while(1){
        currKey = getKey();

        char currKeyint = getKey();
        switch(my_state){
            case UPDATE: //display Welcome Screen
                uint32_t temp0,temp1,temp2;
                float tempDegC0,tempDegC1,tempDegC2;
                REFCTL0&=~REFMSTR;

                 //multiplechannels,use2.5Vinternalreferencevoltage
                ADC12CTL0=ADC12SHT0_9+ADC12SHT1_9+ADC12REF2_5+ADC12REFON+ADC12ON+ADC12MSC;
                ADC12CTL1=ADC12SHP|ADC12CONSEQ_1;
                ADC12MCTL0=ADC12SREF_1+ADC12INCH_2;
                ADC12MCTL1=ADC12SREF_1+ADC12INCH_3;
                ADC12MCTL2=ADC12SREF_1+ADC12INCH_7+ADC12EOS;

                 //configurefunctionalmode
                P6SEL |=(BIT2|BIT3|BIT7);
                ADC12CTL0&=~ADC12SC;
                while(1){
                    if(sample_flag){//if 0.1shasoccurred
                        sample_flag=false;
                        ADC12CTL0&=~ADC12SC;//clearthestartbit
                        ADC12CTL0|=ADC12SC+ADC12ENC;
                        while(ADC12CTL1&ADC12BUSY)
                            __no_operation();
                        temp0=ADC12MEM0&0x0FFF;
                        temp1=ADC12MEM1&0x0FFF;
                        temp2=ADC12MEM2&0x0FFF;
                        tempDegC0=((temp0*2.5)/4096-1.65)/0.00650;
                        tempDegC1=((temp1*2.5)/4096-1.65)/0.00650;
                        tempDegC2=((temp2*2.5)/4096-1.65)/0.00650;
                    }
                }
                break;
            case EDIT_MONTH: //counts down
                 break;
            case EDIT_DAY:
                break;
            case  EDIT_HOURS:
                break;
            case EDIT_MINS:
                break;
            case EDIT_SEC:
                break;
        }
    }
}

void runtimerA2(void){
// Use ACLK, 16 Bit, up mode, 1 divider
    TA2CTL = TASSEL_1 + MC_1 + ID_0;
    TA2CCR0 = 32767; //interrupts every second
    TA2CCTL0 = CCIE; // TA2CCR0 interrupt enabled
}

void stoptimerA2(int reset)
{
    TA2CTL = MC_0; // stop timer
    TA2CCTL0 &= ~CCIE; // TA2CCR0 interrupt disabled
        if(reset)
            timer_cnt=0;
}

// Timer A2 interrupt service routine
#pragma vector=TIMER2_A0_VECTOR
__interrupt void TimerA2_ISR (void){
    if(tdir){
        timer_cnt++;
    }
    else
        timer_cnt--;
}

void configUserButtons(void){
    P7SEL &= ~(BIT0|BIT4); //S1, S4
    P3SEL &= ~(BIT6); //S2
    P2SEL &= ~(BIT2); //S3

    P7DIR &= ~(BIT0|BIT4); //S1, S4
    P3DIR &= ~(BIT6); //S2
    P2DIR &= ~(BIT2); //S3

    P7REN |= (BIT0|BIT4); //S1, S4
    P3REN |= (BIT6); //S2
    P2REN |= (BIT2); //S3

    P7OUT |= (BIT0|BIT4); //S1, S4
    P3OUT |= (BIT6); //S2
    P2OUT |= (BIT2); //S3
}

uint8_t getState(void){
        uint8_t result = 0x00;
        if (~P7IN & BIT0) {//sw1
            result |= BIT3;
        }
        if (~P3IN & BIT6) {
            result |= BIT2;
        }
        if (~P2IN & BIT2) {
            result |= BIT1;
        }
        if (~P7IN & BIT4) { //bit 4 is set
            result |= BIT0;
        }

        return result;
}


void displayTime(long unsigned int inTime)
{
    //int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
    char time[14];
    char date[13];

    int days = ((inTime / 86400)%365); //86400 sec in a day
    int hours = ((inTime - (86400*days)) / 3600); //3600 sec in an hour
    int minutes = ((inTime - (86400*days) - (3600*hours)) / 60); //60 sec in a min
    int seconds = (inTime - (86400*days) - (3600*hours) - (60*minutes)); //remainder

    date[0] = 'D';
    date[1] = 'A';
    date[2] = 'T';
    date[3] = 'E';
    date[4] = '=';
    date[5] = '>';

    if(days <= 31)
    {
        date[6] = 'J';
        date[7] = 'A';
        date[8] = 'N';
        date[9] = ' ';
        date[10] = (days/10)+48;
        date[11] = (days%10)+48;
    }
    else if ((days > 31)&&(days <= 59))
    {
        date[6] = 'F';
        date[7] = 'E';
        date[8] = 'B';
        date[9] = ' ';
        date[10] = ((days-31)/10)+48;
        date[11] = ((days-31)%10)+48;
    }
    else if ((days > 59)&&(days <= 90))
    {
        date[6] = 'M';
        date[7] = 'A';
        date[8] = 'R';
        date[9] = ' ';
        date[10] = ((days-59)/10)+48;
        date[11] = ((days-59)%10)+48;
    }
    else if ((days > 90)&&(days <= 120))
    {
        date[6] = 'A';
        date[7] = 'P';
        date[8] = 'R';
        date[9] = ' ';
        date[10] = ((days-90)/10)+48;
        date[11] = ((days-90)%10)+48;
    }
    else if ((days > 120)&&(days <= 151))
    {
        date[6] = 'M';
        date[7] = 'A';
        date[8] = 'Y';
        date[9] = ' ';
        date[10] = ((days-120)/10)+48;
        date[11] = ((days-120)%10)+48;
    }
    else if ((days > 151)&&(days <= 181))
    {
        date[6] = 'J';
        date[7] = 'U';
        date[8] = 'N';
        date[9] = ' ';
        date[10] = ((days-151)/10)+48;
        date[11] = ((days-151)%10)+48;
    }
    else if ((days > 181)&&(days <= 212))
    {
        date[6] = 'J';
        date[7] = 'U';
        date[8] = 'L';
        date[9] = ' ';
        date[10] = ((days-181)/10)+48;
        date[11] = ((days-181)%10)+48;
    }
    else if ((days > 212)&&(days <= 243))
    {
        date[6] = 'A';
        date[7] = 'U';
        date[8] = 'G';
        date[9] = ' ';
        date[10] = ((days-212)/10)+48;
        date[11] = ((days-212)%10)+48;
    }
    else if ((days > 243)&&(days <= 273))
    {
        date[6] = 'S';
        date[7] = 'E';
        date[8] = 'P';
        date[9] = ' ';
        date[10] = ((days-243)/10)+48;
        date[11] = ((days-243)%10)+48;
    }
    else if ((days > 273)&&(days <= 304))
    {
        date[6] = 'O';
        date[7] = 'C';
        date[8] = 'T';
        date[9] = ' ';
        date[10] = ((days-273)/10)+48;
        date[11] = ((days-273)%10)+48;
    }
    else if ((days > 304)&&(days <= 334))
    {
        date[6] = 'N';
        date[7] = 'O';
        date[8] = 'V';
        date[9] = ' ';
        date[10] = ((days-304)/10)+48;
        date[11] = ((days-304)%10)+48;
    }
    else if ((days > 334)&&(days <= 365))
    {
        date[6] = 'D';
        date[7] = 'E';
        date[8] = 'C';
        date[9] = ' ';
        date[10] = ((days-334)/10)+48;
        date[11] = ((days-334)%10)+48;
    }

    time[0] = 'T';
    time[1] = 'i';
    time[2] = 'm';
    time[3] = 'e';
    time[4] = '=';
    time[5] = '>';
    time[6] = ((hours/10) + 48);
    time[7] = ((hours%10) + 48);
    time[8] = ':';
    time[9] = ((minutes/10) + 48);
    time[10] = ((minutes%10) + 48);
    time[11] = ':';
    time[12] = ((seconds/10) + 48);
    time[13] = ((seconds%10) + 48);

    Graphics_drawStringCentered(&g_sContext, time, AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, date, AUTO_STRING_LENGTH, 48, 20, TRANSPARENT_TEXT);
}

void displayTemp(float inAvgTempC)
{
    char tempC[14];
    char tempF[14];

    //multiplied by 10
    float tempCf = (floor (inAvgTempC* 10));
    int tempCtens = (tempCf/100);
    int tempCones = ((tempCf - tempCtens)/10);
    int tempCtenths = (tempCf - tempCtens - tempCones);

    float tempFf = (floor ((inAvgTempC * (9.f/5) + 32) * 10));
    int tempFtens = (tempFf/100);
    int tempFones = ((tempFf - tempFtens)/10);
    int tempFtenths = (tempFf - tempFtens - tempFones);

    tempC[0] = 'T';
    tempC[1] = 'e';
    tempC[2] = 'm';
    tempC[3] = 'p';
    tempC[4] = '(';
    tempC[5] = 'C';
    tempC[6] = ')';
    tempC[7] = '=';
    tempC[8] = '>';
    tempC[9] = (tempCtens + 48);
    tempC[10] = (tempCones + 48);
    tempC[11] = '.';
    tempC[12] = (tempCtenths + 48);
    tempC[13] ='C';

    tempF[0] = 'T';
    tempF[1] = 'e';
    tempF[2] = 'm';
    tempF[3] = 'p';
    tempF[4] = '(';
    tempF[5] = 'F';
    tempF[6] = ')';
    tempF[7] = '=';
    tempF[8] = '>';
    tempF[9] = (tempFtens + 48);
    tempF[10] = (tempFones + 48);
    tempF[11] = '.';
    tempF[12] = (tempFtenths + 48);
    tempF[13] ='F';

    Graphics_drawStringCentered(&g_sContext, tempC, AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, tempF, AUTO_STRING_LENGTH, 48, 20, TRANSPARENT_TEXT);
}




