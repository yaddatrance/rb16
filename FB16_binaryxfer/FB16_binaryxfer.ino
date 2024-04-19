#include <spi4teensy3.h>
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//#define KSDEBUG

Adafruit_SSD1306 tft(-1);

IntervalTimer myTimer;

#define U_CS    9
#define U_LDAC  24

#define TRANSPARENT_DACS // need to figure out ldac timing
//#define SOFT_LATCH

#ifdef TRANSPARENT_DACS
  // writethrough (LDAC stays low all times)
  #define DAC_X  0b00110001
  #define DAC_Y  0b00110010
  #define DAC_R  0b00110001
  #define DAC_G  0b00110010
  #define DAC_B  0b00110100
  #define DAC_Z  0b00111000
#else
  // buffered (LDAC normally HIGH, pulsed low 30ns to update or soft latched)
  #define DAC_X  0b00010001
  #define DAC_Y  0b00010010
  #define DAC_R  0b00010001
  #define DAC_G  0b00010010
  #define DAC_B  0b00010100
  #define DAC_Z  0b00011000
#endif
#define DAC_NOOP 0b00100000 // used to skip to next dac in daisychain

#define DAC_LDAC 0b00000001 // used for soft latch
#define ALL_DACS 0b00001111 // used for soft latch


#define bufsizeXY 6 // Daisychain
#define bufsizeGZ 3 // no need to send to 16-bit DAC

typedef struct {
    char * const buffer;
    int head;
    int tail;
    const int maxLen;   
} circBuf_t;

#ifdef KSDEBUG
static char textbuf[32]; /* buffer */
#endif

static char buf[16]; /* buffer */
static char frameBuffer[2][5000][5][2]; //[frame=0/1][pointnum][x=0/y/r/g/b/z=4][MSB=0/LSB=1]
static uint16_t numPointsFB[2] = {0, 0};

static uint8_t currentFrame = 0; // 0=A, 1=B (only written to by laserPoint) !currentFrame is the one being written to.
static uint16_t currentPoint = 0; // only used by timer callback laserPoint()
static char nextFrameReady = 0;  // if next frame is ready, finish up current frame and switch currentFrame when done
                                             
static unsigned int loopCount = 0;
static int pointDelay = 30; // microseconds                                             

volatile static char kicklaser=0; // since this can change use volatile and also turn off interrupts cli()/sei() when changing this value

void laserPoint() {
  cli();  kicklaser=0; sei(); // unset flag
  #ifdef KSDEBUG
      tft.setCursor(0,8);
      snprintf(textbuf,22,"CF:%d[%d] PT:%d/%d L:%d                ",currentFrame,nextFrameReady,currentPoint+1,numPointsFB[currentFrame],loopCount);      
      tft.print(textbuf);
      tft.display();  
  #endif  
  char currentX_MSB = frameBuffer[currentFrame][currentPoint][0][0];
  char currentX_LSB = frameBuffer[currentFrame][currentPoint][0][1];
  char currentY_MSB = frameBuffer[currentFrame][currentPoint][1][0];
  char currentY_LSB = frameBuffer[currentFrame][currentPoint][1][1];

  char currentR_MSB = frameBuffer[currentFrame][currentPoint][2][0];
  char currentR_LSB = frameBuffer[currentFrame][currentPoint][2][1];
  char currentG_MSB = frameBuffer[currentFrame][currentPoint][3][0];
  char currentG_LSB = frameBuffer[currentFrame][currentPoint][3][1];
  char currentB_MSB = frameBuffer[currentFrame][currentPoint][4][0];
  char currentB_LSB = frameBuffer[currentFrame][currentPoint][4][1];

/*
          tft.setCursor(0,16);
          tft.println(currentR_MSB, DEC);
          tft.setCursor(0,24);
          tft.println(currentG_MSB, DEC);
          tft.setCursor(0,32);
          tft.println(currentB_MSB, DEC);
          tft.display();
*/
  buf[0] = DAC_X;
  buf[1] = currentX_MSB;
  buf[2] = currentX_LSB;
  buf[4] = DAC_R;
  buf[5] = 255;
  buf[6] = 255;
  digitalWriteFast(U_CS, LOW); // start SPI
  spi4teensy3::send(buf, bufsizeXY); // GZ cause only sending to XY DAC for test
  digitalWriteFast(U_CS, HIGH); // done            
  buf[0] = DAC_Y;
  buf[1] = currentY_MSB;
  buf[2] = currentY_LSB;
  buf[4] = DAC_G;
  buf[5] = 255;
  buf[6] = 255;
  digitalWriteFast(U_CS, LOW); // start SPI
  spi4teensy3::send(buf, bufsizeXY);
  digitalWriteFast(U_CS, HIGH); // done
  buf[0] = DAC_B;
  buf[1] = 0;
  buf[2] = 0;
  buf[4] = DAC_B;
  buf[5] = 255;
  buf[6] = 255;
  digitalWriteFast(U_CS, LOW); // start SPI
  spi4teensy3::send(buf, bufsizeXY);
  digitalWriteFast(U_CS, HIGH); // done
  
  #ifndef TRANSPARENT_DACS
    #ifdef SOFT_LATCH
      buf[0] = DAC_LDAC;
      buf[1] = ALL_DACS;
      buf[3] = 0;
      digitalWriteFast(U_CS, LOW); // start SPI
      spi4teensy3::send(buf, bufsizeGZ);
      digitalWriteFast(U_CS, HIGH); // done
    #else
      digitalWriteFast(U_LDAC, LOW); // update latch on low
      digitalWriteFast(U_LDAC, HIGH); //
    #endif
  #endif

  currentPoint++;
  if (currentPoint>=numPointsFB[currentFrame]) {
    if (nextFrameReady) {
      nextFrameReady=0;
      loopCount=0;
      currentFrame = !currentFrame; // swap frames if next frame ready
    }
    currentPoint=0; // beginning of frame
    loopCount++;
  }
}

void setup() {
  pinMode(U_CS, OUTPUT);
  pinMode(U_LDAC, OUTPUT);

  digitalWriteFast(U_CS, HIGH);
  #ifdef TRANSPARENT_DACS
  digitalWriteFast(U_LDAC, LOW); // transparent
  #else
  digitalWriteFast(U_LDAC, HIGH); // update latch on low
  #endif

  Wire.begin(); // 0=sda, 2=scl
  //Wire.setClock(100000);
  tft.begin(SSD1306_SWITCHCAPVCC, 0x3c);
  tft.clearDisplay();
  tft.setTextSize(1);

  tft.setCursor(0,0);
  tft.setTextColor(WHITE,BLACK);
  #ifndef KSDEBUG
  tft.println("FB-16v2");
  #else
  tft.println("FB-16       [KSDEBUG]");
  #endif
  tft.display();  

  #ifdef KSDEBUG
    tft.setCursor(0,16);
    snprintf(textbuf,22, "[ ] WF:%d",!currentFrame);
    tft.print(textbuf);
    tft.display();  
  #endif

        
  spi4teensy3::init(2, 1, 0); // 8:1 bus divisor, idle high, data on high->low

  // Force midscale X/Y, 0 RGB on boot
  buf[0] = DAC_X;
  buf[1] = highByte(32767);
  buf[2] = lowByte(32767); 
  buf[3] = DAC_R;
  buf[4] = 127;
  buf[5] = 255; 
  digitalWriteFast(U_CS, LOW); // start SPI
  spi4teensy3::send(buf, bufsizeXY); // GZ cause only sending to XY DAC for test
  digitalWriteFast(U_CS, HIGH); // done            
  buf[0] = DAC_Y;
  buf[1] = highByte(32767);
  buf[2] = lowByte(32767); 
  buf[3] = DAC_G;
  buf[4] = 255;
  buf[5] = 255; 
  digitalWriteFast(U_CS, LOW); // start SPI
  spi4teensy3::send(buf, bufsizeXY);
  digitalWriteFast(U_CS, HIGH); // done 
  buf[0] = DAC_NOOP;
  buf[1] = 255;
  buf[2] = 255; 
  buf[3] = DAC_B;
  buf[4] = 255;
  buf[5] = 255; 
  digitalWriteFast(U_CS, LOW); // start SPI
  spi4teensy3::send(buf, bufsizeXY);
  digitalWriteFast(U_CS, HIGH); // done 
  #ifndef TRANSPARENT_DACS
    #ifdef SOFT_LATCH
      buf[0] = DAC_LDAC;
      buf[1] = ALL_DACS;
      buf[3] = 0;
      digitalWriteFast(U_CS, LOW); // start SPI
      spi4teensy3::send(buf, bufsizeGZ);
      digitalWriteFast(U_CS, HIGH); // done            
    #else
      digitalWriteFast(U_LDAC, LOW); // update latch on low
      digitalWriteFast(U_LDAC, HIGH); // 
    #endif
  #endif

  // preload framebuffers with midscale X/Y
  frameBuffer[0][0][0][0]=highByte(32767);
  frameBuffer[0][0][0][1]=lowByte(32767);
  frameBuffer[0][0][1][0]=highByte(32767);
  frameBuffer[0][0][1][1]=lowByte(32767);

  frameBuffer[1][0][0][0]=highByte(32767);
  frameBuffer[1][0][0][1]=lowByte(32767);
  frameBuffer[1][0][1][0]=highByte(32767);
  frameBuffer[1][0][1][1]=lowByte(32767);

  frameBuffer[0][0][2][0]=255;
  frameBuffer[0][0][2][1]=0;
  frameBuffer[0][0][3][0]=255;
  frameBuffer[0][0][3][1]=0;
  frameBuffer[0][0][4][0]=255;
  frameBuffer[0][0][4][1]=0;

  frameBuffer[1][0][2][0]=255;
  frameBuffer[1][0][2][1]=0;
  frameBuffer[1][0][3][0]=255;
  frameBuffer[1][0][3][1]=0;
  frameBuffer[1][0][4][0]=255;
  frameBuffer[1][0][4][1]=0;
  
  numPointsFB[0]=1;
  numPointsFB[1]=1;
  currentFrame = 0;
  currentPoint = 0;

  // Callback kickpoint() every pointDelay microseconds. Default = 30
  myTimer.begin(kickpoint, pointDelay);

  Serial.setTimeout(5000); // max milliseconds when using readbytesuntil, parseint, etc       
}

// kickpoint sets kicklaser flag, which indicates its time to send a point out. This is handled in loop. 
void kickpoint() {
  kicklaser=1;
}

void loop() {

// todo:
// ring buffer of buffers?

  // Listen for start of message '@' followed two bytes indicating number of points in frame
  // then send points in "[X_MSB][X_LSB][Y_MSB][Y_LSB]...[X_MSB][X_LSB][Y_MSB][Y_LSB]" binary format
  if (kicklaser) laserPoint();
  int avail = Serial.available();
  if (avail) {
    char c = Serial.peek();
    if ((c == '@') && (!nextFrameReady)) { // start of frame
      if (kicklaser) laserPoint();
      int startChar = (char)Serial.read(); 
      if (startChar == '@') { // Indicate start of data block, expect number of points 
        
        #ifdef KSDEBUG
          tft.setCursor(0,16);
          snprintf(textbuf,22, "[@] WF:%d",!currentFrame);
          tft.print(textbuf);
          tft.display();  
        #endif

        char numBlocks_MSB = (char)Serial.read();
        char numBlocks_LSB = (char)Serial.read();        
        int numBlocks = (int)numBlocks_MSB<<8|(int)numBlocks_LSB;

        #ifdef KSDEBUG
          tft.setCursor(0,24);
          snprintf(textbuf,22, "    NB:%d%s",numBlocks,"                   ");
          tft.print(textbuf);
          tft.display();  
        #endif

        tft.clearDisplay();
        tft.setCursor(0,0);
        tft.println("FB-16v2");

        numPointsFB[!currentFrame]=0; // we are going to write to the non current frame
        for (int writePoint=0;writePoint<numBlocks;writePoint++) {
          if (kicklaser) laserPoint();
          char valX_MSB, valX_LSB, valY_MSB, valY_LSB;
          char valR_MSB, valR_LSB, valG_MSB, valG_LSB, valB_MSB, valB_LSB;
          valX_MSB=valX_LSB= valY_MSB=valY_LSB= valR_MSB=valR_LSB= valG_MSB=valG_LSB= valB_MSB=valB_LSB=0;
          char val[11];
          Serial.readBytes(val,10);
          valX_MSB = (char) val[0];
          valX_LSB = (char) val[1];
          valY_MSB = (char) val[2];
          valY_LSB = (char) val[3];

          valR_MSB = (char) val[4];
          valR_LSB = (char) val[5];
          valG_MSB = (char) val[6];
          valG_LSB = (char) val[7];
          valB_MSB = (char) val[8];
          valB_LSB = (char) val[9];

                              
          frameBuffer[!currentFrame][writePoint][0][0]=valX_MSB;
          frameBuffer[!currentFrame][writePoint][0][1]=valX_LSB;
          frameBuffer[!currentFrame][writePoint][1][0]=valY_MSB;
          frameBuffer[!currentFrame][writePoint][1][1]=valY_LSB;

          frameBuffer[!currentFrame][writePoint][2][0]=valR_MSB;
          frameBuffer[!currentFrame][writePoint][2][1]=valR_LSB;
          frameBuffer[!currentFrame][writePoint][3][0]=valG_MSB;
          frameBuffer[!currentFrame][writePoint][3][1]=valG_LSB;
          frameBuffer[!currentFrame][writePoint][4][0]=valB_MSB;
          frameBuffer[!currentFrame][writePoint][4][1]=valB_LSB;
          
          numPointsFB[!currentFrame]++;
          tft.drawPixel(valX_MSB/4,valY_MSB/4,WHITE);
//          tft.display();   // debug point by point



          #ifdef KSDEBUG
            tft.setCursor(0,32);
            snprintf(textbuf,22, "    CP:%d%s",writePoint,"                      ");
            tft.print(textbuf);
            tft.display();  
          #endif
        }
        tft.display();  
        cli(); //disable interrupts
        nextFrameReady = 1; // loop done, wont start writing until frame read and flipped
        sei(); //reenable interrupts

        #ifdef KSDEBUG
          tft.setCursor(0,16);
          snprintf(textbuf,22, "[@] WF:  ");

          tft.setCursor(0,40);
          snprintf(textbuf,22, "NB:%d nPFB[%d]               ",numBlocks,numPointsFB[!currentFrame]);
          tft.print(textbuf);
          tft.display();  
        #endif

      }      
    } else if (c == '?') {
      Serial.read();
      Serial.println("Staying Alive");
    } else if (c == '#') {  // return number
      Serial.read();
      char numberMSB = (char)Serial.read();
      char numberLSB = (char)Serial.read();
      Serial.printf("[%d][%d][%d]\n",numberMSB,numberLSB,numberMSB<<8|numberLSB);      
    } else if (c == '$') {  // set intervalTimer
      Serial.read();
      char pointDelay_SB3 = (char)Serial.read();
      char pointDelay_SB2 = (char)Serial.read();
      char pointDelay_SB1 = (char)Serial.read();
      char pointDelay_SB0 = (char)Serial.read();        
      pointDelay = pointDelay_SB3<<24|pointDelay_SB2<<16|pointDelay_SB1<<8|pointDelay_SB0;
      myTimer.update(pointDelay);
    } else if (c == -1) { // empty
    } else { // got something weird, burn it with fire
      Serial.read();
      Serial.clear(); // flush input buffer
    }
  } // if (avail)   
}

