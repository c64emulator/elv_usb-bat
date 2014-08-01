# USB-BAT on UNIX/LINUX/DEBIAN  
  
* USB Speeds  
  * High Speed - 480Mbits/s  
  * Full Speed - 12Mbits/s  
  * Low Speed - 1.5Mbits/s  
  
* lsusb -v -d  18ef:e01a  

* l /dev/usb/hiddev*  
* l /dev/hidraw*  
  
* dmesg:  
>usb 1-1.5: new full-speed USB device number 4 using ehci-pci  
>usb 1-1.5: New USB device found, idVendor=18ef, idProduct=e01a  
>usb 1-1.5: New USB device strings: Mfr=1, Product=2, SerialNumber=0  
>usb 1-1.5: Product: Bedien-Anzeige-Terminal USB-BAT  
>usb 1-1.5: Manufacturer: ELV Elektronik AG  
>hid-generic 0003:18EF:E01A.0005: hiddev0,hidraw2: USB HID v1.01 Device [ELV Elektronik AG Bedien-Anzeige-Terminal USB-BAT] on usb-0000:00:1a.0-1.5/input0  

* udevadm info --attribute-walk -p /sys/class/usbmisc/hiddev1  
 oder udevadm info -a --name /dev/usb/hiddev1  

root@hal:# udevadm info -a --name /dev/usb/hiddev1  

Udevadm info starts with the device specified by the devpath and then  
walks up the chain of parent devices. It prints for every device  
found, all possible attributes in the udev rules key format.  
A rule to match, can be composed by the attributes of the device  
and the attributes from one single parent device.  
  
  looking at device '/devices/pci0000:00/0000:00:1a.0/usb3/3-1/3-1.2/3-1.2:1.0/usbmisc/hiddev1':  
    KERNEL=="hiddev1"  
    SUBSYSTEM=="usbmisc"  
    DRIVER==""  
  
  looking at parent device '/devices/pci0000:00/0000:00:1a.0/usb3/3-1/3-1.2/3-1.2:1.0':  
    KERNELS=="3-1.2:1.0"  
    SUBSYSTEMS=="usb"  
    DRIVERS=="usbhid"  
    ATTRS{bInterfaceClass}=="03"		<===== means HID-Device  
    ATTRS{bInterfaceSubClass}=="00"	<===== means no Boot Interface Subclass  
    ATTRS{bInterfaceProtocol}=="00"  
    ATTRS{bNumEndpoints}=="02"  
    ATTRS{supports_autosuspend}=="1"  
    ATTRS{bAlternateSetting}==" 0"  
    ATTRS{bInterfaceNumber}=="00"  
  
  looking at parent device '/devices/pci0000:00/0000:00:1a.0/usb3/3-1/3-1.2': 
    KERNELS=="3-1.2"  
    SUBSYSTEMS=="usb"  
    DRIVERS=="usb"  
    ATTRS{bDeviceSubClass}=="00"  
    ATTRS{bDeviceProtocol}=="00"  
    ATTRS{devpath}=="1.2"  
    ATTRS{idVendor}=="18ef"  
    ATTRS{speed}=="12"							<===== means Full Speed - 12Mbits/s  
    ATTRS{bNumInterfaces}==" 1"  
    ATTRS{bConfigurationValue}=="1"  
    ATTRS{bMaxPacketSize0}=="64"  
    ATTRS{busnum}=="3"  
    ATTRS{devnum}=="4"  
    ATTRS{configuration}==""  
    ATTRS{bMaxPower}=="64mA"  
    ATTRS{authorized}=="1"  
    ATTRS{bmAttributes}=="80" 
    ATTRS{bNumConfigurations}=="1"  
    ATTRS{maxchild}=="0"  
    ATTRS{bcdDevice}=="0000"  
    ATTRS{avoid_reset_quirk}=="0"  
    ATTRS{quirks}=="0x0"  
    ATTRS{version}==" 1.10"  
    ATTRS{urbnum}=="12"  
    ATTRS{ltm_capable}=="no"  
    ATTRS{manufacturer}=="ELV Elektronik AG"  
    ATTRS{removable}=="unknown"  
    ATTRS{idProduct}=="e01a"  
    ATTRS{bDeviceClass}=="00"  
    ATTRS{product}=="Bedien-Anzeige-Terminal USB-BAT"  
  
  
* udev  
info:  
   <http://www.weather-watch.com/smf/index.php?topic=39257.0> 
   <http://www.stefanux.de/wiki/doku.php/linux/udev>  
  
* /etc/udev/rules.d/10-local.rules  
KERNEL=="hidraw[0-9]*",ATTRS{idVendor}=="18ef",ATTRS{idProduct}=="e01a",  SYMLINK+="usb-bat",MODE="777"  
  
echo -en '\x01\x02\xf3\x7f' | dd ibs=64 conv=sync of=paddedFile.bin  
hexdump -C paddedFile.bin  

output:  
 #beep /bin/echo -en '\x01\x02\xf3\x7f' | dd ibs=64 conv=sync of=/dev/usb-bat  
 #display on:  /bin/echo -en '\x01\x02\xf2\x01' | dd ibs=64 conv=sync of=/dev/usb-bat  
 
input: cat /dev/hidraw0 | hexdump -C  
  
* test 
 udevadm monitor
 udevadm test /class/usbmisc/hiddev1

 

==
# USB-BAT for LCDproc

## DEB:  
 * must-have  
apt-get install libusb-0.1-4 libusb-dev  
 * if you use freetype2 and TTF-fonts (monospace needed):  
apt-get install libfreetype6 libfreetype6-dev ttf-mscorefonts-installer  
  
## configure  
./configure --prefix=/usr/local --enable-drivers=glcd
  
## build  
make

## test  
server/LCDd -c LCDd.conf  
clients/lcdproc/lcdproc -c lcdproc.conf  
clients/lcdexec/lcdexec -c lcdexec.conf  
  
## Quick Demo of Commands:  
>> This assumes LCDd is running, bound to IP 127.0.0.1 on port 13666. From the board, type in:  
>telnet localhost 13666 
>> after it connects, type in :  
>hello  
>> you should receive: connect LCDproc 0.5.7 protocol 0.3 lcd wid 20 hgt 4 cellwid 6 cellhgt 8  
  
>> Type in the following:  
>backlight flash  
>> View the LCD   
>backlight blink  
>> View the LCD  
>backlight on  
>> View the LCD  
>backlight off  
>> View the LCD  
>backlight on  
>> View the LCD  
  
>> now let's get serious, type in the following:   
>client_set -name Example  
>screen_add myscreen  
>screen_set myscreen -duration 10 -priority 2  
>widget_add myscreen simplewidget string  
>widget_set myscreen simplewidget 1 1 "Testing 1 2 3 !"  
>> View the LCD  
>widget_set myscreen simplewidget 1 2 "Testing 1 2 3 !"  
>> View the LCD  
>widget_add myscreen scrolling scroller  
>widget_set myscreen scrolling 7 1 14 1 v 5 "1234567890abcd"  
>> View the LCD  
>widget_set myscreen scrolling 7 1 14 1 m 3 "  Welcome...I Hope you have a pleasant day! :)  "  
>> View the LCD and quit your telnet session  
>”  
>quit  

## installation  
* make install  
* modules:  
root@hal:/usr/local/sbin# cp ../src/LCDproc/lcdproc-0.5.7/server/drivers/glcd.so .  
root@hal:/usr/local/sbin# cp ../src/LCDproc/lcdproc-0.5.7/server/drivers/glcd-glcd-usbbat.o .  
* initscripts:  
cp scripts/init-LCDd.debian /etc/init.d/LCDd 
cp scripts/init-lcdproc.debian /etc/init.d/lcdproc  
cp scripts/init-lcdexec.debian /etc/init.d/lcdexec   
chmod a+x /etc/init.d/lcd*  
update-rc.d LCDd defaults  
  
  
## Wunschliste:
- Seriennummer (bzw. als ID im eeprom abspeicherbar), um mehrere USB-BAT eindeutig unterscheiden zu können
- Kontrast des Displays und Helligkeit der Beleuchtung per Soft-Befehl einstellbar
- SMD-Bauteile nicht so nah an den Lötstellen/Pads für die Selbstbestückung
- Buzzer sollte PC-Speaker/Beep-konform sein, dh. verschiedene Tonhöhen/Frequenzen unterstützen
- mit geringem Aufwand in einen 5.25"-Einschub montierbar
- custom character im eeprom des devices abspeicherbar
- andere Display-Farben(z.B. blau/weiß)
- beleuchtete (dimmbar!) farbige Tasten
- Toggle-Befehl, um die Beleuchtung umzuschalten
- debug modus, in dem die erhaltenen Daten/Kommandos als Hexdump angezeigt werden (64-Byte-Messages: 4 Zeilen à 16 Zeichen)
