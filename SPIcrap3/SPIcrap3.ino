#include <spi4teensy3.h>
#include <Arduino.h>
#include <FlexiTimer2.h>

#define U_CS    9
#define U_LDAC  24

//#define TRANSPARENT_DACS // need to figure out ldac timing
#define SOFT_LATCH

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
    uint8_t * const buffer;
    int head;
    int tail;
    const int maxLen;   
} circBuf_t;

int firstPoint=1;
static uint8_t buf[16]; /* buffer */
static uint16_t frameBuffer[2][10000][5]; /* Framebuffer A*/  // avoid having to use volatile for shared variables by having two buffers
volatile static uint16_t numPointsFB[2] = {0, 0}; // cli()/sei() 

static uint8_t currentFrame = 0; // 0=A, 1=B (only written to by laserPoint) !currentFrame is the one being written to.
static uint16_t currentPoint = 0; // only used by timer callback laserPoint()
volatile static uint8_t nextFrameReady = 0;  // if next frame is ready, finish up current frame and switch currentFrame when done
                                             // since this can change use volatile and also turn off interrupts cli()/sei() when changing this value

static uint16_t writePoint = 0; 

void laserPoint() {
  /*Serial.print("CF:");
  Serial.print(currentFrame);
  Serial.print(", NFR:");
  Serial.print(nextFrameReady);
  Serial.print(", np0:");
  Serial.print(numPointsFB[0]);
  Serial.print(", np1:");
  Serial.print(numPointsFB[1]);
  Serial.print(", CP:");
  Serial.println(currentPoint);
*/
  int16_t currentX = frameBuffer[currentFrame][currentPoint][0];
  int16_t currentY = frameBuffer[currentFrame][currentPoint][1];
  buf[0] = DAC_X;
  buf[1] = highByte(currentX);
  buf[2] = lowByte(currentX); 
  digitalWriteFast(U_CS, LOW); // start SPI
  spi4teensy3::send(buf, bufsizeGZ); // GZ cause only sending to XY DAC for test
  digitalWriteFast(U_CS, HIGH); // done            
  buf[0] = DAC_Y;
  buf[1] = highByte(currentY);
  buf[2] = lowByte(currentY); 
  digitalWriteFast(U_CS, LOW); // start SPI
  spi4teensy3::send(buf, bufsizeGZ);
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
        currentFrame = !currentFrame; // swap frames if next frame ready
    }
    currentPoint=0; // beginning of frame
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
        
  spi4teensy3::init(1, 1, 0); // 8:1 bus divisor, idle high, data on high->low

// Force midscale X&Y
  buf[0] = DAC_X;
  buf[1] = highByte(32767);
  buf[2] = lowByte(32767); 
  digitalWriteFast(U_CS, LOW); // start SPI
  spi4teensy3::send(buf, bufsizeGZ); // GZ cause only sending to XY DAC for test
  digitalWriteFast(U_CS, HIGH); // done            
  buf[0] = DAC_Y;
  buf[1] = highByte(32767);
  buf[2] = lowByte(32767); 
  digitalWriteFast(U_CS, LOW); // start SPI
  spi4teensy3::send(buf, bufsizeGZ);
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

  frameBuffer[0][0][0]=32767;
  frameBuffer[0][0][1]=32767;
  frameBuffer[1][0][0]=32767;
  frameBuffer[1][0][1]=32767;
  numPointsFB[0]=1;
  numPointsFB[1]=1;
  currentFrame = 0;
  currentPoint = 0;
  

//  FlexiTimer2::set(30, laserPoint); // display point every 30ms
  FlexiTimer2::set(30, laserPoint); // display point every 30ms
  FlexiTimer2::start();

  Serial.setTimeout(500); // max milliseconds when using readbytesuntil, parseint, etc
       
}

void loop() {

// todo:
// binary transfer vs human readable
// calculate optimal allocated RAM (tradeoff between underruns vs latency)


// Listen for start of message '@' followed by number of points (binary in future, for now numeric string while debugging), ending in \n
// then send points in "X Y X Y ... X Y"
  int avail = Serial.available();
  if (avail) {
    uint8_t c = Serial.peek();
    if ((c == '@') && (!nextFrameReady)) { // start of frame
      int startChar = Serial.read(); 
      if (startChar == '@') { // Indicate start of data block, expect number of points 
        writePoint = 0;
        int numblocks = Serial.parseInt();
        if (numblocks) { // got a number
          numPointsFB[!currentFrame]=0; // we are going to write to the non current frame
          //Serial.print(numblocks);
          //Serial.println(" blocks.");
          for (int i=0;i<numblocks;i++) {
            //Serial.print(i);
            //Serial.print(": ");
            uint16_t valX = (uint16_t) Serial.parseInt();
            uint16_t valY = (uint16_t) Serial.parseInt();
            //Serial.print(valX);
            //Serial.print(", ");
            //Serial.println(valY);
            //Serial.print(" -> ");
            frameBuffer[!currentFrame][writePoint][0]=valX;
            frameBuffer[!currentFrame][writePoint][1]=valY;
            numPointsFB[!currentFrame]++;
            writePoint++;            
          }   
          cli(); //disable interrupts
          nextFrameReady = 1; // loop done, wont start writing until frame read and flipped
          sei(); //reenable interrupts
          Serial.read(); // console sends LF so read it.
        } else { // timeout!!!
              Serial.println("Timeout!");
        }
      }      
    } else if (c == '?') {
      Serial.read();
      Serial.println("Staying Alive");
      Serial.read();
    } else {
      // got something weird, flush it
      Serial.println("Weird!");
      Serial.print("[");
      Serial.print(Serial.peek());
      Serial.println("]");
      Serial.clear(); // flush input buffer
    }
  } // if (avail)   

}

