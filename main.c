/************** ECE2049 DEMO CODE ******************/
/**************  13 March 2019   ******************/
/***************************************************/


#include <msp430.h>
#include <stdio.h>
#include "peripherals.h"

#define CALADC12_15V_30C  *((unsigned int *)0x1A1A)
#define CALADC12_15V_85C  *((unsigned int *)0x1A1C)

long unsigned int timer_cnt = 86765; //2764800 --> Feb 1
long unsigned int timer_copy = 86400;

long unsigned int prev_time=0;
char tdir = 1;
int SongNote = 0;
uint8_t led;
int flag = 0;
int DAYS_IN_MONTH[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
int MONTHS;
int DAYS;
int HOURS;
int MINUTES;
int SECONDS;


volatile float temperatureDegC;
volatile float temperatureDegF;
volatile float degC_per_bit;
volatile unsigned int bits30, bits85;
int scrollWheel;


//function headers
void runtimerA2(void);
void stoptimerA2(int reset);
__interrupt void TimerA2_ISR (void);
void configUserLED(char inbits);
void configUserButtons(void);
uint8_t getState(void); //uint8_t
void displayGlobalTime(void);
void displayTemp(float inAvgTempC);
void updateGlobalCurrentDateFromSeconds(long unsigned int);

typedef enum {
    UPDATE = 0,
    EDIT_MONTH = 1,
    EDIT_DAY = 2,
    EDIT_HOURS = 3,
    EDIT_MINS = 4,
    EDIT_SEC = 5,
    PUSH_USER_CHANGE = 6,
    REJECT_USER_CHANGE = 7

} GAME_STATE;




int main(void) {
//    uint32_t temp0,temp1,temp2;
//    float tempDegC0,tempDegC1,tempDegC2;
    WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer
    _BIS_SR(GIE); //enables interrupts
    runtimerA2(); //start timer
    configDisplay();
    configKeypad();
    configUserButtons();
    //configADC();

    unsigned char currKey = getKey();
    uint8_t currButton = 0;
    unsigned char currKeyint = getKey();
    int monthsADC;
    float temps[30] = {0};

    //degC_per_bit = ((float)(85.0 - 30.0))/((float)(bits85-bits30));

//    P8SEL &= ~BIT0;
//    P8DIR |= BIT0;
//    P8OUT |= BIT0;
    P6SEL &= ~BIT0;
    P6DIR |= BIT0;

    REFCTL0 &= ~REFMSTR;                      // Reset REFMSTR to hand over control of
    ADC12CTL0 = ADC12SHT0_9 | ADC12SHT1_9 | ADC12REFON | ADC12ON | ADC12MSC;     // Internal ref = 1.5V
    ADC12CTL1 = ADC12SHP | ADC12CONSEQ_1;                     // Enable sample timer
    ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_10;  // ADC i/p ch A10 = temp sense
    ADC12MCTL1 = ADC12SREF_0 + ADC12INCH_0 + ADC12EOS;   // ADC12INCH0 = Scroll wheel = A0
    __delay_cycles(100);                    // delay to allow Ref to settle
    ADC12CTL0 |= ADC12ENC;              // Enable conversion
    bits30 = CALADC12_15V_30C;
    bits85 = CALADC12_15V_85C;
    degC_per_bit = ((float)(85.0 - 30.0))/((float)(bits85-bits30));


    GAME_STATE my_state = UPDATE;
    while(1){
        int monthsADC;
        int daysADC;
        int hoursADC;
        int minutesADC;
        int secondsADC;


        currButton = getState();
//        if((timer_cnt - prev_time) >= 1){
//            //Graphics_clearDisplay(&g_sContext);
//            my_state = UPDATE;
//        }


        ADC12CTL0 |= ADC12ENC;              // Enable conversion
        ADC12CTL0 &= ~ADC12SC;  // clear the start bit
        ADC12CTL0 |= ADC12SC;       // Sampling and conversion start // Single conversion (single channel)
        // Poll busy bit waiting for conversion to complete
        while (ADC12CTL1 & ADC12BUSY)
            __no_operation();
        scrollWheel = ADC12MEM1;


        switch(my_state){
//        Graphics_flushBuffer(&g_sContext);
            case UPDATE: //display Welcome Screen
                ADC12CTL0 |= ADC12ENC;              // Enable conversion

                ADC12CTL0 &= ~ADC12SC;  // clear the start bit
                ADC12CTL0 |= ADC12SC;       // Sampling and conversion start
                                        // Single conversion (single channel)

                    // Poll busy bit waiting for conversion to complete
                    while (ADC12CTL1 & ADC12BUSY)
                        __no_operation();

                    int in_temp = ADC12MEM0;      // Read in results if conversion

                    // Temperature in Celsius. See the Device Descriptor Table section in the
                    // System Resets, Interrupts, and Operating Modes, System Control Module
                    // chapter in the device user's guide for background information on the
                    // used formula.
                    temperatureDegC = (float)((long)in_temp - CALADC12_15V_30C) * degC_per_bit +30.0;

                    // Temperature in Fahrenheit Tf = (9/5)*Tc + 32
                    //temperatureDegF = temperatureDegC * 9.0/5.0 + 32.0;
                    temps[timer_cnt % 30] = temperatureDegC;
                    if(timer_cnt >= 30)
                    {
                        float sum = 0;
                        int i;
                        for(i = 0; i < 30; i++){
                            sum += temps[i];
                        }
                        displayTemp(sum/30);
                    }else{
                        displayTemp(temperatureDegC);
                    }
                    updateGlobalCurrentDateFromSeconds(timer_cnt);
                    displayGlobalTime();
                    Graphics_flushBuffer(&g_sContext);

                    Graphics_flushBuffer(&g_sContext);
                    if(currButton == BIT3)
                    {
                        currButton = 0;
                        my_state = EDIT_MONTH;

                    }
                    prev_time = timer_cnt;
                break;
            case EDIT_MONTH: //counts down
                monthsADC = ((float)scrollWheel/(float)4096)*(float)12+(float)1; //do something to map ADC to days so you add to time
                MONTHS = monthsADC;


                if(currButton == BIT3)
                {
                    currButton = 0;
                    my_state = EDIT_DAY;
                }
                displayGlobalTime();
                Graphics_flushBuffer(&g_sContext);
                break;
            case EDIT_DAY:
                daysADC = ((float)scrollWheel/(float)4096)*(float)getDaysInThisMonth()+1; //do something to map ADC to days so you add to time
                DAYS = daysADC;

                displayGlobalTime();
                if(currButton == BIT3)
                {
                    currButton = 0;
                    my_state = EDIT_HOURS;
                }
                Graphics_flushBuffer(&g_sContext);
                break;
            case  EDIT_HOURS:
                hoursADC = ((float)scrollWheel/(float)4096)*(float)23+1; //do something to map ADC to days so you add to time
                HOURS = hoursADC;
                displayGlobalTime();
                if(currButton == BIT3)
                {
                    my_state = EDIT_MINS;
                }
                Graphics_flushBuffer(&g_sContext);
                break;
            case EDIT_MINS:
                minutesADC = ((float)scrollWheel/(float)4096)*(float)60+1; //do something to map ADC to days so you add to time
                MINUTES = minutesADC;
                displayGlobalTime();
                if(currButton == BIT3)
                {
                    my_state = EDIT_SEC;
                }
                Graphics_flushBuffer(&g_sContext);
                break;
            case EDIT_SEC:
                secondsADC = ((float)scrollWheel/(float)4096)*(float)60+1; //do something to map ADC to days so you add to time
                SECONDS = secondsADC;
                displayGlobalTime();
                if(currButton == BIT3)
                {
                    my_state = EDIT_DAY;
                }
                Graphics_flushBuffer(&g_sContext);
                break;
            case PUSH_USER_CHANGE:
                //push new global date to timer_cnt
               timer_cnt = getSecondsFromGlobalDate();
               my_state = UPDATE;

               break;

            case REJECT_USER_CHANGE:
                //push timer_cnt to rejected global date
                updateGlobalCurrentDateFromSeconds(timer_cnt);
                my_state = UPDATE;
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
        {
            timer_cnt = 0;
            timer_copy = 0;
        }
}

// Timer A2 interrupt service routine
#pragma vector=TIMER2_A0_VECTOR
__interrupt void TimerA2_ISR (void){
    if(tdir){
        timer_cnt++;
        timer_copy++;
        scrollWheel = ADC12MEM1;
    }
    else{
        timer_cnt--;
        timer_copy--;
    }
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

void updateGlobalCurrentDateFromSeconds(long unsigned int inTime)
{
    int totalDays = ((inTime / 86400)%365); //86400 sec in a day

    int monthCounter;
    for(monthCounter = 1; totalDays > getDaysInThisMonth(); monthCounter++){
        totalDays = totalDays - DAYS_IN_MONTH[monthCounter];
    }
    MONTHS = monthCounter;
    DAYS = totalDays;
    HOURS = ((inTime - (86400*DAYS)) / 3600); //3600 sec in an hour
    MINUTES = ((inTime - (86400*DAYS) - (3600*HOURS)) / 60); //60 sec in a min
    SECONDS = (inTime - (86400*DAYS) - (3600*HOURS) - (60*MINUTES)); //remainder

}

int getDaysInThisMonth(){
    return DAYS_IN_MONTH[MONTHS];
}

long unsigned int getSecondsFromGlobalDate(){
    int daysFromMonth = monthIntervalToDays(MONTHS);
    long unsigned int totalDays = daysFromMonth + DAYS;
    long unsigned int totalHours = HOURS + (totalDays * 24);
    long unsigned int totalMinutes = MINUTES + (totalHours * 60);
    long unsigned int totalSeconds = SECONDS + (totalMinutes * 60);
    return totalSeconds;

}

int monthIntervalToDays(int startMonth){
    int monthinDays = 0;
    int i;

    for(i = startMonth; i > 1; i--){
        monthinDays = monthinDays + DAYS_IN_MONTH[i];
    }
    return monthinDays;

}

void displayGlobalTime(){

    char time[14];
    char date[13];

    date[0] = 'D';
    date[1] = 'A';
    date[2] = 'T';
    date[3] = 'E';
    date[4] = '=';
    date[5] = '>';

    if(MONTHS == 1)
    {
        date[6] = 'J';
        date[7] = 'A';
        date[8] = 'N';
        date[9] = ' ';
    }
    else if (MONTHS == 2)
    {
        date[6] = 'F';
        date[7] = 'E';
        date[8] = 'B';
        date[9] = ' ';
    }
    else if (MONTHS == 3)
    {
        date[6] = 'M';
        date[7] = 'A';
        date[8] = 'R';
        date[9] = ' ';
    }
    else if (MONTHS == 4)
    {
        date[6] = 'A';
        date[7] = 'P';
        date[8] = 'R';
        date[9] = ' ';
    }
    else if (MONTHS == 5)
    {
        date[6] = 'M';
        date[7] = 'A';
        date[8] = 'Y';
        date[9] = ' ';
    }
    else if (MONTHS == 6)
    {
        date[6] = 'J';
        date[7] = 'U';
        date[8] = 'N';
        date[9] = ' ';
    }
    else if (MONTHS == 7)
    {
        date[6] = 'J';
        date[7] = 'U';
        date[8] = 'L';
        date[9] = ' ';
    }
    else if (MONTHS == 8)
    {
        date[6] = 'A';
        date[7] = 'U';
        date[8] = 'G';
        date[9] = ' ';
    }
    else if (MONTHS == 9)
    {
        date[6] = 'S';
        date[7] = 'E';
        date[8] = 'P';
        date[9] = ' ';
    }
    else if (MONTHS == 10)
    {
        date[6] = 'O';
        date[7] = 'C';
        date[8] = 'T';
        date[9] = ' ';
    }
    else if (MONTHS == 11)
    {
        date[6] = 'N';
        date[7] = 'O';
        date[8] = 'V';
        date[9] = ' ';
    }
    else if (MONTHS == 12)
    {
        date[6] = 'D';
        date[7] = 'E';
        date[8] = 'C';
        date[9] = ' ';
    }
    date[10] = (DAYS/10)+48;
    date[11] = (DAYS%10)+48;


    time[0] = 'T';
    time[1] = 'i';
    time[2] = 'm';
    time[3] = 'e';
    time[4] = '=';
    time[5] = '>';
    time[6] = ((HOURS/10) + 48);
    time[7] = ((HOURS%10) + 48);
    time[8] = ':';
    time[9] = ((MINUTES/10) + 48);
    time[10] = ((MINUTES%10) + 48);
    time[11] = ':';
    time[12] = ((SECONDS/10) + 48);
    time[13] = ((SECONDS%10) + 48);

    Graphics_drawStringCentered(&g_sContext, time, 14, 48, 35, OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext, date, 13, 48, 45, OPAQUE_TEXT);
}

void displayTemp(float inAvgTempC)
{
    char tempC[14];
    char tempF[14];

    //multiplied by 10
    int tempCf = inAvgTempC*10;
    int tempCtens = (tempCf/100);
    int tempCones = ((tempCf - (tempCtens*100))/10);
    int tempCtenths = (tempCf - (tempCtens*100) - (tempCones*10));

    float tempFf = (inAvgTempC * (9.f/5) + 32) * 10;
    int tempFtens = (tempFf/100);
    int tempFones = ((tempFf - (tempFtens*100))/10);
    int tempFtenths = (tempFf - (tempFtens*100) - (tempFones*10));

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

    Graphics_drawStringCentered(&g_sContext, tempC, 14, 48, 15, OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext, tempF, 14, 48, 25, OPAQUE_TEXT);
}


///************** ECE2049 DEMO CODE ******************/
///**************  13 March 2019   ******************/
///***************************************************/
//
//
//#include <msp430.h>
//#include <stdio.h>
//#include "peripherals.h"
//
//#define CALADC12_15V_30C  *((unsigned int *)0x1A1A)
//#define CALADC12_15V_85C  *((unsigned int *)0x1A1C)
//
//long unsigned int timer_cnt = 86765; //2764800 --> Feb 1
//long unsigned int timer_copy = 86400;
//
//long unsigned int prev_time=0;
//char tdir = 1;
//int SongNote = 0;
//uint8_t led;
//int flag = 0;
//int daysInMonth = 31;
//int months;
//int days;
//int hours;
//int minutes;
//int seconds;
//int daysInMonthArr[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
//
//
//volatile float temperatureDegC;
//volatile float temperatureDegF;
//volatile float degC_per_bit;
//volatile unsigned int bits30, bits85;
//int scrollWheel;
//
//
////function headers
//void runtimerA2(void);
//void stoptimerA2(int reset);
//__interrupt void TimerA2_ISR (void);
//void configUserLED(char inbits);
//void configUserButtons(void);
//uint8_t getState(void); //uint8_t
//void displayTime(long unsigned int inTime);
//void displayTemp(float inAvgTempC);
//
//
//typedef enum {
//    UPDATE = 0,
//    EDIT_MONTH = 1,
//    EDIT_DAY = 2,
//    EDIT_HOURS = 3,
//    EDIT_MINS = 4,
//    EDIT_SEC = 5,
//
//} GAME_STATE;
//
//
//
//
//int main(void) {
////    uint32_t temp0,temp1,temp2;
////    float tempDegC0,tempDegC1,tempDegC2;
//    WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer
//    _BIS_SR(GIE); //enables interrupts
//    runtimerA2(); //start timer
//    configDisplay();
//    configKeypad();
//    configUserButtons();
//    //configADC();
//
//    unsigned char currKey = getKey();
//    uint8_t currButton = 0;
//    unsigned char currKeyint = getKey();
//    int monthsADC;
//    float temps[30] = {0};
//
//    //degC_per_bit = ((float)(85.0 - 30.0))/((float)(bits85-bits30));
//
////    P8SEL &= ~BIT0;
////    P8DIR |= BIT0;
////    P8OUT |= BIT0;
//    P6SEL &= ~BIT0;
//    P6DIR |= BIT0;
//
//    REFCTL0 &= ~REFMSTR;                      // Reset REFMSTR to hand over control of
//    ADC12CTL0 = ADC12SHT0_9 | ADC12SHT1_9 | ADC12REFON | ADC12ON | ADC12MSC;     // Internal ref = 1.5V
//    ADC12CTL1 = ADC12SHP | ADC12CONSEQ_1;                     // Enable sample timer
//    ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_10;  // ADC i/p ch A10 = temp sense
//    ADC12MCTL1 = ADC12SREF_0 + ADC12INCH_0 + ADC12EOS;   // ADC12INCH0 = Scroll wheel = A0
//    __delay_cycles(100);                    // delay to allow Ref to settle
//    ADC12CTL0 |= ADC12ENC;              // Enable conversion
//    bits30 = CALADC12_15V_30C;
//    bits85 = CALADC12_15V_85C;
//    degC_per_bit = ((float)(85.0 - 30.0))/((float)(bits85-bits30));
//
//
//    GAME_STATE my_state = UPDATE;
//    while(1){
//        int monthsADC;
//        int daysADC;
//        int hoursADC;
//        int minutesADC;
//        int secondsADC;
//
//        currKey = getKey();
//        char currKeyint = getKey();
//        currButton |= getState();
////        if((timer_cnt - prev_time) >= 1){
////            //Graphics_clearDisplay(&g_sContext);
////            my_state = UPDATE;
////        }
//
//        updateCurrentDate(timer_cnt);
//        if(currButton == BIT3)
//        {
//            my_state = EDIT_MONTH;
//            updateCurrentDate(timer_cnt);
//            //currButton = 0;
//        }
//        currButton = 0;
//        ADC12CTL0 |= ADC12ENC;              // Enable conversion
//
//        ADC12CTL0 &= ~ADC12SC;  // clear the start bit
//        ADC12CTL0 |= ADC12SC;       // Sampling and conversion start
//                                // Single conversion (single channel)
//
//        // Poll busy bit waiting for conversion to complete
//        while (ADC12CTL1 & ADC12BUSY)
//            __no_operation();
//
//        scrollWheel = ADC12MEM1;
//        switch(my_state){
////        Graphics_flushBuffer(&g_sContext);
//            case UPDATE: //display Welcome Screen
//                ADC12CTL0 |= ADC12ENC;              // Enable conversion
//
//                ADC12CTL0 &= ~ADC12SC;  // clear the start bit
//                ADC12CTL0 |= ADC12SC;       // Sampling and conversion start
//                                        // Single conversion (single channel)
//
//                    // Poll busy bit waiting for conversion to complete
//                    while (ADC12CTL1 & ADC12BUSY)
//                        __no_operation();
//
//                    int in_temp = ADC12MEM0;      // Read in results if conversion
//
//                    // Temperature in Celsius. See the Device Descriptor Table section in the
//                    // System Resets, Interrupts, and Operating Modes, System Control Module
//                    // chapter in the device user's guide for background information on the
//                    // used formula.
//                    temperatureDegC = (float)((long)in_temp - CALADC12_15V_30C) * degC_per_bit +30.0;
//
//                    // Temperature in Fahrenheit Tf = (9/5)*Tc + 32
//                    //temperatureDegF = temperatureDegC * 9.0/5.0 + 32.0;
//                    temps[timer_cnt % 30] = temperatureDegC;
//                    if(timer_cnt >= 30)
//                    {
//                        float sum = 0;
//                        int i;
//                        for(i = 0; i < 30; i++){
//                            sum += temps[i];
//                        }
//                        displayTemp(sum/30);
//                    }else{
//                        displayTemp(temperatureDegC);
//                    }
//                    displayTime(timer_copy);
//                    Graphics_flushBuffer(&g_sContext);
//                    //scrollWheel = ADC12MEM1;               // Read results if conversion done
//
//                    //__no_operation();                       // SET BREAKPOINT HERE
//                    prev_time = timer_cnt;
//                break;
//            case EDIT_MONTH: //counts down
//                monthsADC = ((float)scrollWheel/(float)4096)*(float)12+(float)1; //do something to map ADC to days so you add to time
//                months = monthsADC;
////                //long unsigned int idefk = dateToSeconds();
////                int daysFromMonth = monthIntervalToDays(months);
////                long unsigned int totalDays = daysFromMonth + days;
////                long unsigned int totalHours = hours + (totalDays * 24);
////                long unsigned int totalMinutes = minutes + (totalHours * 60);
////                long unsigned int totalSeconds = seconds + (totalMinutes * 60);
//
//                currButton |= getState();
//                if(currButton == BIT3)
//                {
//                    my_state = EDIT_DAY;
//                    updateCurrentDate(dateToSeconds());
//                }
//                displayTime(totalSeconds());
//                Graphics_flushBuffer(&g_sContext);
//                break;
//            case EDIT_DAY:
//                daysADC = ((float)scrollWheel/(float)4096)*(float)daysInMonth; //do something to map ADC to days so you add to time
//                days = daysADC;
//
//                displayTime(dateToSeconds());
//                if(currButton == BIT3)
//                {
//                    my_state = EDIT_DAY;
//                    updateCurrentDate(timer_cnt);
//                }
//                Graphics_flushBuffer(&g_sContext);
//                break;
//            case  EDIT_HOURS:
//                hoursADC = ((float)scrollWheel/(float)4096)*(float)24; //do something to map ADC to days so you add to time
//                hours = hoursADC;
//                displayTime(dateToSeconds());
//                if(currButton == BIT3)
//                {
//                    my_state = EDIT_DAY;
//                    updateCurrentDate(timer_cnt);
//                }
//                Graphics_flushBuffer(&g_sContext);
//                break;
//            case EDIT_MINS:
//                minutesADC = ((float)scrollWheel/(float)4096)*(float)60; //do something to map ADC to days so you add to time
//                minutes = minutesADC;
//                displayTime(dateToSeconds());
//                if(currButton == BIT3)
//                {
//                    my_state = EDIT_DAY;
//                    updateCurrentDate(timer_cnt);
//                }
//                Graphics_flushBuffer(&g_sContext);
//                break;
//            case EDIT_SEC:
//                secondsADC = ((float)scrollWheel/(float)4096)*(float)60; //do something to map ADC to days so you add to time
//                seconds = secondsADC;
//                displayTime(dateToSeconds());
//                if(currButton == BIT3)
//                {
//                    my_state = EDIT_DAY;
//                    updateCurrentDate(timer_cnt);
//                }
//                Graphics_flushBuffer(&g_sContext);
//                break;
//        }
//
//    currButton = 0;
//    }
//}
//
//void runtimerA2(void){
//// Use ACLK, 16 Bit, up mode, 1 divider
//    TA2CTL = TASSEL_1 + MC_1 + ID_0;
//    TA2CCR0 = 32767; //interrupts every second
//    TA2CCTL0 = CCIE; // TA2CCR0 interrupt enabled
//}
//
//void stoptimerA2(int reset)
//{
//    TA2CTL = MC_0; // stop timer
//    TA2CCTL0 &= ~CCIE; // TA2CCR0 interrupt disabled
//        if(reset)
//        {
//            timer_cnt = 0;
//            timer_copy = 0;
//        }
//}
//
//// Timer A2 interrupt service routine
//#pragma vector=TIMER2_A0_VECTOR
//__interrupt void TimerA2_ISR (void){
//    if(tdir){
//        timer_cnt++;
//        timer_copy++;
//        scrollWheel = ADC12MEM1;
//    }
//    else{
//        timer_cnt--;
//        timer_copy--;
//    }
//}
//
//void configUserButtons(void){
//    P7SEL &= ~(BIT0|BIT4); //S1, S4
//    P3SEL &= ~(BIT6); //S2
//    P2SEL &= ~(BIT2); //S3
//
//    P7DIR &= ~(BIT0|BIT4); //S1, S4
//    P3DIR &= ~(BIT6); //S2
//    P2DIR &= ~(BIT2); //S3
//
//    P7REN |= (BIT0|BIT4); //S1, S4
//    P3REN |= (BIT6); //S2
//    P2REN |= (BIT2); //S3
//
//    P7OUT |= (BIT0|BIT4); //S1, S4
//    P3OUT |= (BIT6); //S2
//    P2OUT |= (BIT2); //S3
//}
//
//uint8_t getState(void){
//        uint8_t result = 0x00;
//        if (~P7IN & BIT0) {//sw1
//            result |= BIT3;
//        }
//        if (~P3IN & BIT6) {
//            result |= BIT2;
//        }
//        if (~P2IN & BIT2) {
//            result |= BIT1;
//        }
//        if (~P7IN & BIT4) { //bit 4 is set
//            result |= BIT0;
//        }
//
//        return result;
//}
//
//void updateCurrentDate(long unsigned int inTime)
//{
//    days = ((inTime / 86400)%365); //86400 sec in a day
//    hours = ((inTime - (86400*days)) / 3600); //3600 sec in an hour
//    minutes = ((inTime - (86400*days) - (3600*hours)) / 60); //60 sec in a min
//    seconds = (inTime - (86400*days) - (3600*hours) - (60*minutes)); //remainder
//    int monthCounter;
//    for(monthCounter = 1; days > daysInMonth; monthCounter++){
//        days = days - daysInMonth;
//    }
//    months = monthCounter;
//}
//
//long unsigned int dateToSeconds(){
//    int daysFromMonth = monthIntervalToDays(months);
//    long unsigned int totalDays = daysFromMonth + days;
//    long unsigned int totalHours = hours + (totalDays * 24);
//    long unsigned int totalMinutes = minutes + (totalHours * 60);
//    long unsigned int totalSeconds = seconds + (totalMinutes * 60);
//    return totalSeconds;
//
//}
//
//int monthIntervalToDays(int startMonth){
//    int monthinDays = 0;
//    int i;
//    for(i = startMonth; i > 1; i--){
//        monthinDays = monthinDays + daysInMonthArr[i];
//    }
//    return monthinDays;
//
//}
//
//void displayTime(long unsigned int inTime)
//{
//    //int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
//    char time[14];
//    char date[13];
//
////    int days = ((inTime / 86400)%365); //86400 sec in a day
////    int hours = ((inTime - (86400*days)) / 3600); //3600 sec in an hour
////    int minutes = ((inTime - (86400*days) - (3600*hours)) / 60); //60 sec in a min
////    int seconds = (inTime - (86400*days) - (3600*hours) - (60*minutes)); //remainder
//
//    updateCurrentDate(inTime);
//
//    date[0] = 'D';
//    date[1] = 'A';
//    date[2] = 'T';
//    date[3] = 'E';
//    date[4] = '=';
//    date[5] = '>';
//
//    if(days <= 31)
//    {
//        daysInMonth = 31;
//        date[6] = 'J';
//        date[7] = 'A';
//        date[8] = 'N';
//        date[9] = ' ';
//        date[10] = (days/10)+48;
//        date[11] = (days%10)+48;
//    }
//    else if ((days > 31)&&(days <= 59))
//    {
//        daysInMonth = 28;
//        date[6] = 'F';
//        date[7] = 'E';
//        date[8] = 'B';
//        date[9] = ' ';
//        date[10] = ((days)/10)+48;
//        date[11] = ((days)%10)+48;
//    }
//    else if ((days > 59)&&(days <= 90))
//    {
//        daysInMonth = 31;
//        date[6] = 'M';
//        date[7] = 'A';
//        date[8] = 'R';
//        date[9] = ' ';
//        date[10] = ((days)/10)+48;
//        date[11] = ((days)%10)+48;
//    }
//    else if ((days > 90)&&(days <= 120))
//    {
//        daysInMonth = 30;
//        date[6] = 'A';
//        date[7] = 'P';
//        date[8] = 'R';
//        date[9] = ' ';
//        date[10] = ((days)/10)+48;
//        date[11] = ((days)%10)+48;
//    }
//    else if ((days > 120)&&(days <= 151))
//    {
//        daysInMonth = 31;
//        date[6] = 'M';
//        date[7] = 'A';
//        date[8] = 'Y';
//        date[9] = ' ';
//        date[10] = ((days)/10)+48;
//        date[11] = ((days)%10)+48;
//    }
//    else if ((days > 151)&&(days <= 181))
//    {
//        daysInMonth = 30;
//        date[6] = 'J';
//        date[7] = 'U';
//        date[8] = 'N';
//        date[9] = ' ';
//        date[10] = ((days)/10)+48;
//        date[11] = ((days)%10)+48;
//    }
//    else if ((days > 181)&&(days <= 212))
//    {
//        daysInMonth = 31;
//        date[6] = 'J';
//        date[7] = 'U';
//        date[8] = 'L';
//        date[9] = ' ';
//        date[10] = ((days)/10)+48;
//        date[11] = ((days)%10)+48;
//    }
//    else if ((days > 212)&&(days <= 243))
//    {
//        daysInMonth = 31;
//        date[6] = 'A';
//        date[7] = 'U';
//        date[8] = 'G';
//        date[9] = ' ';
//        date[10] = ((days)/10)+48;
//        date[11] = ((days)%10)+48;
//    }
//    else if ((days > 243)&&(days <= 273))
//    {
//        daysInMonth = 30;
//        date[6] = 'S';
//        date[7] = 'E';
//        date[8] = 'P';
//        date[9] = ' ';
//        date[10] = ((days)/10)+48;
//        date[11] = ((days)%10)+48;
//    }
//    else if ((days > 273)&&(days <= 304))
//    {
//        daysInMonth = 31;
//        date[6] = 'O';
//        date[7] = 'C';
//        date[8] = 'T';
//        date[9] = ' ';
//        date[10] = ((days)/10)+48;
//        date[11] = ((days)%10)+48;
//    }
//    else if ((days > 304)&&(days <= 334))
//    {
//        daysInMonth = 30;
//        date[6] = 'N';
//        date[7] = 'O';
//        date[8] = 'V';
//        date[9] = ' ';
//        date[10] = ((days)/10)+48;
//        date[11] = ((days)%10)+48;
//    }
//    else if ((days > 334)&&(days <= 365))
//    {
//        daysInMonth = 31;
//        date[6] = 'D';
//        date[7] = 'E';
//        date[8] = 'C';
//        date[9] = ' ';
//        date[10] = ((days)/10)+48;
//        date[11] = ((days)%10)+48;
//    }
//
//    time[0] = 'T';
//    time[1] = 'i';
//    time[2] = 'm';
//    time[3] = 'e';
//    time[4] = '=';
//    time[5] = '>';
//    time[6] = ((hours/10) + 48);
//    time[7] = ((hours%10) + 48);
//    time[8] = ':';
//    time[9] = ((minutes/10) + 48);
//    time[10] = ((minutes%10) + 48);
//    time[11] = ':';
//    time[12] = ((seconds/10) + 48);
//    time[13] = ((seconds%10) + 48);
//
//    Graphics_drawStringCentered(&g_sContext, time, 14, 48, 35, OPAQUE_TEXT);
//    Graphics_drawStringCentered(&g_sContext, date, 13, 48, 45, OPAQUE_TEXT);
//}
//
//void displayTemp(float inAvgTempC)
//{
//    char tempC[14];
//    char tempF[14];
//
//    //multiplied by 10
//    int tempCf = inAvgTempC*10;
//    int tempCtens = (tempCf/100);
//    int tempCones = ((tempCf - (tempCtens*100))/10);
//    int tempCtenths = (tempCf - (tempCtens*100) - (tempCones*10));
//
//    float tempFf = (inAvgTempC * (9.f/5) + 32) * 10;
//    int tempFtens = (tempFf/100);
//    int tempFones = ((tempFf - (tempFtens*100))/10);
//    int tempFtenths = (tempFf - (tempFtens*100) - (tempFones*10));
//
//    tempC[0] = 'T';
//    tempC[1] = 'e';
//    tempC[2] = 'm';
//    tempC[3] = 'p';
//    tempC[4] = '(';
//    tempC[5] = 'C';
//    tempC[6] = ')';
//    tempC[7] = '=';
//    tempC[8] = '>';
//    tempC[9] = (tempCtens + 48);
//    tempC[10] = (tempCones + 48);
//    tempC[11] = '.';
//    tempC[12] = (tempCtenths + 48);
//    tempC[13] ='C';
//
//    tempF[0] = 'T';
//    tempF[1] = 'e';
//    tempF[2] = 'm';
//    tempF[3] = 'p';
//    tempF[4] = '(';
//    tempF[5] = 'F';
//    tempF[6] = ')';
//    tempF[7] = '=';
//    tempF[8] = '>';
//    tempF[9] = (tempFtens + 48);
//    tempF[10] = (tempFones + 48);
//    tempF[11] = '.';
//    tempF[12] = (tempFtenths + 48);
//    tempF[13] ='F';
//
//    Graphics_drawStringCentered(&g_sContext, tempC, 14, 48, 15, OPAQUE_TEXT);
//    Graphics_drawStringCentered(&g_sContext, tempF, 14, 48, 25, OPAQUE_TEXT);
//}

