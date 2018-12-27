#pragma once

#ifndef _COMMANDS_h
#define _COMMANDS_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
//	#include "WProgram.h"
#endif

#include "compile_config.h"

#include <EEPROM.h>
#include "output.h"
#include "cc1101.h"
#include "functions.h"
#include "signalDecoder.h"

extern char IB_1[14];
extern bool hasCC1101;
extern SignalDetectorClass musterDec;
extern volatile bool blinkLED;



namespace commands {

	const char TXT_UNSUPPORTED1[]		PROGMEM = "Unsupported short command";
	const char TXT_EEPROM[]				PROGMEM = "EEPROM";
	const char TXT_MU[]					PROGMEM = "MU";
	const char TXT_MC[]					PROGMEM = "MC";
	const char TXT_MS[]					PROGMEM = "MS";
	const char TXT_EQ[]					PROGMEM = "=";
	const char TXT_FSEP[]				PROGMEM = ";";
#ifdef CMP_CC1101
	const char TXT_CC1101[]				PROGMEM = "CC1101 ";
#endif
	const char TXT_433[]				PROGMEM = "433 ";
	const char TXT_868[]				PROGMEM = "868 ";
	const char TXT_MHZ[]				PROGMEM = "Mhz ";
	const char TXT_WRITE[]				PROGMEM = "write ";
	const char TXT_TPATAB[]				PROGMEM = " to PATABLE done";

	


	inline void getPing()
	{
		MSG_PRINTLN("OK");
		delayMicroseconds(500);
	}

	inline void changeReceiver() {
		if (IB_1[1] == 'Q')
		{
			disableReceive();
		}
		if (IB_1[1] == 'E')
		{
			enableReceive();
		}
	}

	void printHex2(const byte hex) {   // Todo: printf oder scanf nutzen
		if (hex < 16) {
			MSG_PRINT("0");
		}
		MSG_PRINT(hex, HEX);
	}

	inline void getConfig()
	{
		MSG_PRINT(FPSTR(TXT_MS)); MSG_PRINT(FPSTR(TXT_EQ));
		MSG_PRINT(musterDec.MSenabled, DEC);
		MSG_PRINT(FPSTR(TXT_FSEP)); MSG_PRINT(FPSTR(TXT_MU)); MSG_PRINT(FPSTR(TXT_EQ));
		MSG_PRINT(musterDec.MUenabled, DEC);
		MSG_PRINT(FPSTR(TXT_FSEP)); MSG_PRINT(FPSTR(TXT_MC)); MSG_PRINT(FPSTR(TXT_EQ));
		MSG_PRINT(musterDec.MCenabled, DEC);
		MSG_PRINT(FPSTR(TXT_FSEP)); MSG_PRINT("Mred"); MSG_PRINT(FPSTR(TXT_EQ));
		MSG_PRINTLN(musterDec.MredEnabled, DEC);
	}


	inline void configCMD()
	{
		bool *bptr;

		switch (IB_1[2])
		{
			case 'S': //MS
				bptr = &musterDec.MSenabled;
				break;
			case 'U' : //MU
				bptr = &musterDec.MUenabled;
				break;
			case 'C' : //MC
				bptr = &musterDec.MCenabled;
				break;
			case 'R' : //Mreduce
				bptr = &musterDec.MredEnabled;
				break;
			default:
				return;
		}

		switch (IB_1[1])
		{
			case 'E':
				*bptr = true;
				break;
			case 'D':
				*bptr = false;
				break;
			default:
				return;
		}


		storeFunctions(musterDec.MSenabled, musterDec.MUenabled, musterDec.MCenabled, musterDec.MredEnabled);
	}

	inline void configSET()
	{
		//MSG_PRINT(cmdstring.substring(2, 8));
		if (strstr(&IB_1[2],"mcmbl=") != NULL)   // mc min bit len
		{
			musterDec.mcMinBitLen = strtol(&IB_1[8], NULL,10);
			MSG_PRINT(musterDec.mcMinBitLen); MSG_PRINT(" bits set");
		}
	}


	void HandleShortCommand()
	{
		#define  cmd_Version 'V'
		#define  cmd_freeRam 'R'
		#define  cmd_uptime 't'
		#define  cmd_changeReceiver 'X'
		#define  cmd_help '?'
		#define  cmd_ping 'P'
		#define  cmd_ccFactoryReset 'e'  // EEPROM / factory reset
		#define  cmd_config 'C'     // CG get config, set config, C<reg> get CC1101 register
		#define  cmd_patable 'x' 
		#define  cmd_write 'W'      // write EEPROM und write CC1101 register
		#define  cmd_read  'r'      // read EEPROM
		#define  cmd_space ' '
		#define  cmd_send 'S'

		switch (IB_1[0])
		{
		case cmd_help:
			MSG_PRINT(cmd_help);	MSG_PRINT(F(" Use one of "));
			MSG_PRINT(cmd_Version); MSG_PRINT(cmd_space);
			MSG_PRINT(cmd_freeRam); MSG_PRINT(cmd_space);
			MSG_PRINT(cmd_uptime); MSG_PRINT(cmd_space);
			MSG_PRINT(cmd_changeReceiver); MSG_PRINT(cmd_space);
			MSG_PRINT(cmd_send); MSG_PRINT(cmd_space);
			MSG_PRINT(cmd_ping); MSG_PRINT(cmd_space);
			MSG_PRINT(cmd_config); MSG_PRINT(cmd_space);
			MSG_PRINT(cmd_read); MSG_PRINT(cmd_space);
			MSG_PRINT(cmd_write); MSG_PRINT(cmd_space);
#ifdef CMP_CC1101
			if (hasCC1101) {
				MSG_PRINT(cmd_patable); MSG_PRINT(cmd_space);
				MSG_PRINT(cmd_ccFactoryReset); MSG_PRINT(cmd_space);
			}
#endif
			MSG_PRINTLN("");
			break;
		case cmd_ping:
			getPing();
		case cmd_Version:
			MSG_PRINT("V " PROGVERS " SIGNALduino ");
#ifdef CMP_CC1101
			if (hasCC1101) {
				MSG_PRINT(FPSTR(TXT_CC1101));
#endif
#ifdef PIN_MARK433
				MSG_PRINT("(");
				MSG_PRINT(isLow(PIN_MARK433) ? "433" : "868");
				MSG_PRINT(F("Mhz)"));
#else
	#ifdef CMP_CC1101

				switch (cc1101::chipVersion()) {
					//      case 0x08:    // CC1101_VERSION 0x31
					case 0x18:  // CC1101_VERSION 0xF1

						MSG_PRINT(cmd_space);	MSG_PRINT(FPSTR(TXT_433)); MSG_PRINT(FPSTR(TXT_MHZ));
						break;
					case 0x04:  // CC1101_VERSION 0x31
					case 0x14:  // CC1101_VERSION 0xF1
						MSG_PRINT(cmd_space);	MSG_PRINT(FPSTR(TXT_868)); MSG_PRINT(FPSTR(TXT_MHZ));
						break;
				}
			}
	#endif
#endif
			MSG_PRINTLN(" - compiled at " __DATE__ " " __TIME__)
				break;
		case cmd_freeRam:
			MSG_PRINTLN(freeRam());
			break;
		case cmd_uptime:
			MSG_PRINTLN(getUptime());
			break;
		case cmd_changeReceiver:
			changeReceiver();
			break;
		case cmd_config:
			switch (IB_1[1])
			{
				case 'G':
					getConfig();
					break;
				case 'E':
				case 'D':
					configCMD();
					break;
				case 'S':
					configSET();
					break;
	#ifdef CMP_CC1101
				default:
					if (isxdigit(IB_1[1]) && isxdigit(IB_1[2]) && hasCC1101) {
						uint8_t val = (uint8_t)strtol(IB_1+1, nullptr, 16);
						cc1101::readCCreg(val);
					}
	#endif
			}
			break;
#ifdef CMP_CC1101
		case cmd_ccFactoryReset:
			if (hasCC1101) {
				cc1101::ccFactoryReset();
				cc1101::CCinit();
			}
			break;
		case cmd_patable:
			if (isHexadecimalDigit(IB_1[1]) && isHexadecimalDigit(IB_1[2]) && hasCC1101) {
				uint8_t val = (uint8_t)strtol(IB_1+1, nullptr, 16);
				cc1101::writeCCpatable(val);
				MSG_PRINT(TXT_WRITE);
				char b[3];
				sprintf(b, "%02X", val);
				MSG_PRINT(b);
				MSG_PRINTLN(FPSTR(TXT_TPATAB));
			}
			break;

#endif
		case cmd_read:
			// R<adr>  read EEPROM
			if (isHexadecimalDigit(IB_1[1]) && isHexadecimalDigit(IB_1[2]) && hasCC1101) {
				const uint8_t reg = (uint8_t)strtol(IB_1+1, nullptr, 16);
				MSG_PRINT("EEPROM ");

				char b[3];
				sprintf(b, "%2X", reg);
				MSG_PRINT(b);

				if (IB_1[3] == 'n') {
					MSG_PRINT(FPSTR(" :"));
					for (uint8_t i = 0; i < 16; i++) {
						const uint8_t val = EEPROM.read(reg + i);
						sprintf(b, " %02X", val);
						MSG_PRINT(b);
					}
				}
				else {
					MSG_PRINT(FPSTR(" = "));
					const uint8_t val = EEPROM.read(reg);
					sprintf(b, " %02X", val);
					MSG_PRINT(b);
					printHex2(EEPROM.read(reg));
				}
				MSG_PRINTLN("");
			}
			break;
		case cmd_write:
			if (IB_1[1] == 'S' && IB_1[2] == '3')
			{
				#ifdef CMP_CC1101
					cc1101::commandStrobes();
				#endif 
			} else if (isHexadecimalDigit(IB_1[1]) && isHexadecimalDigit(IB_1[2]) && isHexadecimalDigit(IB_1[3]) && isHexadecimalDigit(IB_1[4])) {
				char b[3];
				b[2] = '\0';

				memcpy(b, &IB_1[1], 2);
				uint8_t reg = strtol(b, nullptr, 16);
				memcpy(b, &IB_1[3], 2);
				uint8_t val = strtol(b, nullptr, 16);

				EEPROM.write(reg, val); //Todo pr�fen ob reg hier um 1 erh�ht werden muss
				#ifdef ESP8266
				EEPROM.commit();
				#endif
#ifdef CMP_CC1101
				if (hasCC1101) {
					cc1101::writeCCreg(reg, val);
				}
#endif
			}
			break;
		default:
			MSG_PRINTLN(TXT_UNSUPPORTED1);
			return;
		}
		blinkLED = true;
	}


}


#endif