# USB-BAT for LCDproc

## DEB:  
 * must-have:  
apt-get install libusb-0.1-4 libusb-dev  
 * if you use freetype2 and TTF-fonts (monospace needed):  
apt-get install libfreetype6 libfreetype6-dev ttf-mscorefonts-installer  

## get LCDproc v0.5.7
  
## configure  
./configure --prefix=/usr/local --enable-drivers=glcd
  
## build  
make  
#### Note:  added compilerflag "-std=c99". This will produce errors during compilation of other drivers (usleep)!

## test  
server/LCDd -c LCDd.conf  
clients/lcdproc/lcdproc -c clients/lcdproc/lcdproc.conf  
clients/lcdexec/lcdexec -c clients/lcdexec/lcdexec.conf  
  
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
```
client_set -name Example  
screen_add myscreen  
screen_set myscreen -duration 10 -priority 2  
widget_add myscreen simplewidget string  
widget_set myscreen simplewidget 1 1 "Testing 1 2 3 !"  
## View the LCD  
widget_set myscreen simplewidget 1 2 "Testing 1 2 3 !"  
## View the LCD  
widget_add myscreen scrolling scroller  
widget_set myscreen scrolling 7 1 14 1 v 5 "1234567890abcd"  
## View the LCD  
widget_set myscreen scrolling 7 1 14 1 m 3 "  Welcome...I Hope you have a pleasant day! :)  "  
## View the LCD and quit your telnet session  
”  
quit  
```


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
update-rc.d lcdproc defaults  
update-rc.d lcdexec defaults  

#### Note: CPU-use is between 1.5% and 2% (at least on my XEON E3-1220L V2 @ 2.30GHz). Energysaving will suffer from polling the device.
