#include <msp430.h>
#include <stdint.h>
#include "./FatFS/ff.h"
#include "./FatFS/diskio.h"

//SD card variables
FATFS sdVolume;     // FatFs work area needed for each volume
FIL logfile;        // File object needed for each open file
uint16_t fp;        // Used for sizeof
uint8_t status = 17;    // Status variable that should change if successful
unsigned int backupCtr; // Counter for data backup
unsigned int mode; // operating mode(standby mode / measurement mode)
unsigned int measurementInit; //check if new file has to be created and opened
//float testFloat = 1.0;//85432.123;	// Sample floating point number
//int32_t printValue[2];	// Size 2 array that will hold the split float for printing

//Sensor variables
unsigned char RX_Data[14];
unsigned char TX_Data[2];
unsigned char RX_ByteCtr;
unsigned char TX_ByteCtr;
unsigned char slaveAddress = 0x69;  // Set slave address for ICM20948 0x68 for ADD pin=0/0x69 for ADD pin=1
unsigned int G_MODE;
unsigned int DPS_MODE;
int xAccel;
int yAccel;
int zAccel;
int xGyro;
int yGyro;
int zGyro;
int Temp;
int whoami;

void i2cInit(void);
void i2cWrite(unsigned char);
void i2cRead(unsigned char);

//*********************************************************************************************
//select sensitivity for sensors
const unsigned int AccelSensitivity = 2;
const unsigned int GyroSensitivity = 250;

//*********************************************************************************************
//helper function to initialize USCIB0 I2C
void i2cInit(void)
{

    // Configure USCI_B0 for I2C mode
     UCB0CTLW0 = UCSWRST;                      // put eUSCI_B in reset state
     UCB0CTLW0 |= UCMODE_3 | UCMST | UCSSEL__SMCLK; // I2C master mode, SMCLK
     UCB0BRW = 0x2;                            // baudrate = SMCLK / 2
     UCB0CTLW0 &= ~UCSWRST;                    // clear reset register
     UCB0IE |= UCTXIE0 | UCNACKIE;             // transmit and NACK interrupt enable
}

//*********************************************************************************************
//helper function to access ICM20948 registers
void i2cWrite(unsigned char address)
{
    __disable_interrupt();
    UCB0I2CSA = address;                // Load slave address
    UCB0IE |= UCTXIE;                   //Enable TX interrupt
    while(UCB0CTL1 & UCTXSTP);          // Ensure stop condition sent
    UCB0CTL1 |= UCTR + UCTXSTT;         // TX mode and START condition
    __bis_SR_register(CPUOFF + GIE);    // sleep until UCB0RXIFG is set ...
    //__bis_SR_register(GIE);    // sleep until UCB0TXIFG is set ...
}

//*********************************************************************************************
// helper function to read ICM20948 registers
void i2cRead(unsigned char address)
{
    __disable_interrupt();
    UCB0I2CSA = address;                // Load slave address
    UCB0IE |= UCRXIE;                   // Enable RX interrupt
    while(UCB0CTL1 & UCTXSTP);          // Ensure stop condition sent
    UCB0CTL1 &= ~UCTR;                  // RX mode
    UCB0CTL1 |= UCTXSTT;                // Start Condition
    __bis_SR_register(CPUOFF + GIE);    // sleep until UCB0RXIFG is set ...
    //__bis_SR_register(GIE);    // sleep until UCB0RXIFG is set ...
}

//*********************************************************************************************
////helper function to print floating point numbers
//void FloatToPrint(float floatValue, int32_t splitValue[2]){
//	int32_t i32IntegerPart;
//	int32_t i32FractionPart;
//
//	i32IntegerPart = (int32_t) floatValue;
//	i32FractionPart = (int32_t) (floatValue * 1000.0f);
//	i32FractionPart = i32FractionPart - (1000 * i32IntegerPart);
//	if(i32FractionPart < 0){
//		i32FractionPart *= -1;
//	}
//
//	splitValue[0] = i32IntegerPart;
//	splitValue[1] = i32FractionPart;
//}



//*********************************************************************************************
int main(void){

	  WDTCTL = WDTPW | WDTHOLD;       // Stop WDT

//--------------------------------------GPIO Config----------------------------------------------------------------------------

	  // Prepare LEDs
	  P1DIR = 0xFF ^ (BIT1 | BIT3);             // Set all but P1.1, 1.3 to output direction
	  P1OUT |= BIT0;							// LED On
	  P1OUT &= ~BIT0;                           //LED1=OFF initially
	  P4DIR = 0xFF;							  	// P4 output
	  P4OUT |= BIT6;							// P4.6 LED on
	  P4OUT &= ~BIT6;                           //LED2=OFF initially

//	  // Prepare I2C and clock pins
//	  P1SEL1 |= BIT6 | BIT7;                    // I2C pins
//	  PJSEL0 |= BIT4 | BIT5;					// Set J.4 & J.5 to accept crystal input for ACLK

	  // Prepare P1.1 and P1.3 for switch
	  //P1OUT = BIT1 | BIT3;                      // Pull-up resistor on P1.1, 1.3
	  //P1REN = BIT1 | BIT3;                      // Select pull-up mode for P1.1, 1.3

	  P1DIR &= ~BIT1;                           //set P1.1 (Switch2) to input
	  P1REN |= BIT1;                            //turn on resistor
	  P1OUT |= BIT1;                            //makes resistor a pull up
	  P1IES |= BIT1;                            //make sensitive to High-to-Low

	  P4DIR &= ~BIT5;                           //set P4.5 (Switch1) to input
	  P4REN |= BIT5;                            //turn on register
	  P4OUT |= BIT5;                            //makes resistor a pull up
	  P4IES |= BIT5;                            //make sensitive to High-to-Low

	  P1SEL1 |= BIT6 + BIT7;                    //for I2C functionality P1SEL1 high,P1SEL0 low
	  //P1SEL0|= BIT6 + BIT7;                   //for I2C functionality P1SEL1 high,P1SEL0 low

//	  P1IE |= BIT1;                             //Enable P1.1 IRQ (Switch2)
//	  P1IFG &= ~BIT1;                           //clear flag


	  // Disable the GPIO power-on default high-impedance mode to activate
	  // previously configured port settings
	  PM5CTL0 &= ~LOCKLPM5;

	  P4IE |= BIT5;               //Enable P4.5 IRQ (Switch1)
	  P4IFG &= ~BIT5;             //clear flag

	  // Initialize the I2C state machine
	  i2cInit();

//--------------------------------------CLOCK Config----------------------------------------------------------------------------


	  CSCTL0_H = CSKEY >> 8;                    // Unlock CS registers
	  CSCTL1 = DCOFSEL_6;                       // Set DCO to 8MHz
	  CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK; // Set ACLK = LFXTCLK; SMCLK = MCLK = DCO
	  CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;     // Set all dividers to 1
	  CSCTL4 &= ~LFXTOFF;                       // Turn on LFXT
	  CSCTL0_H = 0;                             // Lock CS registers

	  // Clock System Setup 8MHz Beispiel
//	    CSCTL0_H = CSKEY >> 8;                    // Unlock CS registers
//	    CSCTL1 = DCOFSEL_6;                       // Set DCO to 8MHz
//	    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;  // Set SMCLK = MCLK = DCO
//	                                              // ACLK = VLOCLK
//	    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;     // Set all dividers to 1
//	    CSCTL0_H = 0;                             // Lock CS registers

      // Clock System Setup 16MHz Beispiel
//	      // Configure one FRAM waitstate as required by the device datasheet for MCLK
//	        // operation beyond 8MHz _before_ configuring the clock system.
//	        FRCTL0 = FRCTLPW | NWAITS_1;
//	        CSCTL0_H = CSKEY >> 8;                    // Unlock CS registers
//	        CSCTL1 = DCORSEL | DCOFSEL_4;             // Set DCO to 16MHz
//	        CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK; // Set SMCLK = MCLK = DCO,
//	                                                  // ACLK = VLOCLK
//	        CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;     // Set all dividers
//	        CSCTL0_H = 0;                             // Lock CS registers


	  // Enable interrupts
	  __bis_SR_register(GIE);

//--------------------------------------Initialize SD card--------------------------------------------------------------------------------------------

	  //_delay_cycles(100000);

	    // Mount the SD Card
	    switch( f_mount(&sdVolume, "", 0) ){
	        case FR_OK:
	            status = 42;
	            break;
	        case FR_INVALID_DRIVE:
	            status = 1;
	            break;
	        case FR_DISK_ERR:
	            status = 2;
	            break;
	        case FR_NOT_READY:
	            status = 3;
	            break;
	        case FR_NO_FILESYSTEM:
	            status = 4;
	            break;
	        default:
	            status = 5;
	            break;
	    }

	    if(status != 42){
	        // Error has occurred
	        P4OUT |= BIT6;
	        while(1);
	    }

	      //init counter for backup --> swap out if RTC is working
	      backupCtr = 0;


//--------------------------------------Initialize ICM20948--------------------------------------------------------------------------------------------

	  // Wake up the ICM20948
	  //slaveAddress = 0x69;                  // ICM20948 address
	  TX_Data[1] = 0x06;                      // address of PWR_MGMT_1 register
      TX_Data[0] = 0b00000000;                // set register to zero (wakes up the ICM20948) and low power off
	  TX_ByteCtr = 2;
	  i2cWrite(slaveAddress);

	  _delay_cycles(20000);

	  TX_Data[1] = 0x7F;                      // address of BANK SEL register
	  TX_Data[0] = 0b00000000;                // BANK 0
	  TX_ByteCtr = 2;
	  i2cWrite(slaveAddress);

	  TX_Data[1] = 0x06;                      // address of PWR_MGMT_1 register
	  TX_Data[0] = 0b10000000;                // set register to zero (wakes up the ICM20948) RESET
	  TX_ByteCtr = 2;
	  i2cWrite(slaveAddress);

	  _delay_cycles(20000);

	  // Wake up the ICM20948
	  //slaveAddress = 0x69;                  // ICM20948 address
	  TX_Data[1] = 0x06;                      // address of PWR_MGMT_1 register
	  TX_Data[0] = 0b00000001;                //BIT6=0(wake up),BIT5=0(low power disable),BIT[2:0]=001(autoselect best clock)
	  TX_ByteCtr = 2;
	  i2cWrite(slaveAddress);

	  _delay_cycles(20000);

	  //    TX_Data[1] = 0x00;
	  //    TX_Data[0] = 0x00;                      // register address WHO AM I CHECK
	  //    TX_ByteCtr = 2;
	  //    i2cWrite(slaveAddress);
	  //    RX_ByteCtr = 2;
	  //    i2cRead(slaveAddress);
	  //    whoami |= RX_Data[1];
	  //    RX_ByteCtr = 1;

	  TX_Data[1] = 0x05;                      // address of LP_CONFIG register
	  TX_Data[0] = 0b00000000;                // set ACCEL/GYRO/I2CMST to continuous
	  TX_ByteCtr = 2;
	  i2cWrite(slaveAddress);

	  TX_Data[1] = 0x7F;                      // address of BANK SEL register
	  TX_Data[0] = 0b00100000;                // Select BANK 2
	  TX_ByteCtr = 2;
	  i2cWrite(slaveAddress);

	  switch(AccelSensitivity){
	      case 2:
	          G_MODE = 0b00000000;
	          break;
	      case 4:
	          G_MODE = 0b00000010;
	          break;
	      case 8:
	          G_MODE = 0b00000100;
	          break;
	      case 16:
	          G_MODE = 0b00000110;
	          break;
	      default:
	          G_MODE = 0b00000000;
	          break;
	  }

	  switch(GyroSensitivity){
	      case 250:
	          DPS_MODE = 0b00000000;
	          break;
	      case 500:
	          DPS_MODE = 0b00000010;
	          break;
	      case 1000:
	          DPS_MODE = 0b00000100;
	          break;
	      case 2000:
	          DPS_MODE = 0b00000110;
	          break;
	      default:
	          DPS_MODE = 0b00000000;
	          break;
	  }


	  TX_Data[1] = 0x14;                      // address of ACCEL_CONFIG_1 register
	  TX_Data[0] = G_MODE;                    // switch to selected accel mode
	  TX_ByteCtr = 2;
	  i2cWrite(slaveAddress);

	  TX_Data[1] = 0x7F;                      // address of BANK SEL register
	  TX_Data[0] = 0b00100000;                // Select BANK 2
	  TX_ByteCtr = 2;
	  i2cWrite(slaveAddress);

	  TX_Data[1] = 0x01;                      // address of GYRO_CONFIG_1 register
	  TX_Data[0] = DPS_MODE;                  // switch to selected gyro mode
	  TX_ByteCtr = 2;
	  i2cWrite(slaveAddress);

	  TX_Data[1] = 0x7F;                      // address of BANK SEL register
	  TX_Data[0] = 0b00000000;                // Select BANK 0
	  TX_ByteCtr = 2;
	  i2cWrite(slaveAddress);


	  mode = 1;                                 //start with standby mode after init
	  measurementInit = 0;

//--------------------------------------measurement loop-----------------------------------------------------------------------------------------

	  while(1){

	      if(mode == 1){
	      //standby mode
	          //wait in low power mode 0
	          P1OUT |= BIT0;                // LED2 on
	          __bis_SR_register(LPM0_bits);
	      }


	      else if(mode == 2){
	      //measurement mode including creating file, opening file, writing data to file and closing file
	          if(measurementInit == 0){
	          //measurement init phase = creating file, opening file

	              //char filename[] = "LOG2_00.CSV";
	              char filename[] = "RAW_00.CSV";
	              FILINFO fno;
	              FRESULT fr;
	              uint8_t i;
	              for(i = 0; i < 100; i++){
	                  //filename[5] = i / 10 + '0';
	                  //filename[6] = i % 10 + '0';
	                  filename[4] = i / 10 + '0';
	                  filename[5] = i % 10 + '0';
	                  fr = f_stat(filename, &fno);
	                      if(fr == FR_OK){
	                          continue;
	                      }
	                      else if(fr == FR_NO_FILE){
	                              break;
	                      }
	                      else{
	                          // Error occurred
	                          P4OUT |= BIT6;
	                          P1OUT |= BIT0;
	                          while(1);
	                      }
	              }

	              if(f_open(&logfile, filename, FA_WRITE | FA_OPEN_ALWAYS) == FR_OK) {    // Open file - If nonexistent, create
	                  f_lseek(&logfile, logfile.fsize);           // Move forward by filesize; logfile.fsize+1 is not needed in this application
	              }
	              f_printf(&logfile, "%d,%s,%d,%s\n",AccelSensitivity,"g",GyroSensitivity,"dps");
	              f_printf(&logfile, "%s,%s,%s,%s,%s,%s\n", "xAccel","yAccel","zAccel","xGyro","yGyro","zGyro");

	              P1OUT |= BIT0;                  //LED2 on
	              measurementInit++;
	          }
	          else{
	          //writing data to file

	              // Point to the ACCEL_XOUT_H register in the ICM20948
	              slaveAddress = 0x69;                      // ICM20948 address
	              TX_Data[0] = 0x2D;                        // register address
	              TX_ByteCtr = 1;
	              i2cWrite(slaveAddress);

	              slaveAddress = 0x69;                      // ICM20948 address
	              RX_ByteCtr = 12;
	              i2cRead(slaveAddress);

	              __disable_interrupt();                    // prevent getting new sensor data while saving values

	              xAccel  = RX_Data[11] << 8;               // MSB
	              xAccel |= RX_Data[10];                    // LSB
	              yAccel  = RX_Data[9] << 8;
	              yAccel |= RX_Data[8];
	              zAccel  = RX_Data[7] << 8;
	              zAccel |= RX_Data[6];
	              xGyro  = RX_Data[5] << 8;
	              xGyro |= RX_Data[4];
	              yGyro  = RX_Data[3] << 8;
	              yGyro |= RX_Data[2];
	              zGyro  = RX_Data[1] << 8;
	              zGyro |= RX_Data[0];
	              //Temp |= RX_Data[1] << 8;
	              //Temp |= RX_Data[0];


	              f_printf(&logfile, "%d,%d,%d,%d,%d,%d\n", xAccel,yAccel,zAccel,xGyro,yGyro,zGyro);

	              backupCtr++;
	              if(backupCtr == 500){
	                  P1OUT ^= BIT0;              // LED2 blinking
	                  //f_sync(&logfile);
	                  backupCtr = 0;
	              }

	              //backup every 1000 data writes - every ~3s --> swap out if RTC is working
//	              backupCtr++;
//	              if(backupCtr == 1000){
//	                  f_sync(&logfile);
//	                  backupCtr = 0;
//	              }


	              __enable_interrupt();

	              //__no_operation();                            // Set breakpoint >>here<< and read
	          }
	      }

	      else{
	          mode = 1;                                        //something went wrong --> switch to standby mode
	      }



	  }
}

/**********************************************************************************************/
// USCIAB0TX_ISR
#pragma vector = USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
{
    if(UCB0CTL1 & UCTR)                 // TX mode (UCTR == 1)
    {
        if (TX_ByteCtr)                     // TRUE if more bytes remain
        {
            TX_ByteCtr--;               // Decrement TX byte counter
            UCB0TXBUF = TX_Data[TX_ByteCtr];    // Load TX buffer
        }
        else                        // no more bytes to send
        {
            UCB0CTL1 |= UCTXSTP;            // I2C stop condition
            UCB0IFG &= ~UCTXIFG;         // Clear USCI_B0 TX int flag
            __bic_SR_register_on_exit(CPUOFF);  // Exit LPM0
        }
    }
    else // (UCTR == 0)                 // RX mode
    {
        RX_ByteCtr--;                       // Decrement RX byte counter
        if (RX_ByteCtr)                     // RxByteCtr != 0
        {
            RX_Data[RX_ByteCtr] = UCB0RXBUF;    // Get received byte
            if (RX_ByteCtr == 1)            // Only one byte left?
            UCB0CTL1 |= UCTXSTP;            // Generate I2C stop condition
        }
        else                        // RxByteCtr == 0
        {
            RX_Data[RX_ByteCtr] = UCB0RXBUF;    // Get final received byte
            __bic_SR_register_on_exit(CPUOFF);  // Exit LPM0
        }
    }
}


/**********************************************************************************************/
//Switch2 interrupt
//#pragma vector = PORT1_VECTOR
//__interrupt void ISR_Port1_S2(void){
//    f_close(&logfile);          // Close the file
//    P1OUT |= BIT0;              // LED2 on
//    P4IFG &= ~BIT5;             // clear flag
//    while(1);
//}

#pragma vector = PORT4_VECTOR
__interrupt void ISR_Port4_S1(void){
    if(mode == 1){                  //button press in standby mode
        P1OUT &= ~BIT0;             //LED2 off
        mode = 2;                   //switch to measurement mode
        P4IFG &= ~BIT5;             // clear flag
        __bic_SR_register_on_exit(LPM0_bits); //exit LPM0
    }
    else if(mode == 2){             //button press in measurement mode
        P1OUT &= ~BIT0;             //LED2 off
        mode = 1;                   //switch to standby mode
        f_close(&logfile);          // Close the file
        measurementInit = 0;        //reset value to open new file for the next measurement
        P4IFG &= ~BIT5;             // clear flag
    }
    else{                           //something went wrong --> switch to standby mode
        mode = 1;
        P4IFG &= ~BIT5;             // clear flag
    }
}
