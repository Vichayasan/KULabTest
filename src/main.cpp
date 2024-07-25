

#include <Arduino.h>
#include <EEPROM.h>
#include <ESPUI.h>

#include <AccelStepper.h>
#include <Wire.h>
#include <SPI.h>
// Define some steppers and the pins the will use
// AccelStepper stepper1; // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5

AccelStepper stepperX(AccelStepper::FULL2WIRE, 4, 2);   //Axis X
AccelStepper stepperY(AccelStepper::FULL2WIRE, 15, 13);   //Axis Y
AccelStepper stepperZ(AccelStepper::FULL2WIRE, 14, 27);     //Axis Z

#include <ESPmDNS.h>

//#include <Adafruit_ADS1X15.h>

#include "ADS1X15.h"


//#include <WebServer.h>
#include <SD.h>
#include "FS.h"
#include "SPI.h"
#include <time.h>
#include <WiFi.h>
#include <ESP32Time.h>
#include "HX711.h"

//Settings
#define SLOW_BOOT 0
#define HOSTNAME "KU.Physics.LAB"
#define FORCE_USE_HOTSPOT 0
#define trigWDTPin 32
#define ledHeartPIN 0


//UI handles
uint16_t wifi_ssid_text, wifi_pass_text;
uint16_t resultLabel, mainLabel, grouplabel, grouplabel2, mainSwitcher, mainSlider, mainText, settingZNumber, resultButton, mainTime, downloadButton, selectDownload;
uint16_t styleButton, styleLabel, styleSwitcher, styleSlider, styleButton2, styleLabel2, styleSlider2;
uint16_t nameText, loopText, posText, moveText, saveConfigButton;
uint16_t graph;
volatile bool updates = false;

String fileNameResult = "";

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 26;
const int LOADCELL_SCK_PIN = 25;
// REPLACE WITH YOUR CALIBRATION FACTOR
#define CALIBRATION_FACTOR 184.703557312253

#define zero_factor 58700

#define DEC_POINT  3
HX711 scale;
ADS1115 ads[4]; // Array to store ADS1115 objects
File myFile;
ESP32Time rtc;

//For VS code IDE
void generalCallback(Control *sender, int type);
void startButtonCallback(Control *sender, int type);
void loadResultCallback(Control *sender, int type);
void downloadCallback(Control *sender, int type);
void moveAxisXY(Control *sender, int type);
void posButtonCallback(Control *sender, int type);
void setTextInputCallback(Control *sender, int type);
void configButtonCallback(Control *sender, int type);
void enterWifiDetailsCallback(Control * sender, int type);
void randomString(char *buf, int len);
void connectWifi();

String yearStr = "";
String monthStr = "";
String dayStr = "";
String hourStr = "";
String minStr = "";
String secStr = "";

String record = "";

//WebServer server(80);
//String htmlView = "";
struct tm tmstruct;
// WiFi credentials
const char *ssid = "TP-Link_AD28";  //TP-Link_AD28
const char *password = "96969755"; //96969755




String dateTimeStr = "";
long timezone = 7;
byte daysavetime = 0;
int testCount = 1;
int loopCount = 1;

unsigned long diffMillis = 0;
unsigned long diff = 0;
const int limit = 1000;
int axisX = 1;
int axisY = 1;
int axisZ = 1;

int x = 0;
int y = 0;

int operatingPercen = 0;
TaskHandle_t Task1;
TaskHandle_t Task2;
TaskHandle_t Task3;
boolean isStopStart = false;
String fileName = "/KUPhysicsLabTester_";

int previousX = 0;
int previousY = 0;
int previousZ = 0;
// Define a struct to hold configuration parameters

struct Config01 {
    String name;
    int loop;
    int numPos;
    int **pos; // Dynamically allocated array
};

struct Config02 {
    String name;
    int loop;
    int numPos;
    int **pos; // Dynamically allocated array
};

struct Config03 {
    String name;
    int loop;
    int numPos;
    int **pos; // Dynamically allocated array
};

Config01 config01;
Config02 config02;
Config03 config03;

void heartBeat()
{
  //   Sink current to drain charge from watchdog circuit
  pinMode(trigWDTPin, OUTPUT);
  digitalWrite(trigWDTPin, LOW);

  // Led monitor for Heartbeat
  digitalWrite(ledHeartPIN, LOW);
  delay(100);
  digitalWrite(ledHeartPIN, HIGH);

  // Return to high-Z
  pinMode(trigWDTPin, INPUT);

  Serial.println("Heartbeat");
  // SerialBT.println("Heartbeat");
}

void split(const String &str, char delimiter, int **arr, int row) {
    int startIndex = 0;
    int endIndex = str.indexOf(delimiter);
    
    arr[row][0] = str.substring(startIndex, endIndex).toInt();
    startIndex = endIndex + 1;
    endIndex = str.indexOf(delimiter, startIndex);
    arr[row][1] = str.substring(startIndex, endIndex).toInt();
    startIndex = endIndex + 1;
    arr[row][2] = str.substring(startIndex).toInt();
}

// Function to read configuration data from file and populate the Config struct
bool readConfig(const String &filename, Config01 &config01, Config02 &config02, Config03 &config03) {
    // Initialize SD card
    if (!SD.begin()) {
        Serial.println("SD card initialization failed.");
        return false;
    }

    // Open the config file
    File configFile = SD.open(filename);
    if (!configFile) {
        Serial.println("Config file not found.");
        return false;
    }

    // Read the contents of the config file line by line
    String line;
    int rows = 0;
    int **arr = nullptr;

    if (filename.equals("/test01.config")) {
        rows = 9;
        config01.pos = new int*[rows];
        for (int i = 0; i < rows; ++i) {
            config01.pos[i] = new int[3]();
        }
        arr = config01.pos;
    } else if (filename.equals("/test02.config")) {
        rows = 1;
        config02.pos = new int*[rows];
        for (int i = 0; i < rows; ++i) {
            config02.pos[i] = new int[3]();
        }
        arr = config02.pos;
    } else if (filename.equals("/test03.config")) {
        rows = 990;
        config03.pos = new int*[rows];
        for (int i = 0; i < rows; ++i) {
            config03.pos[i] = new int[3]();
        }
        arr = config03.pos;
    }

    Serial.println("FileconfigName: " + filename);

    while (configFile.available()) {
        line = configFile.readStringUntil('\n');
        line.trim();
        if (line.startsWith("Name=")) {
            if (filename.equals("/test01.config")) {
                config01.name = line.substring(5); // Extract the name from the line
                Serial.println("Config name: " + config01.name);
                ESPUI.updateControlValue(nameText, config01.name);
            } else if (filename.equals("/test02.config")) {
                config02.name = line.substring(5); // Extract the name from the line
                Serial.println("Config name: " + config02.name);
                ESPUI.updateControlValue(nameText, config02.name);
            } else if (filename.equals("/test03.config")) {
                config03.name = line.substring(5); // Extract the name from the line
                Serial.println("Config name: " + config03.name);
                ESPUI.updateControlValue(nameText, config03.name);
            }
        } else if (line.startsWith("Loop=")) {
            if (filename.equals("/test01.config")) {
                config01.loop = line.substring(5).toInt(); // Convert loop value to integer
                Serial.println("Loop value: " + String(config01.loop));
                ESPUI.updateControlValue(loopText, String(config01.loop));
            } else if (filename.equals("/test02.config")) {
                config02.loop = line.substring(5).toInt(); // Convert loop value to integer
                Serial.println("Loop value: " + String(config02.loop));
                ESPUI.updateControlValue(loopText, String(config02.loop));
            } else if (filename.equals("/test03.config")) {
                config03.loop = line.substring(5).toInt(); // Convert loop value to integer
                Serial.println("Loop value: " + String(config03.loop));
                ESPUI.updateControlValue(loopText, String(config03.loop));
            }
        } else if (line.startsWith("Pos=")) {
            if (filename.equals("/test01.config")) {
                config01.numPos = line.substring(4).toInt(); // Convert numPos value to integer
                Serial.println("Number of positions: " + String(config01.numPos));
                ESPUI.updateControlValue(posText, String(config01.numPos));
            } else if (filename.equals("/test02.config")) {
                config02.numPos = line.substring(4).toInt(); // Convert numPos value to integer
                Serial.println("Number of positions: " + String(config02.numPos));
                ESPUI.updateControlValue(posText, String(config02.numPos));
            } else if (filename.equals("/test03.config")) {
                config03.numPos = line.substring(4).toInt(); // Convert numPos value to integer
                Serial.println("Number of positions: " + String(config03.numPos));
                ESPUI.updateControlValue(posText, String(config03.numPos));
            }
        } else if (line.startsWith("Move=")) {
            Serial.println("Move positions:");
            if (filename.equals("/test01.config")) {
                for (int i = 0; i < config01.numPos; i++) {
                    line = configFile.readStringUntil('\n');
                    split(line, ',', config01.pos, i);
                    Serial.print(config01.pos[i][0]);
                    Serial.print(",");
                    Serial.print(config01.pos[i][1]);
                    Serial.print(",");
                    Serial.println(config01.pos[i][2]);
                }
            } else if (filename.equals("/test02.config")) {
                for (int i = 0; i < config02.numPos; i++) {
                    line = configFile.readStringUntil('\n');
                    split(line, ',', config02.pos, i);
                    Serial.print(config02.pos[i][0]);
                    Serial.print(",");
                    Serial.print(config02.pos[i][1]);
                    Serial.print(",");
                    Serial.println(config02.pos[i][2]);
                }
            } else if (filename.equals("/test03.config")) {
                for (int i = 0; i < config03.numPos; i++) {
                    line = configFile.readStringUntil('\n');
                    split(line, ',', config03.pos, i);
                    Serial.print(config03.pos[i][0]);
                    Serial.print(",");
                    Serial.print(config03.pos[i][1]);
                    Serial.print(",");
                    Serial.println(config03.pos[i][2]);
                }
            }
        }
    }
    configFile.close();

    return true;
}

// Function to clean up dynamically allocated memory
void cleanUp(Config01 &config01, Config02 &config02, Config03 &config03) {
    if (config01.pos) {
        for (int i = 0; i < 9; ++i) {
            delete[] config01.pos[i];
        }
        delete[] config01.pos;
    }
    if (config02.pos) {
        for (int i = 0; i < 1; ++i) {
            delete[] config02.pos[i];
        }
        delete[] config02.pos;
    }
    if (config03.pos) {
        for (int i = 0; i < 961; ++i) {
            delete[] config03.pos[i];
        }
        delete[] config03.pos;
    }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  int countFile = 0;

  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root)
  {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  fileNameResult = "16:36:48.365 -> server.log.2023-07-25Nserver.log.2023-09-26Nserver.log.2023-09-23Nserver.log.2023-09-22Nserver.log.2023-09-21Nserver.log.2023-09-18Nserver.log.2023-09-08Nserver.log.2023-09-04Nserver.log.2023-09-01Nserver.log.2023-08-30Nserver.log.2023-08-29Nserver.log.2023-08-16Nserver.log.2023-08-15Nserver.log.2023-08-10Nserver.log.2023-07-31Ntest.txtNfoo.txtNtest01.configN";
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.print(file.name());

      time_t t = file.getLastWrite();
      struct tm *tmstruct = localtime(&t);
      Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
      if (levels)
      {
        listDir(fs, file.name(), levels - 1);
      }
    }
    else
    {

      Serial.print("  FILE: ");
      Serial.print(file.name());
      fileNameResult.concat(file.name());
      //      fileNameResult.concat("\\N");

      Serial.print("  SIZE: ");
      Serial.print(file.size());
      time_t t = file.getLastWrite();
      struct tm *tmstruct = localtime(&t);
      Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char *path)
{
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path))
  {
    Serial.println("Dir created");
  }
  else
  {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char *path)
{
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path))
  {
    Serial.println("Dir removed");
  }
  else
  {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }


  while (file.available())
  {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("File written");
  }
  else
  {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message)
{
  // Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file)
  {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message))
  {
    // Serial.println("Message appended");
  }
  else
  {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2)
{
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2))
  {
    Serial.println("File renamed");
  }
  else
  {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path)
{
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path))
  {
    Serial.println("File deleted");
  }
  else
  {
    Serial.println("Delete failed");
  }
}


String a0(int n)
{
  return (n < 10) ? "0" + String(n) : String(n);
}
String a00(int n)
{
  return (n < 100) ? "0" + String(n) : String(n);
}

float readLoadCell()
{


  float reading = 0;
  if (scale.is_ready()) {
    reading = scale.get_units(1);
    //    Serial.print("HX711 reading: ");
    //    Serial.println(reading);
    if (reading < 10 || reading > -10){
      reading = 0;
    }
    
  } else {
    Serial.println("HX711 not found.");
  }

  
  return reading;
}

void moveLeft()
{
  // Serial.println("moveLeft");
  // delay(100);
}

void moveRight()
{
  // Serial.println("moveRight");
  // delay(100);
}

void lift()
{
  Serial.println("lift");
  // delay(100);
}
void down()
{
  Serial.println("down");//
  // delay(100);
}

void press()
{

}
void armLogic()
{
  lift();
  moveRight();
  down();
  Serial.println(config01.numPos);


}

boolean isStarted()
{

  return true;
}

void textCallback(Control *sender, int type) {
  //This callback is needed to handle the changed values, even though it doesn't do anything itself.
}
void moveAxisZ(Control* sender, int type)
{
  int currentZ = String(sender->value).toInt();
  currentZ = currentZ * (axisZ * 10);
  Serial.println("moveAxisZ");
  Serial.println(currentZ);

  if (currentZ > previousZ) {
    Serial.println("up");
    stepperZ.moveTo(100);
    stepperZ.runToPosition();
    stepperZ.setCurrentPosition(0);

  }
  if (currentZ < previousZ ) {
    Serial.println("down");
    stepperZ.moveTo(-100);
    stepperZ.runToPosition();
    stepperZ.setCurrentPosition(0);
  }
  previousZ = currentZ;
}


// This is the main function which builds our GUI
void setUpUI() {

  //Turn off verbose  ging
  ESPUI.setVerbosity(Verbosity::Quiet);

  //Make sliders continually report their position as they are being dragged.
  ESPUI.sliderContinuous = true;

  //This GUI is going to be a tabbed GUI, so we are adding most controls using ESPUI.addControl
  //which allows us to set a parent control. If we didn't need tabs we could use the simpler add
  //functions like:
  //    ESPUI.button()
  //    ESPUI.label()
  String clearLabelStyle = "background-color: unset; width: 100%;";

  /*
     Tab: Basic Controls
     This tab contains all the basic ESPUI controls, and shows how to read and update them at runtime.
    -----------------------------------------------------------------------------------------------------------*/
  auto maintab = ESPUI.addControl(Tab, "", "Test Plan");

  ESPUI.addControl(Separator, "General controls", "", None, maintab);
  mainLabel = ESPUI.addControl(Label, "Processing", String(loopCount), Emerald, maintab, generalCallback);


  //  mainSwitcher = ESPUI.addControl(Switcher, "Switcher", "", Sunflower, maintab, generalCallback);
  grouplabel = ESPUI.addControl(Label, "Start/Stop", "File name", Dark, maintab);

  grouplabel2 = ESPUI.addControl(Label, "", "filename", Emerald, grouplabel);

  ESPUI.addControl(Switcher, "", "1", Sunflower, grouplabel, startButtonCallback);
  ESPUI.setElementStyle(grouplabel2, "font-size: x-large; font-family: serif;");





  auto resulttab = ESPUI.addControl(Tab, "", "Results");

  /*
     Tab: Results
     This tab shows how multiple control can be grouped into the same panel through the use of the
     parentControl value. This also shows how to add labels to grouped controls, and how to use vertical controls.
      -----------------------------------------------------------------------------------------------------------*/
  ESPUI.addControl(Separator, "Result download", "", None, resulttab);




  //The parent of this button is a tab, so it will create a new panel with one control.
  auto groupbutton = ESPUI.addControl(Button, "Reload results", "Reload", Dark, resulttab, loadResultCallback);



  resultLabel = ESPUI.addControl(Label, "Result", String(fileName), Emerald, resulttab, loadResultCallback);
  selectDownload = ESPUI.addControl( ControlType::Select, "Select Title", "", Emerald, resultLabel );

  downloadButton = ESPUI.addControl(Button, "", "Download", Dark, resultLabel, downloadCallback);


  ESPUI.updateLabel(resultLabel, String(fileNameResult));



  /*
     Tab: Example UI
     An example UI for the documentation

    -----------------------------------------------------------------------------------------------------------*/

  auto settingTab = ESPUI.addControl(Tab, "Setting", "Setting");
  //  ESPUI.addControl(Separator, "Control and Status", "", None, settingTab);
  //  ESPUI.addControl(Switcher, "Power", "1", Alizarin, settingTab, generalCallback);
  //  ESPUI.addControl(Label, "Status", "System status: OK", Wetasphalt, settingTab, generalCallback);

  //  ESPUI.addControl(Separator, "Settings", "", None, settingTab);
  //  ESPUI.addControl(PadWithCenter, "Attitude Control", "Axis", Dark, settingTab, generalCallback);
  //  auto examplegroup1 = ESPUI.addControl(Button, "Activate Features", "Feature A", Carrot, settingTab, generalCallback);
  //  ESPUI.addControl(Button, "Activate Features", "Feature B", Carrot, examplegroup1, generalCallback);
  //  ESPUI.addControl(Button, "Activate Features", "Feature C", Carrot, examplegroup1, generalCallback);
  //  ESPUI.addControl(Slider, "Value control", "45", Peterriver, settingTab, generalCallback);

  ESPUI.addControl(Separator, "Calibrating Axis", "", None, settingTab);
  //  ESPUI.addControl(Pad, "Normal", "", Peterriver, settingTab, generalCallback);
  ESPUI.addControl(PadWithCenter, "Axis X,Y", "", Peterriver, settingTab, moveAxisXY);

  //Number inputs also accept Min and Max components, but you should still validate the values.
  settingZNumber = ESPUI.addControl(Number, "Axis Z", String(axisZ), Emerald, settingTab, &moveAxisZ);

  ESPUI.addControl(Min, "", "-50", None, settingZNumber);
  ESPUI.addControl(Max, "", "50", None, settingZNumber);

  ESPUI.addControl(Separator, "Setting Config File", "", None, settingTab);

  nameText = ESPUI.addControl(ControlType::Text, "Name", "", ControlColor::None, settingTab, setTextInputCallback);
  loopText = ESPUI.addControl(ControlType::Text, "Loop", "", ControlColor::None, settingTab, setTextInputCallback);

  posText = ESPUI.addControl(Select, "Pattern", "", None, settingTab, setTextInputCallback);
  ESPUI.addControl(Option, "3x3", "1", None, posText, setTextInputCallback);
  ESPUI.addControl(Option, "Center Point", "2", None, posText, setTextInputCallback);
  ESPUI.addControl(Option, "30x30", "3", None, posText, setTextInputCallback);
  
  moveText = ESPUI.addControl(ControlType::Text, "Move", "", ControlColor::None, settingTab, setTextInputCallback);

  saveConfigButton = ESPUI.addControl(ControlType::Button, "Save Config", "Save", ControlColor::None, settingTab, configButtonCallback);



  /*
     Tab: WiFi Credentials
     You use this tab to enter the SSID and password of a wifi network to autoconnect to.
    -----------------------------------------------------------------------------------------------------------*/
  auto wifitab = ESPUI.addControl(Tab, "", "WiFi Credentials");
  wifi_ssid_text = ESPUI.addControl(Text, "SSID", "", Alizarin, wifitab, textCallback);
  //Note that adding a "Max" control to a text control sets the max length
  ESPUI.addControl(Max, "", "32", None, wifi_ssid_text);
  wifi_pass_text = ESPUI.addControl(Text, "Password", "", Alizarin, wifitab, textCallback);
  ESPUI.addControl(Max, "   ", "64", None, wifi_pass_text);
  ESPUI.addControl(Button, "Save", "Save", Peterriver, wifitab, enterWifiDetailsCallback);
  String printValue = "Download<script>document.getElementById('btn12').onclick = function() {location.replace('/file?filename=/' + document.querySelector('#select11').value)};fetch('/filelist').then(function(data){return data.text()}).then(function(text){console.log(text.split('\\n').forEach(function(i){var option = document.createElement('option');option.value = i;option.innerText = i;document.querySelector('#select11').append(option);}))})</script>";
  ESPUI.print(downloadButton, printValue);

  //Finally, start up the UI.
  //This should only be called once we are connected to WiFi.
  ESPUI.begin(HOSTNAME);

}

//This callback generates and applies inline styles to a bunch of controls to change their colour.

void setTextInputCallback(Control *sender, int type) {
  Serial.println(sender->value);
  ESPUI.updateControl(sender);
  /*Control* pos_ = ESPUI.getControl(posText);
  Serial.println("posText: " + pos_->value);*/
  
}

void configButtonCallback(Control *sender, int type) {
  /*String file = "";
  if (type == B_UP) {
    Control* name_ = ESPUI.getControl(nameText);
    //  ESPUI.updateControl(nameText);
    Control* loop_ = ESPUI.getControl(loopText);
    //  ESPUI.updateControl(loopText);
    Control* pos_ = ESPUI.getControl(posText);
    //  ESPUI.updateControl(posText);
    file += "Name=";
    file += name_->value;
    file += "\n";
    file += "Loop=";
    file += loop_->value;
    file += "\n";
    file += "Pos=";
    file += pos_->value;
    file += "\n";
    file += "Move=\n";
    file += "-1000,-1000,-4\n1000,0,-4\n1000,0,-4\n0,1000,-4\n-1000,0,-4\n-1000,0,-4\n0,1000,-4\n1000,0,-4\n1000,0,-4\nEnd";
    Serial.println(file);
    writeFile(SD, "/test01.config", file.c_str());
    config.name = name_->value;
    config.loop = loop_->value.toInt();
    config.numPos = pos_->value.toInt();
  }*/
}

void styleCallback(Control *sender, int type) {
  //Declare space for style strings. These have to be static so that they are always available
  //to the websocket layer. If we'd not made them static they'd be allocated on the heap and
  //will be unavailable when we leave this function.
  static char stylecol1[60], stylecol2[30];
  if (type == B_UP) {
    //Generate two random HTML hex colour codes, and print them into CSS style rules
    sprintf(stylecol1, "border-bottom: #999 3px solid; background-color: #%06X;", (unsigned int) random(0x0, 0xFFFFFF));
    sprintf(stylecol2, "background-color: #%06X;", (unsigned int) random(0x0, 0xFFFFFF));

    //Apply those styles to various elements to show how controls react to styling
    ESPUI.setPanelStyle(styleButton, stylecol1);
    ESPUI.setElementStyle(styleButton, stylecol2);
    ESPUI.setPanelStyle(styleLabel, stylecol1);
    ESPUI.setElementStyle(styleLabel, stylecol2);
    ESPUI.setPanelStyle(styleSwitcher, stylecol1);
    ESPUI.setElementStyle(styleSwitcher, stylecol2);
    ESPUI.setPanelStyle(styleSlider, stylecol1);
    ESPUI.setElementStyle(styleSlider, stylecol2);
  }
}


//This callback updates the "values" of a bunch of controls
void scrambleCallback(Control *sender, int type) {
  static char rndString1[10];
  static char rndString2[20];
  static bool scText = false;

  if (type == B_UP) { //Button callbacks generate events for both UP and DOWN.
    //Generate some random text
    randomString(rndString1, 10);
    randomString(rndString2, 20);

    //Set the various controls to random value to show how controls can be updated at runtime

    ESPUI.updateSwitcher(mainSwitcher, ESPUI.getControl(mainSwitcher)->value.toInt() ? false : true);
    ESPUI.updateSlider(mainSlider, random(10, 400));
    ESPUI.updateText(mainText, String(rndString2));
    ESPUI.updateNumber(settingZNumber, random(100000));
    ESPUI.updateButton(resultButton, scText ? "Scrambled!" : "Scrambled.");
    scText = !scText;
  }
}

void updateCallback(Control *sender, int type) {
  updates = (sender->value.toInt() > 0);
}

void getTimeCallback(Control *sender, int type) {
  if (type == B_UP) {
    ESPUI.updateTime(mainTime);
  }
}

void graphAddCallback(Control *sender, int type) {
  if (type == B_UP) {
    ESPUI.addGraphPoint(graph, random(1, 50));
  }
}

void graphClearCallback(Control *sender, int type) {
  if (type == B_UP) {
    ESPUI.clearGraph(graph);
  }
}

void downloadCallback(Control *sender, int type) {
  //  Control* labelControl = ESPUI.getControl(grouplabel2);
  //  sender->callback = nullptr;  // Prevent infinite loop
  //  //ESPUI.print(sender->id, redirectScript);
  //  sender->callback = downloadCallback;  // Restore the callback
}

void startButtonCallback(Control *sender, int type) {
  Serial.print("CB: id(");
  Serial.print(sender->id);
  Serial.print(") Type(");
  Serial.print(type);
  Serial.print(") '");
  Serial.print(sender->label);
  Serial.print("' = ");
  Serial.println(sender->value);
  boolean isPress = String(sender->value).toInt();
  Serial.print("isPress:"); Serial.println(isPress);
  if (isPress)
    isStopStart = true;
  else
    isStopStart = false;

}
//Most elements in this test UI are assigned this generic callback which prints some
//basic information. Event types are defined in ESPUI.h
void generalCallback(Control *sender, int type) {
  Serial.print("CB: id(");
  Serial.print(sender->id);
  Serial.print(") Type(");
  Serial.print(type);
  Serial.print(") '");
  Serial.print(sender->label);
  Serial.print("' = ");
  Serial.println(sender->value);
}
void loadResultCallback(Control *sender, int type) {
  Serial.print("CB: id(");
  Serial.print(sender->id);
  Serial.print(") Type(");
  Serial.print(type);
  Serial.print(") '");
  Serial.print(sender->label);
  Serial.print("' = ");
  Serial.println(sender->value);
  ESPUI.updateLabel(grouplabel2, String(fileName));
  ESPUI.updateControl(resultLabel);
  //  listDir(SD, "/", 0);
  // readFile(SD, fileName.c_str());
  //  readFile(SD, "/server.log.2023-07-31");
  record.concat("Date/Time,Loop,Round,Value1,Value2,Value3,Value4,Value5,Value6,Value7,Value8,Value9,Value10,Value11,Value12,LoadCell\n");
  appendFile(SD, fileNameResult.c_str(), record.c_str());
  record = "";
  Serial.println("Result:");
  Serial.println(fileNameResult);


}
void moveAxisXY(Control *sender, int type) {
  Serial.println("moveAxisXY");
  Serial.print("CB: id(");
  Serial.print(sender->id);
  Serial.print(") Type(");
  Serial.print(type);
  Serial.print(") '");
  Serial.println(sender->label);


  if (4 == abs(type)) {
    Serial.println("up");
    stepperY.moveTo(100);
    stepperY.runToPosition();
    stepperY.setCurrentPosition(0);

  }
  if (3 == abs(type) ) {
    Serial.println("right");
    stepperX.moveTo(-100);
    stepperX.runToPosition();
    stepperX.setCurrentPosition(0);
  }


  if (2 == abs(type)) {
    Serial.println("left");
    stepperX.moveTo(100);
    stepperX.runToPosition();
    stepperX.setCurrentPosition(0);

  }
  if (5 == abs(type) ) {
    Serial.println("down");
    stepperY.moveTo(-100);
    stepperY.runToPosition();
    stepperY.setCurrentPosition(0);
  }



}

// Most elements in this test UI are assigned this generic callback which prints some
// basic information. Event types are defined in ESPUI.h
// The extended param can be used to hold a pointer to additional information
// or for C++ it can be used to return a this pointer for quick access
// using a lambda function
void extendedCallback(Control* sender, int type, void* param)
{
  Serial.print("CB: id(");
  Serial.print(sender->id);
  Serial.print(") Type(");
  Serial.print(type);
  Serial.print(") '");
  Serial.print(sender->label);
  Serial.print("' = ");
  Serial.println(sender->value);
  Serial.println(String("param = ") + String((int)param));
}

// Task1code: blinks an LED every 1000 ms
void Task1code(void *pvParameters)
{

  for (;;)
  {
    //    Serial.print("Task1 running on core ");
    //    Serial.println(xPortGetCoreID());
    //    heartBeat();
    vTaskDelay((2000) / portTICK_PERIOD_MS);
  }
}

// Task2code: blinks an LED every 700 ms
void Task2code(void *pvParameters)
{
  // Serial.print("Task2 running on core ");
  // Serial.println(xPortGetCoreID());
  //  Serial.println(ESP.getFreeHeap());
  for (;;)
  {
  }
}

// Task2code: blinks an LED every 700 ms
void Task3code(void *pvParameters)
{

  for (;;)
  {
    //    if (isStopStart && (testCount <= config.loop) ) {
    //
    //
    //
    //      //      diffMillis = millis();
    //      //      Serial.println(); // Move to the next row
    //      Serial.println(fileName);
    //
    //    }
    //    vTaskDelay((1000) / portTICK_PERIOD_MS);
  }
}


void setup() {


  randomSeed(0);
  Serial.begin(115200);
  while (!Serial);
  if (SLOW_BOOT) delay(5000); //Delay booting to give time to connect a serial monitor
  connectWifi();
#if defined(ESP32)
  WiFi.setSleep(false); //For the ESP32: turn off sleeping to increase UI responsivness (at the cost of power use)
#endif
  setUpUI();

  Wire.begin();
  Wire.setClock(800000); // Set I2C clock frequency to 400 kHz
  Serial.println("Getting differential reading from AIN0 (P) and AIN1 (N)");
  Serial.println("+/- 1.024V  1 bit = 0.5mV    0.03125mV");

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Contacting Time Server");
  configTime(3600 * timezone, daysavetime * 3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");

  delay(2000);
  tmstruct.tm_year = 0;
  getLocalTime(&tmstruct, 5000);
  yearStr = String(tmstruct.tm_year + 1900, DEC);
  monthStr = String(tmstruct.tm_mon + 1, DEC);
  dayStr = String(tmstruct.tm_mday, DEC);
  hourStr = String(a0(tmstruct.tm_hour));
  minStr = String(a0(tmstruct.tm_min));
  secStr = String(a0(tmstruct.tm_sec));
  fileName.concat(yearStr);
  fileName.concat(monthStr);
  fileName.concat(dayStr);
  fileName.concat(hourStr);
  fileName.concat(minStr);
  fileName.concat(".csv");
  Serial.printf("\nNow is : %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct.tm_year) + 1900, (tmstruct.tm_mon) + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);
  Serial.println("");

  if (!SD.begin())
  {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE)
  {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC)
  {
    Serial.println("MMC");
  }
  else if (cardType == CARD_SD)
  {
    Serial.println("SDSC");
  }
  else if (cardType == CARD_SDHC)
  {
    Serial.println("SDHC");
  }
  else
  {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);





  ads[0].setGain(3); // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  ads[0].setDataRate(4);
  ads[0].begin();

  ads[1].setGain(3); // 4x gain   +/- 1._024V  1 bit = 0.5mV    0.03125mV
  ads[1].setDataRate(4);
  ads[1].begin();

  ads[2].setGain(3); // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  ads[2].setDataRate(4);
  ads[2].begin();

  ads[3].setGain(3); // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  ads[3].setDataRate(4);
  ads[3].begin();
  diff = millis();

  // Initialize SD card
  //  if (!SD.begin())
  //  {
  //    Serial.println("SD Card initialization failed!");
  //    return;
  //  }
  //  Serial.println("SD Card initialized");


  //  xTaskCreate(Task1code, "Task1", 10000, NULL, tskIDLE_PRIORITY, NULL);
  //
  //  xTaskCreate(Task2code, "Task2", 10000, NULL, tskIDLE_PRIORITY, NULL);
  //  xTaskCreate(Task3code, "Task3", 20000, NULL, 1, NULL);



  stepperX.setMaxSpeed(4000);
  stepperX.setAcceleration(6000);
  stepperX.setCurrentPosition(0);

  stepperY.setMaxSpeed(4000);
  stepperY.setAcceleration(6000);
  stepperY.setCurrentPosition(0);

  stepperZ.setMaxSpeed(4000);
  stepperZ.setAcceleration(6000);
  stepperZ.setCurrentPosition(0);


  if (readConfig("/test01.config", config01, config02, config03)) {
    // Print configuration data

  }

  if (readConfig("/test02.config", config01, config02, config03)) {
    // Print configuration data

  }

  if (readConfig("/test03.config", config01, config02, config03)) {
    // Print configuration data

  }


  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  scale.set_scale(CALIBRATION_FACTOR); // this value is obtained by calibrating the scale with known weights
  scale.set_offset(zero_factor);
  scale.tare();
  

  Serial.print("Free heap memory: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");

}

void moveToStart() {
  //  Serial.println("move2start");
  
  Control* pos_ = ESPUI.getControl(posText);

  if (pos_->value.equals("1")){ //20x20 mm.
    stepperX.moveTo(-1000);
    stepperX.runToPosition();
    stepperX.setCurrentPosition(0);

    stepperY.moveTo(-1000);
    stepperY.runToPosition();
    stepperY.setCurrentPosition(0);
  }

  else if (pos_->value.equals("2")){
    stepperX.moveTo(0);
    stepperX.runToPosition();
    stepperX.setCurrentPosition(0);

    stepperY.moveTo(0);
    stepperY.runToPosition();
    stepperY.setCurrentPosition(0);
  }

  else if (pos_->value.equals("3")){ //300x300 mm.

    for (int x = 0; x < 15; x++){
      stepperX.moveTo(-1000);
      stepperX.runToPosition();
      stepperX.setCurrentPosition(0);
    }

    for (int x = 0; x < 15; x++){
      stepperY.moveTo(-1000);
      stepperY.runToPosition();
      stepperY.setCurrentPosition(0);
      }
    

  }

}

void loop() {
  static long unsigned lastTime = 0;

  //Send periodic updates if switcher is turned on
  if (updates && millis() > lastTime + 500) {
    static uint16_t sliderVal = 10;

    //Flick this switcher on and off
    ESPUI.updateSwitcher(mainSwitcher, ESPUI.getControl(mainSwitcher)->value.toInt() ? false : true);

    lastTime = millis();
  }

  //Simple   UART interface
  if (Serial.available()) {
    switch (Serial.read()) {
      case 'w': //Print IP details
        Serial.println(WiFi.localIP());
        break;
      case 'W': //Reconnect wifi
        connectWifi();
        break;
      case 'C': //Force a crash (for testing exception decoder)
#if !defined(ESP32)
        ((void (*)())0xf00fdead)();
#endif
      default:
        break;
    }
  }

#if !defined(ESP32)
  //We don't need to call this explicitly on ESP32 but we do on 8266
  MDNS.update();
#endif
  if (isStopStart && (loopCount <= 100) ) {
    //    Serial.print("loopCount: ");
    //    Serial.println(loopCount);
    //    Serial.print("Loop: ");
    //    Serial.println(config.loop);

    yearStr = String(tmstruct.tm_year + 1900, DEC);
    monthStr = String(tmstruct.tm_mon + 1, DEC);


    Control* pos_ = ESPUI.getControl(posText);
    
    if (pos_->value.equals("1")){  //20x20 mm

      Serial.println("test count: " + testCount);

    for ( int x = 0; x < config01.numPos; x++) {


      int _value = 0;
            Serial.print("x:");
            Serial.print(config01.pos[x][0]);
      stepperX.moveTo(config01.pos[x][0]);
      stepperX.runToPosition();
      stepperX.setCurrentPosition(0);


            Serial.print("y:");
            Serial.print(config01.pos[x][1]);
      stepperY.moveTo(config01.pos[x][1]);
      stepperY.runToPosition();
      stepperY.setCurrentPosition(0);
      //      Serial.println("");

      int depthPress = abs(config01.pos[0][2]);

            Serial.println("press...");

          
      getLocalTime(&tmstruct, 5000);
      dayStr = String(tmstruct.tm_mday, DEC);
      hourStr = String(a0(tmstruct.tm_hour));
      minStr = String(a0(tmstruct.tm_min));
      secStr = String(a0(tmstruct.tm_sec));
      dateTimeStr.concat(dayStr);
      dateTimeStr.concat("/");
      dateTimeStr.concat(monthStr);
      dateTimeStr.concat("/");
      dateTimeStr.concat(yearStr);
      dateTimeStr.concat(" ");
      dateTimeStr.concat(hourStr);
      dateTimeStr.concat(":");
      dateTimeStr.concat(minStr);
      dateTimeStr.concat(":");
      dateTimeStr.concat(secStr);
      dateTimeStr.concat(",");

      record.concat(dateTimeStr.c_str());

      record.concat(loopCount);
      record.concat(",");
      record.concat(testCount);
      record.concat(",");
      testCount++;


      //  Serial.println(depthPress);
      stepperZ.moveTo(-400);
      stepperZ.runToPosition();
      stepperZ.setCurrentPosition(0);


      
      for (int i = 0; i < 4; i++)
      {

        for (int j = 0; j < 3; j++)
        {


          int16_t raw00 = ads[i].readADC(j);



          float volt = ads[i].toVoltage(raw00);

          //          Serial.print(volt); Serial.print(",");
          record.concat(String(volt, 3));
          record.concat(",");
        }
      }
      //      Serial.println("");
      
 
      
      stepperZ.moveTo(400);
      stepperZ.runToPosition();
      stepperZ.setCurrentPosition(0);
      
      
      record.concat(readLoadCell());
      scale.tare(); // reset the scale to 0
      record.concat("\n");
      



      appendFile(SD, fileName.c_str(), record.c_str());



      Serial.println(record);


      dateTimeStr = "";
      record = "";


    if (testCount > config01.numPos) {
     
      testCount = 1;
      moveToStart();

    }
          } 
    }

    else if (pos_->value.equals("2")){

    for ( int x = 0; x < config02.numPos; x++) {


      int _value = 0;
            Serial.print("x:");
            Serial.print(config02.pos[x][0]);
      stepperX.moveTo(config02.pos[x][0]);
      stepperX.runToPosition();
      stepperX.setCurrentPosition(0);


            Serial.print("y:");
            Serial.print(config02.pos[x][1]);
      stepperY.moveTo(config02.pos[x][1]);
      stepperY.runToPosition();
      stepperY.setCurrentPosition(0);
      //      Serial.println("");

      int depthPress = abs(config02.pos[0][2]);

      //      Serial.println("press...");
      getLocalTime(&tmstruct, 5000);
      dayStr = String(tmstruct.tm_mday, DEC);
      hourStr = String(a0(tmstruct.tm_hour));
      minStr = String(a0(tmstruct.tm_min));
      secStr = String(a0(tmstruct.tm_sec));
      dateTimeStr.concat(dayStr);
      dateTimeStr.concat("/");
      dateTimeStr.concat(monthStr);
      dateTimeStr.concat("/");
      dateTimeStr.concat(yearStr);
      dateTimeStr.concat(" ");
      dateTimeStr.concat(hourStr);
      dateTimeStr.concat(":");
      dateTimeStr.concat(minStr);
      dateTimeStr.concat(":");
      dateTimeStr.concat(secStr);
      dateTimeStr.concat(",");

      record.concat(dateTimeStr.c_str());

      record.concat(loopCount);
      record.concat(",");
      record.concat(testCount);
      record.concat(",");
      testCount++;

      //  Serial.println(depthPress);
      stepperZ.moveTo(-400);
      stepperZ.runToPosition();
      stepperZ.setCurrentPosition(0);

      delay(3000);
      
      for (int i = 0; i < 4; i++)
      {

        for (int j = 0; j < 3; j++)
        {


          int16_t raw00 = ads[i].readADC(j);



          float volt = ads[i].toVoltage(raw00);

          //          Serial.print(volt); Serial.print(",");
          record.concat(String(volt, 3));
          record.concat(",");
        }
      }
      
      
      //      Serial.println("");
      stepperZ.moveTo(400);
      stepperZ.runToPosition();
      stepperZ.setCurrentPosition(0);
      record.concat(readLoadCell());
      scale.tare(); // reset the scale to 0
      record.concat("\n");

      appendFile(SD, fileName.c_str(), record.c_str());

      Serial.println(record);
      dateTimeStr = "";
      record = "";

      
    if (testCount > config02.numPos) {
      
      testCount = 1;
      moveToStart();

    }
   
    } 
   }

    else if (pos_->value.equals("3")){ //300x300 mm.

      //if (){}

    for ( int x = 0; x < config03.numPos; x++) {

      int _value = 0;
            Serial.println("x: " + config03.pos[x][0]);
      stepperX.moveTo(config03.pos[x][0]);
      stepperX.runToPosition();
      stepperX.setCurrentPosition(0);


            Serial.println("y: " + config03.pos[x][1]);
      stepperY.moveTo(config03.pos[x][1]);
      stepperY.runToPosition();
      stepperY.setCurrentPosition(0);
      //      Serial.println("");

      int depthPress = abs(config03.pos[x][2]);

      Serial.println("depthPress: " + depthPress);

      //      Serial.println("press...");
      getLocalTime(&tmstruct, 5000);
      dayStr = String(tmstruct.tm_mday, DEC);
      hourStr = String(a0(tmstruct.tm_hour));
      minStr = String(a0(tmstruct.tm_min));
      secStr = String(a0(tmstruct.tm_sec));
      dateTimeStr.concat(dayStr);
      dateTimeStr.concat("/");
      dateTimeStr.concat(monthStr);
      dateTimeStr.concat("/");
      dateTimeStr.concat(yearStr);
      dateTimeStr.concat(" ");
      dateTimeStr.concat(hourStr);
      dateTimeStr.concat(":");
      dateTimeStr.concat(minStr);
      dateTimeStr.concat(":");
      dateTimeStr.concat(secStr);
      dateTimeStr.concat(",");

      record.concat(dateTimeStr.c_str());

      record.concat(loopCount);
      record.concat(",");
      record.concat(testCount);
      record.concat(",");
      testCount++;

      //  Serial.println(depthPress);
      if (depthPress == 4){
        stepperZ.moveTo(-400);
        stepperZ.runToPosition();
        stepperZ.setCurrentPosition(0);
      
        }
      

      
      for (int i = 0; i < 4; i++)
      {

        for (int j = 0; j < 3; j++)
        {


          int16_t raw00 = ads[i].readADC(j);



          float volt = ads[i].toVoltage(raw00);

          //          Serial.print(volt); Serial.print(",");
          record.concat(String(volt, 3));
          record.concat(",");
        }
      }
      //      Serial.println("");
      
      if (depthPress == 4){
      stepperZ.moveTo(400);
      stepperZ.runToPosition();
      stepperZ.setCurrentPosition(0);
      }
      
      record.concat(readLoadCell());
      scale.tare(); // reset the scale to 0
      record.concat("\n");
      

      appendFile(SD, fileName.c_str(), record.c_str());

      Serial.println(record);
      dateTimeStr = "";
      record = "";

      

    if (testCount > config03.numPos) {
      
      testCount = 1;
      moveToStart();

    }
    
    


      } 
    }

    loopCount++;
    ESPUI.updateLabel(mainLabel, String(loopCount));

  } else {
    isStopStart = false;
        loopCount = 0;
  }



}




//Utilities
//
//If you are here just to see examples of how to use ESPUI, you can ignore the following functions
//------------------------------------------------------------------------------------------------
void readStringFromEEPROM(String & buf, int baseaddress, int size) {
  buf.reserve(size);
  for (int i = baseaddress; i < baseaddress + size; i++) {
    char c = EEPROM.read(i);
    buf += c;
    if (!c) break;
  }
}

void connectWifi() {
  int connect_timeout;

#if defined(ESP32)
  WiFi.setHostname(HOSTNAME);
#else
  WiFi.hostname(HOSTNAME);
#endif
  //  Serial.println("Begin wifi...");

  //Load credentials from EEPROM
  if (!(FORCE_USE_HOTSPOT)) {
    yield();
    EEPROM.begin(100);
    String stored_ssid, stored_pass;
    readStringFromEEPROM(stored_ssid, 0, 32);
    readStringFromEEPROM(stored_pass, 32, 96);
    EEPROM.end();

    //Try to connect with stored credentials, fire up an access point if they don't work.
#if defined(ESP32)
    WiFi.begin(stored_ssid.c_str(), stored_pass.c_str());
#else
    WiFi.begin(stored_ssid, stored_pass);
#endif
    connect_timeout = 28; //7 seconds
    while (WiFi.status() != WL_CONNECTED && connect_timeout > 0) {
      delay(250);
      Serial.print(".");
      connect_timeout--;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(WiFi.localIP());
    Serial.println("Wifi started");

    if (!MDNS.begin(HOSTNAME)) {
      Serial.println("Error setting up MDNS responder!");
    }
  } else {
    Serial.println("\nCreating access point...");
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP(HOSTNAME);

    connect_timeout = 20;
    do {
      delay(250);
      Serial.print(",");
      connect_timeout--;
    } while (connect_timeout);
  }
}

void enterWifiDetailsCallback(Control * sender, int type) {
  if (type == B_UP) {
    Serial.println("Saving credentials to EPROM...");
    Serial.println(ESPUI.getControl(wifi_ssid_text)->value);
    Serial.println(ESPUI.getControl(wifi_pass_text)->value);
    unsigned int i;
    EEPROM.begin(100);
    for (i = 0; i < ESPUI.getControl(wifi_ssid_text)->value.length(); i++) {
      EEPROM.write(i, ESPUI.getControl(wifi_ssid_text)->value.charAt(i));
      if (i == 30) break; //Even though we provided a max length, user input should never be trusted
    }
    EEPROM.write(i, '\0');

    for (i = 0; i < ESPUI.getControl(wifi_pass_text)->value.length(); i++) {
      EEPROM.write(i + 32, ESPUI.getControl(wifi_pass_text)->value.charAt(i));
      if (i == 94) break; //Even though we provided a max length, user input should never be trusted
    }
    EEPROM.write(i + 32, '\0');
    EEPROM.end();
  }
}  



void randomString(char *buf, int len) {
  for (auto i = 0; i < len - 1; i++)
    buf[i] = random(0, 26) + 'A';
  buf[len - 1] = '\0';
}