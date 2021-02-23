#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <Wire.h>//Set Buffer size to Dynamic
#include <ArduinoJson.h>
#include "Esp.h"
#define EEPROM_SIZE 1000
#define isStickerPin   32    
#define power_loss_pin_detection 13     //34  // It is used for power loss detction interrupt.
#define sim_pinReset 25                       // It is used for sim 800l hardware reset.
char aux_str[100];
char consumption [100];
char buffer[100];
char thingsboard_url[] = "demo.thingsboard.io";
char port[] = "80";
char rx_byte = 0;
String rx_str = "";// PORT Connected on
String getStr = "";
String AccessToken = "kgVF47JnrEVJCND1aymm";  //Add thr access token for the device which get it from thingsboard platform
String stringOne;
String stringTwo;
String json = "";
volatile unsigned long unit = 0;
volatile int count = 0;
volatile bool power_flag = false;
unsigned long temp = 0;
unsigned long delay_hour;
unsigned long delay_mini;
unsigned long delaytime = 0;
unsigned long result = 0;
int m = 0;
int lengthOfJSON;
int tempcount = 0;
int l = 0;
int countresult = 0;
int year;
int month;
int day;
int hour;
int mini;
int contentLength = 0;// 1 means worked before   0 means needs to configure
byte unitaddress;
volatile byte countaddress;
byte firstrun;
byte status_Network ; // to check the sim card registeration status; error.0= Not registered, 1 registered, 2 searching, 3 denied, 5 roaming, 6 net error, 7 unkowun
byte pos = 0;
byte isSticker ;  // HIGH MEANS NO OBSTACLE
byte Power = HIGH;
byte connection_tries = 0; // trying 6 times to connect the server before move on
// for just test the spread sheet
bool  flag = false ;  // to see if first running card
bool not_number = false;
bool Sendflag = true;
bool Timeget = false; // to ensure time calcualted is finished before enter sleep. true means time calculated , false no yet
//bool GPRSflag = false;  // to check if GPRS connected or not   false not connected,   true = connected
bool Sendingsuccess = false;  // to chech sending data success or not by default is considered true to send data at
bool ServerConnected = false; // to check connecting with server sucess or not
bool gprs_Request = false;
bool gprs_Ready_flag = false;
bool minflag;
bool hourflag;
bool nighbore_Help_request = false;
bool init_Mesh = false;
bool sim_Ready_request = false;
bool sim_Ready_flag = false;
bool net_Status_request = false ;
bool save = true;
EspClass myESP;
DynamicJsonDocument root(1024);              //Create an object 'root' which is called later to print JSON Buffer
SoftwareSerial GSM(17, 16); // TX, RX
xQueueHandle xQueue;
TaskHandle_t xTask1;
TaskHandle_t xTask2;

////////
enum NetworkRegistration {NOT_REGISTERED, REGISTERED_HOME, SEARCHING, DENIED, NET_UNKNOWN, REGISTERED_ROAMING, NET_ERROR};
///////

///////////////////////////////////
void IRAM_ATTR Handler_power() {

  static unsigned long lastInterrupt = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterrupt > 200 && power_flag == false)
 
  {
    
    Serial.println(" called1 ");
    power_flag = true;

  }
  lastInterrupt = interruptTime;
  Serial.println(" called out ");
  //detachInterrupt(power_loss_pin_detection);
}
///////////////////////////////////
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

void sendGSM(const char* msg, int waitMs = 500) {
  GSM.println(msg);
  delay_count(waitMs);
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

          else if ((pos == 2 ) && (strcmp(buffer, "OK")) == 0) {

            if (sim_Ready_request == true) {
              sim_Ready_flag = true;  // to check sim model is ready or not
            }
            if (gprs_Request == true) {
              gprs_Ready_flag = true; // to check
            }

            parseState = PS_IGNORING_COMMAND_ECHO;
          }

          else if (pos == 10 && strcmp(buffer, "CONNECT OK") == 0 )
          {
            //  Serial.println(" *********Detect OK CONNECT **********");
            parseState = PS_IGNORING_COMMAND_ECHO;
            ServerConnected = true;
          }
          else if (pos == 12 && strcmp(buffer, "HTTP/1.1 200") == 0 )
          {
            Serial.println(" *********Detect HTTP/1.1 200 **********");
            parseState = PS_IGNORING_COMMAND_ECHO;
            Sendingsuccess = true;
          }
          else if ( b == ':' ) {                                    // case 3    if  :
            String str((char*)buffer);

            if (( str.indexOf(",") == 2) && (str.length() == 17))
            {
              str = str.substring(str.indexOf(",") + 1, str.length());
              String dt = str.substring(0, str.indexOf(",")); // it is not important to know the year, month, day layth 22/02/2021
              String tm = str.substring(str.indexOf(",") + 1, str.length()) ;
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
              Serial.print(F( "the year = "));
              Serial.println ( year);
              Serial.print(F( "the Month = "));
              Serial.println ( month);
              Serial.print(F( "the day = "));
              Serial.println ( day);
              Serial.print( F("the houer = "));
              Serial.println ( hour + 3 );
              Serial.print( F("the min = "));
              Serial.println ( mini );
              minflag = true;



            }

            ///////////////////////////////
            if ( ( hourflag == true) && ( minflag == true))
            {
              // Serial.println( " Timeget=true");
              Timeget = true;  ////  point false to Timeget flag after send seccess to forct the sendmeter  taske to get time for another period
            }
            else
            {
              // Serial.println( " Timeget=false");
              Timeget = false;
            }
            //////////////////////////////

            if ( strcmp(buffer, "+HTTPACTION:") == 0 ) {
              Serial.println("Received HTTPACTION");
              parseState = PS_HTTPACTION_TYPE;                      //  after received : and + HTTPACTION  then go to PS_HTTPACTION_TYPE
            }


            else if (strcmp(buffer, "+CIPGSMLOC:") == 0 ) {         //  after received : and + CIPGSMLOC  then go to PS_HTTPREAD_TYPE  THEN GO TO LENGTH
              //  Serial.println("Received CIPGSMLOC");
              //  Serial.println(buffer);
            }

            else if ( strcmp(buffer, "+HTTPREAD:") == 0 ) {
              Serial.println("Received HTTPREAD");
              parseState = PS_HTTPREAD_LENGTH;
            }
            ////////////////////////////////////
            else if ( strcmp(buffer, "+CREG:") == 0 ) {
              // Serial.println(F("Received CREG"));
              //  Serial.println(buffer);
              parseState = PS_CREG_TYPE;
            }
            else if (( net_Status_request == true) && (strcmp(buffer, "ERROE") == 0))
            {
              status_Network = 6; // ERROR
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
          // Serial.print("Ignoring echo: ");
          // Serial.println(buffer);
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
        Serial.print(F("HTTPREAD length is "));
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
          //  Serial.print(F("CREG type is "));
          parseState = PS_CREG_CONTENT;
          resetBuffer();
        }
      }
      break;

    case PS_CREG_CONTENT:
      {
        if ( b == '1' ) {
          Serial.println(F(" GPRS CONNECTED" ));
          //  GPRSflag = true;
          status_Network = 1; // Registered
        }
        else if ( b == '0' ) {
          Serial.println(F(" GPRS NOT  CONNECTED") );
          // GPRSflag = false;
          status_Network = 0; // not registered
        }
        else if ( b == '2' ) {
          Serial.println(F(" Searching network" ));
          //  GPRSflag = false;
          status_Network = 2; // Searching
        }
        else if ( b == '3' ) {
          Serial.println(F(" DENIED " ));
          //  GPRSflag = false;
          status_Network = 3; // denied
        }
        else if ( b == '5' ) {
          Serial.println(F(" REGISTERED_ROAMING" ));
          // GPRSflag = true;
          status_Network = 5; // Registering roaming
        }
        else {
          Serial.println(F( " NET _UNKOWN"));
          // GPRSflag = false;
          status_Network = 7 ; // Net_ Unknown
        }
        parseState = PS_DETECT_MSG_TYPE;
        resetBuffer();

      }
      break;

  }



}

void sendMessage() ;

///////////////////////////////
///////////////////////////////////////////////////  SETUP ////////////////////////////////////////////////////////////////////////////////////////////

void setup() {

  delay(4000);
  pinMode(isStickerPin, INPUT);
  pinMode(power_loss_pin_detection, INPUT);
  pinMode(sim_pinReset, INPUT);

  attachInterrupt(power_loss_pin_detection, Handler_power, FALLING);
  ///////////////////////////////////////// SPFIIS /////////////////////////////
  
  //////////////////////////////////part 2 code of IR Revision 3 ///////////////////////////
  Serial.begin(115200);
  if (!EEPROM.begin(1000)) {
    Serial.println("Failed to initialise EEPROM");
    Serial.println("Restarting...");
    delay(1000);
    myESP.restart();
  }

  Serial.println();
  unitaddress = 50;
  countaddress = 200;   // store the count before restarting after timemesssage3
  firstrun = EEPROM.read(5);
  Serial.print("run_before =   ");
  Serial.println(firstrun);
  // write 1 at address 5 to indentify the board is run before at next time
  if (firstrun == 1) //   1 means the retrofitted meter has been installed before
  {
    count = EEPROM.readUInt(countaddress);
    unit = EEPROM.readULong(unitaddress);
    ////////////////////////////

    //  unit = EEPROM.get(unitaddress, temp);
    //  count = EEPROM.get(countaddress, tempcount);
    // Serial.println("");

    //  count = count + 50;     // it is added 50 counts after restoring the power as a substitute to the uncertainty of losing a number of revolutions.

    Serial.print(" The count = " );
    Serial.println ( count);

    Serial.print("The KWh is = ");
    Serial.println (unit);


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
    EEPROM.writeUInt(countaddress, countresult);
    EEPROM.writeULong(unitaddress, result);
    EEPROM.commit();

    /////////////////////////////
    // EEPROM.put(unitaddress, result);   //Write the entering value of Kwh to address 50 Starting to store unit value
    // EEPROM.commit();
    // EEPROM.put(countaddress, 0);   //Write the entering value of Kwh to address 50 Starting to store unit value
    //  EEPROM.commit();
    unit = result;
    count = countresult  ;   // this 50 has been added to overcome somehow the count lost casue the power lost.
    Serial.println("The unit after configuared  = " + unit);

    Serial.println(" The count after configuared= " + count);

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



  GSM.begin(9600);

  //////////////////////////////////End of code  of IR Revision 3////////////////////////////
  /* create the queue which size can contains 5 elements of Data */
  xTaskCreatePinnedToCore(
    SendMeterData,           /* Task function. */
    "SendMeterData",        /* name of task. */
    10000,                    /* Stack size of task */
    NULL,                     /* parameter of the task */
    1,                        /* priority of the task */
    &xTask2,            /* Task handle to keep track of created task */
    0);                /* pin task to core 0 */

  xTaskCreatePinnedToCore(
    MeterRotation,           /* Task function. */
    "MeterRotation",        /* name of task. */
    10000,                    /* Stack size of task */
    NULL,                     /* parameter of the task */
    1,                        /* priority of the task */
    &xTask1,                /* Task handle to keep track of created task */
    1);                     /* pin task to core 1 */




  Serial.println( "Setup complete" );

}


//////////////////////////////////////////////////// End of SETUP ////////////////////////////////////////////


void MeterRotation( void * parameter )
{

  for (;;) {

    ////////////////////////////////read IR revision 3 read meter ////////////////////////////



    isSticker = digitalRead(isStickerPin);
    if (isSticker == HIGH)            // Sticker in direction of sensor at starting the board TEST 1
    {
      ++count ;
      snprintf(consumption, sizeof(consumption), "Counter / KW Consumed = %d / %d", count, unit);
      Serial.println(consumption);
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

      if ( (count == 5) || (count == 10) || (count == 15) || (count == 20) || (count == 25) || (count == 30) || (count == 35) || (count == 40))
      {
        //save_Count();

      }

      /////////////////////////////////////////////////

      if (( count == 40) || (count > 40))
      {
        unit =  unit + 1 ;
        count = 0;
        //  EEPROM.put(unitaddress, unit);
        //  EEPROM.commit();
        //   save_Unit();
        //   save_Count();
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
  unsigned long m = millis();
  byte send_tries = 0;
  Sleep_estimation( );
  delay_count( delay_hour);
  delay_count( delay_mini);
  sendGSM("AT+CFUN=1,500");  //Restoring the sim card all functions for sending mode preparation
  Timeget = false;
  minflag = false;
  hourflag = false;

  for (;;) {

    // sendGSM("AT+CFUN=0,500"); // to take away the sim card functions

    unsigned long start_sleep1 = millis();
    save_Count();
    save_Unit();
    while (!Sendingsuccess) {    // at first try to send data to server and check (200 ok)so Sendingsuccess = true if not tries 3 times only
      SendData (unit);          // After sleeping, The KWh valuse have to be sent to the sever
      if (send_tries++ <= 3) {
        // Serial.println("The connection to server is failed for " + connection_tries + " times" + );
      }
      else {
        Serial.println("After 3 times,the sending data has failed" );
        break;
      }
    }
    Sendingsuccess = false;
    send_tries = 0;
    delay_count(250);
    unsigned long start_sleep2 = millis();
    unsigned long sleep_again = 3 * 60 * 60 * 1000 - (start_sleep2 - start_sleep1);

    delay_count( sleep_again);

  }

  vTaskDelete( NULL );
}

void SendData (unsigned long Kwh)
{

  updateThingsboard();
  delay_count (10000);
}
////////////////////////////////////////////////////////////////////////////////////JSON///////////////////////////////////////
void updateThingsboard()
{
  lengthOfJSON = 0;                  //Set size of JSON text as '0' initially
  if (!sim_Ready_flag)
  {
    IsReady(10000);
  }
  GPRS_Connect();
  // Selects Single-connection mode
  //sendGSM("AT+CIPMUX=0");//, "OK","ERROR", 1000) == 1)      // CIMPUX=0 is already set in Single-connection mode

  delay_count(2000);                                                   //wait 2 sec


  // Sets the APN, user name and password

  // Waits for status IP START
  //  sendGSM("AT+SAPBR=1,1") ;
  sendGSM("AT+CIPSTATUS");
  delay_count(2000);

  // Brings Up Wireless Connection
  sendGSM("AT+CIICR");

  delay_count(1000);
  Serial.println("\n Bringup Wireless Connection ...........");
  // Waits for status IP GPRSACT

  delay_count(2000);

  // Waits for status IP STATUS
  sendGSM("AT+CIPSTATUS", 500) ;
  delay_count(2000);
  Serial.println("Opening TCP");
  snprintf(aux_str, sizeof(aux_str), "AT+CIPSTART=\"TCP\",\"%s\",\"%s\"", thingsboard_url, port); //IP_address
  Serial.println( "Trying to connect thingsboard server");
  while (! ServerConnected) {

    sendGSM(aux_str, 15000) ;
    delay_count(100);
    Serial.print(".");
    if (connection_tries++ <= 6) {
      // Serial.println("The connection to server is failed for " + connection_tries + " times" + );
    }
    else {
      break;
    }
  }
  Serial.println (F( "Thingsboard server connected"));
  connection_tries = 0;
  ServerConnected = false;
  //Serial.println(aux_str);
  makeJson(unit, count);
  serializeJson(root, json);
  //root.printTo(json);                       //Store JSON in a String named 'json'
  lengthOfJSON = json.length();             //This gives us total size of our json text

  /////////////////

  ////////////////
  //TCP packet to send POST Request on https (Thingsboard)
  getStr = "POST /api/v1/" ;
  getStr +=  AccessToken;
  getStr += "/telemetry HTTP/1.1\r\nHost: demo.thingsboard.io\r\nUser-Agent: curl/7.55.1\r\nAccept: */*\r\nContent-Type: application/json\r\nContent-Length:";
  getStr += lengthOfJSON;
  getStr += "\r\n\r\n";
  getStr += json;

  //TCP packet to send POST Request on https (Thingsboard)

  String sendcmd = "AT+CIPSEND=" + String(getStr.length());

  delay_count(3000);
  sendGSM(sendcmd.c_str(), 500);      //Sending Data Here******************** to check
  Serial.println("Sending Data Here");
  Serial.println(getStr);
  delay_count(10000);
  sendGSM(getStr.c_str(), 500);
  delay_count (5000);

  Serial.println("Closing the Socket............");
  sendGSM("AT+CIPCLOSE", 10000);
  Serial.println("Shutting down the connection.........");
  sendGSM("AT+CIPSHUT", 10000);
  delay_count(1000);
}

void makeJson( float val11, int vall2)
{
  Serial.println("\nMaking JSON text meanwhile\n");
  // root["SIM"] = sim_Ready_flag;
  // root["Node"] = node_Number;
  // root["Acesstoken"] = AccessToken;
  root["KWh"] = val11;
  root["count"] = vall2;
  // root["state"] = "Help";

}


void loop()
{

  while (GSM.available()) {
    parseATText(GSM.read());
    delay(100);
  }

}


void Gettime ()
{
  IsReady(10000) ;
  delay_count(120);
  sendGSM("AT+CFUN=1"); // OK sets the level of functionality, 1 full functionality
  delay_count (120);
  NetworkRegistration network = getRegistrationStatus();
  while (network != REGISTERED_HOME && network != REGISTERED_ROAMING) {
   delay_count(1000);
    network = getRegistrationStatus();
    Serial.print(".");

  }
  Serial.println(F("Network registration OK"));
  delay_count(1000);
  ////////////////////////
  /*  while (GPRSflag == false)    // to check GPRS connected or not and that depends on return value; 1 connected , 0 not connected
    {
      //  delay(500);
      sendGSM("AT+CREG?", 500);

    }*/

  Serial.println(" Getting time .........");
  sendGSM("AT+SAPBR=1,1");   // for activate bearer context
  delay_count (500);
  sendGSM("AT+CIPGSMLOC=2,1");    // for location and time
  delay_count(5000);
}

void Entersleepmode( unsigned long sleep)
{

  sendGSM("AT+CFUN=0");
  Serial.println( " Enter flight air mode for period = ");
  Serial.println ( sleep / 60000);
  delay(sleep);
  sendGSM("AT+CFUN=1");
  delay_count(30000);
  sendGSM("AT+SAPBR=3,1,\"APN\",\"net.asiacell.com\"");

}

bool IsReady(uint16_t timeout)
{
  uint32_t timerStart = millis();
  sim_Ready_request = true;
  Serial.println( "trying to connect the SIM Model");
  while (!sim_Ready_flag )
  {

    sendGSM("AT", 1000);
    delay_count (500);
    Serial.print(".");
    // If timeout, abord the reading
    if (millis() - timerStart > timeout) {
      Serial.println( " The sim model is not responding, The reset must be taken");
      Reset();
      timerStart = millis();
    }


  }
  Serial.println( F("SIM Model is ready"));
  sim_Ready_request = false;
  sim_Ready_flag = false;
  return true;
}
void Reset( )
{
  digitalWrite(sim_pinReset, HIGH);
  delay_count(500);
  digitalWrite(sim_pinReset, LOW);
  delay_count(500);
  digitalWrite(sim_pinReset, HIGH);
  delay_count(1000);
  Serial.println(" The model has been reset");
}

NetworkRegistration getRegistrationStatus()
{
  net_Status_request == true;

  sendGSM("AT+CREG?", 2000); // +CREG= 0,1 then OK  gives information about the registration status and access technology of the serving cell.
  switch (status_Network)
  {
    case 0 : return NOT_REGISTERED;
    case 1 : return REGISTERED_HOME;
    case 2 : return SEARCHING;
    case 3 : return DENIED;
    case 5 : return REGISTERED_ROAMING;
    case 6 : return NET_ERROR;
    case 7 : return NET_UNKNOWN;
  }
  net_Status_request = false;
}

bool GPRS_Connect( )
{
  gprs_Request = true;
  sendGSM("AT+SAPBR=3,1,\"APN\",\"net.asiacell.com\"");
  delay_count(1000);
  if (gprs_Ready_flag == true)
  {
    gprs_Request = false;
    gprs_Ready_flag = false;
    return true;
  }
  else {
    delay_count (2000);
    GPRS_Connect( );
  }
  Serial.println(F( " GPRS connected "));
}

void Sleep_estimation( )
{
  if (!IsReady(10000)) {
    Serial.println("The model is not responding");
  };

  while ( Timeget == false)
  {
    Gettime();
    Serial.println(" time is not calculated yet ");
    delay_count(1000);
  }



  while ( 1 )
  {

    delay_count (1000);
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
}

void save_Count()
{
  EEPROM.writeUInt(countaddress, count);
  EEPROM.commit();
  Serial.print(" The count is = ");
  Serial.println ( count);

}


void save_Unit()
{

  EEPROM.writeULong(50, unit);
  EEPROM.commit();
  Serial.print(" The unit is = ");
  Serial.println ( unit);

}

double heap_Checking()
{
  log_d("Total heap: %d", myESP.getHeapSize());
  log_d("Free heap: %d", myESP.getFreeHeap());
  log_d("Total PSRAM: %d", myESP.getPsramSize());
  log_d("Free PSRAM: %d", myESP.getFreePsram());

  double c = ((myESP.getHeapSize() - myESP.getFreeHeap()) / myESP.getHeapSize());

  return c;
}

void delay_count( unsigned long delayed)
{
  unsigned long m = millis();
  while ( millis() - m < delayed)
  {
    if ( power_flag == true) {
      save_Count();
      save_Unit();
      Serial.println(" will be restart");
      myESP.restart();    // JUST TO SIMULATE THE POWER DOWN STATE. IT WILL BE REMOVED LATER 
      break;
    }
    delay(10);
  }
  return ;
}
