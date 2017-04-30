/*
Rollosteuerung  Ausstellung
 3 Fernbedienungen mit digitalen Eingaengen up, stop und down
 1 Inputsignal (Interrupt 2 = PIN 21 ) zur Auslösung
 
 Historie:
 15.2.2014 Steuerung AktiveMode verbessert
 */

// Hardware Interface

#define INTERRUPT_INPUT_MAGNET_NUMBER  2

#define PIN_OUTPUT_UNIT_1_TASTE_UP    43
#define PIN_OUTPUT_UNIT_1_TASTE_STOP  45
#define PIN_OUTPUT_UNIT_1_TASTE_DOWN  47

#define PIN_OUTPUT_UNIT_2_TASTE_UP    37
#define PIN_OUTPUT_UNIT_2_TASTE_STOP  39
#define PIN_OUTPUT_UNIT_2_TASTE_DOWN  41

#define PIN_OUTPUT_UNIT_3_TASTE_UP    31
#define PIN_OUTPUT_UNIT_3_TASTE_STOP  33
#define PIN_OUTPUT_UNIT_3_TASTE_DOWN  35

#define Rollo_1_TASTE_UP    1
#define Rollo_1_TASTE_STOP  2
#define Rollo_1_TASTE_DOWN  3
#define Rollo_2_TASTE_UP    4
#define Rollo_2_TASTE_STOP  5
#define Rollo_2_TASTE_DOWN  6
#define Rollo_3_TASTE_UP    7
#define Rollo_3_TASTE_STOP  8
#define Rollo_3_TASTE_DOWN  9


#define TIME_TO_MOTION_DOWN  15   //Motion Time Rollo Up-Position -> Down-Position
#define TIME_TO_MOTION_UP    15   //Motion Time Rollo Down-Position -> Up-Position
#define TIME_DOWN_TO_UP      10   //Max Time in Down Position
#define TIME_STOP_TO_UP      10   //May Time in Stop Position


#define ROTATION_DELAY_START  1
#define ROTATION_DELAY_STOP  1
int  RotationTime;

// Software Parameter

#define BEDIEN_TIME  3000  // Zeit pro Taste

#define MAX_RolloS 3

#define STATE_UP  0
#define STATE_MOTION_DOWN  1
#define STATE_MOTION_UP  2
#define STATE_DOWN  3
#define STATE_MOTION_DOWN_STOP  4


#define HystereseActivMode 3
#define HysteresePassivMode 3

int AktiveRolloNumber;
int MotionTimerDown[MAX_RolloS];
int MotionTimerUp[MAX_RolloS];
int RolloState [MAX_RolloS];
int RolloStateTime [MAX_RolloS];

int TimerStateUp[MAX_RolloS];
int TimerStateMotionDown[MAX_RolloS];
int TimerStateMotionUp[MAX_RolloS];
int TimerStateDown[MAX_RolloS];
int TimerStateMotionDownStop[MAX_RolloS];

int    DauerPassivMode;
int    DauerActivMode;

int RefreshRolloControlCounter;
#define MAX_RefreshRolloControl    600 // 10 Minuten = 600 Seconds

int CounterSekundenWithoutBewegung;
#define CounterSekundenWithoutBewegung 3

#define CounterSekundenDownPosition	   20


#define WAITTIME  20000
volatile  int actCounter;
volatile  int oldCounter;

int oldPosition[3];

void setup(){

  AktiveRolloNumber = 0;

  RotationTime =0;

  actCounter = 0;
  //start serial connection
  Serial.begin(9600);
  //configure pin2 as an input and enable the internal pull-up resistor
  pinMode(2, INPUT_PULLUP);   

  pinMode( PIN_OUTPUT_UNIT_1_TASTE_UP   , OUTPUT); 
  pinMode( PIN_OUTPUT_UNIT_1_TASTE_STOP , OUTPUT); 
  pinMode( PIN_OUTPUT_UNIT_1_TASTE_DOWN  , OUTPUT); 
  pinMode( PIN_OUTPUT_UNIT_2_TASTE_UP   , OUTPUT); 
  pinMode( PIN_OUTPUT_UNIT_2_TASTE_STOP  , OUTPUT); 
  pinMode( PIN_OUTPUT_UNIT_2_TASTE_DOWN  , OUTPUT); 
  pinMode( PIN_OUTPUT_UNIT_3_TASTE_UP    , OUTPUT); 
  pinMode( PIN_OUTPUT_UNIT_3_TASTE_STOP  , OUTPUT); 
  pinMode( PIN_OUTPUT_UNIT_3_TASTE_DOWN  , OUTPUT); 

  ClearAllButton();

  pinMode(23, INPUT_PULLUP); 
  pinMode(25, OUTPUT); 
  pinMode(27, OUTPUT); 

  digitalWrite(25, HIGH); 
  digitalWrite(27, LOW); 

  //Pin 21 mit Interrupt belegt
  attachInterrupt(INTERRUPT_INPUT_MAGNET_NUMBER,IncrementCounter, FALLING);

  DauerPassivMode=0;
  DauerActivMode=0;
}

void ClearAllButton()
{
  digitalWrite( PIN_OUTPUT_UNIT_1_TASTE_UP   , LOW); 
  digitalWrite( PIN_OUTPUT_UNIT_1_TASTE_STOP , LOW); 
  digitalWrite( PIN_OUTPUT_UNIT_1_TASTE_DOWN  , LOW); 
  digitalWrite( PIN_OUTPUT_UNIT_2_TASTE_UP   , LOW); 
  digitalWrite( PIN_OUTPUT_UNIT_2_TASTE_STOP  , LOW); 
  digitalWrite( PIN_OUTPUT_UNIT_2_TASTE_DOWN  , LOW); 
  digitalWrite( PIN_OUTPUT_UNIT_3_TASTE_UP    , LOW); 
  digitalWrite( PIN_OUTPUT_UNIT_3_TASTE_STOP  , LOW); 
  digitalWrite( PIN_OUTPUT_UNIT_3_TASTE_DOWN  , LOW); 
}


void PressButton (int aktiveButton)
{
  Serial.print("Press Button ");
  Serial.println(aktiveButton);

  ClearAllButton();

  switch (aktiveButton)
  {
  case Rollo_1_TASTE_UP:
    digitalWrite( PIN_OUTPUT_UNIT_1_TASTE_UP   , HIGH); 
    break;
  case Rollo_1_TASTE_STOP:
    digitalWrite( PIN_OUTPUT_UNIT_1_TASTE_STOP , HIGH); 
    break;
  case Rollo_1_TASTE_DOWN:
    digitalWrite( PIN_OUTPUT_UNIT_1_TASTE_DOWN  , HIGH); 
    break;
  case Rollo_2_TASTE_UP:
    digitalWrite( PIN_OUTPUT_UNIT_2_TASTE_UP   , HIGH); 
    break;
  case Rollo_2_TASTE_STOP:
    digitalWrite( PIN_OUTPUT_UNIT_2_TASTE_STOP  , HIGH); 
    break;
  case Rollo_2_TASTE_DOWN:
    digitalWrite( PIN_OUTPUT_UNIT_2_TASTE_DOWN  , HIGH); 
    break;
  case Rollo_3_TASTE_UP:
    digitalWrite( PIN_OUTPUT_UNIT_3_TASTE_UP    , HIGH); 
    break;
  case Rollo_3_TASTE_STOP:
    digitalWrite( PIN_OUTPUT_UNIT_3_TASTE_STOP  , HIGH); 
    break;
  case Rollo_3_TASTE_DOWN:
    digitalWrite( PIN_OUTPUT_UNIT_3_TASTE_DOWN  , HIGH);
  default:
    break;
  }
  delay (BEDIEN_TIME);
  ClearAllButton();
}


int HandleRotationDetection()
{
  if (actCounter == oldCounter)
  {
    return false;
  }
  else
  {
    oldCounter = actCounter;
    return true;
  }
}

void RefreshRolloControl()
{
  for (int RolloNumber = 0; RolloNumber <MAX_RolloS; RolloNumber++)
  {
    switch ( RolloState [RolloNumber])
    {
    case STATE_UP:  
      SendModeUp(RolloNumber);
      break;
    case STATE_MOTION_DOWN:
      SendModeDown(RolloNumber);
      break;
    case STATE_MOTION_UP:
      SendModeUp(RolloNumber);
      break;        
    case STATE_DOWN:
      SendModeDown(RolloNumber);
      break;        
    case STATE_MOTION_DOWN_STOP:
      SendModeStop(RolloNumber);
      break;
    }
  }
}



void  SendModeUp (int RolloNumber)
{
  int aktiveButton = 0;

  Serial.print("Send UP -> "); 
  Serial.println(RolloNumber+1); 

  switch (RolloNumber)
  {
  case 0:
    aktiveButton = Rollo_1_TASTE_UP;
    break;
  case 1:
    aktiveButton = Rollo_2_TASTE_UP;
    break;
  case 2:
    aktiveButton = Rollo_3_TASTE_UP;
    break;
  default:
    return;
  }
  PressButton (aktiveButton);  
}

void  SendModeStop (int RolloNumber)
{
  int aktiveButton = 0;

  Serial.print("Send STOP -> "); 
  Serial.println(RolloNumber+1); 

  switch (RolloNumber)
  {
  case 0:
    aktiveButton = Rollo_1_TASTE_STOP;
    break;
  case 1:
    aktiveButton = Rollo_2_TASTE_STOP;
    break;
  case 2:
    aktiveButton = Rollo_3_TASTE_STOP;
    break;
  default:
    return;
  }
  PressButton (aktiveButton);  
}

void  SendModeDown (int RolloNumber)
{
  int aktiveButton = 0;

  Serial.print("Send DOWN -> "); 
  Serial.println(RolloNumber+1); 


  switch (RolloNumber)
  {
  case 0:
    aktiveButton = Rollo_1_TASTE_DOWN;
    break;
  case 1:
    aktiveButton = Rollo_2_TASTE_DOWN;
    break;
  case 2:
    aktiveButton = Rollo_3_TASTE_DOWN;
    break;
  default:
    return;
  }
  PressButton (aktiveButton);  
}


void initRolloState()
{
  for (int RolloNumber = 0; RolloNumber <MAX_RolloS; RolloNumber++)
  {
    RolloState [RolloNumber] = STATE_UP;
    TimerStateUp[RolloNumber] =0;
    TimerStateMotionDown[RolloNumber]=0;
    TimerStateMotionUp[RolloNumber]=0;
    TimerStateMotionDownStop[RolloNumber]=0;
  }
}


void printState ( int StateToPrint )
{
  switch(StateToPrint)
  {
  case STATE_UP                : 
    Serial.print("STATE_UP              "); 
    break;
  case STATE_MOTION_DOWN       : 
    Serial.print("STATE_MOTION_DOWN     "); 
    break;
  case STATE_MOTION_UP         : 
    Serial.print("STATE_MOTION_UP       "); 
    break;
  case STATE_DOWN              : 
    Serial.print("STATE_DOWN            "); 
    break;
  case STATE_MOTION_DOWN_STOP  : 
    Serial.print("STATE_MOTION_DOWN_STOP"); 
    break;
  }
}

void printRolloState()
{
  Serial.print("Counter = ");
  Serial.println(actCounter);

  Serial.print("Counter = ");
  Serial.println(actCounter);

  Serial.print("AktiveRolloNumber = ");
  Serial.println(AktiveRolloNumber+1);

  for (int RolloNumber = 0; RolloNumber <MAX_RolloS; RolloNumber++)
  {
    Serial.print("Rollo ");
    Serial.print(RolloNumber+1);

    Serial.print("State = "); 
    printState(RolloState [RolloNumber]);

    Serial.print("; TUp = ");
    Serial.print(TimerStateUp[RolloNumber]); 

    Serial.print("; TDown = ");
    Serial.print(TimerStateDown[RolloNumber]); 

    Serial.print("; TMotionDown = ");
    Serial.print(TimerStateMotionDown[RolloNumber]); 

    Serial.print("; TMotionUp = ");
    Serial.print( TimerStateMotionUp[RolloNumber]); 

    Serial.print("; TDownStop = ");
    Serial.println(TimerStateMotionDownStop[RolloNumber]);
  }
}

void handlePassivMode()
{
  for (int RolloNumber = 0; RolloNumber <MAX_RolloS; RolloNumber++)
  {
    //   Serial.print("Passiv Rollo ");
    //   Serial.print(RolloNumber+1);

    switch (RolloState [RolloNumber])
    {
    case STATE_UP:  
      //    Serial.print(" ");
      TimerStateUp[RolloNumber]++;
      break;
    case STATE_MOTION_DOWN:
      //    Serial.print(" ");
      TimerStateMotionDown[RolloNumber]++;
      break;
    case STATE_MOTION_UP:
      //    Serial.print(" ");
      TimerStateMotionUp[RolloNumber]++;
      if (TimerStateMotionUp[RolloNumber] > TIME_TO_MOTION_UP)
      {
        TimerStateMotionUp[RolloNumber] = 0;
        RolloState [RolloNumber] = STATE_UP;
        SendModeUp (RolloNumber);
      }
      break;        
    case STATE_DOWN:
      //     Serial.print(" ");
      TimerStateDown[RolloNumber]++;
      if (TimerStateDown[RolloNumber] > TIME_DOWN_TO_UP)
      {
        TimerStateDown[RolloNumber] = 0;
        RolloState [RolloNumber] = STATE_MOTION_UP;
        SendModeUp (RolloNumber);
      }
      break;        
    case STATE_MOTION_DOWN_STOP:
      //    Serial.print(" ");
      TimerStateMotionDownStop[RolloNumber]++;
      if (TimerStateMotionDownStop[RolloNumber] > TIME_DOWN_TO_UP)
      {
        TimerStateMotionDownStop[RolloNumber] = 0;
        TimerStateMotionDown[RolloNumber] = 0;
        RolloState [RolloNumber] = STATE_MOTION_UP;
        SendModeUp (RolloNumber);
      }
      break;
    }
  }
}

void handleActivMode()
{
  for (int RolloNumber = 0; RolloNumber <MAX_RolloS; RolloNumber++)
  {
    //   Serial.print("Aktiv Rollo ");
    //   Serial.print(RolloNumber+1);

    switch (RolloState [RolloNumber])
    {
    case STATE_UP:  
      //     Serial.print(" STATE_UP ");
      TimerStateUp[RolloNumber]++;
      break;
    case STATE_MOTION_DOWN:
      //     Serial.print(" STATE_MOTION_DOWN");
      TimerStateMotionDown[RolloNumber]++;
      if (TimerStateMotionDown[RolloNumber] > TIME_TO_MOTION_DOWN)
      {
        TimerStateMotionDown[RolloNumber] = 0; 
        RolloState [RolloNumber] = STATE_DOWN;

        DauerActivMode = 0;
        findAktiveRolloNumber();
      }
      break;
    case STATE_MOTION_UP:
      //    Serial.print(" ");
      TimerStateMotionUp[RolloNumber]++;
      if (TimerStateMotionUp[RolloNumber] > TIME_TO_MOTION_UP)
      {
        TimerStateMotionUp[RolloNumber] = 0;
        RolloState [RolloNumber] = STATE_UP;
      }
      break;        
    case STATE_DOWN:
      //   Serial.print(" ");
      TimerStateDown[RolloNumber]++;
      break;        
    case STATE_MOTION_DOWN_STOP:
      //  Serial.print(" ");
      TimerStateMotionDownStop[RolloNumber]++;
      if (TimerStateMotionDownStop[RolloNumber] > TIME_STOP_TO_UP)
      {
        TimerStateDown[RolloNumber] = 0;
        RolloState [RolloNumber] = STATE_MOTION_DOWN;
        SendModeUp (RolloNumber);
      }
      break;
    }
  }
}

int findAktiveRolloNumber()
{  
  int TimerMax = 0;
  int RolloReturnNumber = 0;
  int found = false;

  for (int RolloNumber = 0; RolloNumber < MAX_RolloS; RolloNumber++)
  {
    if (RolloState [RolloNumber] == STATE_MOTION_DOWN_STOP)
    {
      AktiveRolloNumber = RolloNumber;
      found = true;
      return found;
    }
  } 

  for (int RolloNumber = 0; RolloNumber <MAX_RolloS; RolloNumber++)
  {
    if (RolloState [RolloNumber] == STATE_UP)
    {
      if (TimerStateUp[RolloNumber] > TimerMax)
      {
        TimerMax = TimerStateUp[RolloNumber];
        AktiveRolloNumber = RolloNumber;
        found = true;
      }
    }
  }
  return found;
}

void StartActivMode()
{
  if (findAktiveRolloNumber())
  {
    if ((RolloState [AktiveRolloNumber] == STATE_UP)
      ||(RolloState [AktiveRolloNumber] == STATE_MOTION_DOWN_STOP))
    {
      RolloState [AktiveRolloNumber] = STATE_MOTION_DOWN;
      SendModeDown (AktiveRolloNumber);
      TimerStateUp[AktiveRolloNumber] = 0;
      TimerStateMotionDownStop[AktiveRolloNumber] = 0;
    }
  }
  else
  {
    DauerActivMode = 0;
  }
}

void StartPassivMode()
{
  for (int RolloNumber = 0; RolloNumber <MAX_RolloS; RolloNumber++)
  {
    if (RolloState [RolloNumber] == STATE_MOTION_DOWN)
    {
      RolloState [RolloNumber] = STATE_MOTION_DOWN_STOP;
      SendModeStop (RolloNumber);
    }
  }
}


void loop()
{
  int Rotation = HandleRotationDetection();
  if (Rotation)
  {
    DauerActivMode++;

    if (DauerActivMode == HystereseActivMode)
    {
      DauerPassivMode=0;
      Serial.println("ActivMode ++++++++++++++++ Start");
      StartActivMode();
      Serial.print("HystereseActivMode ");
    }
    if (DauerActivMode > HystereseActivMode)
    {
      DauerPassivMode=0;
      Serial.println("ActivMode ++++++++++++++++");
      handleActivMode();
    }
  }
  else
  {
    DauerPassivMode++;

    if (DauerPassivMode == HysteresePassivMode)
    {
      DauerActivMode=0;
      Serial.println("PassivMode -------------- Start");
      StartPassivMode();
      Serial.print("HysteresePassivMode ");
    }
    if (DauerPassivMode > HysteresePassivMode)
    {      
      DauerActivMode=0;
      Serial.println("PassivMode --------------");
      handlePassivMode();
    }
  }

  RefreshRolloControlCounter++;
  if ( RefreshRolloControlCounter > MAX_RefreshRolloControl)
  {
    Serial.println("RefreshRolloControl");

    RefreshRolloControlCounter= 0;
    RefreshRolloControl();
  }
  printRolloState();

  delay (1000); // 1 Sekunde
}


void WaiteBedienZeit_ClearButton()
{
  delay (BEDIEN_TIME);
  ClearAllButton();
}


int CalculatePosition (int Counter)
{
  int newPosition = 0;
  if (actCounter < 25)
  {
    newPosition = 0;
  }
  else if ( actCounter < 50 )
  {
    newPosition = 1;
  }
  else if (  actCounter < 75 )
  {
    newPosition = 2;
  }
  return newPosition;
}




// Interrupt Routine zum Incrementieren der Rotationen
void IncrementCounter()
{
  actCounter = actCounter + 1;
  //Serial.println(actCounter);
}






























