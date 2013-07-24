/*
 *  selftest.cpp
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *	Licensed under GPLv3
 *
 */

#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "tldefs.h"
#include "LCD_Term.h"
#include "5110LCD.h"
#include "clock.h"
#include "button.h"
#include "Menu.h"
#include "hardware.h"
#include "shutter.h"
#include "IR.h"
#include "timelapseplus.h"
#include "VirtualSerial.h"
#include "TWI_Master.h"
#include "debug.h"
#include "bluetooth.h"
#include "settings.h"
#include "selftest.h"
#include "math.h"

extern shutter timer;
extern LCD lcd;
extern MENU menu;
extern Clock clock;
extern Button button;
extern BT bt;
extern IR ir;

/******************************************************************
 *
 *   test
 *
 *
 ******************************************************************/

int8_t test()
{
//	int8_t failed = 0;

    termInit();

    termPrintStrP(PSTR("\nReady for Testing\n\nWaiting on PC\n"));

    for(;;)
    {
        VirtualSerial_Task();
        wdt_reset();

        if(VirtualSerial_connected == 1)
        {
            if(run_tests())
            {
                debug('1');
                termPrintStrP(PSTR("Passed All Tests\n"));
                VirtualSerial_Reset();
                return 1;
            } 
            else
            {
                debug('0');
                termPrintStrP(PSTR("Failed Tests\n"));
                VirtualSerial_Reset();
                return 0;
            }
        }


        if(VirtualSerial_CharWaiting())
        {
            switch(VirtualSerial_GetChar())
            {
               case 'P':
                   termPrintStrP(PSTR("Passed All Tests\n"));
                   VirtualSerial_Reset();
                   return 1;

               case 'E':
//				failed = 1;		// set but never referenced
                   termPrintStrP(PSTR("Tests Failed\n"));
                   FOREVER;
                   break;

               case 'R':
                   lcd.color(1);
                   termPrintStrP(PSTR("LCD BL RED\n"));
                   break;

               case 'W':
                   lcd.color(0);
                   termPrintStrP(PSTR("LCD BL WHITE\n"));
                   break;

               case 'D':
                   for(char i = 0; i < 48; i++)
                   {
                       lcd.drawLine(0, i, 83, i);
                   }
                   
                   lcd.update();
                   break;

               case 'c':
                   lcd.cls();
                   lcd.update();
                   break;

               case 'T':
                   debug('E');
                   termPrintStrP(PSTR("USB Connected\n"));
                   break;

               case 'O':
                   timer.off();
                   termPrintStrP(PSTR("Shutter Closed\n"));
                   break;

               case 'F':
                   timer.full();
                   termPrintStrP(PSTR("Shutter Full\n"));
                   break;

               case 'H':
                   timer.half();
                   termPrintStrP(PSTR("Shutter Half\n"));
                   break;

               case 'C':
                   if(timer.cableIsConnected()) 
                       debug('1');
                   else 
                       debug('0');
                   break;

               case 'S':

                   if(run_tests())
                   {
                       debug('1');
                       termPrintStrP(PSTR("Passed All Tests\n"));
                       VirtualSerial_Reset();
                       return 1;
                   } 
                   else
                   {
                       debug('0');
                       termPrintStrP(PSTR("Failed Tests\n"));
                       VirtualSerial_Reset();
                       return 0;
                   }
                   break;

               case 'B':
                   termPrintStrP(PSTR("Checking charger\n"));
                   debug((char)battery_status());
                   break;

               case 'X': // RESET USB
                   VirtualSerial_Reset();
                   break;
            }
        }
    }
    return 0;
}

/******************************************************************
 *
 *   test_assert
 *
 *
 ******************************************************************/

int8_t test_assert(char test_case)
{
    if(test_case)
    {
        termPrintStrP(PSTR("   Passed\n"));
        return 1;
    } 
    
    termPrintStrP(PSTR("   Failed\n"));
    return 0;
}

/******************************************************************
 *
 *   run_tests
 *
 *
 ******************************************************************/

int8_t run_tests()
{
    int8_t pass = 1;

    wdt_reset();
    
    if(pass)
    {
        if(bt.present)
        {
            termPrintStrP(PSTR("Testing BlueTooth\n"));
            pass &= test_assert(bt.version() == 3);
        }
    }
    
    wdt_reset();
    
    if(pass)
    {
        termPrintStrP(PSTR("Testing Battery\n"));
        pass &= test_assert(battery_read_raw() > 400);
        termPrintStrP(PSTR("Testing Charger\n"));
        pass &= test_assert(battery_status() > 0);
    }
    
    wdt_reset();
    
    if(pass)
    {
        termPrintStrP(PSTR("Testing Timer\n"));
        clock.tare();
        _delay_ms(100);
        uint32_t ms = clock.eventMs();
        //termPrintByte((uint8_t) ms);
        pass &= test_assert(ms >= 80 && ms <= 120);
    }
    
    wdt_reset();
    
    if(pass)
    {
        termPrintStrP(PSTR("Testing Shutter\n"));
        ENABLE_SHUTTER;
        ENABLE_MIRROR;
        ENABLE_AUX_PORT;
        uint8_t i = 0;      // this was uninitialized. I set it to 0 -- John
        
        while (i < 30)
        {
            wdt_reset();
            _delay_ms(100);

            if(AUX_INPUT1) 
                break;
        }
        pass &= test_assert(AUX_INPUT1);
    }
    
    wdt_reset();
    
    if(pass)
    {
        termPrintStrP(PSTR("Press top l key\n"));
        pass &= test_assert(button.waitfor(FL_KEY));
    }
    
    wdt_reset();
    
    if(pass)
    {
        termPrintStrP(PSTR("Press top r key\n"));
        pass &= test_assert(button.waitfor(FR_KEY));
    }
    
    wdt_reset();
    
    if(pass)
    {
        termPrintStrP(PSTR("Press left key\n"));
        pass &= test_assert(button.waitfor(LEFT_KEY));
    }
    
    wdt_reset();
    
    if(pass)
    {
        termPrintStrP(PSTR("Press right key\n"));
        pass &= test_assert(button.waitfor(RIGHT_KEY));
    }
    
    wdt_reset();
    
    if(pass)
    {
        termPrintStrP(PSTR("Press up key\n"));
        pass &= test_assert(button.waitfor(UP_KEY));
    }
    
    wdt_reset();
    
    if(pass)
    {
        termPrintStrP(PSTR("Press down key\n"));
        pass &= test_assert(button.waitfor(DOWN_KEY));
    }
    
    wdt_reset();
    
    if(pass)
    {
        lcd.color(1);
        termPrintStrP(PSTR("LCD BL RED\n"));
        pass &= test_assert(button.waitfor(DOWN_KEY));
    }
    
    wdt_reset();
    
    if(pass)
    {
        lcd.color(0);
        termPrintStrP(PSTR("LCD BL WHITE\n"));
        pass &= test_assert(button.waitfor(DOWN_KEY));
    }
    
    wdt_reset();
    
    if(pass)
    {
        for(char i = 0; i < 48; i++)
        {
            lcd.drawLine(0, i, 83, i);
        }
        
        lcd.update();
        pass &= test_assert(button.waitfor(DOWN_KEY));
    }
    
    wdt_reset();
    
    if(pass)
    {
        lcd.cls();
        lcd.update();
        pass &= test_assert(button.waitfor(DOWN_KEY));
    }
    
    wdt_reset();
    
    if(pass)
    {
        if(!bt.present)
        {
            termPrintStrP(PSTR("NO BLUETOOTH\n  CONFIRM\n"));
            pass &= test_assert(button.waitfor(DOWN_KEY));
        }
    }
    
    return pass;
}

light_reading light_test_results[120]EEMEM;

/******************************************************************
 *
 *   lightTest
 *
 *
 ******************************************************************/

void lightTest()
{
    light_reading result;

    termInit();

    lcd.backlight(0);

    termPrintStrP(PSTR("\nRunning light\nsensor test\n\n"));

    uint8_t i;

    clock.tare();

    for(i = 0; i < 120; i++)
    {
        timer.half();
        _delay_ms(500);
        timer.full();
        _delay_ms(50);
        timer.off();
        hardware_readLightAll(&result);
        eeprom_write_block((const void*)&result, &light_test_results[i], sizeof(light_reading));
        termPrintStrP(PSTR("Photo "));
        termPrintByte(i + 1);
        termPrintStrP(PSTR(" of 120\n"));
        
        while (clock.eventMs() < 120000) 
            wdt_reset();

        clock.tare();
    }
    termPrintStrP(PSTR("\nDone!\n"));
}

/******************************************************************
 *
 *   readLightTest
 *
 *
 ******************************************************************/

void readLightTest()
{
    light_reading result;

    termInit();

    termPrintStrP(PSTR("\nReading light\nsensor test\ndata...\n\n"));

    uint8_t i;

    for(i = 0; i < 120; i++)
    {
        eeprom_read_block((void*)&result, &light_test_results[i], sizeof(light_reading));

        debug(i);
        debug(STR(", "));
        debug((uint16_t)result.level1);
        debug(STR(", "));
        debug((uint16_t)result.level2);
        debug(STR(", "));
        debug((uint16_t)result.level3);
        debug_nl();

        termPrintStrP(PSTR("Sent "));
        termPrintByte(i + 1);
        termPrintStrP(PSTR(" of 120\n"));

        VirtualSerial_Task();
        wdt_reset();
    }
    
    termPrintStrP(PSTR("\nDone!\n"));
}

