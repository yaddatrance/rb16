/* 
 *	nexgen.h 1.09
 *
 *  Copyrights(c) 2003-2005 by Kyongsu Son, Photon Dream.
 *  All Rights Reserved.
 *
 *  Note: Becayse of a limitation in gcc and egcs, you must compile with
 *  optimisation turned ON (-O1 or higher), or alternatively #define extern
 *  to be empty before you #include <asm/io.h>
 */

#ifndef __NEXGEN_H__
#define __NEXGEN_H__

#define HEADREV "0003:26A"

//#define DEBUGHI 1
int DEBUG=0;


#define MAX_CHUNKSIZE 512

#define CMDB_INVALID		0x80	// No Arguments
#define CMDB_RESET			0x81	// No Arguments
#define CMDB_DISABLE		0x82	// No Arguments
#define CMDB_SPEEDCHANGE	0x83	// 3 Arguements (Bigval, Prescalar, Count)
#define CMDB_COLORMODE		0x84	// 1 Argument (datamode)
#define CMDB_PING			0x85	// No Arguements
#define CMDB_PINGEXT		0x86	// No Arguements
#define CMDB_IDENTIFY		0x87	// No Arguements
#define CMDB_FRAMESWITCH	0x88	// No Arguements
#define CMDB_SHUTTERON		0x89	// No Arguements
#define CMDB_SHUTTEROFF		0x8a	// No Arguements
#define CMDB_MEMDUMP		0x8b	// No Arguements
#define CMDB_FLUSHBUF		0x8c	// No Arguements

#define DUT_NOCLRNOBLNK		0x00	// No color update, don't blank
#define DUT_NOCLRBLNK		0x10	// No color update, blank
#define DUT_CLRNOBLNK		0x20	// Color updated, don't blank
#define DUT_CLRBLNK			0x30	// Color updated, blank
#define DUT_INFRAMESPD		0x40	// In Frame speed change

#define SET_RGBCMY			0x01	// 48-bit color, R,G,B,C,M,Y
#define SET_ZAXIS			0x02	// Z axis support
#define SET_ABLANK			0x04	// Analog Blanking
#define SET_CALCCMY			0x08	// Calc CMY from RGB
#define SET_CMYSUBRGB		0x10	// Calc CMY from RGB and subtract RG
#define SET_INVBLANK		0x20	// Invert Blanking

// bit pattern order
#define DM_RGBCMY			0x00	// 48-bit color, R,G,B,C,M,Y
#define DM_ZAXIS			0x01	// Z axis support
#define DM_ABLANK			0x02	// Analog Blanking
#define DM_CALCCMY			0x03	// Calc CMY from RGB
#define DM_CMYSUBRGB		0x04	// Calc CMY from RGB and subtract RG
#define DM_INVBLANK			0x05	// Invert Blanking

#define SWAP_2(x) ((((x)&0xff)<<8)|((unsigned short)(x)>>8))
#define FIX_SHORT(x) (*(unsigned short *)&(x)=SWAP_2(*unsigned short *)&(x)))

#define LP0 0x3bc
#define LP1 0x378
#define LP2 0x278

#define DELAY 0x80

#define SETCLR  8
#define SETX    10
#define SETY    9
#define SETXY   11
#define SETC    12

#define LASERLED        0		// just for errors
#define LASERNONE       0
#define LASERBLANK      1
#define LASERLATCH      2
#define LASERWRITE      4
#define LASERAXIS       8

#define LASERZERO       0
#define LASERONE      	1
#define LASERTWO      	2
#define LASERFOUR       4
#define LASEREIGHT      8

#define PI 3.14159265

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/io.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <math.h>
//#include <kb.h>
#include <ctype.h>
#include <string.h>
//#include <asm/delay.h>
//#include <linux/delay.h>
#include <usb.h>
#include "bitmask.h"
#include "monkeypatch.h"
#include "ildacolors.h"

unsigned short BASE=0;

int resetboard=1; // default 0, 1 = always reset
void init(void);
void help(char *str);
void sendtolaser(int data, int command);
int readILDA2(char *filename);
int readhello(void);
double get_clock_time();
void gen_sincos();
u_int8_t s16tou8_X(int16_t ic);
u_int8_t s16tou8_Y(int16_t ic);
u_int8_t s32tou8_X(int32_t ic);
u_int8_t s32tou8_Y(int32_t ic);

int s32tosiX(int32_t ic);
int s32tosiY(int32_t ic);

int32_t sx, sy;

int moda, modb, modc;

int MAXFRAMEPTS = 3000;
int MAXFRAMES = 16000;

//static int16_t tc=0;
int color[256][3];
//static int16_t ldata[80000000]; // normal sane values of ldata
static int16_t ldata[180000000];
int32_t liter=0;
static int32_t fpos[65536];
int32_t fiter=0;
int32_t totalpointsinshow;

// fpos stores start byte of frame
// ldata order goes LX_16 LY_16 LZ_16 LB_16 LC_16 rinse repeat

//static int16_t 	lx[16000][3000];
//static int16_t 	ly[16000][3000];
//static int16_t 	lz[16000][3000];
//static char	lb[16000][3000];

int firstframe=1;

static char	lf[65536];  // 0=3D 1=2D
int16_t			realtp;
int16_t 		totalpoints[65536];
int16_t 		totalpointstmp;

int32_t tcarx;
int32_t tcary;
int32_t tcarz;
int32_t scarx[65535];
int32_t scary[65535];



int noblank=0;
char stat1[65535];
char stat2[65535];
int16_t initframe=0;
int16_t initframeset=0;
int16_t currframe=0;
int16_t workingframer=0;
int16_t workingframeg=0;
int16_t workingframeb=0;

int16_t framedelta=1;
int16_t setcurrframetoend=0;
int looptype=0;
int16_t totalframes;
int16_t lastframestanding;
int framedelaytmp=0;
int pointdelay=0;
int scaleset=0;
double scale=1.0;
double xscale=1.0;
double yscale=1.0;
char *ildafile;
struct timeval tv;
double whenisnow;
double backintheday;
double framedelay=0.0;
int runtime=0;
int16_t maxframes=0;
int16_t dispframe=0;
int16_t explicitrange=0;
int16_t rangestart=0;
int16_t rangeend=0;
//int blankahead=-2; // Ivan's setting
//int visibleahead=0;
int blankahead=0;
int visibleahead=0;
int ba_red=-4;
int ba_grn=-4;
int ba_blu=-4;
int va_red=0;
int va_grn=0;
int va_blu=0;

int baoverride=0;
int pdoverride=0;
int blankpause=0;
//int blankpause=5; 
int bitter=0;
int statechange=0;
int realstate=0;
int nextstate=0;
char blankcheck=0;
char vischeck=0;
int frameeven=0;

int colorchannel[8]; // 0 = Red, 1 = Grn, 2 = Blue, 3 = Cyan, 4 = Magenta, 5 = Yellow in standard parlance
int channelon[8]; // each laser has a slew rate for turning on and off
int channeloff[8]; // each laser has a slew rate for turning on and off
#define RED 0
#define GREEN 1
#define BLUE 2
#define CYAN 3
#define MAGENTA 4
#define YELLOW 5

#define BLANK 6
#define ZSTATE 7


int laserstate=0;
int currlaserstate=0;
int nextlaserstate=0;
int oldlaserstate=0;
int realoldlaserstate=0;
int flipoff=0;
int laseron=1, laseroff=0;
int repeatcount=-1;
int invx=0, invy=0;
int invr=0;
int invg=0;
int invb=0;
int swapaxis=0;
int monkeypatch=0;
int monkeyhack=0;
int illusionhack=0;

double xcoord=0, ycoord=0;


double sn[1024];
double cs[1024];

int perspective=32767;

int roadwidth[10];

double xa,za,ya;
int32_t mx=0,mz=32768,my=0;
int yaw=0, roll=0, pitch=0;
int yawdelta=0, rolldelta=0, pitchdelta=0;

int addtailblank=1;

unsigned long currtick=0, nexttick=0, nextbtick=0;

char *myname;

int rotateimage;
int rotateinframedelay=0;

int turnonwhileyoucan=0;
int turnoffwhileyoucan=0;
int thinktoomuch=0;
int offprime=0;

int bca;
int rba=4;
char *filetoopen;
int tc;
int hiya;
int tcr, tcg, tcb;

void initlaser();
void movelaser_nexgen(int16_t datax, int16_t datay, int blank);
void laserto(int datax, int datay, char blank);
int odatax=0, odatay=0;

void turnmeoff();
unsigned long microseconds(void);
int premod(int number, int scalar);

// Desired Pattern for single-bit twiddling
// 000, 001, 011, 010, 110, 100
// Inverting to compensate for LPT ports
// 011, 010, 000, 001, 101, 111
//   3    2,   0,   1,   5,   7
int command_order[]={3,2,0,1,5,7};

int16_t s16clip(int32_t ic);

int32_t rawframe[15000][6];

void dumpframe(int utotalpts);
void dumpnotframe();
int nolmb=0;
int overflow=0;
int onepiece=0;
int speedchange=0;
int XTAL_initval=0;
int inittrans[16]={0,0,0,1,2,3,4,5,6,7};
int XTAL_prescalar=0;

void dumpvadjust();
struct usb_device *current_device;
usb_dev_handle *current_handle;
unsigned long data_buffer;
unsigned char iobuffer[MAX_CHUNKSIZE];    	// Buffer for usb-transfers
int cmy=0;
int allinred=0;
int invblank=0;
//int blankonly=1;
int blankonly=0;
int pingme=0;
int setcolormode=0;
int setdumpnotframe=0;
int setterminate=0;
int disableshutter=0;
int enableshutter=0;
int memorydump=0;
int datamode[16];
int anablank; // average of RGB
int setdumpvadjust=0;
int setdeadbeef=0;
int identifyme=0;
int setflush=0;
int identifyproduct=0;
int programeeprom=0;
int getserial=0;
int pulsemode=0;
int maxpoint=0;
int reallypingme=0;
int laraio=1;
int centerbeam=0;

char hellostr[256];
int hellostr_size;

int minred=0;
//int maxred=200;
int maxred=255;
int mingrn=0;
//int maxgrn=130;
int maxgrn=255;
int minblu=0;
//int maxblu=255;
int maxblu=255;
float rangec;
float adjcv;

#endif
