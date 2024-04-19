/*
 *	nexgen.c
 *
 *  Copyright(c) 2003, 2004 by Kyongsu Son, Photon Dream.
 *  All Rights Reserved.
 *
 *  Note: Becayse of a limitation in gcc and egcs, you must compile with
 *  optimisation turned ON (-O1 or higher), or alternatively #define extern
 *  to be empty before you #include <asm/io.h>
 *
 */

#define GVERSION "v2.0.2a"

#include "nexgen.h"

#ifndef MAX_CHUNKSIZE
#define MAX_CHUNKSIZE 512
#endif

struct usb_device *locate_usb_dev (void) {
	struct usb_bus *bus_search = usb_busses;
	struct usb_device *device_search;
	struct usb_device *device_found = NULL;

	printf ("Dump of USB subsystem:\n");

	while (bus_search != NULL) {
		device_search = bus_search->devices;
		while (device_search != NULL) {
			printf(" bus %s device %s vendor id=0x%04x product id=0x%04x %s\n",
				 bus_search->dirname, device_search->filename,
				 device_search->descriptor.idVendor,
				 device_search->descriptor.idProduct,
				 (device_search->descriptor.idVendor == 0x4b4) &&
				 //(device_search->descriptor.idProduct == 0x8613) ? (device_found == NULL ? "(FX2 Device - USED)" : "(FX2 Device - Unused)") : "");
				 (device_search->descriptor.idProduct == 0x7114) ? (device_found == NULL ? "(FX2 Device - USED)" : "(FX2 Device - Unused)") : "");

			if ((device_search->descriptor.idVendor == 0x4b4) && 
				//(device_search->descriptor.idProduct == 0x8613) && 
				(device_search->descriptor.idProduct == 0x7114) && 
				(device_found == NULL)) {
				device_found = device_search;
			}

			device_search = device_search->next;
		}
		bus_search = bus_search->next;

	}
	fflush (stdout);

	return device_found;

}

int write_bulkdata (int len, unsigned char *buffer, usb_dev_handle *writeto) {
	int a;

	if (usb_claim_interface (writeto, 0) < 0) {
		fprintf (stderr, "Could not claim interface 0: %s\n", usb_strerror());
		return -1;
	}

	usb_set_altinterface (writeto, 1);

	a = usb_bulk_write (writeto, 0x02, buffer, len, 1000); // 1s timeout
	if (a < 0) {
		fprintf (stderr, "Request for bulk write of %d bytes failed: %s\n",
				len, usb_strerror ());
		usb_release_interface (writeto, 0);
		return -1;
	}

	fflush (stdout);

	usb_release_interface (writeto, 0);
	return 0;
}

int write_resetdata (int len, unsigned char *buffer, usb_dev_handle *writeto) {
	int a;

	if (usb_claim_interface (writeto, 0) < 0) {
		fprintf (stderr, "Could not claim interface 0: %s\n", usb_strerror());
		return -1;
	}

	usb_set_altinterface (writeto, 1);

	a = usb_bulk_write (writeto, 0x01, buffer, len, 1000); // 1s timeout
	if (a < 0) {
		fprintf (stderr, "Request for bulk write of %d bytes failed: %s\n",
				len, usb_strerror ());
		usb_release_interface (writeto, 0);
		return -1;
	}

	fflush (stdout);

	usb_release_interface (writeto, 0);
	return 0;
}


void read_bulkdata (int len, unsigned char *buffer, usb_dev_handle *readof) {
	int a;
	if (usb_claim_interface (readof, 0) < 0) {
		fprintf (stderr, "Could not claim interface 0: %s\n", usb_strerror());
		return;
	}

	usb_set_altinterface (readof, 1);

	a = usb_bulk_read (readof, 0x84, buffer, len, 1000);

	if (a < 0) {
		fprintf (stderr, "Request for bulk read of %d bytes failed: %s\n",
				len, usb_strerror ());
		usb_release_interface (readof, 0);
		return;
	}

	fflush (stdout);
	usb_release_interface (readof, 0);
}

void read_resetdata (int len, unsigned char *buffer, usb_dev_handle *readof) {
	int a;
	if (usb_claim_interface (readof, 0) < 0) {
		fprintf (stderr, "Could not claim interface 0: %s\n", usb_strerror());
		return;
	}

	usb_set_altinterface (readof, 1);

	a = usb_bulk_read (readof, 0x81, buffer, len, 1000);

	if (a < 0) {
		fprintf (stderr, "Request for bulk read of %d bytes failed: %s\n",
				len, usb_strerror ());
		usb_release_interface (readof, 0);
		return;
	}

	fflush (stdout);
	usb_release_interface (readof, 0);
}


int main(int argc, char **argv) {
	int i, k;
	int iter=0;
	unsigned long Start, Stop;
	myname=argv[0];
	rotateimage=0;

	if (argc>1) {
		for (k=1;k<argc;k++) {
			if (!strcmp(argv[k],"-h")) { 
				help(argv[0]);
			} else if (!strcmp(argv[k],"-p")) { // Parallel Port
				k++;
				if (argv[k]!=NULL) {
					switch (argv[k][0]) {
						case '0': BASE = LP0; break;
						case '1': BASE = LP1; break;
						case '2': BASE = LP2; break;
						default : help(argv[0]);
					}
				} else {
					help(argv[0]);
				}
			} else if (!strcmp(argv[k],"-blankonly")) {
				blankonly=1;
			} else if (!strcmp(argv[k],"-k")) {
				invblank=1;
				laseroff=laseron;    // Invert on/off signals
				laseron=!laseroff;
			} else if (!strcmp(argv[k],"-offprime")) {
				offprime=1;
			} else if (!strcmp(argv[k],"-dumpvadjust")) {
				setcolormode=1;
				setdumpvadjust=1;
				cmy=0x1F;
			} else if (!strcmp(argv[k],"-cmy")) {
				setcolormode=1;
				k++;
				if (argv[k]!=NULL) {
					cmy=atoi(argv[k]);
					for(zz=0;zz<8;zz++) {
						datamode[zz]=(cmy & bm[zz])>>zz;
					}
					printf("Data Mode [");
					for (zz=7;zz>-1;zz--) {
						printf("%d", datamode[zz]);
					}
					printf("]\n");
					if (datamode[DM_RGBCMY]) {
						printf("48bit color\n"); // needs CMY enabled
						if (!datamode[DM_CALCCMY]) {
							printf("You did not enable CMY Channels!\n");
							exit(0);
						}
					} else {
						printf("24bit color\n");
					}
					if (datamode[DM_ZAXIS]) {
						printf("Z axis support\n");
					}
					if (datamode[DM_ABLANK]) {
						printf("Analog Blanking\n");
					}
					if (datamode[DM_CALCCMY]) {
						printf("Enable CMY Channels\n"); // RGB
					}
					if (datamode[DM_CMYSUBRGB]) {
						printf("CMY (-RG Mode)\n"); // RGB, needs CMY enabled
						if (!datamode[DM_CALCCMY]) {
							printf("You did not enable CMY Channels\n");
							exit(0);
						}
					}
					if (datamode[DM_INVBLANK]) {
						printf("Inverted Blanking\n");
					}
				} else {
					help(argv[0]);
				}
			} else if (!strcmp(argv[k],"-flushlmb")) {
				nolmb=1;
				setflush=1;
			} else if (!strcmp(argv[k],"-LMB")) {
				nolmb=1;
			} else if (!strcmp(argv[k],"-onepiece")) {
				printf ("-onepiece disabled in this code\n");
				onepiece=1;
			} else if (!strcmp(argv[k],"-overflow")) {
				k++;
				if (argv[k]!=NULL) {
					overflow=atoi(argv[k]);
				} else {
					help(argv[0]);
				}
			} else if (!strcmp(argv[k],"-pulsemode")) {
				pulsemode=1;
			} else if (!strcmp(argv[k],"-laraio")) {
				laraio=1;
			} else if (!strcmp(argv[k],"-speedchange")) {
				speedchange=1;
				k++;
				if (argv[k]!=NULL) {
					XTAL_prescalar=atoi(argv[k]);
					if (DEBUG) printf ("XTAL Prescalar %u\n",XTAL_prescalar);
				} else {
					help(argv[0]);
				}
				k++;
				if (argv[k]!=NULL) {
					XTAL_initval=atoi(argv[k]);
					if (DEBUG) printf ("Crystal Initval %u\n",XTAL_initval);
				} else {
					help(argv[0]);
				}
				if ((XTAL_prescalar>7)||(XTAL_prescalar<1)) {
					printf("PRESCALAR must be between 1 and 7 !!\n");
					help(argv[0]);
				}			
				if ((XTAL_initval>127)||(XTAL_initval<1)) {
					printf("INITVAL must be less than 128 and greater than 0!!!\n");
					help(argv[0]);
				}			
			} else if (!strcmp(argv[k],"-dumpnotframe")) {
				setdumpnotframe=1;
			} else if (!strcmp(argv[k],"-deadbeef")) {
				setdumpnotframe=1;
			} else if (!strcmp(argv[k],"-term")) {
				setterminate=1;
			} else if (!strcmp(argv[k],"-ttm")) {
				thinktoomuch=1;
			} else if (!strcmp(argv[k],"-n")) {
				noblank=1;
			} else if (!strcmp(argv[k],"-x")) {
				invx=1;
			} else if (!strcmp(argv[k],"-y")) {
				invy=1;
			} else if (!strcmp(argv[k],"-ping")) {
				pingme=1;
			} else if (!strcmp(argv[k],"-pingext")) {
				reallypingme=1;
			} else if (!strcmp(argv[k],"-whoami")) {
				identifyme=1;
			} else if (!strcmp(argv[k],"-serial")) {
                getserial=1;
			} else if (!strcmp(argv[k],"-w")) {
				swapaxis=1;
			} else if (!strcmp(argv[k],"-invr")) {
				invr=1;
			} else if (!strcmp(argv[k],"-invg")) {
				invg=1;
			} else if (!strcmp(argv[k],"-invb")) {
				invb=1;
			} else if (!strcmp(argv[k],"-allinred")) {
				allinred=1;
			} else if (!strcmp(argv[k],"-reset")) {
				resetboard=1;
			} else if (!strcmp(argv[k],"-center")) {
				centerbeam=1;
			} else if (!strcmp(argv[k],"-m")) {
				// #$@! Monkey Tools starts at Frame 1 
				monkeypatch=1;
			} else if (!strcmp(argv[k],"-whoisusb")) {
				identifyproduct=1;

			} else if (!strcmp(argv[k],"-flasheeprom")) {
				programeeprom=1;
				k++;
				if (argv[k]!=NULL) {
					strncpy(hellostr,argv[k],256);
				} else {
					help(argv[0]);
				}
			} else if (!strcmp(argv[k],"-s")) {
				k++;
				if (argv[k]!=NULL) {
					scale=atof(argv[k]);
					if (DEBUG) printf ("scale %f\n",scale);
					scaleset=1;
				} else {
					help(argv[0]);
				}
			} else if (!strcmp(argv[k],"-r")) {
				explicitrange=1;
				k++;
				if (argv[k]!=NULL) {
					rangestart=atoi(argv[k]);
					if (DEBUG) printf ("start frame %us\n",rangestart);
				} else {
					help(argv[0]);
				}
				k++;
				if (argv[k]!=NULL) {
					rangeend=atoi(argv[k]);
					if (DEBUG) printf ("end frame %us\n",rangeend);
				} else {
					help(argv[0]);
				}
				if (rangestart>rangeend) {
					printf ("Range End must be larger!\n");
					help(argv[0]);
				}
			} else if ((!strcmp(argv[k],"-3D"))||(!strcmp(argv[k],"-3d"))) {
				rotateimage=1;
				k++;
				if (argv[k]!=NULL) {
					pitchdelta=atoi(argv[k]);
					if (DEBUG) printf ("pitch delta %us\n",pitchdelta);
				} else {
					help(argv[0]);
				}
				k++;
				if (argv[k]!=NULL) {
					yawdelta=atoi(argv[k]);
					if (DEBUG) printf ("yaw delta %us\n",yawdelta);
				} else {
					help(argv[0]);
				}
				k++;
				if (argv[k]!=NULL) {
					rolldelta=atoi(argv[k]);
					if (DEBUG) printf ("roll delta %us\n",rolldelta);
				} else {
					help(argv[0]);
				}
			} else if (!strcmp(argv[k],"-o")) {
				rotateimage=1;
				k++;
				if (argv[k]!=NULL) {
					pitch=atoi(argv[k]);
					if (DEBUG) printf ("pitch %us\n",pitch);
				} else {
					help(argv[0]);
				}
				k++;
				if (argv[k]!=NULL) {
					yaw=atoi(argv[k]);
					if (DEBUG) printf ("yaw %us\n",yaw);
				} else {
					help(argv[0]);
				}
				k++;
				if (argv[k]!=NULL) {
					roll=atoi(argv[k]);
					if (DEBUG) printf ("roll %us\n",roll);
				} else {
					help(argv[0]);
				}
			} else if (!strcmp(argv[k],"-l")) {
				k++;
				if (argv[k]!=NULL) {
					looptype=atoi(argv[k]);
					if (DEBUG) printf ("LOOPTYPE %d us\n",looptype); 
				} else {
					help(argv[0]);
				}
			} else if (!strcmp(argv[k],"-z")) {
				k++;
				if (argv[k]!=NULL) {
					framedelta=atoi(argv[k]);
					if (DEBUG) printf ("framedelta %d\n",framedelta); 
				} else {
					help(argv[0]);
				}
				if (framedelta<0) setcurrframetoend=1;
			} else if (!strcmp(argv[k],"-i")) {
				k++;
				if (argv[k]!=NULL) {
					initframe=atoi(argv[k]);
					initframeset=1;
					if (DEBUG) printf ("initframe %d us\n",initframe); 
				} else {
					help(argv[0]);
				}
			} else if (!strcmp(argv[k],"-t")) {
				rotateinframedelay=1;
			} else if (!strcmp(argv[k],"-g")) {
				DEBUG=1;
			} else if (!strcmp(argv[k],"-c")) {
				k++;
				if (argv[k]!=NULL) {
					repeatcount=atoi(argv[k]);
					if (DEBUG) printf ("Repeat %d\n",repeatcount); 
				} else {
					help(argv[0]);
				}
			} else if (!strcmp(argv[k],"-d")) {
				k++;
				if (argv[k]!=NULL) {
					pointdelay=atoi(argv[k]);
					if (DEBUG) printf ("Delaying for %d us\n",pointdelay); 
					pdoverride=1;
				} else {
					help(argv[0]);
				}
			} else if (!strcmp(argv[k],"-b")) {
				k++;
				if (argv[k]!=NULL) {
					blankahead=atoi(argv[k]);
					if (DEBUG) printf ("Blanking set at %d frames\n",blankahead); 
					baoverride=1;
				} else {
					help(argv[0]);
				}
			} else if (!strcmp(argv[k],"-bphack")) {
				k++;
				if (argv[k]!=NULL) {
					blankpause=atoi(argv[k]);
					if (DEBUG) printf ("Blank Pause Hack engaged for %d points\n",blankpause); 
				} else {
					help(argv[0]);
				}
			} else if (!strcmp(argv[k],"-v")) {
				k++;
				if (argv[k]!=NULL) {
					visibleahead=atoi(argv[k]);
					if (DEBUG) printf ("Visible set at %d frames\n",visibleahead); 
				} else {
					help(argv[0]);
				}
			} else if (!strcmp(argv[k],"-maxpoint")) {
				k++;
				if (argv[k]!=NULL) {
					maxpoint=atoi(argv[k]);
					printf("limiting frames to %d points per frame\n",maxpoint);
				} else {
					help(argv[0]);
				}
			} else if (!strcmp(argv[k],"-f")) {
				k++;
				if (argv[k]!=NULL) {
					framedelaytmp=atoi(argv[k]);
					framedelay=framedelaytmp * 1.e-3;
					if (DEBUG) printf ("Delaying for %f us\n",framedelay); 
					if (framedelaytmp>1000) dispframe=1;
				} else {
					help(argv[0]);
				}
			} else if (argv[k]!=NULL) { // filename
				filetoopen=argv[k];
			} else {
				help(argv[0]);
			}
		}
	}

	printf("\nInitializing NexGen %s %s\n\n", GVERSION, HEADREV);
	init();
	initlaser();

	Start=microseconds();
	if (filetoopen!=NULL) {
		readILDA2(filetoopen);
	} else {
		readILDA2("/fx2/data/9x9grid.ild");
	}
	Stop=microseconds();
	printf("Load time: %fs\n",(float)(((float)Stop-(float)Start)/1000000)); 
	if (lastframestanding >= totalframes) {
		printf("Patching Pangolin's new buggy ILDA totalframe!\n");
		totalframes=lastframestanding+1;
	}

	if (DEBUG) printf ("ILDA FILE SUCCESSFULLY READ.\n");

	backintheday=get_clock_time();
	if (DEBUG) printf ("Whenisnow = %f\n",whenisnow);

	atexit(turnmeoff);

	if ((rotateimage)&&(!scaleset)) {
		scale=0.5;
	}
	if (dispframe) printf("Current Frame: %d\n",currframe);
	for (;;) {
		iter++;
		if (iter>5) iter=0;
		if (repeatcount!=-1) {
			if (repeatcount==0) {
				turnmeoff();
			} 
		}

		for (i=0;i<totalpoints[currframe];i++) {
			tcarx=ldata[fpos[currframe]+i*5]*scale;  
			tcary=ldata[fpos[currframe]+i*5+1]*scale;  
			tcarz=ldata[fpos[currframe]+i*5+2]*scale;  

			// sanity for x to make right-hand rule work
			tcarx=-1*tcarx;

			// rotation about Y-axis (yaw)
			xa=cs[yaw]*(double)tcarx-sn[yaw]*(double)tcarz;
			za=sn[yaw]*(double)tcarx+cs[yaw]*(double)tcarz;

			// rotation about Z-axis (roll)
			tcarx=(int32_t) (cs[roll]*xa+sn[roll]*(double)tcary);
			ya=cs[roll]*(double)tcary-sn[roll]*xa;

			// rotation about X-axis (pitch)
			tcarz=(int32_t) (cs[pitch]*za-sn[pitch]*ya);
			tcary=(int32_t) (sn[pitch]*za+cs[pitch]*ya);

			// translation
			tcarx=tcarx+mx;
			tcary=tcary+my;
			tcarz=tcarz+mz;

			if (tcarz==0) tcarz=1; // sanity check
			scarx[i]=perspective*tcarx/tcarz;
			scary[i]=perspective*tcary/tcarz;


			if (!offprime) {
			  if (!laraio) {
				moda=premod(i+visibleahead,totalpoints[currframe]);
				vischeck=ldata[fpos[currframe]+moda*5+3];
				modb=premod(i+blankahead,totalpoints[currframe]);
				blankcheck=ldata[fpos[currframe]+modb*5+3];

				if (vischeck==0) laserstate=laseron;
				if (blankcheck==1) laserstate=laseroff;

			  } else {
				// adjust for laraio's creepy red system

				// with slower lasers, you have two choices, default off and default on.
				// this means that when the frequency of modulation is higher than what
				// is natively supported, one leaves the laser off/on respectively.

				// this configuration is testing the laser OFF defaults.
				moda=premod(i+visibleahead,totalpoints[currframe]);
				vischeck=ldata[fpos[currframe]+moda*5+3];
				modb=premod(i+blankahead,totalpoints[currframe]);
				blankcheck=ldata[fpos[currframe]+modb*5+3];

				if (vischeck==0) laserstate=laseron;
				if (blankcheck==1) laserstate=laseroff;
					

			  }
			} else {
				// offprime selected

				blankcheck=0;
				rba=0;
				laserstate=laseron;
				for (bca=-2;bca<rba;bca++) {
					modb=premod(i+bca,totalpoints[currframe]);
					if (ldata[fpos[currframe]+modb*5+3]) {
						laserstate=laseroff;
					}
				}
			}

			if (noblank) laserstate=laseron;

			tc=ldata[fpos[currframe]+i*5+4];
			rawframe[i][2]=ilda_standard_color_palette[tc][0];
			rawframe[i][3]=ilda_standard_color_palette[tc][1];
			rawframe[i][4]=ilda_standard_color_palette[tc][2];

			//printf("%d) R: %d G: %d B: %d\n",i,rawframe[i][2],rawframe[i][3],rawframe[i][4]);
	if (allinred) {
                //if (laseron) {
                 //   rawframe[i][2]=0;
                //} else {
                    //rawframe[i][2]=255;
               // }
               // if (noblank) rawframe[i][2]=0;
            }

			if (noblank) {
				laserstate=laseron;
			}
			if (laserstate==laseroff) {
				rawframe[i][2]=0;
				rawframe[i][3]=0;
				rawframe[i][4]=0;
			}
			rawframe[i][5]=laserstate;

			if (swapaxis) {
				rawframe[i][0]=scary[i];
				rawframe[i][1]=scarx[i];
			} else {
				rawframe[i][0]=scarx[i];
				rawframe[i][1]=scary[i];
			}
		}
		if (setdumpvadjust) dumpvadjust(); // and then quit

		if (setdumpnotframe) {
			dumpnotframe();
		}else{
			dumpframe(totalpoints[currframe]);
		}
		if (overflow) sleep(overflow);
		whenisnow=get_clock_time();
		if (whenisnow>backintheday+framedelay) {
			backintheday=whenisnow;
			//currframe++;
			currframe+=framedelta;
			if (explicitrange) {
				if (looptype) { // pingpong
					if (currframe>rangeend) {
						currframe=rangeend-1;
						framedelta=-1*framedelta;
						if (repeatcount!=-1) repeatcount--;
					} else if (currframe<rangestart) {
						currframe=rangestart+1;
						framedelta=-1*framedelta;
						if (repeatcount!=-1) repeatcount--;
					}
				} else { // loop
					if (currframe>rangeend) {
						currframe=rangestart;
						if (repeatcount!=-1) repeatcount--;
					} else if (currframe<rangestart) {
						currframe=rangeend;
						if (repeatcount!=-1) repeatcount--;
					}
				}
			} else {
				if (looptype) { // pingpong
					if (currframe>=totalframes) {
						currframe=totalframes-2;
						framedelta=-1*framedelta;
						if (repeatcount!=-1) repeatcount--;
					} else if (currframe<0) {
						currframe=1;
						framedelta=-1*framedelta;
						if (repeatcount!=-1) repeatcount--;
					}
				} else { // loop
					if (currframe>=totalframes) {
						currframe=0;
						if (repeatcount!=-1) repeatcount--;
					} else if (currframe<0) {
						currframe=totalframes-1;
						if (repeatcount!=-1) repeatcount--;
					}
				}
			}
			frameeven=currframe%2;
			if (dispframe) printf("Current Frame: %d\n",currframe);
			if ((rotateimage)&&(rotateinframedelay)) {
				// Sanity check
				yaw=(yaw+yawdelta)%256;
				if (yaw>255) yaw=0; 
				if (yaw<0) yaw=255;
				roll=(roll+rolldelta)%256;
				if (roll>255) roll=0;
				if (roll<0) roll=255;
				pitch=(pitch+pitchdelta)%256;
				if (pitch>255) pitch=0;
				if (pitch<0) pitch=255;
			}
		}
		if ((rotateimage)&&(!rotateinframedelay)) {
			// Sanity check
			yaw=(yaw+yawdelta)%256;
			if (yaw>255) yaw=0; 
			if (yaw<0) yaw=255;
			roll=(roll+rolldelta)%256;
			if (roll>255) roll=0;
			if (roll<0) roll=255;
			pitch=(pitch+pitchdelta)%256;
			if (pitch>255) pitch=0;
			if (pitch<0) pitch=255;
		}
	}

	exit(0); /* AOK! */
}

void init() {
	int i;
	int ctr;
	if (pointdelay==0) pointdelay=50;
	//	framedelay=0.07;
	if ((explicitrange)&&(!initframeset)) {
		currframe=rangestart;
	} else {
		currframe=initframe;
	}

	gen_sincos();

	for (i=0;i<256;i++) {
		color[i][0]=ilda_standard_color_palette[i][0];
		color[i][1]=ilda_standard_color_palette[i][1];
		color[i][2]=ilda_standard_color_palette[i][2];
	}
	for (i=0;i<8;i++) {
		colorchannel[i]=0; // preset all channels to off
	}

	usb_init();                  				// Always needed 4 USB
	usb_find_busses();           				// Get all usb busses
	usb_find_devices();          				// Get all usb devices
	current_device = locate_usb_dev();       	// Find the first FX2 Device
	if (current_device == NULL) {               // If no device is present
		fprintf (stderr, "Cannot find FX2 Device on any Bus\n");
		exit(-1);
	}
	current_handle = usb_open(current_device);	// Open USB dev for RW
	if (resetboard) {
		printf("Sending explicit RESET\n");
		iobuffer[0]='R'; // CMD_B: speed change
		write_resetdata(1, &iobuffer[0], current_handle);
		usleep(250000);
	}
	if (centerbeam) {
		printf("Sending explicit RESET\n");
		iobuffer[0]='R'; // CMD_B: speed change
		write_resetdata(1, &iobuffer[0], current_handle);
		exit(0);
	}
	if (!nolmb) {
		printf("Sending Handshake!\n");
		if (write_bulkdata(3, "LMB", current_handle)==-1) {
			// 0x4C, 0x4D, 0x42 handshake
			printf ("Handshake FAILED!\n");
		}
	} else {
		if (setflush) { // -flushlmb
			iobuffer[0]=CMDB_FLUSHBUF;
			if (write_bulkdata(1, iobuffer, current_handle)==-1) {
				printf ("No can flush!\n");
			}
		}
	}

	if (speedchange) { // default is initval=83, prescalar=16
		iobuffer[0]=CMDB_SPEEDCHANGE; // CMD_B: speed change
		iobuffer[1]=1;
		iobuffer[2]=XTAL_prescalar;
		iobuffer[3]=XTAL_initval;
		write_bulkdata(4, &iobuffer[0], current_handle);
		
	}
	if (setcolormode) {
		ctr=0;
 		iobuffer[ctr]=CMDB_COLORMODE; // CMD_B: set color mode
		ctr++;
 		iobuffer[ctr]=cmy; // 0x00 RGB, 0x01 CMY, 0x02 CMY w/ correction
		ctr++;
		if (((cmy&7)!=0)||(cmy==0)) {
			printf("Sending Second Handshake, Jon you bastard!\n");
 			iobuffer[ctr]='L'; 
			ctr++;
 			iobuffer[ctr]='M'; 
			ctr++;
 			iobuffer[ctr]='B'; 
			ctr++;
		}
		write_bulkdata(ctr, iobuffer, current_handle);
		ctr=0;
	}

	if (disableshutter) {
		iobuffer[0]=CMDB_SHUTTEROFF;
		write_bulkdata(1, &iobuffer[0], current_handle);
	}
	if (enableshutter) {
		iobuffer[0]=CMDB_SHUTTERON;
		write_bulkdata(1, &iobuffer[0], current_handle);
	}

	if (pingme) {
		iobuffer[0]=CMDB_PING; // CMD_B: pingme
		write_bulkdata(1, &iobuffer[0], current_handle);
		usleep(250000);
		read_bulkdata(256, iobuffer, current_handle);
		printf("PONG! 0x%02X 0x%02X \n", iobuffer[0], iobuffer[1]);

	 	if (setterminate) {
   	     	exit(0);
		}
	}

	if (reallypingme) {
		iobuffer[0]=CMDB_PINGEXT; // CMD_B: pingme
		write_bulkdata(1, &iobuffer[0], current_handle);
		usleep(250000);
		read_bulkdata(256, iobuffer, current_handle);
		printf("EPONG! 0x%02X\n", iobuffer[0]);
		printf("Status Register: 0x%02x\n",iobuffer[1]);
		printf("Data Mode: 0x%02x\n",iobuffer[2]);
		printf("Current Loop Count: 0x%02x\n",iobuffer[3]);
		printf("Last Loop Count: 0x%02x\n",iobuffer[4]);
		printf("FIFO Count: 0x%02x\n",iobuffer[5]);
		printf("ISR Big Count: 0x%02x\n",iobuffer[6]);

	 	if (setterminate) {
   	     	exit(0);
		}
	 

	}
	if (identifyme) {
		iobuffer[0]=CMDB_IDENTIFY; // CMD_B: identify
		write_bulkdata(1, &iobuffer[0], current_handle);

		sleep(1);
		read_bulkdata(256, iobuffer, current_handle);
		printf(" 0x%02X : [", iobuffer[0]);
		for (i=1;i<256;i++) {
			if (iobuffer[i]==0xFF) break;
			printf("%c",iobuffer[i]);
		}
		printf("]\n");
	    if (setterminate) {
   	       exit(0);
	    }
	}
	if (identifyproduct) {
		iobuffer[0]='V'; // CMD_B: identify
		write_resetdata(1, &iobuffer[0], current_handle);

		sleep(1);
		read_resetdata(256, iobuffer, current_handle);
		printf("[");
		for (i=0;i<256;i++) {
			if (iobuffer[i]==0xFF) break;
			printf("%c",iobuffer[i]);
		}
		printf("]\n");
	    if (setterminate) {
   	       exit(0);
	    }

	}

	if (getserial) {
        iobuffer[0]='I'; // CMD_B: identify
        write_resetdata(1, &iobuffer[0], current_handle);

        sleep(1);
        read_resetdata(256, iobuffer, current_handle);
        printf("[");
        for (i=0;i<6;i++) {
            //if (iobuffer[i]==0xFF) break;
            printf("%c",iobuffer[i]);
        }
        printf("]\n");
        if (setterminate) {
           exit(0);
        }

    }

	if (programeeprom) {
		ctr=0;
		iobuffer[ctr]='S'; // CMD_B: identify
		ctr++;
		iobuffer[ctr]='K'; // CMD_B: identify
		ctr++;
		iobuffer[ctr]='I'; // CMD_B: identify
		ctr++;
		iobuffer[ctr]='V'; // CMD_B: identify
		ctr++;
		iobuffer[ctr]='A'; // CMD_B: identify
		ctr++;

		hellostr_size=strlen(hellostr);
		if (hellostr_size==0) {
			printf("No string specified, terminating\n");
		}
		for (i=0;i<hellostr_size;i++) {
			iobuffer[ctr]=hellostr[i]; // CMD_B: identify
			ctr++;
		}
		write_resetdata(ctr, &iobuffer[0], current_handle);
		printf("wrote to eeprom, good luck!\n");

		sleep(1);
        read_resetdata(256, iobuffer, current_handle);
        printf("[");
        for (i=0;i<6;i++) {
            //if (iobuffer[i]==0xFF) break;
            printf("%c",iobuffer[i]);
        }
		printf("]\n");

		exit(0);
	}
}


int readILDA2(char *filename) {
	FILE *fd;
	int i;
	int rawinput;
	u_int8_t tmpvar1, tmpvar2;
	char inputchar;
	//char ildaheader[5];
	u_int8_t formatcode;
	char framename[8];
	char companyname[8];
	int16_t framenumber;
	int ildavalid=0;
	char scanhead;

	if (DEBUG) printf("Reading ILDA file [%s]\n",filename);
	fd=fopen(filename, "r");
	if (!fd) {
		printf("\nWARNING! Unable to open file %s\n\n",filename);
		help(myname);
	}

	for (;;) {
		/* ILDA Header
		 * 4 bytes, 'I','L','D','A'
		 */
		ildavalid=0;
		for (;;) {
			rawinput=fgetc(fd);
			if (rawinput==EOF) { // sanity check AURA doesn't
				return(0);   // end with proper ILDA end frame
			}
			inputchar=(char) rawinput;
			if (inputchar=='I') {
				ildavalid=1;
			} else if (inputchar=='L') {
				if (ildavalid==1) {
					ildavalid=2;
				} else ildavalid=0;	
			} else if (inputchar=='D') {
				if (ildavalid==2) {
					ildavalid=3;
				} else ildavalid=0;	
			} else if (inputchar=='A') {
				if (ildavalid==3) {
					break;
				} else ildavalid=0;	
			} else {
				ildavalid=0;	
			}

		}

		/* ILDA Header: 3 Binary Zeros
		 * Not used, but required for file format
		 */
		for (i=0;i<3;i++) {
			inputchar=(char)fgetc(fd);
		}

		/* ILDA Header: Format Code
		 * Binary 0 for 3D (4 words/pt)
		 * Binary 1 for 2D (3 words/pt)
		 */
		formatcode=(char)fgetc(fd);
		//		if (formatcode==0) printf("Formatcode: 3D\n"); 
		//		else printf("Formatcode: 2D\n");
		if (formatcode>2) printf ("Mental Note: This ILDA set has a nonstandard (or new) formatcode: %d\n", formatcode);
		if ((formatcode==0)||(formatcode==1)) {

			/* ILDA Header: Frame Name
			 */
			for (i=0;i<8;i++) { // frame name
				framename[i]=(char)fgetc(fd);
			}
			framename[8]='\0';

			/* ILDA Header: Company Name
			 */
			for (i=0;i<8;i++) { // company name
				companyname[i]=(char)fgetc(fd);
			}
			companyname[8]='\0';
			if (!strcmp(companyname,"MNKYTOOL")) {
				printf("A REAL MONKEY!\n");
				monkeyhack=1;
				if (!baoverride) blankahead=-3;
				if (!pdoverride) pointdelay=100;
			}

			/* ILDA Header: Total Points
			 * (1-65535) Total points in frame.
			 * If number is 0, interpreted as EOF
			 */

			tmpvar1=fgetc(fd);
			tmpvar2=fgetc(fd);
			totalpointstmp=tmpvar1*256+tmpvar2;
			if (totalpointstmp==0) {
				fclose(fd);
				return(1);
			}

			/* ILDA Header: Frame Number
			 * (0-65535) Represents frame number in a sequence
			 */
			lastframestanding=framenumber=(u_char)fgetc(fd)*256+(u_char)fgetc(fd);
			if (firstframe) {
				firstframe=0; // no longer first frame
				if (framenumber==1) {
					if (!monkeypatch) monkeypatch=1;
					//if (DEBUG) printf("MONKEY ILLUSIONS!\n");
					printf("Implemented MONKEY/ILLUSION Shift patch!\n");
				}
			}
			if (monkeypatch) framenumber--;

			totalpoints[framenumber]=totalpointstmp;

			/* ILDA Header: Total Frames
			 */ 
			totalframes=(u_char)fgetc(fd)*256+(u_char)fgetc(fd);
			if (DEBUG) printf ("Total Points: [%d]\nFrame Number: [%d]\nTotal Frames: [%d]\n",totalpointstmp, framenumber, totalframes);
			if ((setcurrframetoend)&&(!explicitrange)) {
				currframe=totalframes-1;
			}

			/* ILDA Header: Scanhead
			 * (0-255) the scanhead to display on
			 */
			scanhead=fgetc(fd);

			/* ILDA Header: zero byte 
			 */
			inputchar=(char)fgetc(fd);
			if (DEBUG) printf ("Junk [%x]\n",inputchar);
			if (formatcode==0) {
				//rotateimage=1;
				if (DEBUG) printf("Frame Type: 3D\n"); 
				lf[framenumber]=0;
				fpos[fiter]=liter;
				fiter++;
				for (i=0;i<totalpoints[framenumber];i++) {

					/* Coordinate Data: X coordinate
					 * signed 16-bit binary number.
					 * extreme left is -32768
					 * extreme right is 32768
					 */
					tmpvar1=fgetc(fd);
					tmpvar2=fgetc(fd);
					ldata[liter+i*5]=tmpvar1*256+tmpvar2;

					/* Coordinate Data: Y coordinate
					 * signed 16-bit binary number.
					 * extreme bottom is -32768
					 * extreme top is 32768
					 */
					tmpvar1=fgetc(fd);
					tmpvar2=fgetc(fd);
					ldata[liter+i*5+1]=tmpvar1*256+tmpvar2;

					/* Coordinate Data: Z coordinate 
					 * (Only for 3D)
					 * signed 16-bit binary number.
					 * extreme rear is -32768
					 * extreme front is 32768
					 */
					tmpvar1=fgetc(fd);
					tmpvar2=fgetc(fd);
					ldata[liter+i*5+2]=tmpvar1*256+tmpvar2;
					if (ldata[liter+i*5+2]!=0) rotateimage=1;

					/* Coordinate Data: Status Code
					 *
					 * Bytes 39-40
					 * bits 0-7, (lsb) color tbl lookup
					 * bits 8-13 reserved, set to 0
					 * bit 14, blanking
					 * bit 15, (1=last point, 0=all others)
					 */
					stat1[i]=fgetc(fd);
					stat2[i]=fgetc(fd);
					ldata[liter+i*5+3]=(stat1[i] & 0x40)>>6; // status blanking
					ldata[liter+i*5+4]= stat2[i]; // color table

					for (zz=0;zz<8;zz++) {
						bp1[zz]=(stat1[i] & bm[zz])>>zz;
						bp2[zz]=(stat2[i] & bm[zz])>>zz;
					}
					if (monkeypatch) {
						if (monkeyhack) {
							if (stat2[i]==0) { // off
								monkeylaser=1;
							} else { // on
								monkeylaser=0;
							}
							ldata[liter+i*5+3]=monkeylaser;
						} else {
							if (bp1[5]==1) { // off
								monkeylaser=1;
							}
							if (bp1[4]==1) { // on
								monkeylaser=0;
							}
							ldata[liter+i*5+3]=monkeylaser;
						}
					}


					// Display coordinate data info
				}
/*				if (addtailblank) {
					realtp=totalpoints[framenumber];
					totalpoints[framenumber]+=4;
					for (i=realtp;i<totalpoints[framenumber];i++) {
						ldata[liter+i*5+0]=ldata[liter+(realtp-1)*5+0];
						ldata[liter+i*5+1]=ldata[liter+(realtp-1)*5+1];
						ldata[liter+i*5+2]=ldata[liter+(realtp-1)*5+2];
						ldata[liter+i*5+3]=1;
					}


				}
				//				printf("liter: %d\n",liter);
*/
				liter=liter+totalpoints[framenumber]*5;
			} else if (formatcode==1) {
				lf[framenumber]=1;
				fpos[fiter]=liter;
				fiter++;
				if (DEBUG) printf("Frame Type: 2D\n");
				for (i=0;i<totalpoints[framenumber];i++) {

					/* Coordinate Data: X coordinate
					 * signed 16-bit binary number.
					 * extreme left is -32768
					 * extreme right is 32768
					 */
					tmpvar1=fgetc(fd);
					tmpvar2=fgetc(fd);
					ldata[liter+i*5]=tmpvar1*256+tmpvar2;

					/* Coordinate Data: Y coordinate
					 * signed 16-bit binary number.
					 * extreme bottom is -32768
					 * extreme top is 32768
					 */
					tmpvar1=fgetc(fd);
					tmpvar2=fgetc(fd);
					ldata[liter+i*5+1]=tmpvar1*256+tmpvar2;

					ldata[liter+i*5+2]=0;

					/* Coordinate Data: Status Code
					 *
					 * Bytes 39-40
					 * bits 0-7, (lsb) color tbl lookup
					 * bits 8-13 reserved, set to 0
					 * bit 14, blanking
					 * bit 15, (1=last point, 0=all others)
					 */
					stat1[i]=fgetc(fd);
					stat2[i]=fgetc(fd);
					ldata[liter+i*5+3]=(stat1[i] & 0x40)>>6;
					ldata[liter+i*5+4]= stat2[i]; // color table
					for (zz=0;zz<8;zz++) {
						bp1[zz]=(stat1[i] & bm[zz])>>zz;
						bp2[zz]=(stat2[i] & bm[zz])>>zz;
					}

					// Display coordinate data info
				}
				liter=liter+totalpoints[framenumber]*5;
				//				printf("liter: %d\n",liter);
			}
		} else if (formatcode==2) {
			// color table
		}
	}
	fclose(fd);
	return(1);
}

double get_clock_time() {
	gettimeofday( &tv, NULL );
	return (double) tv.tv_sec + (double) tv.tv_usec * 1.e-6;
}

void gen_sincos() {
	int i;
	for (i=0;i<256;i++) {
		sn[i]=sin(i*PI*2/256);
		cs[i]=cos(i*PI*2/256);
	}	
}

int16_t s16clip(int32_t ic) {
	if (ic>32767) ic=32767;
	if (ic<-32768) ic=-32768;
	return (int16_t) ic;
}

void initlaser() {
}

void laserto(int datax, int datay, char blank) {
	odatax=datax;
	odatay=datay;
}

void turnmeoff() {
	fprintf(stderr, "byebye\n");
	exit(0);
}

void help(char *str) {
	printf("Usage: %s [options] ILDAfile.ild\n\
			Options:\n\
			-3D PITCHDELTA YAWDELTA ROLLDELTA  # Range [0 - 256], 256 = 360 degrees.\n\
			-b BLANK_AHEAD # Lookahead for blanking.\n\
			-c COUNT       # repeat COUNT times.\n\
			-d DELAY       # Delay between points in microseconds.\n\
			-f FRAMEDELAY	 # Delay between frames in microseconds.\n\
			-g             # DEBUG: Increase verbosity.\n\
			-k 		 # Invert Blanking & Visible signal.\n\
			-h             # This help file.\n\
			-i INITFRAME   # The frame to start with.\n\
			-l LOOPTYPE    # LOOPTYPE (range: 0 or 1), 0 = loop, 1 = pingpong\n\
			-m             # MonkeyTools patch. (data starts at Frame 1 instead of 0)\n\
			-n             # No blanking.\n\
			-o PITCH YAW ROLL # Range [0 - 256], 256 = 360 degrees.\n\
			-offprime      # An alternate think too much blanking thingie.\n\
			-p PORT        # LPT PORT (range: 0 to 2), 0 = 0x3bc, 1 = 0x378, 2 = 0x278.\n\
			-r START END   # START = Start frame, END = End frame.\n\
			-s SCALE       # Prescalar to change size of object. default scale=1.0\n\
			-t             # Perform 3D transforms between Framedelay.\n\
			-ttm		 # Think Too Much\n\
			-v VIS_AHEAD   # Lookahead for visible.\n\
			-w             # Swap X & Y axis.\n\
			-x             # Invert X axis.\n\
			-y             # Invert Y axis.\n\
			-z FRAMEDELTA	 # Frame delta to increment currframe with.\n\n",str);
	//  Unimplemented Flags: -a,-e,-j,-q,-u
	exit(-1);
}

unsigned long microseconds(void) {
	struct timeval tv;
	struct timezone tz;

	if( gettimeofday( &tv,&tz ) == -1 ) return( 0 );
	return( (tv.tv_sec*1000*1000) + tv.tv_usec );
}

int premod(int number, int scalar) {

//	i + -2
	if (number<0) number=scalar+number;

	int retval=number%scalar; 
	if (retval<0) return (scalar+retval);
	else return retval;
}
void dumpvadjust() {
	int ctr=0;
	iobuffer[ctr]=0x30; // CMDB
	ctr++;
	iobuffer[ctr]=0;	// X
	ctr++;
    iobuffer[ctr]=0;
	ctr++;
    iobuffer[ctr]=0;	// Y
	ctr++;
    iobuffer[ctr]=0;
	ctr++;
    iobuffer[ctr]=255;  // Z
    ctr++;
    iobuffer[ctr]=73;	// R
	ctr++;
    iobuffer[ctr]=109;	// G
	ctr++;
    iobuffer[ctr]=146;	// B
	ctr++;
    iobuffer[ctr]=182;	// C
	ctr++;
    iobuffer[ctr]=219;	// M
	ctr++;
    iobuffer[ctr]=255;	// Y
	ctr++;
	iobuffer[ctr]=37;	// K
	ctr++;
	iobuffer[ctr]=0x88; // frame switch baby
	ctr++;
	write_bulkdata(ctr, iobuffer, current_handle);
	ctr=0;
	exit(0);
}
void dumpnotframe() {
	int ctr=0;
	if (!pulsemode ) {
		iobuffer[ctr]=0x20;
		ctr++;
		iobuffer[ctr]=0xDE;
		ctr++;
        iobuffer[ctr]=0xAD;
		ctr++;
        iobuffer[ctr]=0xBE;
		ctr++;
        iobuffer[ctr]=0xEF;
		ctr++;
		if (datamode[DM_ZAXIS]) {
			iobuffer[ctr]=0x69;
			ctr++;
		}
        iobuffer[ctr]=0x31;
		ctr++;
        iobuffer[ctr]=0x32;
		ctr++;
        iobuffer[ctr]=0x33;
		ctr++;
		if (datamode[DM_CALCCMY]) {
	        iobuffer[ctr]=0x21;
			ctr++;
   	     	iobuffer[ctr]=0x22;
			ctr++;
        	iobuffer[ctr]=0x23;
			ctr++;
		}
		if (datamode[DM_ABLANK]) {
        	iobuffer[ctr]=0x42;
			ctr++;
		}
		iobuffer[ctr]=0x88; // frame switch baby
		ctr++;

		write_bulkdata(ctr, iobuffer, current_handle);
		ctr=0;
	} else {
		iobuffer[ctr]=0x20;
		ctr++;
		iobuffer[ctr]=0x00;
		ctr++;
        iobuffer[ctr]=0x00;
		ctr++;
        iobuffer[ctr]=0xBE;
		ctr++;
        iobuffer[ctr]=0xEF;
		ctr++;
		if (datamode[DM_ZAXIS]) {
			iobuffer[ctr]=0x69;
			ctr++;
		}
        iobuffer[ctr]=0x31;
		ctr++;
        iobuffer[ctr]=0x32;
		ctr++;
        iobuffer[ctr]=0x33;
		ctr++;
		if (datamode[DM_CALCCMY]) {
	        iobuffer[ctr]=0x21;
			ctr++;
   	     	iobuffer[ctr]=0x22;
			ctr++;
        	iobuffer[ctr]=0x23;
			ctr++;
		}
		if (datamode[DM_ABLANK]) {
        	iobuffer[ctr]=0x42;
			ctr++;
		}
		iobuffer[ctr]=0x20;
		ctr++;
		iobuffer[ctr]=0xFF;
		ctr++;
		iobuffer[ctr]=0xFF;
		ctr++;
        iobuffer[ctr]=0xBE;
		ctr++;
        iobuffer[ctr]=0xEF;
		ctr++;
		if (datamode[DM_ZAXIS]) {
			iobuffer[ctr]=0x69;
			ctr++;
		}
        iobuffer[ctr]=0x31;
		ctr++;
        iobuffer[ctr]=0x32;
		ctr++;
        iobuffer[ctr]=0x33;
		ctr++;
		if (datamode[DM_CALCCMY]) {
	        iobuffer[ctr]=0x21;
			ctr++;
   	     	iobuffer[ctr]=0x22;
			ctr++;
        	iobuffer[ctr]=0x23;
			ctr++;
		}
		if (datamode[DM_ABLANK]) {
        	iobuffer[ctr]=0x42;
			ctr++;
		}
		iobuffer[ctr]=0x88; // frame switch baby
		ctr++;

		write_bulkdata(ctr, iobuffer, current_handle);
		ctr=0;
	}
	if (setterminate) {
		exit(0);
	}
}

void dumpframe(int utotalpts) {
	int i;
	int ctr=0;
	u_int16_t ndatax, ndatay;
	int xmsb, xlsb, ymsb, ylsb, colr=0, colg=0, colb=0, cmdb=0;
	if (maxpoint) {
		if (utotalpts>maxpoint) utotalpts=maxpoint;
	}

	for (i=0;i<utotalpts;i++) {
		ndatax=(u_int16_t)(rawframe[i][0]+32768);
		ndatay=(u_int16_t)(rawframe[i][1]+32768);
		if (!invx) ndatax=65535-ndatax;
		if (invy) ndatay=65535-ndatay;

		xlsb=ndatax&0xff;
		xmsb=ndatax>>8;

		ylsb=ndatay&0xff;
		ymsb=ndatay>>8;

		anablank=(int)(((float)rawframe[i][2]+(float)rawframe[i][3]+(float)rawframe[i][4])/3.0);

		if (laraio) {
			colr=rawframe[premod(i+ba_red,utotalpts)][2]; 
			colg=rawframe[premod(i+ba_grn,utotalpts)][3]; 
			colb=rawframe[premod(i+ba_blu,utotalpts)][4]; 
		} else {
			colr=rawframe[i][2]; 
			colg=rawframe[i][3]; 
			colb=rawframe[i][4]; 
		}

		if (invr) {
			colr=255-colr; 
		}
		if (invg) {
			colg=255-colg; 
		}
		if (invb) {
			colb=255-colb; 
		}

		if (laraio) { 	// each of the color channels turns on and off at a different voltage level,
						// this maps the 0-255 standard levels to the custom levels required 
			rangec=(float)maxred-(float)minred;
			adjcv=rangec/256.0;
			colr=(int) ((float)minred+(float)colr*adjcv);
			rangec=(float)maxgrn-(float)mingrn;
			adjcv=rangec/256.0;
			colg=(int) ((float)mingrn+(float)colg*adjcv);
			rangec=(float)maxblu-(float)minblu;
			adjcv=rangec/256.0;
			colb=(int) ((float)minblu+(float)colb*adjcv);
		}

        if (!invblank) {
            if ((colr==0)&&(colg==0)&&(colb==0)) {
                cmdb=DUT_CLRBLNK; // color updated blanked 
            } else {
                cmdb=DUT_CLRNOBLNK; // color updated 
            }
        } else {
          	if ((colr==0)&&(colg==0)&&(colb==0)) {
                cmdb=DUT_CLRNOBLNK; // color updated 
            } else {
                cmdb=DUT_CLRBLNK; // color updated blanked 
            }
        }
		if (laraio) {
			cmdb=DUT_CLRNOBLNK;
		}
		if (setdeadbeef) {
			xlsb=0xDE; xmsb=0xAD; ylsb=0xBE;ymsb=0xEF;
			colr=1;colg=2;colb=3;
		}

		iobuffer[ctr]=cmdb;
		ctr++;
		iobuffer[ctr]=xlsb;
		ctr++;
        iobuffer[ctr]=xmsb;
		ctr++;
        iobuffer[ctr]=ylsb;
		ctr++;
        iobuffer[ctr]=ymsb;
		ctr++;
		if (datamode[DM_ZAXIS]) {
			iobuffer[ctr]=105; // 0x69 for debugging purposes
			ctr++;
		}
	    iobuffer[ctr]=colr;
	    //iobuffer[ctr]=0;
		ctr++;
	    iobuffer[ctr]=colg;
//	    iobuffer[ctr]=0;
		ctr++;
   	    iobuffer[ctr]=colb;
 //  	    iobuffer[ctr]=0;
		ctr++;

		if (datamode[DM_RGBCMY]) {
		    iobuffer[ctr]=colr; // C
			ctr++;
		    iobuffer[ctr]=colg; // M
			ctr++;
   		   	iobuffer[ctr]=colb; // Y
			ctr++;
		}	
		if (datamode[DM_ABLANK]) {
			iobuffer[ctr]=anablank;
			//iobuffer[ctr]=0x42;
			ctr++;
		}

	//	printf("cmdb:0x%X x(l:%X/m:%X) y(l:%X/y:%X) r:0x%X g:0x%X b:0x%X\n",cmdb,xlsb,xmsb,ylsb,ymsb,colr,colg,colb);

		if (ctr+13>MAX_CHUNKSIZE) {
			// not enough space for a newframe so dump it already!
			write_bulkdata(ctr, iobuffer, current_handle);
			ctr=0;
		}
	}
	iobuffer[ctr]=CMDB_FRAMESWITCH; // frame switch baby
	ctr++;
	if (ctr) write_bulkdata(ctr, iobuffer, current_handle);
	ctr=0;
 if (setterminate) {
        exit(0);
    }

}
