//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"

#include "user_interface.h"

#include "code.h"
#include "siren.h"
#include "smart_home_system.h"
#include "fire_alarm.h"
#include "date_and_time.h"
#include "temperature_sensor.h"
#include "gas_sensor.h"
#include "matrix_keypad.h"
#include "display.h"

//=====[Declaration of private defines]========================================

#define DISPLAY_REFRESH_TIME_MS 1000

//=====[Declaration of private data types]=====================================

//=====[Declaration and initialization of public global objects]===============

DigitalOut incorrectCodeLed(LED3);
DigitalOut systemBlockedLed(LED2);

//=====[Declaration of external public global variables]=======================

//=====[Declaration and initialization of public global variables]=============

char codeSequenceFromUserInterface[CODE_NUMBER_OF_KEYS];

//=====[Declaration and initialization of private global variables]============

static bool incorrectCodeState = OFF;
static bool systemBlockedState = OFF;

static bool codeComplete = false;
static int numberOfCodeChars = 0;

//my scummy variables 
bool unlockedinterface = false; 
char codeinputinterface[5]; 
char unlockcodeinterface[5] = {'1', '5', '8', '0', '6'}; 
int howmanykeyspressedinterface = 0; 


//=====[Declarations (prototypes) of private functions]========================

static void userInterfaceMatrixKeypadUpdate(char keyReleased);
static void incorrectCodeIndicatorUpdate();
static void systemBlockedIndicatorUpdate();

static void userInterfaceDisplayInit();
static void userInterfaceDisplayUpdate(char pressedkey);

void fireAlarmDeactivate();

//=====[Implementations of public functions]===================================

void userInterfaceInit()
{
    incorrectCodeLed = OFF;
    systemBlockedLed = OFF;
    matrixKeypadInit( SYSTEM_TIME_INCREMENT_MS );
    userInterfaceDisplayInit();
}

void userInterfaceUpdate()
{
    char keypadselection = matrixKeypadUpdate();
    userInterfaceMatrixKeypadUpdate(keypadselection); //hands the pressed key to this function
    incorrectCodeIndicatorUpdate();
    systemBlockedIndicatorUpdate();
    userInterfaceDisplayUpdate(keypadselection); //hands the pressed key to this function
}

bool incorrectCodeStateRead()
{
    return incorrectCodeState;
}

void incorrectCodeStateWrite( bool state )
{
    incorrectCodeState = state;
}

bool systemBlockedStateRead()
{
    return systemBlockedState;
}

void systemBlockedStateWrite( bool state )
{
    systemBlockedState = state;
}

bool userInterfaceCodeCompleteRead()
{
    return codeComplete;
}

void userInterfaceCodeCompleteWrite( bool state )
{
    codeComplete = state;
}

//=====[Implementations of private functions]==================================

static void userInterfaceMatrixKeypadUpdate(char keyReleased) //variable keypadselection is mapped to keyReleased
{
    static int numberOfHashKeyReleased = 0;
    
    if( keyReleased != '\0' ) {

        if( sirenStateRead() && !systemBlockedStateRead() ) {
            if( !incorrectCodeStateRead() ) {
                codeSequenceFromUserInterface[numberOfCodeChars] = keyReleased;
                numberOfCodeChars++;
                if ( numberOfCodeChars >= CODE_NUMBER_OF_KEYS ) {
                    codeComplete = true;
                    numberOfCodeChars = 0;
                }
            } else {
                if( keyReleased == '#' ) {
                    numberOfHashKeyReleased++;
                    if( numberOfHashKeyReleased >= 2 ) {
                        numberOfHashKeyReleased = 0;
                        numberOfCodeChars = 0;
                        codeComplete = false;
                        incorrectCodeState = OFF;
                    }
                }
            }
        }
    }
}

static void userInterfaceDisplayInit()
{
    displayInit( DISPLAY_CONNECTION_I2C_PCF8574_IO_EXPANDER );
     
    displayCharPositionWrite ( 0,0 );
    displayStringWrite( "Temperature:" );

    displayCharPositionWrite ( 0,1 );
    displayStringWrite( "Gas:" );
    
    displayCharPositionWrite ( 0,2 );
    displayStringWrite( "Alarm:" );
}

static void userInterfaceDisplayUpdate(char pressedkey)
{
    static int accumulatedDisplayTime = 0;
    char temperatureString[5] = ""; 

    if (pressedkey != '\0'){
switch (pressedkey){
    
    case '4':  
        printf ("Key 4 Pressed on Keypad");//debugging line for evidence capture
        userInterfaceDisplayInit(); //resets the screen

        displayCharPositionWrite ( 4,1 ); 

        if ( gasDetectorStateRead() ) {
            displayStringWrite( "Detected    " );
        } else {
            displayStringWrite( "Not Detected" );
        }
        break; 

    case '5': 
        printf("Key 5 Pressed on Keypad"); //debugging line for evidence capture
        userInterfaceDisplayInit(); //resets the screen

        sprintf(temperatureString, "%d", (int)temperatureSensorReadCelsius());
        displayCharPositionWrite ( 12,0 );
        displayStringWrite( temperatureString );
        displayCharPositionWrite ( 14,0 );
        displayStringWrite( "'C" );
        break; 
    
    case '#': 
    {
    printf("Key # Pressed on Keypad"); 
    char keyReleased = matrixKeypadUpdate(); 
    displayCharPositionWrite( 0,0 ); 
    displayStringWrite( "    ENTER CODE      " );
    displayCharPositionWrite( 0,1 ); 
    displayStringWrite( "  TO RESET SYSTEM  " );
    displayCharPositionWrite( 0,2 ); 
    displayStringWrite( "                    " ); 
    displayCharPositionWrite( 0,3 ); 
    displayStringWrite( "                    " ); 

    while (!unlockedinterface) { 
        char keypad = matrixKeypadUpdate(); 
        
        if (keypad != '\0'){ 
            codeinputinterface[howmanykeyspressedinterface] = keypad;
            
            displayCharPositionWrite( 7 + howmanykeyspressedinterface, 2 );
            displayStringWrite( "*" );
            
            howmanykeyspressedinterface++; 

            if (howmanykeyspressedinterface == 5){ 
                unlockedinterface = true;

                for (int i = 0; i < 5; i++){ 
                    if (codeinputinterface[i] != unlockcodeinterface[i]){
                        unlockedinterface = false;
                       
                    }
                }
                
                if (!unlockedinterface){
                    howmanykeyspressedinterface = 0; 
                  
                    displayCharPositionWrite( 0, 2 ); 
                    displayStringWrite( "                    " );
                }
            }
        }
        delay(10); 
    }
    fireAlarmDeactivate();
    
    displayCharPositionWrite( 0,0 ); displayStringWrite( "                    " );
    displayCharPositionWrite( 0,1 ); displayStringWrite( "                    " );
    displayCharPositionWrite( 0,2 ); displayStringWrite( "                    " );

    displayCharPositionWrite( 0,0 ); 
    displayStringWrite( "Temperature: " );
    displayCharPositionWrite( 0,1 ); 
    displayStringWrite( "Gas: " );
    displayCharPositionWrite( 0,2 ); 
    displayStringWrite( "Alarm: " );
    break; 
    }
    default:
    break;
  }
}
     
if( accumulatedDisplayTime >=
        DISPLAY_REFRESH_TIME_MS ) {
        accumulatedDisplayTime = 0;

    if (overTemperatureDetectedRead() && gasDetectedRead() && sirenStateRead()){
        printf("All alarms active\r\n");
        displayCharPositionWrite( 0, 0 );
        displayStringWrite( "     EVACUATE!       " ); 
        displayCharPositionWrite( 0, 1 );
        displayStringWrite( " DEATH IS IMMINENT!   " );
        displayCharPositionWrite( 0, 2 );
        displayStringWrite( "                     " );
        displayCharPositionWrite( 0, 3 );
        displayStringWrite( "PRESS # TO RESET" );
    }

    else if (overTemperatureDetectedRead() && sirenStateRead() ){
            printf("Temp alarm active\r\n");
            displayCharPositionWrite ( 0, 0);
            displayStringWrite("      WARNING!  ");

            displayCharPositionWrite ( 0, 1);
            displayStringWrite( "  MAX TEMP EXCEEDED"); 
            printf("overtemp detector triggered\r\n");

            displayCharPositionWrite ( 0,2); 
            displayStringWrite("           ");
            displayCharPositionWrite( 0, 3 );
            displayStringWrite( "PRESS # TO DEACTIVATE" );
 }
    else if (gasDetectedRead() && sirenStateRead() ){
            printf("Gas alarm active\r\n");
            displayCharPositionWrite ( 0, 0);
            displayStringWrite("      WARNING!  ");

            displayCharPositionWrite ( 0, 1);
            displayStringWrite( "  GAS LEAK DETECTED"); 
            printf("gas alarm triggered\r\n");

            displayCharPositionWrite ( 0,2); 
            displayStringWrite("           ");
            displayCharPositionWrite( 0, 3 );
            displayStringWrite( "PRESS # TO DEACTIVATE" );
 }

       else if (sirenStateRead()){
                printf("Siren alarm active\r\n");
                displayCharPositionWrite ( 7,2 );
                displayStringWrite( "WARNING!");
                printf("WARNING! ALARM ACTIVATED\r\n");
                }
        else {
	            displayCharPositionWrite ( 7,2 );
                printf("No alarms active\r\n");
                displayStringWrite( "SAFE");
     
        }
   } else {
        accumulatedDisplayTime =
            accumulatedDisplayTime + SYSTEM_TIME_INCREMENT_MS;

}
}

static void incorrectCodeIndicatorUpdate()
{
    incorrectCodeLed = incorrectCodeStateRead();
}

static void systemBlockedIndicatorUpdate()
{
    systemBlockedLed = systemBlockedState;
}

