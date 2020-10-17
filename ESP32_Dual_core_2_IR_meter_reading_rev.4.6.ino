#include <ArduinoJson.h>
#include "SPIFFS.h"
#include <SoftwareSerial.h>
#include <EEPROM.h>
//#include <MsTimer2.h>
#include <Wire.h>//Set Buffer size to Dynamic
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
char aux_str[100];
int lengthOfJSON;
char thingsboard_url[] = "demo.thingsboard.io";
char port[] = "80";                    // PORT Connected on
String getStr = "";
String AccessToken = "#Acess token";   //Add the access token for the device which get it from thingsboard platform
unsigned long delay_hour;
unsigned long delay_mini;
unsigned long delaytime = 0;
String stringOne;
String stringTwo;
unsigned long time_1 = 0;
unsigned long time_2 = 0;
unsigned long time_3 = 0;
#define EEPROM_SIZE 4000
int isStickerPin = 34;///15;  // This is our input pin, Pin 34 ESP32
int isSticker ;  // HIGH MEANS NO OBSTACLE
byte Power = HIGH;
int count = 0;
int m = 0;
unsigned long unit = 0;
unsigned long temp = 0;
int tempcount = 0;
byte unitaddress;
byte countaddress;
byte firstrun;                // 1 means worked before   0 means needs to configure
// for just test the spread sheet
int l = 0;
bool  flag = false ;  // to see if first running card
char rx_byte = 0;
String rx_str = "";
boolean not_number = false;
unsigned long result = 0;
int countresult = 0;
boolean Sendflag = true;
int year;
int month;
int day;
int hour;
int mini;
bool Timeget = false; // to ensure time calcualted is finished before enter sleep. true means time calculated , false no yet
bool GPRSflag = false;  // to check if GPRS connected or not   false not connected,   true = connected
bool Sendingsuccess = true;  // to chech sending data success or not by default is considered true to send data at
bool minflag;
bool hourflag;
char buffer[100];
byte pos = 0;
int contentLength = 0;
DynamicJsonBuffer jsonBuffer;
JsonObject& root = jsonBuffer.createObject();                 //Create an object 'root' which is called later to print JSON Buffer
SoftwareSerial GSM(17, 16); // TX, RX
xQueueHandle xQueue;
TaskHandle_t xTask1;
TaskHandle_t xTask2;
TaskHandle_t xTask3;


//////////////////////////////////

enum _parseState {
  PS_DETECT_MSG_TYPE,

  PS_IGNORING_COMMAND_ECHO,

  PS_HTTPACTION_TYPE,
  PS_HTTPACTION_RESULT,
  PS_HTTPACTION_LENGTH,

  PS_HTTPREAD_LENGTH,
  PS_CREG_TYPE,
  PS_HTTPREAD_CONTENT,
  PS_CREG_CONTENT

};

byte parseState = PS_DETECT_MSG_TYPE;

void resetBuffer() {
  memset(buffer, 0, sizeof(buffer));
  pos = 0;
}

void sendGSM(String msg, int waitMs = 500) {
  GSM.println(msg);
  delay(waitMs);
  while (GSM.available()) {
    parseATText(GSM.read());
  }
}

void parseATText(byte b) {

  // Serial.print ( " the b is = ");
  //Serial.println(buffer[b]);

  // {Serial.println("connected");}
  buffer[pos++] = b;

  if ( pos >= sizeof(buffer) )
    resetBuffer(); // just to be safe

  switch (parseState) {
    case PS_DETECT_MSG_TYPE:
      {
        if ( b == '\n' )
          resetBuffer();                                            // new line so reset buffer  case 1  layth
        else {
          if ( pos == 3 && strcmp(buffer, "AT+") == 0 ) {
            parseState = PS_IGNORING_COMMAND_ECHO;                  // case 2 if AT command  PS_IGNORING_COMMAND_ECHO
          }
          else if ( b == ':' ) {                                    // case 3    if  :
            //  Serial.print("Checking message type: ");
            String str((char*)buffer);
            //   Serial.println(str);
            //////////////////////////////
            //  Serial.println(str);
            //   Serial.print( "The string length of checking PRS IS =  ");
            //   Serial.println( str.length());
            //////////////////////////////////
            /////////////////////////////////
            if (( str.indexOf(",") == 2) && (str.length() == 17))
            {
              str = str.substring(str.indexOf(",") + 1, str.length());
              Serial.print(" first substring =  ");
              Serial.println (str);
              String dt = str.substring(0, str.indexOf(","));
              Serial.print(" dt ************** =  ");
              Serial.println (dt);
              Serial.print(" The lenght of dt is = ");
              Serial.println(dt.length());
              String tm = str.substring(str.indexOf(",") + 1, str.length()) ;
              Serial.print(" tm *****************=  ");
              Serial.println (tm);
              year = dt.substring(0, 4).toInt();
              month = dt.substring(5, 7).toInt();
              day = dt.substring(8, 10).toInt(); // 8-10
              hour = tm.substring(0, 2).toInt();  // 0-2
              hourflag = true;

            }
            else if (( str.indexOf(",") == -1 ) && (str.length() == 3))
            {
              str = str.substring(str.indexOf(",") + 1, str.length());
              //String dt = str.substring(0, str.indexOf(","));
              String tm = str.substring(str.indexOf(",") + 1, str.length()) ;

              mini = tm.substring(0, 2).toInt();
              Serial.print( "the year = ");
              Serial.println ( year);
              Serial.print( "the Month = ");
              Serial.println ( month);
              Serial.print( "the day = ");
              Serial.println ( day);
              Serial.print( "the houer = ");
              Serial.println ( hour + 3 );
              Serial.print( "the min = ");
              Serial.println ( mini );
              minflag = true;



            }

            ///////////////////////////////
            if ( ( hourflag == true) && ( minflag == true))
            {
              Serial.println( " Timeget=true");
              Timeget = true;  ////  point false to Timeget flag after send seccess to forct the sendmeter  taske to get time for another period
            }
            else
            {
              Serial.println( " Timeget=false");
              Timeget = false;
            }
            //////////////////////////////

            if ( strcmp(buffer, "+HTTPACTION:") == 0 ) {
              Serial.println("Received HTTPACTION");
              parseState = PS_HTTPACTION_TYPE;                      //  after received : and + HTTPACTION  then go to PS_HTTPACTION_TYPE
            }


            else if (strcmp(buffer, "+CIPGSMLOC:") == 0 ) {         //  after received : and + CIPGSMLOC  then go to PS_HTTPREAD_TYPE  THEN GO TO LENGTH
              Serial.println("Received CIPGSMLOC");
              Serial.println(buffer);
            }

            else if ( strcmp(buffer, "+HTTPREAD:") == 0 ) {
              Serial.println("Received HTTPREAD");
              parseState = PS_HTTPREAD_LENGTH;
            }
            ////////////////////////////////////
            else if ( strcmp(buffer, "+CREG:") == 0 ) {
              Serial.println("Received CREG");
              Serial.println(buffer);
              parseState = PS_CREG_TYPE;
            }

            ////////////////////////////
            resetBuffer();
          }
        }
      }
      break;

    case PS_IGNORING_COMMAND_ECHO:
      {
        if ( b == '\n' ) {
          Serial.print("Ignoring echo: ");
          Serial.println(buffer);
          parseState = PS_DETECT_MSG_TYPE;
          resetBuffer();
        }
      }
      break;

    case PS_HTTPACTION_TYPE:
      {
        if ( b == ',' ) {
          Serial.print("HTTPACTION type is ");
          Serial.println(buffer);
          parseState = PS_HTTPACTION_RESULT;
          resetBuffer();
        }
      }
      break;

    case PS_HTTPACTION_RESULT:
      {
        if ( b == ',' ) {
          Serial.print("HTTPACTION result is ");
          Serial.println(buffer);
          if ( (buffer[4] = 50) && (buffer[5] = 48) && (buffer[6] = 48)) {
            // Sendflag = true;
            //  Serial.write(buffer);
            Serial.println(" Sending successed" );

          }

          else {
            Sendflag = false;
          }

          parseState = PS_HTTPACTION_LENGTH;
          resetBuffer();
        }
      }
      break;

    case PS_HTTPACTION_LENGTH:
      {
        if ( b == '\n' ) {
          Serial.print("HTTPACTION length is ");
          Serial.println(buffer);
          parseState = PS_DETECT_MSG_TYPE;   // layth made disable
          resetBuffer();
        }
      }
      break;

      if ( b == '\n' ) {
        contentLength = atoi(buffer);
        Serial.print("HTTPREAD length is ");
        Serial.println(contentLength);

        Serial.print("HTTPREAD content: ");

        parseState = PS_HTTPREAD_CONTENT;
        resetBuffer();
      }

      break;
    case PS_HTTPREAD_CONTENT:
      {
        // for this demo I'm just showing the content bytes in the serial monitor
        //  Serial.write(b);
        contentLength--;

        if ( contentLength <= 0 ) {

          // all content bytes have now been read

          parseState = PS_DETECT_MSG_TYPE;
          resetBuffer();
        }
      }

      break;

    ///////////////////////////////
    case PS_CREG_TYPE:
      {
        if ( b == ',' ) {
          Serial.print("CREG type is ");
          Serial.println(buffer);
          parseState = PS_CREG_CONTENT;
          resetBuffer();
        }
      }
      break;

    case PS_CREG_CONTENT:
      {
        if ( b == '1' ) {
          Serial.println(" GPRS CONNECTED" );
          GPRSflag = true;
        }
        else if ( b == '0' ) {
          Serial.println(" GPRS NOT  CONNECTED" );
          GPRSflag = false;
        }
        else {
          Serial.println( " UNKOWN");
          GPRSflag = false;
        }
        parseState = PS_DETECT_MSG_TYPE;
        resetBuffer();

      }
      break;

  }



}

///////////////////////////////
///////////////////////////////////////////////////  SETUP ////////////////////////////////////////////////////////////////////////////////////////////

void setup() {

  delay(4000);
  ///////////////////////////////////////// SPFIIS /////////////////////////////
  Serial.println("An Error has occurred while mounting SPIFFS");
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  //////////////////////////////////part 2 code of IR Revision 3 ///////////////////////////
  pinMode(isStickerPin, INPUT);
  Serial.begin(9600);
  EEPROM.begin (EEPROM_SIZE);
  GSM.begin(9600);
  Serial.println();
  unitaddress = 50;
  countaddress = 200;   // store the count before restarting after timemesssage3
  firstrun = EEPROM.read(5);
  Serial.print("firstrun =   ");
  Serial.println(firstrun);
  // write 1 at address 5 to indentify the board is run before at next time
  if (firstrun == 1) //   1 means the retrofitted meter has been installed before
  {

    File countfile = SPIFFS.open("/count1.txt", "r");

    if (!countfile) {
      Serial.println("There was an error opening the count file for writing");
      return;
    }

    /////////////////////////////
    while (countfile.available()) {
      //Lets read line by line from the file
      String line = countfile.readStringUntil('\n');
      count = line.toInt();
      break; //if left in, we'll just read the first line then break out of the while.
    }
    countfile.close();
    ////////////////////////////
    File unitfile = SPIFFS.open("/unit1.txt", "r");
    if (!unitfile) {
      Serial.println("There was an error opening the unit file for writing");
      return;
    }

    //////////////////////////
    while (unitfile.available()) {
      //Lets read line by line from the file
      String line = unitfile.readStringUntil('\n');
      unit = line.toInt();
      break; //if left in, we'll just read the first line then break out of the while.
    }
    unitfile.close();
    /////////////////////////


    //  unit = EEPROM.get(unitaddress, temp);
    //  count = EEPROM.get(countaddress, tempcount);
    // Serial.println("");

    count = count + 50;     // it is added 50 counts after restoring the power as a substitute to the uncertainty of losing a number of revolutions.
    Serial.print( " Count after added 50");
    Serial.println(count);

    Serial.print("unit is = " );
    Serial.println(unit);

  }

  else {     // any number in the memory address means the retrofitted meter is needed to configure
    Serial.println("Enter a value of Kwh in the meter");   // At installing new ESP, KWh have to be entered as in existing mechanical meter.
    while (!Serial.available() )
    { // is a character available?

    } // end: if (Serial.available() > 0)
    while (Serial.available() > 0) {

      rx_byte = Serial.read();       // get the character
      //Serial.println(rx_byte);
      if ((rx_byte >= '0') && (rx_byte <= '9')) {
        rx_str += rx_byte;
      }
      else if (rx_byte == '\n') {
        // end of string
        if (not_number) {
          Serial.println("Not a number");
        }
        else {
          // multiply the number by 2
          result = rx_str.toInt();
          // print the result
          Serial.println("");
        }
        not_number = false;         // reset flag
        rx_str = "";                // clear the string for reuse
      }
      else {
        // non-number character received
        not_number = true;    // flag a non-number
        Serial.println("not_number");
      }
    }

    //////////////////////////////////////////////
    Serial.println("Enter a value of Count in the meter");
    while (!Serial.available() )
    { // is a character available?

    } // end: if (Serial.available() > 0)
    while (Serial.available() > 0) {

      rx_byte = Serial.read();       // get the character
      //Serial.println(rx_byte);
      if ((rx_byte >= '0') && (rx_byte <= '9')) {
        rx_str += rx_byte;
      }
      else if (rx_byte == '\n') {
        // end of string
        if (not_number) {
          Serial.println("Not a number");
        }
        else {
          // multiply the number by 2
          countresult = rx_str.toInt();
          // print the result
          Serial.println("");
        }
        not_number = false;         // reset flag
        rx_str = "";                // clear the string for reuse
      }
      else {
        // non-number character received
        not_number = true;    // flag a non-number
        Serial.println("not_number");
      }
    }

    //////////////////////////////////////////////
    EEPROM.write(5, 1);     // write 1 at address 5 to indentify the board is run before at next time
    EEPROM.commit();


    //////////////////////////////
    File countfile = SPIFFS.open("/count1.txt", FILE_WRITE);

    if (!countfile) {
      Serial.println("There was an error opening the count file for writing");
      return;
    }

    if (countfile.print(countresult)) {
      Serial.println("countFile at setup was written");
    } else {
      Serial.println("countFile at setup write failed");
    }

    countfile.close();

    File unitfile = SPIFFS.open("/unit1.txt", FILE_WRITE);

    if (!unitfile) {
      Serial.println("There was an error opening the unit file for writing");
      return;
    }


    if (unitfile.print(result)) {
      Serial.println("unitFile at setup was written");
    } else {
      Serial.println("unitFile at setup  write failed");
    }

    unitfile.close();

    /////////////////////////////
    // EEPROM.put(unitaddress, result);   //Write the entering value of Kwh to address 50 Starting to store unit value
    // EEPROM.commit();
    // EEPROM.put(countaddress, 0);   //Write the entering value of Kwh to address 50 Starting to store unit value
    //  EEPROM.commit();
    unit = result;
    count = countresult + 50 ;   // this 50 has been added to overcome somehow the count lost casue the power lost.
    Serial.println("The unit  = " + unit);

    Serial.println(" The count = " + count);

  }

  isSticker = digitalRead(isStickerPin);
  if (isSticker == HIGH)            // Sticker in direction of sensor at starting the board TEST 1
  {
    delay(1500);
    isSticker = digitalRead(isStickerPin);
    if  (isSticker == HIGH)        // Sticker in direction of sensor at starting the board TEST 2   after 4 sec
    {
      delay(2000);
      isSticker = digitalRead(isStickerPin);
      if (isSticker == HIGH)     // Sticker in direction of sensor at starting the board TEST 3  after 2 sec
      {
        Serial.println( " Power is shutdown and sticker is in direction of sensor");
        delay (2000);
        while ( digitalRead(isStickerPin) == HIGH)
        {
          Serial.println( " Still ///Power is shutdown and sticker is in direction of sensor////");  //Sticker in direction of sensor at starting the board TEST 3
          delay (2000);                                                                              // after 11 sec
        }

      }
    }
  }
  Serial.println( " Power on ");                 // DISK of meter is rotated

  // MsTimer2::set(10,poweloss_detector);           // to check if power off or On
  //  MsTimer2::start();
  //stringTwo += "unit updated at EEPROM = ";



  //////////////////////////////////End of code  of IR Revision 3////////////////////////////
  /* create the queue which size can contains 5 elements of Data */

  xTaskCreatePinnedToCore(
    MeterRotation,           /* Task function. */
    "MeterRotation",        /* name of task. */
    10000,                    /* Stack size of task */
    NULL,                     /* parameter of the task */
    1,                        /* priority of the task */
    &xTask1,                /* Task handle to keep track of created task */
    1);                    /* pin task to core 1 */
  xTaskCreatePinnedToCore(
    SendMeterData,           /* Task function. */
    "SendMeterData",        /* name of task. */
    10000,                    /* Stack size of task */
    NULL,                     /* parameter of the task */
    1,                        /* priority of the task */
    &xTask2,            /* Task handle to keep track of created task */
    0);                 /* pin task to core 0 */

}



void MeterRotation( void * parameter )
{

  for (;;) {


    ////////////////////////////////read IR revision 3 read meter ////////////////////////////

    stringOne = String("Counter / KW Consumed =  ");
    isSticker = digitalRead(isStickerPin);
    if (isSticker == HIGH)            // Sticker in direction of sensor at starting the board TEST 1
    {
      ++count ;
      stringOne += count ;
      stringOne += "/";
      stringOne += unit;
      Serial.println(stringOne);

      delay(750);
      isSticker = digitalRead(isStickerPin);
      if  (isSticker == HIGH)        // Sticker in direction of sensor at starting the board TEST 2   after 4 sec
      {
        delay(1000);

        while ( digitalRead(isStickerPin) == HIGH)
        {
          Serial.println( " Still ///Power is shutdown and sticker is in direction of sensor////");  //Sticker in direction of sensor at starting the board TEST 3
          delay (400);                                                                              // after 11 sec
        }


      }

      //////////////////////////////////////////////////  Store count value/////////////////////////////////

      if ( (count == 50) || (count == 100) || (count == 150) || (count == 200) || (count == 250) || (count == 300) || (count == 350))
      {
        File fileToWrite = SPIFFS.open("/count1.txt", FILE_WRITE);

        if (!fileToWrite) {
          Serial.println("There was an error opening the file for writing");
          return;
        }

        if (fileToWrite.println(count)) {
          Serial.println("Filecount was written = " + count);
        } else {
          Serial.println("Filecount was  write failed");
        }

        fileToWrite.close();

      }

      /////////////////////////////////////////////////

      if (( count == 360) || (count > 360))
      {
        unit =  unit + 1 ;
        count = 0;
        //  EEPROM.put(unitaddress, unit);
        //  EEPROM.commit();
        File unitfile = SPIFFS.open("/unit1.txt", FILE_WRITE);

        if (!unitfile) {
          Serial.println("There was an error opening the Unitfile at count = 360 for writing");
          return;
        }
        if (unitfile.print(unit)) {
          Serial.println("UnitFile  at count = 360 was written");
        } else {
          Serial.println("UnitFile at count = 360 write failed");
        }

        unitfile.close();

        File fileToWrite = SPIFFS.open("/count1.txt", FILE_WRITE);

        if (!fileToWrite) {
          Serial.println("There was an error opening the file for writing at count = 0 when unit = 360");
          return;
        }

        if (fileToWrite.println(count)) {
          Serial.println("Filecount was written at count = 0 when unit = 360");;
        } else {
          Serial.println("Filecount was  write failed at count = 0 when unit = 360");
        }

        fileToWrite.close();
        stringTwo += unit;
        Serial.println(stringTwo);
      }


      delay(100);

    }



  }
  vTaskDelete( NULL );
}




void SendMeterData( void * parameter )
{


  for (;;) {



    while ( Timeget == false)
    {
      Gettime();
      Serial.println(" time is not calculated yet ");
      delay(1000);
    }



    while ( 1 )
    {

      delay (1000);
      if ( (Timeget == true) && ( hourflag == true) && ( minflag == true))
      {
        break;
      }
    }
Serial.println(" ************Time has been gotten ************** ");
    ////////////////////////////////////////////////


    if (   0 <= hour && hour < 3)
    {
      //delay_hour = 1000*(( ((3-hour)-1)*3600)+((60-mini)*60));
      delay_hour = ((3 - hour) - 1) * 3600 * 1000;
      delay_mini = ((60 - mini) * 60) * 1000;
      //Serial.println(" 0 <= hour<3");
    }

    else if ( 3 <= hour && hour < 6)
    {
      // delaytime = 1000*(( ((6-hour)-1)*3600)+((60-mini)*60));
      delay_hour = ((6 - hour) - 1) * 3600 * 1000;
      delay_mini = ((60 - mini) * 60) * 1000;
      //Serial.println(" 3 <= hour<6");
    }

    else if ( 6 <= hour && hour < 9)
    {
      delay_hour = ((9 - hour) - 1) * 3600 * 1000;
      delay_mini = ((60 - mini) * 60) * 1000;
      //Serial.println(" 6 <= hour<9");

    }

    else if ( 9 <= hour && hour < 12)
    {
      delay_hour = ((12 - hour) - 1) * 3600 * 1000;
      delay_mini = ((60 - mini) * 60) * 1000;
     // Serial.println(" 9 <= hour<12");
    }

    else if ( 12 <= hour && hour < 15)
    {
      delay_hour = ((15 - hour) - 1) * 3600 * 1000;
      delay_mini = ((60 - mini) * 60) * 1000;
      //Serial.println(" 12 <= hour<15");
    }

    else if ( 15 <= hour && hour < 18)
    {
      delay_hour = ((18 - hour) - 1) * 3600 * 1000;
      delay_mini = ((60 - mini) * 60) * 1000;
      //Serial.println(" 15 <= hour<18");
    }

    else if ( 18 <= hour && hour < 21)
    {

      delay_hour = ((21 - hour) - 1) * 3600 * 1000;
      delay_mini = ((60 - mini) * 60) * 1000;
      //Serial.println(" 18 <= hour<21");
    }

    else if ( 21 <= hour && hour < 24)
    {

      delay_hour = ((24 - hour) - 1) * 3600 * 1000;
      delay_mini = ((60 - mini) * 60) * 1000;
     // Serial.println(" 21 <= hour<24");
    }

    // unsigned long  Sleeptime = delaytime - 180000;
    Serial.print( "Delay hour = ");
    Serial.println (delay_hour / 3600000);
    Serial.print( "Delay min = ");
    Serial.println(delay_mini / 60000);
   sendGSM("AT+CFUN=0,500"); // to take away the sim card functions
   delay (delay_hour);
    delay(delay_mini);
    delay(180000);
    sendGSM("AT+CFUN=1,500");  //Restoring the sim card all functions for sending mode preparation

    Timeget = false;
    minflag = false;
    hourflag = false;
    Serial.println(unit);
    SendData (unit);     // After sleeping, The KWh valuse have to be sent to the sever
    delay(250);
  }
  vTaskDelete( NULL );
}

void SendData (unsigned long Kwh)
{

  // byte inttemp = ((temprature_sens_read() - 32) / 1.8);
  byte timedelay = delaytime / 60000;
  makeJson(Kwh, count);                    //Making JSON text here
  /*You can send any values from here and make a json file to send to thingsboard*/
  updateThingsboard();
  delay (10000);
}
////////////////////////////////////////////////////////////////////////////////////JSON///////////////////////////////////////
void updateThingsboard()
{
  lengthOfJSON = 0;                  //Set size of JSON text as '0' initially

  // Selects Single-connection mode
  sendGSM("AT+CIPMUX=0");//, "OK","ERROR", 1000) == 1)      // CIMPUX=0 is already set in Single-connection mode

  delay(2000);                                                   //wait 2 sec


  // Sets the APN, user name and password
  sendGSM("AT+SAPBR=3,1,\"APN\",\"net.asiacell.com\"");      // For Asia, APN: net.asiacell, password: "",
                                                             // For Zain, APN : "internet", Password: ""
  // Waits for status IP START
  //  sendGSM("AT+SAPBR=1,1") ;
  sendGSM("AT+CIPSTATUS");
  delay(2000);

  // Brings Up Wireless Connection
  sendGSM("AT+CIICR");

  delay(1000);
  Serial.println("\n Bringup Wireless Connection ...........");
  // Waits for status IP GPRSACT

  delay(2000);

  // Waits for status IP STATUS
  sendGSM("AT+CIPSTATUS", 500) ;
  delay(2000);
  Serial.println("Opening TCP");
  snprintf(aux_str, sizeof(aux_str), "AT+CIPSTART=\"TCP\",\"%s\",\"%s\"", thingsboard_url, port); //IP_address
  sendGSM(aux_str, 30000) ;

  Serial.println(aux_str);
  String json = "";
  root.printTo(json);                       //Store JSON in a String named 'json'
  lengthOfJSON = json.length();             //This gives us total size of our json text

  //TCP packet to send POST Request on https (Thingsboard)
  getStr = "POST /api/v1/" + AccessToken + "/telemetry HTTP/1.1\r\nHost: demo.thingsboard.io\r\nAccept: */*\r\nContent-Type: application/json\r\nContent-Length:" + lengthOfJSON + "\r\n\r\n" + json;
  //TCP packet to send POST Request on https (Thingsboard)

  String sendcmd = "AT+CIPSEND=" + String(getStr.length());

  delay(3000);
  sendGSM(sendcmd, 500);      //Sending Data Here
  Serial.println("Sending Data Here");
  Serial.println(getStr);
  delay(10000);
  sendGSM(getStr, 500);
  delay (5000);

  Serial.println("Closing the Socket............");
  sendGSM("AT+CIPCLOSE", 10000);
  Serial.println("Shutting down the connection.........");
  sendGSM("AT+CIPSHUT", 10000);
  delay(1000);
}

void  makeJson( float val11, int vall2)
{
  Serial.println("\nMaking JSON text meanwhile\n");
  root["KWh"] = val11;
  root["count"] = vall2;
}
void loop()
{

  while (GSM.available()) {
    parseATText(GSM.read());
    delay(10);
  }


}


void Gettime ()
{
  sendGSM("AT");
  delay(120);
  sendGSM("AT+CFUN=1"); // sets the level of functionality, 1 full functionality
  delay (120);
  sendGSM("AT+CREG?"); // gives information about the registration status and access technology of the serving cell.
  delay(120);
  while (GPRSflag == false)    // to check GPRS connected or not and that depends on return value; 1 connected , 0 not connected
  {
    delay(500);
    sendGSM("AT+CREG?");

  }
  delay(120);
  sendGSM("AT+SAPBR=3,1,\"APN\",\"net.asiacell.com\"");
  delay(120);
  sendGSM("AT+SAPBR=1,1");   // for activate bearer context
  delay (500);
  sendGSM("AT+CIPGSMLOC=2,1");    // for location and time
  delay (5000);
}

void Entersleepmode( unsigned long sleep)
{

  sendGSM("AT+CFUN=0");
  Serial.println( " Enter flight air mode for period = ");
  Serial.println ( sleep / 60000);
  delay(sleep);
  sendGSM("AT+CFUN=1");
  delay(30000);
  sendGSM("AT+SAPBR=3,1,\"APN\",\"net.asiacell.com\"");

}
