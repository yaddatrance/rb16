#include <spi4teensy3.h>
#include <Arduino.h>
#include <FlexiTimer2.h>

#define U_CS    9
#define U_LDAC  24

// writethrough (LDAC low to enable transparency)
#define DAC_X  0b00110001
#define DAC_Y  0b00110010
#define DAC_NO 0b00100000

#define DAC_R  0b00110001
#define DAC_G  0b00110010
#define DAC_B  0b00110100
#define DAC_Z  0b00111000

/* buffered (use with LDAC, normally HIGH)
#define DAC_X 0b010001
#define DAC_Y 0b010010
#define DAC_R 0b010001
#define DAC_G 0b010010
#define DAC_B 0b010100
#define DAC_Z 0b011000
*/

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
static uint16_t frameBufferA[10000][5]; /* Framebuffer A*/  // avoid having to use volatile for shared variables by having two buffers
static uint16_t frameBufferB[10000][5]; /* Framebuffer B*/  // one being read from, and one being written to.
static uint16_t numPointsFBA = 0;
static uint16_t numPointsFBB = 0;

static uint8_t currentFrame = 0; // 0=A, 1=B (only written to by laserPoint) !currentFrame is the one being written to.
static uint16_t currentPoint = 0; // only used by timer callback laserPoint()
volatile static uint8_t nextFrameReady = 0;  // if next frame is ready, finish up current frame and switch currentFrame when done
                                             // since this can change use volatile and also turn off interrupts cli()/sei() when changing this value

static uint16_t writePoint = 0; 

void laserPoint() {
  Serial.print("CF:");
  Serial.print(currentFrame);
  Serial.print(", NFR:");
  Serial.print(nextFrameReady);
  Serial.print(", npA:");
  Serial.print(numPointsFBA);
  Serial.print(", npB:");
  Serial.print(numPointsFBB);
  Serial.print(", CP:");
  Serial.println(currentPoint);

  if (currentFrame) { // read from FB A
    if (numPointsFBA) { // there are points to render
      int16_t currentX = frameBufferA[currentPoint][0];
      int16_t currentY = frameBufferA[currentPoint][1];
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
      Serial.print(", Pos:");
      Serial.print(frameBufferA[currentPoint][0]);
      Serial.print(", ");
      Serial.println(frameBufferA[currentPoint][1]);

      currentPoint++;
      if (currentPoint>=numPointsFBA) {
          if (nextFrameReady) {
            nextFrameReady=0;
            currentFrame = !currentFrame; // swap frames if next frame ready
          }
          currentPoint=0; // beginning of frame
      }
    }    
  } else { // read from FB B
    if (numPointsFBB) { // there are points to render
      int16_t currentX = frameBufferB[currentPoint][0];
      int16_t currentY = frameBufferB[currentPoint][1];
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
      Serial.print(", Pos:");
      Serial.print(frameBufferA[currentPoint][0]);
      Serial.print(", ");
      Serial.println(frameBufferA[currentPoint][1]);

      currentPoint++;
      if (currentPoint>=numPointsFBB) {
          currentPoint=0;
          if (nextFrameReady) {
            nextFrameReady=0;
            currentFrame = !currentFrame; // swap frames if next frame ready
            Serial.print("Ready for next frame!");
          }
      }
    }    
    
  } 
}

void setup() {
  pinMode(U_CS, OUTPUT);
  pinMode(U_LDAC, OUTPUT);

  digitalWriteFast(U_CS, HIGH);
  digitalWriteFast(U_LDAC, LOW); // transparent
        
  spi4teensy3::init(1, 1, 0); // 8:1 bus divisor, idle high, data on high->low

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

  frameBufferA[0][0]=32767;
  frameBufferA[0][1]=32767;
  frameBufferB[0][0]=32767;
  frameBufferB[0][1]=32767;
  numPointsFBA=1;
  numPointsFBB=1;
  

  FlexiTimer2::set(1000, laserPoint); // display point every 30ms
  FlexiTimer2::start();
//  while(!Serial);
//  Serial.print("\r\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nStart\r\n");
//  Serial.flush();

  Serial.setTimeout(500); // max milliseconds when using readbytesuntil, parseint, etc
       
}

void loop() {

// allocate XX KB RAM (tradeoff between underruns vs latency)
// Send 'H\r' if buffer is half full
// Send 'E\r' if buffer is empty
// Listen for start of message '@' followed by number of points (binary in future, for now numeric string while debugging), ending in \r

  int avail = Serial.available();
  if (avail) {
    uint8_t c = Serial.peek();
    if ((c == '@') && (!nextFrameReady)) { // start of frame
      int startChar = Serial.read(); 
      if (startChar == '@') { // Indicate start of data block, expect number of points 
        writePoint = 0;
        int numblocks = Serial.parseInt();
        if (numblocks) { // got a number
          if (currentFrame) {
            numPointsFBA=0;
          } else {
            numPointsFBB=0;            
          }
          Serial.print(numblocks);
          Serial.println(" blocks.");
          for (int i=0;i<numblocks;i++) {
            Serial.print(i);
            Serial.print(": ");
            uint16_t valX = (uint16_t) Serial.parseInt();
            uint16_t valY = (uint16_t) Serial.parseInt();
            Serial.print(valX);
            Serial.print(", ");
            Serial.print(valY);
            Serial.print(" -> ");
            if (currentFrame) { // write to A, probably some sanity checking for nextFrameReady required
              frameBufferA[writePoint][0]=valX;
              frameBufferA[writePoint][1]=valY;
              numPointsFBA++;
            } else { // write to B
              frameBufferB[writePoint][0]=valX;
              frameBufferB[writePoint][1]=valY;
              numPointsFBB++;
            }

            writePoint++;
            
          }   
          if (firstPoint) {
            firstPoint=0;
          } else {
            cli(); //disable interrupts
            nextFrameReady = 1; // loop done, wont start writing until frame read and flipped
            sei(); //reenable interrupts
          }
        } else { // timeout!!!
              Serial.println("Timeout!");
        }
      }      
    } else {
      // got something weird, flush it
      Serial.println("Weird!");
      Serial.print("[");
      Serial.print(Serial.peek());
      Serial.println("]");
      Serial.clear();
      Serial.print("[");
      Serial.print(nextFrameReady);
      Serial.println("]");
    }
  }    
  //delay(2);
  //Serial.print("\r\nYo\r\n");

}
/*
  buf[0] = DAC_X;
  buf[1] = 255;
  buf[2] = 0; 
  buf[3] = DAC_R;
  buf[4] = 255;
  buf[5] = 0; 
  digitalWriteFast(U_CS, LOW); 
  spi4teensy3::send(buf, bufsizeXY);
  digitalWriteFast(U_CS, HIGH); // done

  buf[0] = DAC_Y;
  buf[1] = 0;
  buf[2] = 0; 
  buf[3] = DAC_G;
  buf[4] = 0;
  buf[5] = 0; 
  digitalWriteFast(U_CS, LOW); 
  spi4teensy3::send(buf, bufsizeXY);
  digitalWriteFast(U_CS, HIGH); // done

  buf[0] = DAC_NO;
  buf[1] = 85;
  buf[2] = 85; 
  buf[3] = DAC_B;
  buf[4] = 85;
  buf[5] = 85; 
  digitalWriteFast(U_CS, LOW); 
  spi4teensy3::send(buf, bufsizeXY);
  digitalWriteFast(U_CS, HIGH); // done
*/
