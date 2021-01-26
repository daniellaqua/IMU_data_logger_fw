#include <msp430.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
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
uint8_t testdata[] = {7,7,7,7,7,7,7};

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
unsigned int measurementInit = 0; //check if new file has to be created and opened
bool RTCnewer = false;

int16_t comp_year = 7;
int16_t comp_month = 7;
int16_t comp_date = 7;
int16_t comp_hour = 7;
int16_t comp_minute = 7;
int16_t comp_second = 7;
char testdate[10];
char testtime[10];

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
//compareTimes -- subtract compiler time from RTC register time
//if compiler time is newer --> update RTC registers
//TimeArray[Sec,Min,Hou,Day,Dat,Mon,Yea]
void compareTimes(){

    _time[TIME_SECONDS] = BUILD_SECOND;
    _time[TIME_MINUTES] = BUILD_MINUTE;
    _time[TIME_HOURS] = BUILD_HOUR;
    _time[TIME_MONTH] = BUILD_MONTH;
    _time[TIME_DATE] = BUILD_DATE;
    _time[TIME_YEAR] = BUILD_YEAR - 2000;

    comp_year = TimeArray[6] - _time[TIME_YEAR];
    comp_month = TimeArray[5] - _time[TIME_MONTH];
    comp_date = TimeArray[4] - _time[TIME_DATE];
    comp_hour = TimeArray[2] - _time[TIME_HOURS];
    comp_minute = TimeArray[1] - _time[TIME_MINUTES];
    comp_second = TimeArray[0] - _time[TIME_SECONDS];

/*
    int year = TimeArray[6] - (BUILD_YEAR - 2000);
    int month = TimeArray[5] - BUILD_MONTH;
    int date = TimeArray[4] - BUILD_DATE;
    int hour = TimeArray[2] - BUILD_HOUR;
    int minute = TimeArray[1] - BUILD_MINUTE;
    int second = TimeArray[0] - BUILD_SECOND;
*/

    if(comp_year < 0){RTCnewer = false;}
    else if(comp_year > 0){RTCnewer = true;}
    else{
        if(comp_month < 0){RTCnewer = false;}
        else if(comp_month > 0){RTCnewer = true;}
        else{
            if(comp_date < 0){RTCnewer = false;}
            else if(comp_date > 0){RTCnewer = true;}
            else{
                if(comp_hour < 0){RTCnewer = false;}
                else if(comp_hour > 0){RTCnewer = true;}
                else{
                    if(comp_minute < 0){RTCnewer = false;}
                    else if(comp_minute > 0){RTCnewer = true;}
                    else{
                        if(comp_second < 0){RTCnewer = false;}
                        else{RTCnewer = true;}
                    }
                }
            }
        }
    }
}
//*********************************************************************************************

//*********************************************************************************************
// autoTime -- Fill DS3234 time registers with compiler time/date
void autoTime()
{
    if(RTCnewer == false){
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

      // Enable interrupts
      __bis_SR_register(GIE);


      sprintf(testdate,__DATE__);
      sprintf(testtime,__TIME__);

      //compare RTC register time to compiler time and use newer time
      DS3234GetCurrentTime();
      compareTimes();
      autoTime();
      DS3234GetCurrentTime();
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

      mode = 1;                                 //start with standby mode after init

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
                  f_printf(&logfile,"%s", "Ich bin ein funktionstuechtiger Prototyp\n");
                  f_printf(&logfile, "%s,%d,%s,%d,%s,%d,%s,%d,%s,%d,%s,%d,%s,%d\n","weekday: ",TimeArray[3],"//date: ",TimeArray[4],".",TimeArray[5],".",TimeArray[6],"//time: ",TimeArray[2],":",TimeArray[1],":",TimeArray[0]);
                  f_close(&logfile);          // Close the file
                  mode = 1;

              }
          else{
              mode = 1;                                        //something went wrong --> switch to standby mode
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
