#pragma once

#ifndef _SEND_h
#define _SEND_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
//	#include "WProgram.h"
#endif

extern bool hasCC1101;
extern char IB_1[14];


const char TXT_SENDCMD[]		PROGMEM = "send cmd ";
const char TXT_TOLONG[]			PROGMEM = "to long ";
const char TXT_CORRUPT[]		PROGMEM = "corrupt";
//================================= RAW Send ======================================
void send_raw(char *startpos, char *endpos, const int16_t *buckets)
{
	uint8_t index = 0;
	unsigned long stoptime = micros();
	bool isLow;
	uint16_t dur;

	for (char *i = startpos; i < endpos; i++)
	{
		//DBG_PRINT(cmdstring.substring(i,i+1));
		index = *i - '0';
		//DBG_PRINT(index);
		isLow = buckets[index] >> 15;
		dur = abs(buckets[index]); 		//isLow ? dur = abs(buckets[index]) : dur = abs(buckets[index]);

		while (stoptime > micros()) {
			yield();
			;
		}
		isLow ? digitalLow(PIN_SEND) : digitalHigh(PIN_SEND);
		stoptime += dur;
	}
	while (stoptime > micros()) {
		yield();
		;
	}
	//DBG_PRINTLN("");

}
//SM;R=2;C=400;D=AFAFAF;




void send_mc(const char *startpos, const char *endpos, const int16_t clock)
{
	int8_t b;
	uint8_t bit;

	unsigned long stoptime = micros();
	for (char *i = (char*)startpos; i < endpos; i++) {
		b = ((byte)*i) - (*i <= '9' ? 0x30 : 0x37);

		for (bit = 0x8; bit > 0; bit >>= 1) {
			for (byte i = 0; i <= 1; i++) {
				if ((i == 0 ? (b & bit) : !(b & bit)))
					digitalLow(PIN_SEND);
				else
					digitalHigh(PIN_SEND);

				stoptime += clock;
				while (stoptime > micros())
					yield();
			}

		}

	}
	// MSG_PRINTLN("");
}



// SC;R=4;SM;C=400;D=AFFFFFFFFE;SR;P0=-2500;P1=400;D=010;SM;D=AB6180;SR;D=101;
// SC;R=4;SM;C=400;D=FFFFFFFF;SR;P0=-400;P1=400;D=101;SM;D=AB6180;SR;D=101;
// SR;R=3;P0=1230;P1=-3120;P2=-400;P3=-900;D=030301010101010202020202020101010102020202010101010202010120202;
// SM;C=400;D=AAAAFFFF;
// SR;R=10;P0=-2000;P1=-1000;P2=500;P3=-6000;D=2020202021212020202121212021202021202121212023;

// SC;R=6;SR;P0=-2560;P1=2560;P3=-640;D=10101010101010113;SM;C=645;D=A1E7E7D6F88D88;F=10AB85550A;   # SOMFY

struct s_sendcmd {
	int16_t sendclock = 0;
	uint8_t type;
	char    *datastart;
	char    *dataend;
	int16_t buckets[6];
	uint8_t repeats = 1;
};

void send_cmd()
{
#define combined 0
#define manchester 1
#define raw 2
	disableReceive();

	uint8_t repeats = 1;  // Default is always one iteration so repeat is 1 if not set
	int16_t start_pos = 0;
	uint8_t counter = 0;
	bool extraDelay = true;

	s_sendcmd command[5];

	uint8_t ccParamAnz = 0;   // Anzahl der per F= uebergebenen cc1101 Register
	uint8_t ccReg[6];
	uint8_t val;

	uint8_t cmdNo = 255;


	char buf[256] = {}; // Second Buffer 256 Bytes 0-255
	char *msg_beginptr = IB_1;
	char *msg_endptr = buf;
	uint8_t buffer_left = 255;
	do
	{

		//if (cmdNo == 255)  msg_part = IB_1;


		//DBG_PRINT(msg_beginptr);
		if (msg_beginptr[0] == 'S')
		{
			if (msg_beginptr[1] == 'C')  // send combined information flag
			{
				cmdNo++;
				command[cmdNo].type = combined;
extraDelay = false;
			}
			else if (msg_beginptr[1] == 'M') // send manchester
			{
			cmdNo++;
			command[cmdNo].type = manchester;
			DBG_PRINTLN("Adding manchester");

			}
			else if (msg_beginptr[1] == 'R') // send raw
			{
			cmdNo++;
			command[cmdNo].type = raw;
			DBG_PRINTLN("Adding raw");
			extraDelay = false;
			}
			if (cmdNo == 0) {
				DBG_PRINTLN("rearrange beginptr");
				MSG_PRINT(IB_1); // echo command
				msg_endptr = buf; // rearrange to beginning of buf
				msg_beginptr = nullptr;
			}
		}
		else if (msg_beginptr[0] == 'P' && msg_beginptr[2] == '=') // Do some basic detection if data matches what we expect
		{
		counter = msg_beginptr[1] - '0'; // Convert to dec value
		command[cmdNo].buckets[counter] = strtol(&msg_beginptr[3], &msg_endptr, 10);
		//*(msg_endptr + 1) = '\0';
		DBG_PRINTLN("Adding bucket");
		if (cmdNo == 0) {
			MSG_PRINT(msg_beginptr);
			msg_endptr = buf; // rearrange to beginning of buf
			msg_beginptr = nullptr;
		}
		}
		else if (msg_beginptr[0] == 'R' && msg_beginptr[1] == '=') {
		command[cmdNo].repeats = strtoul(&msg_beginptr[2], &msg_endptr, 10);
		//*(msg_endptr + 1) = '\0';
		DBG_PRINT("Adding repeats: "); DBG_PRINTLN(command[cmdNo].repeats);
		if (cmdNo == 0) {
			MSG_PRINT(msg_beginptr);
			msg_endptr = buf; // rearrange to beginning of buf
			msg_beginptr = nullptr;

		}
		}
		else if (msg_beginptr[0] == 'D' && msg_beginptr[1] == '=') {
		command[cmdNo].datastart = msg_beginptr + 2;
		command[cmdNo].dataend = msg_endptr = (char*)memchr(msg_beginptr + 3, ';', buf + 255 - msg_beginptr + 3);
		//if (command[cmdNo].dataend != NULL) command[cmdNo].dataend= command[cmdNo].dataend-1;
		DBG_PRINT("locating data start:");
		DBG_PRINT(command[cmdNo].datastart);
		DBG_PRINT(" end:");
		DBG_PRINTLN(command[cmdNo].dataend);
		}
		else if (msg_beginptr[0] == 'C' && msg_beginptr[1] == '=')
		{
		//sendclock = msg_part.substring(2).toInt();
		command[cmdNo].sendclock = strtoul(&msg_beginptr[2], &msg_endptr, 10);
		DBG_PRINTLN("adding sendclock");
		}
		else if (msg_beginptr[0] == 'F' && msg_beginptr[1] == '=')
		{
		ccParamAnz = (msg_endptr - msg_beginptr - 1) / 2;

		if (ccParamAnz > 0 && ccParamAnz <= 5 && hasCC1101) {
			//uint8_t hex;
			DBG_PRINT("write new ccregs #");			DBG_PRINTLN(ccParamAnz);
			char b[3];
			b[2] = '\0';
			for (uint8_t i = 0; i < ccParamAnz; i++)
			{
				ccReg[i] = cc1101::readReg(0x0d + i, 0x80);    // alte Registerwerte merken
				memcpy(b, msg_beginptr + 2 + (i * 2), 2);
				val = strtol(b, nullptr, 16);
				cc1101::writeReg(0x0d + i, val);            // neue Registerwerte schreiben
				DBG_PRINT(b);
			}
			DBG_PRINTLN("");
		}
		}
		if (msg_endptr == msg_beginptr)
		{
			DBG_PRINTLN("break loop");
			break; // break the loop now

		}
		else {
			if (msg_endptr != buf)
				msg_endptr++;
			msg_beginptr = msg_endptr;
			//MSG_PRINTER.setTimeout(1000);
			//msg_endptr = MSG_PRINTER.readBytesUntil(';', msg_endptr, 128) + msg_beginptr ;
			uint8_t l = 0;
			do {
				msg_endptr += l;
				buffer_left -= l;
				l = MSG_PRINTER.readBytes(msg_endptr, 1);
				if (l == 0) {
					DBG_PRINTLN("TOUT");  break;
				}
			} while (msg_endptr[0] != ';' && msg_endptr[0] != '\n' && buffer_left > 0);
			if (l == 0)
			{
				MSG_PRINT(TXT_SENDCMD);
				MSG_PRINTLN(TXT_CORRUPT);
				return;
			}
			if (buffer_left == 0)
			{
				delayMicroseconds(300);
				MSG_PRINTER.readBytesUntil('\n', buf, 255);
				MSG_PRINT(TXT_SENDCMD);
				MSG_PRINTLN(TXT_TOLONG);
				return;
			}
			*(msg_endptr + 1) = '\0'; // Nullterminate the string

		}
	} while (msg_beginptr != NULL);

#ifdef CMP_CC1101
	if (hasCC1101) cc1101::setTransmitMode();
#endif


	if (command[0].type == combined && command[0].repeats > 0) {
		repeats = command[0].repeats;
	}
	for (uint8_t i = 0; i < repeats; i++)
	{
		DBG_PRINT("repeat "); DBG_PRINT(i); DBG_PRINT("/"); DBG_PRINT(repeats - 1);

		for (uint8_t c = 0; c <= cmdNo; c++)
		{
			DBG_PRINT(" cmd "); DBG_PRINT(c); DBG_PRINT("/"); DBG_PRINT(cmdNo);
			DBG_PRINT(" reps "); DBG_PRINTLN(command[c].repeats);

			if (command[c].type == raw) { for (uint8_t rep = 0; rep < command[c].repeats; rep++) send_raw(command[c].datastart, command[c].dataend, command[c].buckets); }
			else if (command[c].type == manchester) { for (uint8_t rep = 0; rep < command[c].repeats; rep++)send_mc(command[c].datastart, command[c].dataend, command[c].sendclock); }
			digitalLow(PIN_SEND);
			DBG_PRINT(".");

		}
		DBG_PRINTLN(" ");

		if (extraDelay) delay(1);
	}

	if (ccParamAnz > 0) {
		DBG_PRINT("ccreg write back ");
		//char b[3];
		for (uint8_t i = 0; i < ccParamAnz; i++)
		{
			//val = ccReg[i];
			//sprintf_P(b,PSTR("%02X"), val);
			cc1101::writeReg(0x0d + i, ccReg[i]);    // gemerkte Registerwerte zurueckschreiben
			//MSG_PRINT(b);
		}
		DBG_PRINTLN("");
	}
	DBG_PRINT(IB_1);
	MSG_PRINTLN(buf); // echo data of command
	musterDec.reset();
	FiFo.flush();
	enableReceive();	// enable the receiver
}




#endif