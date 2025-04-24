#include <Wire.h>
#include "lib/Adafruit_Sensor.h"
#include "Adafruit_ADXL345_U.h"
#include <Mouse.h>
#include <Keyboard.h>
#include <EEPROM.h>

#define VERSION "RR_PICKAXE_1"
#define TESTMODE false // true prevents keyboard and mouse init and just posts input values over Serial \
                       // use to test if all components are connected and functioning normally
#define THUMB_X A0
#define THUMB_Y A1
#define BUTTON_A 10
#define BUTTON_B 16

#define CONTROL_KB 0
#define CONTROL_KB_CHAR 3
#define CONTROL_MOUSEBUTTON 1
#define CONTROL_MOUSEAXIS 2
#define CONTROL_DISABLED 4

//                         buttonA, buttonB, thumbUp, thumbDown, thumbLeft, thumbRight, accUp, accDown, accLeft, accRight, accWhip
int keyMap[] = {1, 2, 0xDA, 0xD9, 0xD8, 0xD7, 0xDA, 0xD9, 0xD8, 0xD7, 1};
int keyMapControlType[] = {CONTROL_MOUSEBUTTON, CONTROL_MOUSEBUTTON, CONTROL_MOUSEAXIS, CONTROL_MOUSEAXIS, CONTROL_MOUSEAXIS, CONTROL_MOUSEAXIS,
                           CONTROL_DISABLED, CONTROL_DISABLED, CONTROL_DISABLED, CONTROL_DISABLED, CONTROL_MOUSEBUTTON};

/* Assign a unique ID to this sensor at the same time */
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

long whipStart = -1000;
long lastWhip = -1000;
bool isWhipping = false;

struct Inputs
{
  int input_thumb_x = 0;
  int input_thumb_y = 0;
  int input_button_A = 0;
  int input_button_B = 0;
  float x_accel = 0;
  float y_accel = 0;
  float z_accel = 0;
  float roll = 0;
  float pitch = 0;
};

struct ThumbCalibration
{
  int center[2] = {474, 518};
  int deadZone[2] = {80, 80};
  int min[2] = {150, 150};
  int max[2] = {770, 800};
  int mouseMax[2] = {15, 15};
  bool mouseMode = false;
};

float mouseMove[2] = {0, 0};

Inputs currentInputs;
Inputs prevInputs;
ThumbCalibration thumb;
ThumbCalibration acc;

int incomingByte;
bool recievingUpdatedConfig = false;
int configCount = 0;

void setup()
{
  GetValuesFromEEPROM();
  Serial.begin(115200);
  //SerialIntroduction();

  acc.center[0] = 0;
  acc.center[1] = 20;
  acc.deadZone[0] = 15;
  acc.min[0] = 0;
  acc.max[0] = 80;

  acc.deadZone[1] = 15;
  acc.min[1] = 0;
  acc.max[1] = 80;

  Serial.begin(115200);
  /* Initialise the sensor */
  accel.begin();

  accel.setRange(ADXL345_RANGE_16_G);

  pinMode(THUMB_X, INPUT);
  pinMode(THUMB_Y, INPUT);
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);

  if (!TESTMODE)
  {
    Keyboard.begin();
    Mouse.begin();
  }
}

void loop(void)
{
  while (Serial.available())
  {
    incomingByte = Serial.read();

    if (incomingByte == 42)
    {
      recievingUpdatedConfig = true;
      configCount = -1;
      // Serial.println("Initiating config update");
    }
    else if (incomingByte == 38)
    {
      ReportCurrentConfig();
      WriteValuesToEEPROM();
      return;
    }
    //Serial.print("I received: ");
    //Serial.println(incomingByte, DEC);

    if (recievingUpdatedConfig)
    {
      if (configCount >= 0)
      {
        keyMap[configCount] = incomingByte;
        if (incomingByte < 8)
        {
          keyMapControlType[configCount] = CONTROL_MOUSEBUTTON;
        }
        else if (incomingByte < 11)
        {
          keyMapControlType[configCount] = CONTROL_MOUSEAXIS;
        }
        else if (incomingByte < 6 + 47)
        {
          keyMapControlType[configCount] = CONTROL_KB;
        }
        else
        {
          keyMapControlType[configCount] = CONTROL_KB_CHAR;
        }
      }
      configCount++;

      if (configCount == 11)
      {
        //Serial.println("Config updated successfully");
        ReportCurrentConfig();
        recievingUpdatedConfig = false;
      }
      return;
    }
  }

  mouseMove[0] = 0;
  mouseMove[1] = 0;
  UpdateInputValues();

  if (TESTMODE)
  {
    PrintInputValues();
    delay(50);
  }
  else
  {
    CheckButtonStates();
    CheckThumbState();
    CheckAccelState();
  }

  Mouse.move(mouseMove[0], mouseMove[1], 0);
  prevInputs = currentInputs;
  delay(10);
}

void ReportCurrentConfig()
{
  Serial.write('*');
  for (int i = 0; i < 11; i++)
  {
    Serial.write(keyMap[i]);
  }
  Serial.write('@');
  Serial.write('\n');
}

void UpdateInputValues()
{
  sensors_event_t event;
  accel.getEvent(&event);

  currentInputs.x_accel = event.acceleration.z;
  currentInputs.y_accel = event.acceleration.x;
  currentInputs.z_accel = event.acceleration.y;
  currentInputs.pitch = atan2(currentInputs.y_accel, currentInputs.z_accel) * 57.3;
  currentInputs.roll = atan2((-currentInputs.x_accel), sqrt(currentInputs.y_accel * currentInputs.y_accel + currentInputs.z_accel * currentInputs.z_accel)) * 57.3;

  currentInputs.input_button_A = !digitalRead(BUTTON_A);
  currentInputs.input_button_B = !digitalRead(BUTTON_B);
  currentInputs.input_thumb_x = analogRead(THUMB_X);
  currentInputs.input_thumb_y = analogRead(THUMB_Y);
}

void SerialIntroduction()
{
  Serial.print(VERSION);

  Serial.println();
}

void OnButtonDown(int button)
{
  if (button == 0) // A
  {
    if (keyMapControlType[0] == CONTROL_KB)
    {
      Keyboard.press(keyMap[0]);
    }
    else if (keyMapControlType[0] == CONTROL_KB_CHAR)
    {
      char v = keyMap[0];
      Keyboard.press(v);
    }
    else if (keyMapControlType[0] == CONTROL_MOUSEAXIS)
    {
    }
    else if (keyMapControlType[0] == CONTROL_MOUSEBUTTON)
    {
      Mouse.press(keyMap[0]);
    }
  }
  else // B
  {
    if (keyMapControlType[1] == CONTROL_KB)
    {
      Keyboard.press(keyMap[1]);
    }
    else if (keyMapControlType[1] == CONTROL_KB_CHAR)
    {
      char v = keyMap[1];
      Keyboard.press(v);
    }
    else if (keyMapControlType[1] == CONTROL_MOUSEAXIS)
    {
    }
    else if (keyMapControlType[1] == CONTROL_MOUSEBUTTON)
    {
      Mouse.press(keyMap[1]);
    }
  }
}

void OnButtonUp(int button)
{
  if (button == 0) // A
  {
    if (keyMapControlType[0] == CONTROL_KB)
    {
      Keyboard.release(keyMap[0]);
    }
    else if (keyMapControlType[0] == CONTROL_KB_CHAR)
    {
      char v = keyMap[0];
      Keyboard.release(v);
    }
    else if (keyMapControlType[0] == CONTROL_MOUSEAXIS)
    {
    }
    else if (keyMapControlType[0] == CONTROL_MOUSEBUTTON)
    {
      Mouse.release(keyMap[0]);
    }
  }
  else // B
  {
    if (keyMapControlType[1] == CONTROL_KB)
    {
      Keyboard.release(keyMap[1]);
    }
    else if (keyMapControlType[1] == CONTROL_KB_CHAR)
    {
      char v = keyMap[1];
      Keyboard.release(v);
    }
    else if (keyMapControlType[1] == CONTROL_MOUSEAXIS)
    {
    }
    else if (keyMapControlType[1] == CONTROL_MOUSEBUTTON)
    {
      Mouse.release(keyMap[1]);
    }
  }
}

void OnButton(int button)
{
  if (button == 0) // A
  {
    if (keyMapControlType[0] == CONTROL_KB)
    {
      //Keyboard.release(keyMap[0]);
    }
    else if (keyMapControlType[0] == CONTROL_MOUSEAXIS)
    {
    }
    else if (keyMapControlType[0] == CONTROL_MOUSEBUTTON)
    {
      //Mouse.release(keymap[0]);
    }
  }
  else // B
  {
    if (keyMapControlType[1] == CONTROL_KB)
    {
      //Keyboard.release(keyMap[1]);
    }
    else if (keyMapControlType[1] == CONTROL_MOUSEAXIS)
    {
    }
    else if (keyMapControlType[1] == CONTROL_MOUSEBUTTON)
    {
      //Mouse.release(keymap[1]);
    }
  }
}

void CheckButtonStates()
{
  // Check Button states and call events
  if (currentInputs.input_button_A && !prevInputs.input_button_A)
  {
    OnButtonDown(0);
  }
  if (currentInputs.input_button_B && !prevInputs.input_button_B)
  {
    OnButtonDown(1);
  }
  if (!currentInputs.input_button_A && prevInputs.input_button_A)
  {
    OnButtonUp(0);
  }
  if (!currentInputs.input_button_B && prevInputs.input_button_B)
  {
    OnButtonUp(1);
  }
  if (currentInputs.input_button_A)
  {
    OnButton(0);
  }
  if (currentInputs.input_button_B)
  {
    OnButton(1);
  }
}

void CheckThumbState()
{

  long thumbResult[] = {0, 0};
  if (currentInputs.input_thumb_x < thumb.center[0] - thumb.deadZone[0])
  {
    thumbResult[0] = map(currentInputs.input_thumb_x, thumb.center[0], thumb.min[0], 0, -thumb.mouseMax[0]);
  }
  else if (currentInputs.input_thumb_x > thumb.center[0] + thumb.deadZone[0])
  {
    thumbResult[0] = map(currentInputs.input_thumb_x, thumb.center[0], thumb.max[0], 0, thumb.mouseMax[0]);
  }

  if (currentInputs.input_thumb_y < thumb.center[1] - thumb.deadZone[1])
  {
    thumbResult[1] = map(currentInputs.input_thumb_y, thumb.center[1], thumb.min[1], 0, thumb.mouseMax[1]);
  }
  else if (currentInputs.input_thumb_y > thumb.center[1] + thumb.deadZone[0])
  {
    thumbResult[1] = map(currentInputs.input_thumb_y, thumb.center[1], thumb.max[1], 0, -thumb.mouseMax[1]);
  }

  if (keyMapControlType[2] == CONTROL_MOUSEBUTTON)
  {
    //ThumbUp
    if (thumbResult[1] < 0)
    {
      Mouse.press(keyMap[2]);
    }
    else if (thumbResult[1] >= 0)
    {
      Mouse.release(keyMap[2]);
    }
  }

  if (keyMapControlType[3] == CONTROL_MOUSEBUTTON)
  {
    //ThumbDown
    if (thumbResult[1] > 0)
    {
      Mouse.press(keyMap[3]);
    }
    else if (thumbResult[1] <= 0)
    {
      Mouse.release(keyMap[3]);
    }
  }

  if (keyMapControlType[4] == CONTROL_MOUSEBUTTON)
  {
    //ThumbLeft
    if (thumbResult[0] < 0)
    {
      Mouse.press(keyMap[4]);
    }
    else if (thumbResult[0] >= 0)
    {
      Mouse.release(keyMap[4]);
    }
  }

  if (keyMapControlType[5] == CONTROL_MOUSEBUTTON)
  {
    //ThumbRight
    if (thumbResult[0] > 0)
    {
      Mouse.press(keyMap[5]);
    }
    else if (thumbResult[0] <= 0)
    {
      Mouse.release(keyMap[5]);
    }
  }
  if (keyMapControlType[2] == CONTROL_KB)
  {
    //ThumbUp
    if (thumbResult[1] < 0)
    {
      Keyboard.press(keyMap[2]);
    }
    else if (thumbResult[1] >= 0)
    {
      Keyboard.release(keyMap[2]);
    }
  }

  if (keyMapControlType[3] == CONTROL_KB)
  {
    //ThumbDown
    if (thumbResult[1] > 0)
    {
      Keyboard.press(keyMap[3]);
    }
    else if (thumbResult[1] <= 0)
    {
      Keyboard.release(keyMap[3]);
    }
  }

  if (keyMapControlType[4] == CONTROL_KB)
  {
    //ThumbLeft
    if (thumbResult[0] < 0)
    {
      Keyboard.press(keyMap[4]);
    }
    else if (thumbResult[0] >= 0)
    {
      Keyboard.release(keyMap[4]);
    }
  }

  if (keyMapControlType[5] == CONTROL_KB)
  {
    //ThumbRight
    if (thumbResult[0] > 0)
    {
      Keyboard.press(keyMap[5]);
    }
    else if (thumbResult[0] <= 0)
    {
      Keyboard.release(keyMap[5]);
    }
  }

  if (keyMapControlType[2] == CONTROL_KB_CHAR)
  {
    //ThumbUp
    if (thumbResult[1] < 0)
    {
      char v = keyMap[2];
      Keyboard.press(v);
    }
    else if (thumbResult[1] >= 0)
    {
      char v = keyMap[2];
      Keyboard.release(v);
    }
  }

  if (keyMapControlType[3] == CONTROL_KB_CHAR)
  {
    //ThumbDown
    if (thumbResult[1] > 0)
    {
      char v = keyMap[3];
      Keyboard.press(v);
    }
    else if (thumbResult[1] <= 0)
    {
      char v = keyMap[3];
      Keyboard.release(v);
    }
  }

  if (keyMapControlType[4] == CONTROL_KB_CHAR)
  {
    //ThumbLeft
    if (thumbResult[0] < 0)
    {
      char v = keyMap[4];
      Keyboard.press(v);
    }
    else if (thumbResult[0] >= 0)
    {
      char v = keyMap[4];
      Keyboard.release(v);
    }
  }

  if (keyMapControlType[5] == CONTROL_KB_CHAR)
  {
    //ThumbRight
    if (thumbResult[0] > 0)
    {
      char v = keyMap[5];
      Keyboard.press(v);
    }
    else if (thumbResult[0] <= 0)
    {
      char v = keyMap[5];
      Keyboard.release(v);
    }
  }

  if (keyMapControlType[2] == CONTROL_MOUSEAXIS)
  {
    //ThumbUp
    if (thumbResult[1] < 0)
    {
      mouseMove[1] += thumbResult[1];
    }
  }

  if (keyMapControlType[3] == CONTROL_MOUSEAXIS)
  {
    //ThumbDown
    if (thumbResult[1] > 0)
    {
      mouseMove[1] += thumbResult[1];
    }
  }

  if (keyMapControlType[4] == CONTROL_MOUSEAXIS)
  {
    //ThumbLeft
    if (thumbResult[0] < 0)
    {
      mouseMove[0] += thumbResult[0];
    }
  }

  if (keyMapControlType[5] == CONTROL_MOUSEAXIS)
  {
    //ThumbRight
    if (thumbResult[0] > 0)
    {
      mouseMove[0] += thumbResult[0];
    }
  }
}

void CheckAccelState()
{
  int thumbResult[2] = {0, 0};

  if (currentInputs.roll < acc.center[0] - acc.deadZone[0])
  {
    thumbResult[0] = -map(currentInputs.roll, acc.center[0], acc.max[0], 0, thumb.mouseMax[0]);
  }
  else if (currentInputs.roll > acc.center[0] + acc.deadZone[0])
  {
    thumbResult[0] = map(currentInputs.roll, acc.center[0], acc.max[0], 0, -thumb.mouseMax[0]);
  }

  if (currentInputs.pitch < acc.center[1] - acc.deadZone[1])
  {
    thumbResult[1] = -map(currentInputs.pitch, acc.center[1], acc.max[1], 0, thumb.mouseMax[1]);
  }
  else if (currentInputs.pitch > acc.center[1] + acc.deadZone[1])
  {
    thumbResult[1] = map(currentInputs.pitch, acc.center[1], acc.max[1], 0, -thumb.mouseMax[1]);
  }

  if (millis() - whipStart > 200 && currentInputs.pitch < -50)
  {
    whipStart = millis(); 
    if (keyMapControlType[10] == CONTROL_MOUSEBUTTON && !isWhipping)
    {
      Mouse.press(keyMap[10]);
      //Serial.println("Start Whipping");
      isWhipping = true;
      lastWhip = millis();
      Keyboard.releaseAll();
    }
  }
  else if (millis() - whipStart < 200 && currentInputs.pitch > 50)
  {
    if (!isWhipping)
    {
      if (keyMapControlType[10] == CONTROL_KB)
      {
        Keyboard.press(keyMap[10]);
        Keyboard.release(keyMap[10]);
        whipStart -= millis() - 200;
      }
      else if (keyMapControlType[10] == CONTROL_KB_CHAR)
      {
        char v = keyMap[10];
        Keyboard.write(v);
        whipStart -= millis() - 200;
      }
    }
    else
    {
      lastWhip = millis();
      //Serial.println("Continue Whipping");
    }
  }

  //Serial.println(isWhipping);

  if (isWhipping)
  {
    thumbResult[0] = 0;
    thumbResult[1] = 0;

    if (millis() - lastWhip > 400)
    {
      Mouse.release(keyMap[10]);
      //Serial.println("Finish Whipping");
      isWhipping = false;
    }
    return;
  }

  // Serial.print(thumbResult[0]);
  // Serial.print(", ");
  // Serial.println(thumbResult[1]);

  if (keyMapControlType[6] == CONTROL_MOUSEBUTTON)
  {
    //AccelerometerUp
    if (thumbResult[1] < 0)
    {
      Mouse.press(keyMap[6]);
    }
    else if (thumbResult[1] >= 0)
    {
      Mouse.release(keyMap[6]);
    }
  }

  if (keyMapControlType[7] == CONTROL_MOUSEBUTTON)
  {
    //AccelerometerDown
    if (thumbResult[1] > 0)
    {
      Mouse.press(keyMap[7]);
    }
    else if (thumbResult[1] <= 0)
    {
      Mouse.release(keyMap[7]);
    }
  }

  if (keyMapControlType[8] == CONTROL_MOUSEBUTTON)
  {
    //AccelerometerLeft
    if (thumbResult[0] < 0)
    {
      Mouse.press(keyMap[8]);
    }
    else if (thumbResult[0] >= 0)
    {
      Mouse.release(keyMap[8]);
    }
  }

  if (keyMapControlType[9] == CONTROL_MOUSEBUTTON)
  {
    //AccelerometerRight
    if (thumbResult[0] > 0)
    {
      Mouse.press(keyMap[9]);
    }
    else if (thumbResult[0] <= 0)
    {
      Mouse.release(keyMap[9]);
    }
  }
  if (keyMapControlType[6] == CONTROL_KB)
  {
    //AccelerometerUp
    if (thumbResult[1] < 0)
    {
      Keyboard.press(keyMap[6]);
    }
    else if (thumbResult[1] >= 0)
    {
      Keyboard.release(keyMap[6]);
    }
  }

  if (keyMapControlType[7] == CONTROL_KB)
  {
    //AccelerometerDown
    if (thumbResult[1] > 0)
    {
      Keyboard.press(keyMap[7]);
    }
    else if (thumbResult[1] <= 0)
    {
      Keyboard.release(keyMap[7]);
    }
  }

  if (keyMapControlType[8] == CONTROL_KB)
  {
    //AccelerometerLeft
    if (thumbResult[0] < 0)
    {
      Serial.println(keyMap[8]);
      Keyboard.press(keyMap[8]);
    }
    else if (thumbResult[0] >= 0)
    {
      Serial.println(keyMap[8]);
      Keyboard.release(keyMap[8]);
    }
  }

  if (keyMapControlType[9] == CONTROL_KB)
  {
    //AccelerometerRight
    if (thumbResult[0] > 0)
    {
      Keyboard.press(keyMap[9]);
    }
    else if (thumbResult[0] <= 0)
    {
      Keyboard.release(keyMap[9]);
    }
  }

  if (keyMapControlType[6] == CONTROL_KB_CHAR)
  {
    //AccelerometerUp
    if (thumbResult[1] < 0)
    {
      char v = keyMap[6];
      Keyboard.press(v);
    }
    else if (thumbResult[1] >= 0)
    {
      char v = keyMap[6];
      Keyboard.release(v);
    }
  }

  if (keyMapControlType[7] == CONTROL_KB_CHAR)
  {
    //AccelerometerDown
    if (thumbResult[1] > 0)
    {
      char v = keyMap[7];
      Keyboard.press(v);
    }
    else if (thumbResult[1] <= 0)
    {
      char v = keyMap[7];
      Keyboard.release(v);
    }
  }

  if (keyMapControlType[8] == CONTROL_KB_CHAR)
  {
    //AccelerometerLeft
    if (thumbResult[0] < 0)
    {
      char v = keyMap[8];
      Keyboard.press(v);
    }
    else if (thumbResult[0] >= 0)
    {
      char v = keyMap[8];
      Keyboard.release(v);
    }
  }

  if (keyMapControlType[9] == CONTROL_KB_CHAR)
  {
    //AccelerometerRight
    if (thumbResult[0] > 0)
    {
      char v = keyMap[9];
      Keyboard.press(v);
    }
    else if (thumbResult[0] <= 0)
    {
      char v = keyMap[9];
      Keyboard.release(v);
    }
  }

  if (keyMapControlType[6] == CONTROL_MOUSEAXIS)
  {
    //AccelerometerUp
    if (thumbResult[1] < 0)
    {
      mouseMove[1] += thumbResult[1];
    }
  }

  if (keyMapControlType[7] == CONTROL_MOUSEAXIS)
  {
    //AccelerometerDown
    if (thumbResult[1] > 0)
    {
      mouseMove[1] += thumbResult[1];
    }
  }

  if (keyMapControlType[8] == CONTROL_MOUSEAXIS)
  {
    //AccelerometerLeft
    if (thumbResult[0] < 0)
    {
      mouseMove[0] += thumbResult[0];
    }
  }

  if (keyMapControlType[9] == CONTROL_MOUSEAXIS)
  {
    //AccelerometerRight
    if (thumbResult[0] > 0)
    {
      mouseMove[0] += thumbResult[0];
    }
  }
}

void PrintInputValues()
{
  // Serial.print(currentInputs.input_button_A);
  // Serial.print("\t");
  // Serial.print(currentInputs.input_button_B);
  // Serial.print("\t");
  // Serial.print(currentInputs.input_thumb_x);
  // Serial.print("\t");
  // Serial.print(currentInputs.input_thumb_y);
  // Serial.print("\t");

  // Serial.print(currentInputs.x_accel);
  // Serial.print("\t");
  // Serial.print(currentInputs.y_accel);
  // Serial.print("\t");
  // Serial.print(currentInputs.z_accel);
  // Serial.print("\t");

  // Serial.print(currentInputs.roll);
  // Serial.print("\t");
  Serial.print(currentInputs.pitch);
  Serial.print("\n");
}

void GetValuesFromEEPROM()
{
  for (int i = 0; i < 11; i++)
  {
    keyMap[i] = EEPROM.read(i);
  }
}

void WriteValuesToEEPROM()
{
  for (int i = 0; i < 11; i++)
  {
    EEPROM.update(i, keyMap[i]);
  }
}
