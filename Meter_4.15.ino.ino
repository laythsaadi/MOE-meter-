#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <Wire.h>//Set Buffer size to Dynamic
#include <ArduinoJson.h>
#include "Esp.h"
#define EEPROM_SIZE 1000
#define isStickerPin   2    //13 //23
#define power_loss_pin_detection     32    //27     //34  // It is used for power loss detction interrupt.
#define sim_pinReset 25
RTC_DATA_ATTR int bootCount = 0;// It is used for sim 800l hardware reset.
char aux_str[100];
char consumption[100];
char date_time [100];
char buffer[100];
char thingsboard_url[] = "demo.thingsboard.io";
char port[] = "80";
char rx_byte = 0;
String rx_str = "";// PORT Connected on
String getStr = "";
String AccessToken = "kgVF47JnrEVJCND1aymm";  //Add thr access token for the device which get it from thingsboard platformnlkgVF47JnrEVJCND1aymm
String stringOne;
String stringTwo;
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
bool save = false;
EspClass myESP;
DynamicJsonDocument root(1024);              //Create an object 'root' which is called later to print JSON Buffer
SoftwareSerial GSM(17, 16); // TX, RX
xQueueHandle xQueue;
TaskHandle_t xTask1;
TaskHandle_t xTask2;

////////
enum NetworkRegistration {NOT_REGISTERED, REGISTERED_HOME, SEARCHING, DENIED, NET_UNKNOWN, REGISTERED_ROAMING, NET_ERROR};
///////


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
  //  Serial.println(buffer[b]);

  // {Serial.println("connected");}
  buffer[pos++] = b;

  if ( pos >= sizeof(buffer) )
    resetBuffer(); // just to be safe

  switch (parseState) {
    case PS_DETECT_MSG_TYPE:
      {
        if ( b == '\n' )
          resetBuffer();                                            // new line so reset buffer  case 1
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
            //Serial.println(" *********Detect OK CONNECT **********");
             log_d("*********Detect OK CONNECT **********");
            parseState = PS_IGNORING_COMMAND_ECHO;
            ServerConnected = true;
          }
          else if ( pos == 12 && strcmp(buffer, "HTTP/1.1 200") == 0 )
          {
            //Serial.println(" *********Detect HTTP/1.1 200 **********");
            log_d(" *********Detect HTTP/1.1 200 **********");
            parseState = PS_IGNORING_COMMAND_ECHO;
            Sendingsuccess = true;

          }
          else if ( pos == 7 && strcmp(buffer, "SEND OK") == 0 )
          {
           // Serial.println(" *********SEND OK **********");
            log_d(" *********SEND OK **********");
            parseState = PS_IGNORING_COMMAND_ECHO;
            Sendingsuccess = true;

          }

          else if ( b == ':' ) {                                    // case 3    if  :
            String str((char*)buffer);

            if (( str.indexOf(",") == 2) && (str.length() == 17))
            {
              str = str.substring(str.indexOf(",") + 1, str.length());
              String dt = str.substring(0, str.indexOf(","));
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
              snprintf(date_time, sizeof(date_time), "Date/Time: %d/%d/%d  %d:%d", year, month, day, hour + 3, mini);
              Serial.println(date_time);
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
             // Serial.println("Received HTTPACTION");
             log_d("Received HTTPACTION");
              parseState = PS_HTTPACTION_TYPE;                      //  after received : and + HTTPACTION  then go to PS_HTTPACTION_TYPE
            }


            else if (strcmp(buffer, "+CIPGSMLOC:") == 0 ) {         //  after received : and + CIPGSMLOC  then go to PS_HTTPREAD_TYPE  THEN GO TO LENGTH
              //  Serial.println("Received CIPGSMLOC");
              //  Serial.println(buffer);
            }

            else if ( strcmp(buffer, "+HTTPREAD:") == 0 ) {
            //  Serial.println("Received HTTPREAD");
              log_d("Received HTTPREAD");
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
         Serial.print("Ignoring echo: ");
         Serial.println(buffer);
        // log_d("Ignoring echo: ");
        // log_d(buffer);
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
          parseState = PS_DETECT_MSG_TYPE;
          resetBuffer();
        }
      }
      break;

      if ( b == '\n' ) {
        contentLength = atoi(buffer);
        Serial.print(F("HTTPREAD length is "));
        Serial.println(contentLength);

        log_d("HTTPREAD content: ");

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
          log_d(" GPRS CONNECTED" );
          //  GPRSflag = true;
          status_Network = 1; // Registered
        }
        else if ( b == '0' ) {
          log_d(" GPRS NOT  CONNECTED") ;
          // GPRSflag = false;
          status_Network = 0; // not registered
        }
        else if ( b == '2' ) {
          log_d(" Searching network" );
          //  GPRSflag = false;
          status_Network = 2; // Searching
        }
        else if ( b == '3' ) {
          log_d(" DENIED " );
          //  GPRSflag = false;
          status_Network = 3; // denied
        }
        else if ( b == '5' ) {
          log_d(" REGISTERED_ROAMING" );
          // GPRSflag = true;
          status_Network = 5; // Registering roaming
        }
        else {
          log_d( " NET _UNKOWN");
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

  //delay(2000);
  pinMode(isStickerPin, INPUT);
  pinMode(power_loss_pin_detection, INPUT);
  pinMode(sim_pinReset, INPUT);
  Serial.begin(115200);
  if (!EEPROM.begin(1000)) {
    log_d("Failed to initialise EEPROM");
    log_d("Restarting...");
    delay(1000);
    myESP.restart();
  }
  Serial.println();
  unitaddress = 50;
  countaddress = 200;   // store the count before restarting after timemesssage3
  firstrun = EEPROM.read(5);
  // write 1 at address 05 to indentify the board is run before at next time
  if (firstrun == 1) //        1 means the retrofitted meter has been installed before
  {
    count = EEPROM.readUInt(countaddress);
    unit = EEPROM.readULong(unitaddress);
    ////////////////////////////


    log_d("The count: %d", count);
    log_d("The KWh: %d", unit);



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
    unit = result;
    count = countresult  ;
  }

  isSticker = digitalRead(isStickerPin);
  if (isSticker == HIGH)            // Sticker in direction of sensor at starting the board TEST 1
  {
    delay_core_rotation(1500);
    isSticker = digitalRead(isStickerPin);
    if  (isSticker == HIGH)        // Sticker in direction of sensor at starting the board TEST 2   after 4 sec
    {
      delay_core_rotation(1000);
      isSticker = digitalRead(isStickerPin);
      if (isSticker == HIGH)     // Sticker in direction of sensor at starting the board TEST 3  after 2 sec
      {
       log_d( " Power is shutdown and sticker is in direction of sensor");
        delay_core_rotation (1000);
        while ( digitalRead(isStickerPin) == HIGH)
        {
         // log_d( " Still ///Power is shutdown and sticker is in direction of sensor////");  //Sticker in direction of sensor at starting the board TEST 3
          delay_core_rotation(1000);                                                                              // after 11 sec
        }

      }
    }
  }
  log_d( " Power on ");                 // DISK of meter is rotated
  log_d(" the count = %d", count);
  log_d(" The Kwh = %d", unit);


  GSM.begin(9600);

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
    // log_d("Counter / KW Consumed = %d / %d", count, unit);
      delay_core_rotation(750);
      isSticker = digitalRead(isStickerPin);
      if  (isSticker == HIGH)        // Sticker in direction of sensor at starting the board TEST 2   after 4 sec
      {
        delay_core_rotation(1500);

        while ( digitalRead(isStickerPin) == HIGH)
        {
         // log_d( " Still ///Power is shutdown and sticker is in direction of sensor////");  //Sticker in direction of sensor at starting the board TEST 3
          delay_core_rotation (400);                                                                              // after 11 sec
        }


      }

      //////////////////////////////////////////////////  Store count value/////////////////////////////////

      if ( (count == 5) || (count == 10) || (count == 15) || (count == 20) || (count == 25) || (count == 30) || (count == 35) || (count == 40))
      {
        //save_Count(); // is replaced by saving the count when interrupt raises to save KWh and count before ESP powe off.

      }

      /////////////////////////////////////////////////

      if (( count == 40) || (count > 40))
      {
        unit =  unit + 1 ;
        count = 0;
        stringTwo += unit;
        Serial.println(stringTwo);
      }

      delay_core_rotation(100);

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
    SendData (unit);
    delay_count(250);
    unsigned long start_sleep2 = millis();
    unsigned long sleep_again = 3 * 60 * 60 * 1000 - (start_sleep2 - start_sleep1);
    Serial.println(sleep_again);
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
  log_d("\n Bringup Wireless Connection ...........");
  // Waits for status IP GPRSACT

  delay_count(2000);

  // Waits for status IP STATUS
  sendGSM("AT+CIPSTATUS", 500) ;
  delay_count(2000);
  log_d("Opening TCP");
  snprintf(aux_str, sizeof(aux_str), "AT+CIPSTART=\"TCP\",\"%s\",\"%s\"", thingsboard_url, port); //IP_address
  log_d( "Trying to connect thingsboard server");
  while (! ServerConnected) {

    sendGSM(aux_str, 15000) ;
    delay_count(100);
    log_d(".");
    if (connection_tries++ <= 6) {
      // Serial.println("The connection to server is failed for " + connection_tries + " times" + );
    }
    else {
      break;
    }
  }
  if ( ! ServerConnected) {
    log_d( "Thingsboard server is not connected");
  }
  else {
    log_d( "Thingsboard server connected ******* OKAY ");
  };
  connection_tries = 0;
  ServerConnected = false;
  //Serial.println(aux_str);
  makeJson(unit, count);
  String json = "";
  serializeJson(root, json);
  lengthOfJSON = json.length();             //This gives us total size of our json text
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
  log_d("Sending Data Here");
  Serial.println(getStr);
  delay_count(10000);
  sendGSM(getStr.c_str(), 500);
  delay_count (5000);

  log_d("Closing the Socket............");
  sendGSM("AT+CIPCLOSE", 10000);
  log_d("Shutting down the connection.........");
  sendGSM("AT+CIPSHUT", 10000);
  delay_count(1000);
}

void makeJson( float val11, int vall2)
{
  log_d("\nMaking JSON text meanwhile\n");

  root["KWh"] = val11;
  root["count"] = vall2;
}


void loop()
{

  while (GSM.available()) {
    parseATText(GSM.read());
    delay(50);
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
    log_d(".");

  }
  log_d("Network registration OK");
  delay_count(1000);
  ////////////////////////
  /*  while (GPRSflag == false)    // to check GPRS connected or not and that depends on return value; 1 connected , 0 not connected
    {
      //  delay(500);
      sendGSM("AT+CREG?", 500);
    }*/

  log_d(" Getting time .........");
  sendGSM("AT+SAPBR=1,1");   // for activate bearer context
  delay_count (500);
  sendGSM("AT+CIPGSMLOC=2,1");    // for location and time
  delay_count(5000);
}

void Entersleepmode( unsigned long sleep)
{

  sendGSM("AT+CFUN=0");
  log_d( " Enter flight air mode for period = ");
  //log_d ( sleep / 60000);
  delay(sleep);
  sendGSM("AT+CFUN=1");
  delay_count(30000);
  sendGSM("AT+SAPBR=3,1,\"APN\",\"net.asiacell.com\"");

}

bool IsReady(uint16_t timeout)
{
  uint32_t timerStart = millis();
  sim_Ready_request = true;
  log_d( "trying to connect the SIM Model");
  while (!sim_Ready_flag )
  {

    sendGSM("AT", 1000);
    delay_count (500);
    Serial.print(".");
    // If timeout, abord the reading
    if (millis() - timerStart > timeout) {
      log_d( " The sim model is not responding, The reset must be taken");
      Reset();
      timerStart = millis();
    }


  }
  log_d("SIM Model is ready");
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
  log_d(" The model has been reset");
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
  log_d( " GPRS connected ");
}

void Sleep_estimation( )
{
  if (!IsReady(10000)) {
    log_d("The model is not responding");
  };

  while ( Timeget == false)
  {
    Gettime();
    log_d(" time is not calculated yet ");
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
  log_d(" *Time has been gotten * ");
  ////////////////////////////////////////////////


  if (   0 <= hour && hour < 3)
  {

    delay_hour = ((3 - hour) - 1) * 3600 * 1000;
    delay_mini = ((60 - mini) * 60) * 1000;

  }

  else if ( 3 <= hour && hour < 6)
  {

    delay_hour = ((6 - hour) - 1) * 3600 * 1000;
    delay_mini = ((60 - mini) * 60) * 1000;

  }

  else if ( 6 <= hour && hour < 9)
  {
    delay_hour = ((9 - hour) - 1) * 3600 * 1000;
    delay_mini = ((60 - mini) * 60) * 1000;


  }

  else if ( 9 <= hour && hour < 12)
  {
    delay_hour = ((12 - hour) - 1) * 3600 * 1000;
    delay_mini = ((60 - mini) * 60) * 1000;

  }

  else if ( 12 <= hour && hour < 15)
  {
    delay_hour = ((15 - hour) - 1) * 3600 * 1000;
    delay_mini = ((60 - mini) * 60) * 1000;

  }

  else if ( 15 <= hour && hour < 18)
  {
    delay_hour = ((18 - hour) - 1) * 3600 * 1000;
    delay_mini = ((60 - mini) * 60) * 1000;

  }

  else if ( 18 <= hour && hour < 21)
  {

    delay_hour = ((21 - hour) - 1) * 3600 * 1000;
    delay_mini = ((60 - mini) * 60) * 1000;

  }

  else if ( 21 <= hour && hour < 24)
  {

    delay_hour = ((24 - hour) - 1) * 3600 * 1000;
    delay_mini = ((60 - mini) * 60) * 1000;

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
  log_d(" The count = %d",count);
 // Serial.println ( count);
}


void save_Unit()
{

  EEPROM.writeULong(50, unit);
  EEPROM.commit();
  log_d(" The unit = %d", unit);
  //Serial.println ( unit);
  log_d("Total heap: %d", myESP.getHeapSize());

}

float heap_Checking()
{
  log_d("Total heap: %d", myESP.getHeapSize());
  log_d("Free heap: %d", myESP.getFreeHeap());
  log_d("Total PSRAM: %d", myESP.getPsramSize());
  log_d("Free PSRAM: %d", myESP.getFreePsram());

  float c = ((myESP.getHeapSize() - myESP.getFreeHeap()) / myESP.getHeapSize());

  return c;
}

void delay_count( unsigned long delayed)
{
  //Serial.println(analogRead(power_loss_pin_detection));

if (( analogRead(power_loss_pin_detection) < 400) &&( save== false)) {
     
    save_Count();
    save_Unit();
    save = true;
    delay(90000);
    save = false;
  }
    delay(2);

  unsigned long m = millis();
  while ( millis() - m < delayed)
  {
 // Serial.println(analogRead(power_loss_pin_detection));

    if ( ( analogRead(power_loss_pin_detection) < 400) &&( save== false)) {
     
    save_Count();
    save_Unit();
    save = true;
    delay(90000);
    save = false;
  }
    delay(2);
  }
  return ;
}


////////////////////////////////

void delay_core_rotation( unsigned long delayed)
{
  //Serial.println(analogRead(power_loss_pin_detection));

 if (( analogRead(power_loss_pin_detection) < 400) &&( save== false)) {
     
    save_Count();
    save_Unit();
    save = true;
    delay(90000);
    save = false;
  }
    delay(2);

  unsigned long m = millis();
  while ( millis() - m < delayed)
  {
 // Serial.println(analogRead(power_loss_pin_detection));

    if (( analogRead(power_loss_pin_detection) < 400) &&( save== false)) {
     
    save_Count();
    save_Unit();
    save = true;
    delay(90000);
    save = false;
  }

    
    delay(2);
}
  return ;
}
