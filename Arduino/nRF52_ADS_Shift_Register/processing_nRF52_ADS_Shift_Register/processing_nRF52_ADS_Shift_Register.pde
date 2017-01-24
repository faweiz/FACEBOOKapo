import processing.serial.*;

int cols = 16;
int rows = 10;
Serial myPort;  // The serial port
int maxNumberOfSensors = cols * rows;     
float[] sensorValue = new float[maxNumberOfSensors];  // global variable for storing mapped sensor values
float[] previousValue = new float[maxNumberOfSensors];  // array of previous values
int rectSize = 50;
int averageMIN = 500;
int averageMAX = 4095;

void setup () { 
  size(800, 500);  // set up the window to whatever size you want
  myPort = new Serial(this, "COM4", 9600);
  myPort.clear();
  myPort.bufferUntil('\n');  // don't generate a serialEvent() until you get a newline (\n) byte
  background(255);    // set inital background
  smooth();  // turn on antialiasing
  rectMode(CORNER);
}

void draw () {
  for (int r=0; r<rows; r++) {
    for (int c=0; c<cols; c++) {
 //     fill(sensorValue[(c*rows) + r], 100 - sensorValue[(c*rows) + r],0);
 fill(sensorValue[(c*rows) + r], 100 - sensorValue[(c*rows) + r],0);
      rect(c*rectSize, r*rectSize, rectSize, rectSize);
    } // end for cols
  } // end for rows
}


void serialEvent (Serial myPort) {
  String inString = myPort.readStringUntil('\n');  // get the ASCII string
  if (inString != null) {  // if it's not empty
    inString = trim(inString);  // trim off any whitespace
    int incomingValues[] = int(split(inString, ","));  // convert to an array of ints
    if (incomingValues.length <= maxNumberOfSensors && incomingValues.length > 0) {
      for (int i = 0; i < incomingValues.length; i++) {
        // map the incoming values (0 to  1023) to an appropriate gray-scale range (0-255):
       sensorValue[i] = map(incomingValues[i], averageMIN, averageMAX, 0, 255);
//        sensorValue[i] = incomingValues[i]; 
      }
    }
  }
   print(inString);
}