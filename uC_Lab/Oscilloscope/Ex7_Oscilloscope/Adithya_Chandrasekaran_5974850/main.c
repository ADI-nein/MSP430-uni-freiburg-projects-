/*Exercise_7_Chandrasekaran
 *
 * Connections:
 *
 * PB5 --> P1.5
 * PB6 --> P1.6
 *
 * P3.6(PWM) --> DAC_IN(X2)
 * X8(DAC_OUT) --> P1.7
*/

#include <msp430.h>
#include <templateEMP.h>

// Function Definitions
// To define PWM frequency
void initPWM();

// To generate timer interrupt which can help generate the wave
// and define ADC sampling
void initTimer();

//
void initADC();

void signalGenControlConfig();
void signalGeneratorControl();


// Pre-calculated sine, trapezoidal and rectangular wave table that will help define the width of the PWM.
float sineLookupTable[16] = {0.0, 0.1951, 0.3536, 0.4619, 0.5, 0.4619, 0.3536, 0.1951, 0.0, -0.1951, -0.3536, -0.4619, -0.5, -0.4619, -0.3536, -0.1951};

float trapezoidalLookupTable[16] = {-0.5, -0.25, 0.0, 0.25, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.25, 0.0, -0.25, -0.5, -0.5};

float rectangluarLookupTable[16] = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5};


/* We have 16 samples so, sampling frequency for 3hz is
 * fsample = 3 * 16 = 48Hz
 * Tsample = 1/fsample = 1/48 = 20833 useconds
 *
 * Similarly we can calulate values for each frequency
 *
 * A larger table would've given us a better sampling frequency hence a more accurate output
 * however due to the memory constraints of the MSP430 here I'm sticking to 16 values
 *
 * */
unsigned int frequencyLookupTable[8] = {62500, 20833, 12500, 8929, 6944, 5682, 4808, 4166}; //1Hz, 3Hz, ......, 15Hz

// Multiplying this gain value to vary the amplidtude
float amplitudeLookupTable[8] = {0.1, 0.2, 0.3, 0.5, 0.6, 0.7, 0.8, 0.9};


volatile int adcVal;
// Assigning default output wave of the signal generator to be
volatile unsigned int ampControl = 4, freqControl = 2, waveState = 1, waveIndex = 0;


void main(void) {
    initMSP();

    // Initialising all modules
    // See defnition below
    initPWM();
    initTimer();
    initADC();

    signalGenControlConfig();

    while (1){
        signalGeneratorControl();
    }
}


void initTimer() {
    TA1CCR0 = frequencyLookupTable[freqControl];   // Set timer period for ADC sampling
    TA1CCTL0 = CCIE;          // Enable timer interrupt
    TA1CTL = TASSEL_2 + MC_1; // SMCLK, up mode
}

void initADC() {
    ADC10CTL1 = INCH_7;                // Input - A7
    ADC10CTL0 = ADC10SHT_2 + ADC10ON; // 16 x ADC10CLKs - 16 sample-hold,  ADC turned on
    ADC10AE0 |= BIT7;                // Enable analog input A7
}

void initPWM() {
    P3DIR |= BIT6;           // Set P3.6 as output
    P3SEL |= BIT6;           // Select TA0.2 option for P3.6

    TA0CCR0 = 100;           // PWM period is 100us
    TA0CCTL2 = OUTMOD_7;     // reset/set output mode
    TA0CTL = TASSEL_2 + MC_1; // SMCLK, up mode
}


void signalGenControlConfig(){
    //Set P1.3 and P1.4 as input and pull it high (Ref Circuit Diagram)
    P1DIR &= ~(BIT5+BIT6);
    P1REN |= (BIT5+BIT6);
    P1OUT |= (BIT5+BIT6);

    P2DIR &= ~BIT7;

    /* Below init has been copied from exercise 2 */
    // P2.0 to P2.5 are all outputs
    P2DIR |= (BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5) ;

    // Modify XTAL pins to be I / O
    P2SEL &= ~(BIT6 | BIT7) ;
    P2SEL2  &= ~(BIT6 | BIT7);

    // Reset clock signal to zero.
    P2OUT &= ~BIT4;

    // Clear the shift registers with /CLR and reset it afterwards.
    P2OUT &= ~BIT5;
    P2OUT |= BIT5;

    // Set the shift register 2 mode right shift mode (S0 = 1 , S1 = 0) ,
    P2OUT |= BIT0 ;
    P2OUT &= ~ BIT1 ;
}



void signalGeneratorControl(){
    int i, buttonState;

    /* Code is reused from exercise 2 - to find out which button is pressed */
    // SR1 to parallel (S0 = S1 = 1)
    P2OUT |= (BIT0 + BIT1);

    // Applying a right shift to detect next button Change
    // Next time when the function is called QD is 0
    P2OUT |= BIT4 ;
    // reset the clock
    P2OUT &= ~BIT4 ;

    // 10ms debounce time to change each step
    __delay_cycles(25000);

    for(i = 4; i > 0; --i){
        if(P2IN & BIT7){
            buttonState = i;
            break;
        }
        // Set to right shift mode
        P2OUT &= ~BIT3;

        // Applying clock would shift 1 to the right
        // As SR is pulled high
        P2OUT |= BIT4 ;
        // reset the clock
        P2OUT &= ~ BIT4 ;

        P2OUT |= BIT3;
    }


    __delay_cycles(25000);
    // If button 1 is pressed, then switch to the previous signal
    if(buttonState == 1){
//         serialPrintln("Button 1 pressed-switching to previous signal");

        // Circular fashion - If you want to go back to the previous wave from wave 1
        // then you'll end up in the last wave
        if(waveState == 1)
            waveState = 3;
        else
            waveState--;
    }

    // If button 2 is pressed, then switch to the next signal
    if(buttonState == 2){
//         serialPrintln("Button 2 pressed-switching to next signal");

        // Circular fashion - If you want to go to the next wave from wave 3
        // then you'll end up with the first wave
        if(waveState == 3)
            waveState = 1;
        else
            waveState++;
    }

    // If button 3 is pressed, then reduce frequency
    // and make sure you are already not at the lowest frequency in the table
    if(buttonState == 3 && freqControl != 0){
//        serialPrintln("Button 3 pressed-decreasing frequency");
        freqControl--;
        TA1CCR0 = frequencyLookupTable[freqControl];
    }

    // If button 4 is pressed, then increase frequency
    // and make sure you are already not at the highest frequency in the table
    if(buttonState == 4 && freqControl != 7){
//        serialPrintln("Button 4 pressed-increasing frequency");
        freqControl++;
        TA1CCR0 = frequencyLookupTable[freqControl];
    }

    // If button 5 is pressed, then decrease amplitude
    // and make sure you are already not at the lowest amplitude in the table
    if (!(P1IN & BIT5) && ampControl != 0){
//        serialPrintln("Button 5 pressed-decreasing Amplitude");
        ampControl--;
    }

    // If button 6 is pressed, then increase amplitude
    // and make sure you are already not at the highest amplitude in the table
    else if(!(P1IN & BIT6) && ampControl != 7){
//        serialPrintln("Button 6 pressed-increasing Amplitude");
        ampControl++;
    }

}



//Timer1 ISR for triggering ADC sampling and generating wave
#pragma vector = TIMER1_A0_VECTOR
__interrupt void Timer1_A0_ISR(void) {

    // If the interrupt is generated, start ADC conversion
    ADC10CTL0 |= ADC10SC + ENC;

    // Wait until result is ready
    while (ADC10CTL1 & ADC10BUSY) ;

    // If result is ready , disable ADC conversion and copy it to adcValue
    ADC10CTL0 &= ~(ADC10SC + ENC);
    adcVal = ADC10MEM;

//    serialPrint("$");
    serialPrintInt(adcVal);
    serialPrintln(",");
//    serialPrintln(";");

    if(waveState == 1){
        // Update PWM duty cycle - Amplitude * duty Cycle from sine table
        // 0.5 is added so that the "0 value" is at 0.5V
        TA0CCR2 =  (amplitudeLookupTable[ampControl] * sineLookupTable[waveIndex] + 0.5) * TA0CCR0;

        // Increment wave index (%16 beacuse we have 16 values)
        waveIndex = (waveIndex + 1) % 16;
    }

    else if(waveState == 2){
        // Update PWM duty cycle - Amplitude * duty Cycle from trapezoidal table
        // 0.5 is added so that the "0 value" is at 0.5V
        TA0CCR2 = (amplitudeLookupTable[ampControl] * trapezoidalLookupTable[waveIndex] + 0.5) * TA0CCR0;

        // Increment wave index and wrap around the table
        waveIndex = (waveIndex + 1) % 16;
    }

    else if(waveState == 3){
        // Update PWM duty cycle Amplitude * duty Cycle from rectangular table
        // 0.5 is added so that the "0 value" is at 0.5V
        TA0CCR2 =  (amplitudeLookupTable[ampControl] * rectangluarLookupTable[waveIndex] + 0.5) * TA0CCR0;
        waveIndex = (waveIndex + 1) % 16;    // Increment wave index
    }

    TA1CCTL0 &= ~CCIFG;

}




