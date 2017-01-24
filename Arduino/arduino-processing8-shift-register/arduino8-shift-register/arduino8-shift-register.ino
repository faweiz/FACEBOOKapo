#define numCols 16
#define numRows 6
#define dataPin 3
#define clockPin 5
byte colsOne, colsTwo;
byte colsArrayOne[numCols] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0,0,0,0,0,0,0,0};
byte colsArrayTwo[numCols] = {0,0,0,0,0,0,0,0,0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
//byte colsArrayTwo[numCols] = {0,0,0,0,0,0,0,0,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
int rows[numRows] = {A0, A1, A2, A3, A4, A5}; 
int incomingValues[numCols*numRows]={0};
int TotalSensors = numCols * numRows;

// the setup routine runs once when you press reset:
void setup() {
  Serial.begin(9600);
  pinMode(dataPin,OUTPUT);
  pinMode(clockPin,OUTPUT);
}

// the loop routine runs over and over again forever:
void loop() {
    for(int col=0; col<numCols;col++)
    {
        colsOne = colsArrayOne[col];
        colsTwo = colsArrayTwo[col];
        updateShiftRegister();    
        for(int row=0; row<numRows;row++)
        {
          delay(5);
          incomingValues[(col*numRows)+row]=analogRead(rows[row]);
        } // End "Row" for loop
    } // End "Column" for loop
    for(int k=0; k<TotalSensors;k++)
    {
      Serial.print(incomingValues[k]);
      if(k<TotalSensors-1) Serial.print(",");
    }  
    Serial.println();
}

void updateShiftRegister()
{
   digitalWrite(clockPin, LOW);
   shiftColumn(dataPin, clockPin, colsTwo);
   shiftColumn(dataPin, clockPin, colsOne);
   digitalWrite(clockPin, HIGH);
}

void shiftColumn(uint8_t myDataPin, uint8_t myClockPin, byte myData)
{
    uint8_t i;
    int pinState;
    for (i = 0; i < 8; i++)  {
      if(myData & (1<<i))
        pinState = 1;
      else
        pinState = 0;
          digitalWrite(myDataPin,pinState);  
          digitalWrite(myClockPin, HIGH);
          digitalWrite(myClockPin, LOW);        
    }
}
