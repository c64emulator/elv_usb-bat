// usb-bat - programming example for ELV's USB BAT
// Copyright (C) 2013 Andreas Lohr

// This program is free software; you 
// can redistribute it (source or binary format) and/or modify
// it freely (including commercial purposes).

// This program comes with ABSOLUTELY NO WARRANTY, to the extent
// permitted by applicable law.

// standard C and system includes
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>

// HID driver include
#include <linux/hiddev.h>

// STL (standard template library)
#include <iostream>
#include <string>
#include <vector>

// use std namespace to easily access cout, cerr, string, etc.
using namespace std;

// file handle for USB-BAT driver node access
int iUSBBATHandle = -1;

// temporal string buffer (used for formatting)
#define BUFFER_SIZE 1024
char sBuffer[BUFFER_SIZE];

// display dimensions
#define LCD_SX 122
#define LCD_SY 32

// the bitmap to display
unsigned char theBitmap[LCD_SX * LCD_SY];

// button state and rotary wheel position
unsigned char uButtons = 0;
int iRotaryValue = 0;

// coordinates and type of drawn item
int iItemX = LCD_SX / 2;
int iItemY = LCD_SY / 2;
bool bItemIsCross = false;

bool bLight = false;

// forward declaration
void DrawItem();

// set a pixel within the bitmap - bitmap boundaries will be checked
void SetPixel(int x, int y, unsigned char value = 0)
{
  if (x < 0 || x >= LCD_SX || y < 0 || y >= LCD_SY)
    return;

  theBitmap[y * LCD_SX + x] = value;
}

// draw a circle into the bitmap using a sophisticated algorithm
// see http://en.wikipedia.org/wiki/Midpoint_circle_algorithm for details
void Circle(int x0, int y0, int radius, unsigned char value = 0)
{
  int f = 1 - radius;
  int ddF_x = 1;
  int ddF_y = -2 * radius;
  int x = 0;
  int y = radius;
  
  SetPixel(x0, (y0 + radius), value);
  SetPixel(x0, (y0 - radius), value);
  SetPixel((x0 + radius), y0, value);
  SetPixel((x0 - radius), y0, value);

  while(x < y)
  {
    // ddF_x == 2 * x + 1;
    // ddF_y == -2 * y;
    // f == x*x + y*y - radius*radius + 2*x - y + 1;
    if(f >= 0) 
    {
      --y;
      ddF_y += 2;
      f += ddF_y;
    }
    ++x;
    ddF_x += 2;
    f += ddF_x;    
  
    SetPixel((x0 + x), (y0 + y), value);
    SetPixel((x0 - x), (y0 + y), value);
    SetPixel((x0 + x), (y0 - y), value);
    SetPixel((x0 - x), (y0 - y), value);
    SetPixel((x0 + y), (y0 + x), value);
    SetPixel((x0 - y), (y0 + x), value);
    SetPixel((x0 + y), (y0 - x), value);
    SetPixel((x0 - y), (y0 - x), value);
  }
}

// draw a line into the bitmap with Bresenham's algorithm
void Line(int x0, int y0, int x1, int y1, unsigned char value = 0)
{
  int dx =  abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2; // error value e_xy
 
  while (true)
  {
    SetPixel(x0, y0, value);

    if (x0==x1 && y0==y1)
      break;

    e2 = 2*err;

    if (e2 > dy)// e_xy + e_x > 0
    {
      err += dy;
      x0 += sx;
    }

    if (e2 < dx) // e_xy+e_y < 0
    {
      err += dx;
      y0 += sy;
    }
  }
}

// Open the n'th HID device that matches the given vendor and product ID
// Where uEnum =
// 0 first matching device
// 1 second second matching device
// ... and so on
// this is to support multiple devices of the same vendor/product
int OpenHIDDevice(const char * sDescr, unsigned short uVendor, unsigned short uProduct, unsigned uEnum)
{
  // maximum number of HID devices to examine
  const int iMaxHiddevs = 16;

  // number of matching devices found so far
  unsigned uMatches = 0;

  // device handle
  int iHandle = -1;

  for (int i=0; i<iMaxHiddevs; ++i)
  {
    // try opening the i'th HID node
    snprintf(sBuffer, BUFFER_SIZE, "/dev/usb/hiddev%d", i);
    int iHandle = open(sBuffer, O_RDWR);

    if (iHandle == -1)
    {
      if (errno == EACCES) // show this special case to user
        cerr << "Note: We were not allowed to access '" << sBuffer << "' - probably we've not enough privileges -> we need to be root to access this." << endl;

      continue; // can't open - try next
    }

    // get product and vendor ID
    struct hiddev_devinfo dev;
    memset(&dev, 0, sizeof(dev));

    if (ioctl(iHandle, HIDIOCGDEVINFO, &dev) == -1)
    {
      // oops - HID device won't tell generic info about itself -> ignore, close and go on
      close(iHandle);
      continue;
    }

    if ((unsigned short)dev.vendor != uVendor || (unsigned short)dev.product != uProduct)
    {
      // this is not our device -> close it and go on searching
      close(iHandle);
      continue;
    }

    if (uMatches < uEnum)
    {
      // this is not the n'th device (n = uEnum + 1) --> close and go on searching
      ++uMatches;
      close(iHandle);
      continue;
    }

    // that is the correct device
    return iHandle;
  }

  // maximum number of devices searched without success
  snprintf(sBuffer, BUFFER_SIZE, "No '%s' (subid %u) device found.", sDescr, uEnum);
  throw sBuffer;
}

// compiles and sends a 'report' (this term is used by HID specifications) to the device
void SendReport(unsigned long uReportID, const std::vector<unsigned char> & vData)
{
  struct hiddev_usage_ref_multi uref;
  struct hiddev_report_info rinfo;

  memset(&uref, 0, sizeof(uref));
  memset(&rinfo, 0, sizeof(rinfo));

  uref.uref.report_type = HID_REPORT_TYPE_OUTPUT;
  uref.uref.report_id = uReportID;
  uref.uref.field_index = 0;
  uref.uref.usage_index = 0;
  uref.uref.usage_code = 0xff000001;

  if (vData.size() > HID_MAX_MULTI_USAGES)
  {
    snprintf(sBuffer, BUFFER_SIZE, "SendReport: Too much data (%u bytes) to send in one report.", (unsigned)vData.size());
    throw sBuffer;
  }

  for (unsigned u=0; u<vData.size(); u++)
    uref.values[u] = vData[u];

  uref.num_values = vData.size();

  // send usages
  if (ioctl(iUSBBATHandle, HIDIOCSUSAGES, &uref) == -1)
  {
    perror("HIDIOCSUSAGES");
    throw (const char *)NULL;
  }

  // send report
  rinfo.report_type = HID_REPORT_TYPE_OUTPUT;
  rinfo.report_id = uReportID;
  rinfo.num_fields = 1;
  if (ioctl(iUSBBATHandle, HIDIOCSREPORT, &rinfo) == -1)
  {
    perror("HIDIOCSREPORT");
    throw (const char *)NULL;
  }
}

// send a command (and optional arguments) to the device
// note that the variable list of arguments always needs to be terminated by '-1'
void Send(int iCommand, ...)
{
  int i;
  std::vector<unsigned char> vData(63, 0); // 63 bytes (report ID is declared separately)
  vData[1] = iCommand;
  int iNoBytes = 1; // command + data

  va_list ap;
  va_start(ap, iCommand);

  while (true)
  {
    i = va_arg(ap, int);
    if (i >= 0)
      vData[++iNoBytes] = (unsigned)i & 0xFFu;
    else
      break; // terminating parameter (-1)
  }

  va_end(ap);

  vData[0] = iNoBytes;

  // send this request to the device
  SendReport(1, vData);
}

void SwitchLight(bool bOn)
{
  // see ELV's documentation for details
  Send(bOn ? 0xF1 : 0xF2, 0, -1);
}

void Beeper(bool bOn, double fDurationMilliSec)
{
  if (bOn)
  {
    if (fDurationMilliSec < 0.0)
      fDurationMilliSec = 0.0;

    if (fDurationMilliSec > 2550.0)
      fDurationMilliSec = 2550.0;

    fDurationMilliSec /= 10.0;

    // see ELV's documentation for details
    Send(0xF3, (unsigned char)(fDurationMilliSec + 0.5), - 1);
  }
  else
  {
    // see ELV's documentation for details
    Send(0xF4, -1);
  }
}

void ReadFromUSBBAT()
{
  const int iExpectedEvents = 7; // = 8 bytes - HID-Report-ID byte
  struct hiddev_event events[iExpectedEvents];
  struct timeval tv = {0, 0};
  fd_set rfds;
  int iSelRet;

  do
  {
    // check if there is something to read from USBBAT by using the select function
    FD_ZERO(&rfds);
    FD_SET(iUSBBATHandle, &rfds);
    
    iSelRet = select(iUSBBATHandle + 1, &rfds, NULL, NULL, &tv);
    
    if (iSelRet < 0)
    {
      // error occurred
      perror("select");
      throw (const char *)NULL;
    }
    
    if (iSelRet > 0)
    {
      int iReadRet = read(iUSBBATHandle, events, sizeof(events));
      unsigned char uResponseID;
      unsigned char uVal[2];
      
      if (iReadRet >= iExpectedEvents * (int)sizeof(struct hiddev_event))
      {
        uResponseID = events[1].value;
        
        /*
        snprintf(sBuffer, BUFFER_SIZE, "%02X %02X %02X %02X %02X %02X %02X",
                 events[0].value, events[1].value, events[2].value, events[3].value,
                 events[4].value, events[5].value, events[6].value);
        cout << "Received event from device: " << sBuffer << endl;
        */
        
        if (uResponseID == 0xA0) // general status
        {
          uVal[0] = events[2].value;
          uVal[1] = events[3].value;
          
          switch (uVal[0])
          {
          case 0x00:
            // command executed successfully --> that's fine --> nothing to do
            break;
          case 0x01:
            snprintf(sBuffer, BUFFER_SIZE, "%02hhX.%02hhX",
                     (uVal[1] >> 4) & 0xF, uVal[1] & 0xF);
            cout << "Firmware version: V" << sBuffer << endl;
            break;
          case 0x02:
            cerr << "Unknown command ID." << endl;
            break;
          case 0x03:
            cerr << "Invalid command length." << endl;
            break;
          }
        }
        else if (uResponseID == 0xF5) // buttons / wheel status
        {
          unsigned char uButtonsBefore = uButtons; // remember previous button state in order to detect transitions properly
          
          uButtons = ((unsigned)events[5].value & 0x07) ^ 0x07; // read and invert buttons to human logic
          int iRotaryDelta = (signed char)(events[4].value & 0xFF);
          iRotaryValue += iRotaryDelta;
          
          // assume that we do not have to redraw the item - prove otherwise
          bool bRedrawItem = false;
          
          if (uButtonsBefore ^ uButtons)
          {
            // some button transition occurred
            // examine each button
            for (int iButton=0; iButton<3; ++iButton)
            {
              bool bButtonBefore = (uButtonsBefore & (1 << iButton)) != 0;
              bool bButtonNow = (uButtons & (1 << iButton)) != 0;
              
              if (bButtonNow != bButtonBefore)
              {
                // report to user
                cout << "Button " << iButton << " ";
                if (bButtonNow)
                  cout << "pressed";
                else
                  cout << "released";
                cout << endl;
                
                // do some action upon pressed button
                if (bButtonNow)
                {
                  switch (iButton)
                  {
                  case 0:
                    // toggle light
                    bLight = !bLight;
                    SwitchLight(bLight);
                    // short beep
                    Beeper(1, 100.0);
                    break;

                  case 1:
                    iItemY -= 2;
                    bRedrawItem = true;
                    break;

                  case 2:
                    iItemY += 2;
                    bRedrawItem = true;
                    break;

                  }
                }
              }
            }
          }
          
          if (iRotaryDelta)
          {
            // rotary wheel was moved
            cout << "Rotary wheel moved by " << iRotaryDelta << " increments. Absolute value is now " << iRotaryValue << endl;
            
            // move our item along the X-axis
            iItemX += iRotaryDelta * 2;
            bRedrawItem = true;
          }
          
          if (bRedrawItem)
            DrawItem();
        }
      }
      else
        cerr << "Short read" << endl;
    }
  } while (iSelRet > 0);
}

void QueryVersion()
{
  // see ELV's documentation for details
  Send(0xF0, -1);
}

#define AUTOSEND_OFF 0
#define AUTOSEND_STANDARD 1
#define AUTOSEND_EXTENDED 2
void AutoSendStatus(int eType)
{
  // see ELV's documentation for details
  Send(0xF7, eType, -1);
}

void ClearDisplay()
{
  // see ELV's documentation for details
  Send(0xD9, -1);
}

void ClearDisplayPage(int page)
{
   if (page < 0 || page > 7)
     return;
   Send(0xDA, page, -1);
}

void ClearBitmap()
{
  memset(theBitmap, 255, LCD_SX * LCD_SY); // clear the bitmap
}

void TransferBitmap()
{
  std::vector<unsigned char> vData(63, 0);
  unsigned uX, uY, uPixel;
  bool bBlank;

  for (unsigned uPage=0; uPage<8; ++uPage)
  {
    bBlank = true;
    for (unsigned uCol=0; uCol<61; ++uCol)
    {
      vData[0] = 62;
      vData[1] = 0xD0 + uPage;
      unsigned char & uData = vData[uCol + 2] = 0;

      for (unsigned uBit=0; uBit<8; ++uBit)
      {
        uX = (uPage % 2) * 61 + uCol;
        uY = (uPage / 2) * 8 + uBit;
        uPixel = uY * 122 + uX;
        if (theBitmap[uPixel] < 128)
        {
          uData |= 1 << uBit;
          bBlank = false;
        }
      }
    }

    if (bBlank)
      ClearDisplayPage(uPage);
    else
      SendReport(1, vData);
    
    usleep(5000);
  }
}

// custom draw some symbol to acknowledge user input
void DrawItem()
{
  int iX = iItemX;
  while (iX < 0)
    iX += LCD_SX;
  iX %= LCD_SX;

  int iY = iItemY;
  while (iY < 0)
    iY += LCD_SY;
  iY %= LCD_SY;

  ClearBitmap();

  Circle(iX, iY, 8);
  Line(iX, 0, iX, LCD_SY);
  Line(0, iY, LCD_SX, iY);

  TransferBitmap();
}

void ResetDisplay()
{
  AutoSendStatus(AUTOSEND_EXTENDED); // device shall send events without request
  ClearDisplay();
  SwitchLight(false);
  Beeper(false, 0);
  ClearBitmap();
  QueryVersion();
}

int main(int iArgCount, char * ppArgs [])
{
  try
  {
    iUSBBATHandle = OpenHIDDevice("ELV USB-BAT", 0x18EF, 0xE01A, 0);
    ResetDisplay();

    cout << "Note: Press Ctrl-C to terminate program." << endl;
    DrawItem();
    while (true)
      ReadFromUSBBAT();

    close(iUSBBATHandle);
  }
  catch (const char * pWhat)
  {
    if (pWhat)
      cerr << pWhat << endl;

    return 1;
  }
  catch (...)
  {
  }

  return 0;
}
