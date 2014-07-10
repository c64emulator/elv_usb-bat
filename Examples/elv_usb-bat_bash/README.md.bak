
*  dmesg after connection  
        usb 1-1.5: new full-speed USB device number 4 using ehci-pci  
        usb 1-1.5: New USB device found, idVendor=18ef, idProduct=e01a  
        usb 1-1.5: New USB device strings: Mfr=1, Product=2, SerialNumber=0  
        usb 1-1.5: Product: Bedien-Anzeige-Terminal USB-BAT  
        usb 1-1.5: Manufacturer: ELV Elektronik AG  
        hid-generic 0003:18EF:E01A.0005: hiddev0,hidraw0: USB HID v1.01 Device [ELV Elektronik AG Bedien-Anzeige-Terminal USB-BAT] on usb-0000:00:1a.0-1.5/input0

*  collecting info  
        > lsusb -v -d 18ef:e01a  
        > ls -al /dev/usb/hiddev*  
        > ls -al /dev/hidraw*  
        > udevadm info -a --name /dev/hidraw0

*  adding udev-rule which makes a symlink "/dev/usb-bat" to hidraw device "/dev/hidraw?"  
        # /etc/udev/rules.d/10-local.rules (note: one liner!)  
        KERNEL=="hidraw[0-9]*",ATTRS{idVendor}=="18ef",ATTRS{idProduct}=="e01a", SYMLINK+="usb-bat",MODE="777"  

*  sending command from shell (caution: some shells have buildin echo command)  
        # beep  
        /bin/echo -en '\x01\x02\xf3\x7f' | dd ibs=64 conv=sync of=/dev/usb-bat  
        # display on after keypress for 5 secs  
        /bin/echo -en '\x01\x02\xf2\x01' | dd ibs=64 conv=sync of=/dev/usb-bat  

*  sending short text message to line 2 of diplay  
         usblcd text2 '    hello world!    '  
