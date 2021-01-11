#include <msp430.h>
#include <stdint.h>
#include <stdio.h>
#include "./FatFS/ff.h"
#include "./FatFS/diskio.h"

#define SW1 BIT0 //Port3
#define SW2 BIT5 //Port1
#define SW3 BIT4 //Port1
#define SW4 BIT3 //Port1

#define RTC_SPI_SEL         P2SEL1
#define RTC_SPI_DIR         P2DIR
#define RTC_SPI_REN         P2REN
#define RTC_SPI_OUT         P2OUT
#define RTC_SPI_SOMI        BIT6    // P2.6
#define RTC_SPI_SIMO        BIT5    // P2.5
#define RTC_SPI_CLK         BIT4    // P2.4
#define RTC_CS              BIT2    // P4.2
#define RTC_CS_SEL          P4SEL1
#define RTC_CS_OUT          P4OUT
#define RTC_CS_DIR          P4DIR

#define MAX_BUFFER_SIZE     20
#define DUMMY   0xFF
#define TIME_ARRAY_LENGTH 7 // Total number of writable time values in device
enum time_order {
    TIME_SECONDS, // 0
    TIME_MINUTES, // 1
    TIME_HOURS,   // 2
    TIME_DAY,     // 3
    TIME_DATE,    // 4
    TIME_MONTH,   // 5
    TIME_YEAR,    // 6
};

// take __DATE__ predefined compiler macro to update RTC date registers
// __Date__ Format: MMM DD YYYY (First D may be a space if <10)
// <MONTH>
#define BUILD_MONTH_JAN ((__DATE__[0] == 'J') && (__DATE__[1] == 'a')) ? 1 : 0
#define BUILD_MONTH_FEB (__DATE__[0] == 'F') ? 2 : 0
#define BUILD_MONTH_MAR ((__DATE__[0] == 'M') && (__DATE__[1] == 'a') && (__DATE__[2] == 'r')) ? 3 : 0
#define BUILD_MONTH_APR ((__DATE__[0] == 'A') && (__DATE__[1] == 'p')) ? 4 : 0
#define BUILD_MONTH_MAY ((__DATE__[0] == 'M') && (__DATE__[1] == 'a') && (__DATE__[2] == 'y')) ? 5 : 0
#define BUILD_MONTH_JUN ((__DATE__[0] == 'J') && (__DATE__[1] == 'u') && (__DATE__[2] == 'n')) ? 6 : 0
#define BUILD_MONTH_JUL ((__DATE__[0] == 'J') && (__DATE__[1] == 'u') && (__DATE__[2] == 'l')) ? 7 : 0
#define BUILD_MONTH_AUG ((__DATE__[0] == 'A') && (__DATE__[1] == 'u')) ? 8 : 0
#define BUILD_MONTH_SEP (__DATE__[0] == 'S') ? 9 : 0
#define BUILD_MONTH_OCT (__DATE__[0] == 'O') ? 10 : 0
#define BUILD_MONTH_NOV (__DATE__[0] == 'N') ? 11 : 0
#define BUILD_MONTH_DEC (__DATE__[0] == 'D') ? 12 : 0
#define BUILD_MONTH BUILD_MONTH_JAN | BUILD_MONTH_FEB | BUILD_MONTH_MAR | \
                    BUILD_MONTH_APR | BUILD_MONTH_MAY | BUILD_MONTH_JUN | \
                    BUILD_MONTH_JUL | BUILD_MONTH_AUG | BUILD_MONTH_SEP | \
                    BUILD_MONTH_OCT | BUILD_MONTH_NOV | BUILD_MONTH_DEC
// <DATE>
#define BUILD_DATE_0 ((__DATE__[4] == ' ') ? 0 : (__DATE__[4] - 0x30))
#define BUILD_DATE_1 (__DATE__[5] - 0x30)
#define BUILD_DATE ((BUILD_DATE_0 * 10) + BUILD_DATE_1)
// <YEAR>
#define BUILD_YEAR (((__DATE__[7] - 0x30) * 1000) + ((__DATE__[8] - 0x30) * 100) + \
                    ((__DATE__[9] - 0x30) * 10)  + ((__DATE__[10] - 0x30) * 1))

//take __TIME__ predefined compiler macro to update RTC time registers
// __TIME__ Format: HH:MM:SS (First number of each is padded by 0 if <10)
// <HOUR>
#define BUILD_HOUR_0 ((__TIME__[0] == ' ') ? 0 : (__TIME__[0] - 0x30))
#define BUILD_HOUR_1 (__TIME__[1] - 0x30)
#define BUILD_HOUR ((BUILD_HOUR_0 * 10) + BUILD_HOUR_1)
// <MINUTE>
#define BUILD_MINUTE_0 ((__TIME__[3] == ' ') ? 0 : (__TIME__[3] - 0x30))
#define BUILD_MINUTE_1 (__TIME__[4] - 0x30)
#define BUILD_MINUTE ((BUILD_MINUTE_0 * 10) + BUILD_MINUTE_1)
// <SECOND>
#define BUILD_SECOND_0 ((__TIME__[6] == ' ') ? 0 : (__TIME__[6] - 0x30))
#define BUILD_SECOND_1 (__TIME__[7] - 0x30)
#define BUILD_SECOND ((BUILD_SECOND_0 * 10) + BUILD_SECOND_1)

// DS3234_registers -- Definition of DS3234 registers
enum DS3234_registers {
    DS3234_REGISTER_SECONDS, // 0x00
    DS3234_REGISTER_MINUTES, // 0x01
    DS3234_REGISTER_HOURS,   // 0x02
    DS3234_REGISTER_DAY,     // 0x03
    DS3234_REGISTER_DATE,    // 0x04
    DS3234_REGISTER_MONTH,   // 0x05
    DS3234_REGISTER_YEAR,    // 0x06
    DS3234_REGISTER_A1SEC,   // 0x07
    DS3234_REGISTER_A1MIN,   // 0x08
    DS3234_REGISTER_A1HR,    // 0x09
    DS3234_REGISTER_A1DA,    // 0x0A
    DS3234_REGISTER_A2MIN,   // 0x0B
    DS3234_REGISTER_A2HR,    // 0x0C
    DS3234_REGISTER_A2DA,    // 0x0D
    DS3234_REGISTER_CONTROL, // 0x0E
    DS3234_REGISTER_STATUS,  // 0x0F
    DS3234_REGISTER_XTAL,    // 0x10
    DS3234_REGISTER_TEMPM,   // 0x11
    DS3234_REGISTER_TEMPL,   // 0x12
    DS3234_REGISTER_TEMPEN,  // 0x13
  DS3234_REGISTER_RESERV1, // 0x14
  DS3234_REGISTER_RESERV2, // 0x15
  DS3234_REGISTER_RESERV3, // 0x16
  DS3234_REGISTER_RESERV4, // 0x17
  DS3234_REGISTER_SRAMA,   // 0x18
  DS3234_REGISTER_SRAMD    // 0x19
};

// Base register for complete time/date readings
#define DS3234_REGISTER_BASE DS3234_REGISTER_SECONDS

uint8_t TimeArray[7];    //{seconds, minutes, hours, day, date, month, year}
uint8_t _time[TIME_ARRAY_LENGTH];

typedef enum SPI_ModeEnum{
    IDLE_MODE,
    TX_REG_ADDRESS_MODE,
    RX_REG_ADDRESS_MODE,
    TX_DATA_MODE,
    RX_DATA_MODE,
    TIMEOUT_MODE
} SPI_Mode;

/* Used to track the state of the software state machine*/
SPI_Mode MasterMode = IDLE_MODE;

/* The Register Address/Command to use*/
uint8_t TransmitRegAddr = 0;

/* ReceiveBuffer: Buffer used to receive data in the ISR
 * RXByteCtr: Number of bytes left to receive
 * ReceiveIndex: The index of the next byte to be received in ReceiveBuffer
 * TransmitBuffer: Buffer used to transmit data in the ISR
 * TXByteCtr: Number of bytes left to transfer
 * TransmitIndex: The index of the next byte to be transmitted in TransmitBuffer
 * */
uint8_t ReceiveBuffer[MAX_BUFFER_SIZE] = {0};
uint8_t RXByteCtr = 0;  //RX Counter for SPI
uint8_t ReceiveIndex = 0;
uint8_t TransmitBuffer[MAX_BUFFER_SIZE] = {0};
uint8_t TXByteCtr = 0;
uint8_t TransmitIndex = 0;


//SD card variables
FATFS sdVolume;     // FatFs work area needed for each volume
FIL logfile;        // File object needed for each open file
uint16_t fp;        // Used for sizeof
uint8_t status = 17;    // SD card status variable that should change if successful
unsigned int backupCtr = 0; // Counter for data backup
unsigned int mode; // operating mode(standby mode / measurement mode)
unsigned int measurementInit; //check if new file has to be created and opened

unsigned char RX_Data[23];
unsigned char TX_Data[2];
unsigned char RX_ByteCtr = 0;
unsigned char TX_ByteCtr = 0;
unsigned char slaveAddress = 0x69;  // Set slave address for ICM20948 0x68 for ADD pin=0/0x69 for ADD pin=1
unsigned int G_MODE;
unsigned int DPS_MODE;
int sensorsetting = 0b0000; // DIPswitch position to control sensor mode(accel+gyro setting)
int xAccel = 0;
int yAccel = 0;
int zAccel = 0;
int xGyro = 0;
int yGyro = 0;
int zGyro = 0;
int Temp = 0;
int xMag = 0;
int yMag = 0;
int zMag = 0;
int magstat1 = 0;
int magstat2 = 0;
int whoami = 0;
int register_value = 0;
int slave4done = 0;
int count = 0;
int a = 0;

void i2cInit(void);
void i2cWrite(unsigned char);
void i2cRead(unsigned char);

//*********************************************************************************************
//select sensitivity for sensors
unsigned int AccelSensitivity = 2;
unsigned int GyroSensitivity = 250;

//*********************************************************************************************

// Asserts the CS pin to the RTC
static void RTC_SELECT (void){
    RTC_CS_OUT &= ~RTC_CS;
}
//*********************************************************************************************

// De-asserts (set high) the CS pin to the RTC
static void RTC_DESELECT (void){
    RTC_CS_OUT |= RTC_CS;
}
//*********************************************************************************************

//helper function to initialize USCIB0 I2C
void i2cInit(void)
{

    // Configure USCI_B0 for I2C mode
     UCB0CTLW0 = UCSWRST;                      // put eUSCI_B in reset state
     UCB0CTLW0 |= UCMODE_3 | UCMST | UCSSEL__SMCLK | UCSYNC; // I2C master mode, SMCLK
     //UCB0BRW = 0x2;                            // baudrate = SMCLK / 2
     //UCB0BRW = 0x50;                            // baudrate = SMCLK / 20 = ~400kHz
     UCB0BRW = 8;                            // baudrate = SMCLK / 20 = ~400kHz
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
//helper function for RTC SPI data transfer
void SendUCA1Data(uint8_t val)
{
    while (!(UCA1IFG & UCTXIFG));              // USCI_A1 TX buffer ready?
    UCA1TXBUF = val;
}
//*********************************************************************************************

void CopyArray(uint8_t *source, uint8_t *dest, uint8_t count)
{
    uint8_t copyIndex = 0;
    for (copyIndex = 0; copyIndex < count; copyIndex++)
    {
        dest[copyIndex] = source[copyIndex];
    }
}
//*********************************************************************************************
/* SPI Write and Read Functions */

/* For slave device, writes the data specified in *reg_data
 * reg_addr: The register or command to send to the slave.
 * *reg_data: The buffer to write
 * count: The length of *reg_data
 *  */
//SPI_Mode SPI_Master_WriteReg(uint8_t reg_addr, uint8_t *reg_data, uint8_t count);

/* For slave device, read the data specified in slaves reg_addr.
 * The received data is available in ReceiveBuffer
 * reg_addr: The register or command to send to the slave.
 * count: The length of data to read
 *  */
SPI_Mode SPI_Master_WriteReg(uint8_t reg_addr, uint8_t *reg_data, uint8_t count)
{
    //Port initialization for SPI operation
        RTC_SPI_SEL |= RTC_SPI_CLK | RTC_SPI_SOMI | RTC_SPI_SIMO;
        RTC_SPI_DIR |= RTC_SPI_CLK | RTC_SPI_SIMO;

        RTC_CS_SEL &= ~RTC_CS;
        RTC_CS_OUT |= RTC_CS;
        RTC_CS_DIR |= RTC_CS;

        RTC_SPI_REN |= RTC_SPI_SOMI | RTC_SPI_SIMO;
        RTC_SPI_OUT |= RTC_SPI_SOMI | RTC_SPI_SIMO;

        //Initialize USCI_A1 for SPI Master operation
        UCA1CTLW0 = UCSWRST;                           //Put state machine in reset
        UCA1CTLW0 |= UCCKPL | UCMSB | UCMST | UCSYNC;  //3-pin, 8-bit SPI master
                                                       //Clock polarity select - The inactive state is high
        UCA1CTLW0 |= UCSSEL_2;                         //Use SMCLK, keep RESET
        UCA1BR0 = 3;                                   //Initial SPI clock must be <400kHz
        UCA1BR1 = 0;                                   //f_UCxCLK = 1MHz/(3+1) = 250kHz
        //UCA1BRW = 25;
        UCA1CTLW0 &= ~UCSWRST;                         //Release USCI state machine
        UCA1IFG &= ~UCRXIFG;
        UCA1IE |= UCRXIE;                              // Enable USCI_A1 RX interrupt

    MasterMode = TX_REG_ADDRESS_MODE;
    TransmitRegAddr = reg_addr | 0x80;              //writable RTC registers start with 0x80
    CopyArray(reg_data, TransmitBuffer, count);     //Copy register data to TransmitBuffer
    TXByteCtr = count;
    RXByteCtr = 0;
    ReceiveIndex = 0;
    TransmitIndex = 0;
    RTC_SELECT();
    SendUCA1Data(TransmitRegAddr);

    __bis_SR_register(CPUOFF + GIE);                // Enter LPM0 w/ interrupts

    RTC_DESELECT();
    return MasterMode;
}
//*********************************************************************************************

SPI_Mode SPI_Master_ReadReg(uint8_t reg_addr, uint8_t count)
{
    //Port initialization for SPI operation
        RTC_SPI_SEL |= RTC_SPI_CLK | RTC_SPI_SOMI | RTC_SPI_SIMO;
        RTC_SPI_DIR |= RTC_SPI_CLK | RTC_SPI_SIMO;

        RTC_CS_SEL &= ~RTC_CS;
        RTC_CS_OUT |= RTC_CS;
        RTC_CS_DIR |= RTC_CS;

        RTC_SPI_REN |= RTC_SPI_SOMI | RTC_SPI_SIMO;
        RTC_SPI_OUT |= RTC_SPI_SOMI | RTC_SPI_SIMO;

        //Initialize USCI_A1 for SPI Master operation
        UCA1CTLW0 = UCSWRST;                                //Put state machine in reset
        UCA1CTLW0 |= UCCKPL | UCMSB | UCMST | UCSYNC;       //3-pin, 8-bit SPI master
                                                            //Clock polarity select - The inactive state is high
        UCA1CTLW0 |= UCSSEL_2;                              //Use SMCLK, keep RESET
        UCA1BR0 = 3;                                        //Initial SPI clock must be <400kHz
        UCA1BR1 = 0;                                        //f_UCxCLK = 1MHz/(3+1) = 250kHz
        //UCA1BRW = 25;
        UCA1CTLW0 &= ~UCSWRST;                              //Release USCI state machine
        UCA1IFG &= ~UCRXIFG;
        UCA1IE |= UCRXIE;                                   // Enable USCI_A1 RX interrupt

    MasterMode = TX_REG_ADDRESS_MODE;
    TransmitRegAddr = reg_addr;
    RXByteCtr = count;
    TXByteCtr = 0;
    ReceiveIndex = 0;
    TransmitIndex = 0;
    RTC_SELECT();
    SendUCA1Data(TransmitRegAddr);
    __bis_SR_register(CPUOFF + GIE);              // Enter LPM0 w/ interrupts
    RTC_DESELECT();
    return MasterMode;
}
//*********************************************************************************************

// BCDtoDEC -- convert binary-coded decimal (BCD) to decimal
uint8_t BCDtoDEC(uint8_t val)
{
    return ( ( val / 0x10) * 10 ) + ( val % 0x10 );
}
//*********************************************************************************************

// DECtoBCD -- convert decimal to binary-coded decimal (BCD)
uint8_t DECtoBCD(uint8_t val)
{
    return ( ( val / 10 ) * 0x10 ) + ( val % 10 );
}
//*********************************************************************************************
//read RTC time and date registers
void DS3234GetCurrentTime(void){
   P3OUT |= BIT4;
   RTC_SELECT();
   unsigned int i;  // For loop counter
   SPI_Master_ReadReg(DS3234_REGISTER_BASE,7);

   for(i = 0; i < TIME_ARRAY_LENGTH; i++){
       TimeArray[i] = BCDtoDEC(ReceiveBuffer[i]);
   }
   RTC_DESELECT();
}
//*********************************************************************************************

// setTime -- Set time and date/day registers of DS3234 (using data array)
void setTime(uint8_t * time, uint8_t len)
{
    if (len != TIME_ARRAY_LENGTH){
        return;
    }
    SPI_Master_WriteReg(DS3234_REGISTER_BASE, time, TIME_ARRAY_LENGTH);
}
//*********************************************************************************************

// autoTime -- Fill DS3234 time registers with compiler time/date
void autoTime()
{
    _time[TIME_SECONDS] = DECtoBCD(BUILD_SECOND);
    _time[TIME_MINUTES] = DECtoBCD(BUILD_MINUTE);
    _time[TIME_HOURS] = BUILD_HOUR;
    _time[TIME_HOURS] = DECtoBCD(_time[TIME_HOURS]);
    _time[TIME_MONTH] = DECtoBCD(BUILD_MONTH);
    _time[TIME_DATE] = DECtoBCD(BUILD_DATE);
    _time[TIME_YEAR] = DECtoBCD(BUILD_YEAR - 2000);

    // Calculate weekday (from here: http://stackoverflow.com/a/21235587)
    // Result: 0 = Sunday, 6 = Saturday
    int d = BUILD_DATE;
    int m = BUILD_MONTH;
    int y = BUILD_YEAR;
    int weekday = (d+=m<3?y--:y-2,23*m/9+d+4+y/4-y/100+y/400)%7;
    weekday += 1; // Library defines Sunday=1, Saturday=7
    _time[TIME_DAY] = DECtoBCD(weekday);

    setTime(_time, TIME_ARRAY_LENGTH);
    //setTime(testdata, TIME_ARRAY_LENGTH);
}
//*********************************************************************************************
//helper function to read DIP switch postion for setting accelerometer+gyro modes
void checkDIPswitch(){
    if(!(P3IN & SW1)){
        sensorsetting |= (1<<3);
    }
    else{
        sensorsetting &= ~(1<<3);
    }
    if(!(P1IN & SW2)){
        sensorsetting |= (1<<2);
    }
    else{
        sensorsetting &= ~(1<<2);
    }
    if(!(P1IN & SW3)){
        sensorsetting |= (1<<1);
    }
    else{
        sensorsetting &= ~(1<<1);
    }
    if(!(P1IN & SW4)){
        sensorsetting |= (1<<0);
    }
    else{
        sensorsetting &= ~(1<<0);
    }

    switch(sensorsetting){
        case 0:
            AccelSensitivity = 2;
            GyroSensitivity = 250;
            break;
        case 1:
            AccelSensitivity = 4;
            GyroSensitivity = 250;
            break;
        case 2:
            AccelSensitivity = 8;
            GyroSensitivity = 250;
            break;
        case 3:
            AccelSensitivity = 16;
            GyroSensitivity = 250;
            break;
        case 4:
            AccelSensitivity = 2;
            GyroSensitivity = 500;
            break;
        case 5:
            AccelSensitivity = 4;
            GyroSensitivity = 500;
            break;
        case 6:
            AccelSensitivity = 8;
            GyroSensitivity = 500;
            break;
        case 7:
            AccelSensitivity = 16;
            GyroSensitivity = 500;
            break;
        case 8:
            AccelSensitivity = 2;
            GyroSensitivity = 1000;
            break;
        case 9:
            AccelSensitivity = 4;
            GyroSensitivity = 1000;
            break;
        case 10:
            AccelSensitivity = 8;
            GyroSensitivity = 1000;
            break;
        case 11:
            AccelSensitivity = 16;
            GyroSensitivity = 1000;
            break;
        case 12:
            AccelSensitivity = 2;
            GyroSensitivity = 2000;
            break;
        case 13:
            AccelSensitivity = 4;
            GyroSensitivity = 2000;
            break;
        case 14:
            AccelSensitivity = 8;
            GyroSensitivity = 2000;
            break;
        case 15:
            AccelSensitivity = 16;
            GyroSensitivity = 2000;
            break;
        default:
            P1OUT |= BIT0;
            P4OUT &= ~BIT6;
            while(1){                       //DIP switch position could not be read -> blinking leds
                P1OUT ^= BIT0;
                P4OUT ^= BIT6;
                _delay_cycles(500000);
            }
    }

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
}



//*********************************************************************************************
//*********************************************************************************************
int main(void){

      WDTCTL = WDTPW | WDTHOLD;       // Stop WDT

//--------------------------------------GPIO Config----------------------------------------------------------------------------

      // Prepare LEDs
      P1DIR = 0xFF ^ (BIT1 | BIT3);             // Set all but P1.1, 1.3 to output direction
      P1OUT |= BIT0;                            // LED On
      P1OUT &= ~BIT0;                           //LED1=OFF initially
      P4DIR = 0xFF;                             // P4 output
      P4OUT |= BIT6;                            // P4.6 LED on
      P4OUT &= ~BIT6;                           //LED2=OFF initially

      P4DIR &= ~BIT5;                           //set P4.5 (Switch1) to input
      P4REN |= BIT5;                            //turn on register
      P4OUT |= BIT5;                            //makes resistor a pull up
      P4IES |= BIT5;                            //make sensitive to High-to-Low

      P1DIR &= ~(BIT3+BIT4+BIT5);               //set P1.3 P1.4 P1.5 to input DIPSWITCH
      P3DIR &= ~BIT0;                           //set P3.0 to input DIPSWITCH

      P1SEL1 |= BIT6 + BIT7;                    //for I2C functionality P1SEL1 high,P1SEL0 low
      //P1SEL0|= BIT6 + BIT7;                   //for I2C functionality P1SEL1 high,P1SEL0 low

      // Disable the GPIO power-on default high-impedance mode to activate
      // previously configured port settings
      PM5CTL0 &= ~LOCKLPM5;

      P4IE |= BIT5;               //Enable P4.5 IRQ (Switch1)
      P4IFG &= ~BIT5;             //clear flag



//--------------------------------------CLOCK Config----------------------------------------------------------------------------

/*
      //set clock to 16MHz
      // Configure one FRAM waitstate as required by the device datasheet for MCLK
      // operation beyond 8MHz _before_ configuring the clock system.
      FRCTL0 = FRCTLPW | NWAITS_1;

      // Clock System Setup
      CSCTL0_H = CSKEY_H;                    // Unlock CS registers
      CSCTL1 = DCOFSEL_0;             // Set DCO to 1MHz
      CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK;
      CSCTL3 = DIVA__4 | DIVS__4 | DIVM__4;     // Set all dividers
      CSCTL1 = DCORSEL | DCOFSEL_4;             // Set DCO to 16MHz

      __delay_cycles(60);
      CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;     // Set all dividers
      CSCTL0_H = 0;                             // Lock CS registers


      //set clock to 8MHz
      CSCTL0_H = CSKEY >> 8;                    // Unlock CS registers
      CSCTL1 = DCOFSEL_6;                       // Set DCO to 8MHz
      CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK; // Set ACLK = LFXTCLK; SMCLK = MCLK = DCO
      CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;     // Set all dividers to 1
      CSCTL0_H = 0;                             // Lock CS registers
*/
      //set clock to 8MHz
      CSCTL0_H = CSKEY >> 8;                    // Unlock CS registers
      CSCTL1 = DCOFSEL_6;                       // Set DCO to 8MHz
      CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK; // Set ACLK = LFXTCLK; SMCLK = MCLK = DCO
      CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;     // Set all dividers to 1
      CSCTL4 &= ~LFXTOFF;                       // Turn on LFXT
      CSCTL0_H = 0;                             // Lock CS registers


      // Initialize the I2C state machine
      i2cInit();

      // Enable interrupts
      __bis_SR_register(GIE);

      //set RTC
      //autoTime();
      //DS3234GetCurrentTime();
//--------------------------------------Initialize SD card--------------------------------------------------------------------------------------------


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


//--------------------------------------Initialize ICM20948--------------------------------------------------------------------------------------------

      // Wake up the ICM20948
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
      TX_Data[0] = 0b10000000;                // ICM20948 RESET
      TX_ByteCtr = 2;
      i2cWrite(slaveAddress);

      _delay_cycles(800000);

      // Wake up the ICM20948
      TX_Data[1] = 0x06;                      // address of PWR_MGMT_1 register
      TX_Data[0] = 0b00000001;                //BIT6=0(wake up),BIT5=0(low power disable),BIT[2:0]=001(autoselect best clock)
      TX_ByteCtr = 2;
      i2cWrite(slaveAddress);

      _delay_cycles(80000);

      TX_Data[1] = 0x05;                      // address of LP_CONFIG register
      TX_Data[0] = 0b01000000;                // set ACCEL/GYRO/I2CMST to continuous
      TX_ByteCtr = 2;
      i2cWrite(slaveAddress);

      //check switch setting for accel+gyro modes
      checkDIPswitch();

      TX_Data[1] = 0x7F;                      // address of BANK SEL register
      TX_Data[0] = 0b00100000;                // Select BANK 2
      TX_ByteCtr = 2;
      i2cWrite(slaveAddress);

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

//----------------------------------magnetometer config-----------------------------------------------------------------------
      TX_Data[1] = 0x7F;                      // address of BANK SEL register
      TX_Data[0] = 0b00000000;                // Select BANK 0
      TX_ByteCtr = 2;
      i2cWrite(slaveAddress);

      TX_Data[0] = 0x0F;                        // address of INT_PIN_CFG register
      TX_ByteCtr = 1;
      i2cWrite(slaveAddress);

      RX_ByteCtr = 2;
      i2cRead(slaveAddress);
      register_value = RX_Data[1];
      register_value &= ~(1<<1);              // clear BYPASS_EN BIT to deactivate I2C passthrough mode

      TX_Data[1] = 0x0F;                      // address of INT_PIN_CFG register
      TX_Data[0] = register_value;
      TX_ByteCtr = 2;
      i2cWrite(slaveAddress);


      TX_Data[1] = 0x7F;                      // address of BANK SEL register
      TX_Data[0] = 0b00110000;                // Select BANK 3
      TX_ByteCtr = 2;
      i2cWrite(slaveAddress);

      TX_Data[0] = 0x01;                        // address of I2C_MST_CTRL register
      TX_ByteCtr = 1;
      i2cWrite(slaveAddress);

      RX_ByteCtr = 2;
      i2cRead(slaveAddress);
      register_value = RX_Data[1];
      register_value &= ~(0x0F);                 //clear bits for master clock [3:0]
      register_value |= (0x07);                  //set bits for master clock [3:0], 0x07 corresponds to 345.6 kHz, good for up to 400 kHz
      register_value |= (1<<4);                  //set bit [4] for NSR (next slave read). 0 = restart between reads. 1 = stop between reads.

      TX_Data[1] = 0x01;                      // address of I2C_MST_CTRL register
      TX_Data[0] = register_value;
      TX_ByteCtr = 2;
      i2cWrite(slaveAddress);


      TX_Data[1] = 0x7F;                      // address of BANK SEL register
      TX_Data[0] = 0b00000000;                // Select BANK 0
      TX_ByteCtr = 2;
      i2cWrite(slaveAddress);

      TX_Data[0] = 0x03;                        // address of USER_CTRL register
      TX_ByteCtr = 1;
      i2cWrite(slaveAddress);

      RX_ByteCtr = 2;
      i2cRead(slaveAddress);
      register_value = RX_Data[1];
      register_value |= (1<<5);                // set BIT[5] to enable I2C master

      TX_Data[1] = 0x03;                      // address of USER_CTRL register
      TX_Data[0] = register_value;
      TX_ByteCtr = 2;
      i2cWrite(slaveAddress);

      //Configure magnetometer as I2C Slave
      //for readMag: addr=0x80 | address
      //for writeMag:addr=0x00 | address
      for(a=0;a<5;a++){
          TX_Data[1] = 0x7F;                      // address of BANK SEL register
          TX_Data[0] = 0b00110000;                // Select BANK 3
          TX_ByteCtr = 2;
          i2cWrite(slaveAddress);

          TX_Data[1] = 0x13;                      // address of I2C_SLV4_ADDR register
          TX_Data[0] = 0b10001100;                // BIT[6:0] to I2C slave address 0x0C; BIT[7] for RNW
          TX_ByteCtr = 2;
          i2cWrite(slaveAddress);

          TX_Data[1] = 0x14;                      // address of I2C_SLV4_REG register
          TX_Data[0] = 0b00000000;                // BIT[7:0] to register address 0x00 WIA1
          TX_ByteCtr = 2;
          i2cWrite(slaveAddress);

          TX_Data[1] = 0x15;                      // address of I2C_SLV4_CTRL register
          TX_Data[0] = 0b10000000;                // EN bit enable, rest disabled
          TX_ByteCtr = 2;
          i2cWrite(slaveAddress);


          TX_Data[1] = 0x7F;                      // address of BANK SEL register
          TX_Data[0] = 0b00000000;                // Select BANK 0
          TX_ByteCtr = 2;
          i2cWrite(slaveAddress);

          slave4done = 0;
          while(slave4done == 0){
              TX_Data[0] = 0x17;                        // address of I2C_MST_STATUS register
              TX_ByteCtr = 1;
              i2cWrite(slaveAddress);

              RX_ByteCtr = 2;
              i2cRead(slaveAddress);
              register_value = RX_Data[1];

              if(register_value & (1<<6)){
                  slave4done = 2;
              }
              else{
                 count++;
                 if(count == 1000){
                     slave4done = 1;
                     count = 0;
                 }
              }
          }

          TX_Data[1] = 0x7F;                      // address of BANK SEL register
          TX_Data[0] = 0b00110000;                // Select BANK 3
          TX_ByteCtr = 2;
          i2cWrite(slaveAddress);

          TX_Data[0] = 0x17;                        // address of I2C_SLV4_DI register
          TX_ByteCtr = 1;
          i2cWrite(slaveAddress);

          RX_ByteCtr = 2;
          i2cRead(slaveAddress);            //MagWhoIAm1: register_value== 48h?
          whoami = RX_Data[1];              //MagWhoIAm2: register_value== 09h?
          if(whoami != 0){
              a = a + 6;                    //Mag seems to work --> exit reset loop
                                            //+6 to debug loop counter
          }


          //reset I2C Master
          TX_Data[1] = 0x7F;                      // address of BANK SEL register
          TX_Data[0] = 0b00000000;                // Select BANK 0
          TX_ByteCtr = 2;
          i2cWrite(slaveAddress);

          TX_Data[0] = 0x03;                        // address of USER_CTRL register
          TX_ByteCtr = 1;
          i2cWrite(slaveAddress);

          RX_ByteCtr = 2;
          i2cRead(slaveAddress);
          register_value = RX_Data[1];
          register_value |= (1<<1);

          TX_Data[1] = 0x03;                      // address of USER_CTRL register
          TX_Data[0] = register_value;
          TX_ByteCtr = 2;
          i2cWrite(slaveAddress);
          _delay_cycles(50000);

          TX_Data[0] = 0x03;                        // address of USER_CTRL register
          TX_ByteCtr = 1;
          i2cWrite(slaveAddress);

          RX_ByteCtr = 2;
          i2cRead(slaveAddress);
          register_value = RX_Data[1];
          register_value |= (1<<5);                // set BIT[5] to enable I2C master

          TX_Data[1] = 0x03;                      // address of USER_CTRL register
          TX_Data[0] = register_value;
          TX_ByteCtr = 2;
          i2cWrite(slaveAddress);
      }

      if(a == 5 && whoami == 0){
          while(1){
              P4OUT ^= BIT6;
              _delay_cycles(500000);
          }
      }


      //switch Mag to Mode4: 100Hz continuous measurement
        TX_Data[1] = 0x7F;                      // address of BANK SEL register
        TX_Data[0] = 0b00110000;                // Select BANK 3
        TX_ByteCtr = 2;
        i2cWrite(slaveAddress);

        TX_Data[1] = 0x13;                      // address of I2C_SLV4_ADDR register
        TX_Data[0] = 0b00001100;                // BIT[6:0] to I2C slave address 0x0C; BIT[7] for RNW
        TX_ByteCtr = 2;
        i2cWrite(slaveAddress);

        TX_Data[1] = 0x14;                      // address of I2C_SLV4_REG register
        TX_Data[0] = 0b00110001;                // BIT[7:0] to register address 0x31 -->AK09916_REG_CNTL2
        TX_ByteCtr = 2;
        i2cWrite(slaveAddress);

        TX_Data[1] = 0x16;                      // address of I2C_SLV4_DO register
        TX_Data[0] = 0b00001000;                // BIT[7:0] to mode 4: 100Hz continuous measurement
        TX_ByteCtr = 2;
        i2cWrite(slaveAddress);

        TX_Data[1] = 0x15;                      // address of I2C_SLV4_CTRL register
        TX_Data[0] = 0b10000000;                // EN bit enable, rest disabled
        TX_ByteCtr = 2;
        i2cWrite(slaveAddress);


        TX_Data[1] = 0x7F;                      // address of BANK SEL register
        TX_Data[0] = 0b00000000;                // Select BANK 0
        TX_ByteCtr = 2;
        i2cWrite(slaveAddress);

        slave4done = 0;
        while(slave4done == 0){
            TX_Data[0] = 0x17;                        // address of I2C_MST_STATUS register
            TX_ByteCtr = 1;
            i2cWrite(slaveAddress);

            RX_ByteCtr = 2;
            i2cRead(slaveAddress);
            register_value = RX_Data[1];

            if(register_value & (1<<6)){
                slave4done = 2;
            }
            else{
               count++;
               if(count == 1000){
                   slave4done = 1;
                   count = 0;
               }
            }
        }

        //configure Mag as slave0
        TX_Data[1] = 0x7F;                      // address of BANK SEL register
        TX_Data[0] = 0b00110000;                // Select BANK 3
        TX_ByteCtr = 2;
        i2cWrite(slaveAddress);

        TX_Data[1] = 0x03;                      // address of I2C_SLV0_ADDR register
        TX_Data[0] = 0b10001100;                // BIT[6:0] to I2C slave address 0x0C; BIT[7] for RNW(read mode)
        TX_ByteCtr = 2;
        i2cWrite(slaveAddress);

        TX_Data[1] = 0x04;                      // address of I2C_SLV0_REG register
        TX_Data[0] = 0b00010000;                // BIT[7:0] to register address 0x10 -->AK09916_REG_ST1
        TX_ByteCtr = 2;
        i2cWrite(slaveAddress);

        TX_Data[1] = 0x05;                      // address of I2C_SLV0_CTRL register
        TX_Data[0] = 0b10001001;                // BITS[3:0] for number of transmitted bytes(9),BIT[7] enable reading
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
              //measurement init phase = check DIP switch position, get current time, creating file, opening file

                  //check switch setting for accel+gyro modes
                  checkDIPswitch();

                  TX_Data[1] = 0x7F;                      // address of BANK SEL register
                  TX_Data[0] = 0b00100000;                // Select BANK 2
                  TX_ByteCtr = 2;
                  i2cWrite(slaveAddress);

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

                  DS3234GetCurrentTime();

                  char filename[] = "RAW_00.CSV";
                  FILINFO fno;
                  FRESULT fr;
                  uint8_t i;
                  for(i = 0; i < 100; i++){
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
                  f_printf(&logfile, "%s,%d,%s,%d,%s,%d,%s,%d,%s,%d,%s,%d,%s,%d\n","weekday: ",TimeArray[3],"//date: ",TimeArray[4],".",TimeArray[5],".",TimeArray[6],"//time: ",TimeArray[2],":",TimeArray[1],":",TimeArray[0]);
                  f_printf(&logfile, "%d,%s,%d,%s\n",AccelSensitivity,"g",GyroSensitivity,"dps");
                  f_printf(&logfile, "%s,%s,%s,%s,%s,%s,%s,%s,%s\n", "xAccel","yAccel","zAccel","xGyro","yGyro","zGyro","xMag","yMag","zMag");

                  //P1OUT |= BIT0;                  //LED2 on
                  measurementInit++;
              }
              else{
              //writing data to file

                  // Point to the ACCEL_XOUT_H register in the ICM20948
                  TX_Data[0] = 0x2D;                        // register address
                  TX_ByteCtr = 1;
                  i2cWrite(slaveAddress);

                  RX_ByteCtr = 23;
                  i2cRead(slaveAddress);

                  __disable_interrupt();                    // prevent getting new sensor data while saving values

                  xAccel  = RX_Data[22] << 8;               // MSB
                  xAccel |= RX_Data[21];                    // LSB
                  yAccel  = RX_Data[20] << 8;
                  yAccel |= RX_Data[19];
                  zAccel  = RX_Data[18] << 8;
                  zAccel |= RX_Data[17];
                  xGyro  = RX_Data[16] << 8;
                  xGyro |= RX_Data[15];
                  yGyro  = RX_Data[14] << 8;
                  yGyro |= RX_Data[13];
                  zGyro  = RX_Data[12] << 8;
                  zGyro |= RX_Data[11];
                  Temp  = RX_Data[10] << 8;
                  Temp |= RX_Data[9];
                  magstat1 = RX_Data[8];
                  magstat2 = RX_Data[0];
                  if(!(magstat2 & (1<<3))){
                  xMag = RX_Data[7];               // magnetometer puts data out in little endian
                  xMag  |= RX_Data[6] << 8;
                  yMag = RX_Data[5];
                  yMag  |= RX_Data[4] << 8;
                  zMag = RX_Data[3];
                  zMag  |= RX_Data[2] << 8;
                  }

                  f_printf(&logfile, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n", xAccel,yAccel,zAccel,xGyro,yGyro,zGyro,xMag,yMag,zMag);

                  //backup every 3000 data writes --> every ~10s
                  backupCtr++;
                  if(backupCtr%250 == 0){
                      P1OUT ^= BIT0;
                  }
                  if(backupCtr == 5000){
                      f_sync(&logfile);
                      backupCtr = 0;
                  }

                  __enable_interrupt();

              }
          }

          else{
              mode = 1;                                        //something went wrong --> switch to standby mode
          }



      }
}

/**********************************************************************************************/
// Sensor I2C ISR
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

/******************************************************************************/
//RTC SPI ISR
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_A1_VECTOR))) USCI_A1_ISR (void)
#else
#error Compiler not supported!
#endif
{
    uint8_t uca1_rx_val = 0;
    switch(__even_in_range(UCA1IV, USCI_SPI_UCTXIFG))
    {
        case USCI_NONE: break;
        case USCI_SPI_UCRXIFG:
            uca1_rx_val = UCA1RXBUF;
            UCA1IFG &= ~UCRXIFG;
            switch (MasterMode)
            {
                case TX_REG_ADDRESS_MODE:
                    if (RXByteCtr)
                    {
                        MasterMode = RX_DATA_MODE;   // Need to start receiving now
                        //Send Dummy To Start
                        __delay_cycles(2000);
                        SendUCA1Data(DUMMY);
                    }
                    else
                    {
                        MasterMode = TX_DATA_MODE;        // Continue to transmision with the data in Transmit Buffer
                        //Send First
                        SendUCA1Data(TransmitBuffer[TransmitIndex++]);
                        TXByteCtr--;
                    }
                    break;

                case TX_DATA_MODE:
                    if (TXByteCtr)
                    {
                      SendUCA1Data(TransmitBuffer[TransmitIndex++]);
                      TXByteCtr--;
                    }
                    else
                    {
                      //Done with transmission
                      MasterMode = IDLE_MODE;
                      __bic_SR_register_on_exit(CPUOFF);      // Exit LPM0
                    }
                    break;

                case RX_DATA_MODE:
                    if (RXByteCtr)
                    {
                        ReceiveBuffer[ReceiveIndex++] = uca1_rx_val;
                        //Transmit a dummy
                        RXByteCtr--;
                    }
                    if (RXByteCtr == 0)
                    {
                        MasterMode = IDLE_MODE;
                        __bic_SR_register_on_exit(CPUOFF);      // Exit LPM0
                    }
                    else
                    {
                        SendUCA1Data(DUMMY);
                    }
                    break;

                default:
                    __no_operation();
                    break;
            }
            __delay_cycles(1000);
            break;
        case USCI_SPI_UCTXIFG:
            break;
        default: break;
    }
}
/**********************************************************************************************/

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
    _delay_cycles(800000);
    _delay_cycles(800000);
    _delay_cycles(800000);
    _delay_cycles(800000);

}
