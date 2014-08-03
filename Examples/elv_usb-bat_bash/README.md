# USB-BAT on UNIX/LINUX/DEBIAN  
  
*  USB Speeds  
  * High Speed - 480Mbits/s  
  * Full Speed - 12Mbits/s  
  * Low Speed - 1.5Mbits/s  
  
  
*  dmesg after connection  
   >usb 1-1.5: new full-speed USB device number 4 using ehci-pci  
   >usb 1-1.5: New USB device found, idVendor=18ef, idProduct=e01a  
   >usb 1-1.5: New USB device strings: Mfr=1, Product=2, SerialNumber=0  
   >usb 1-1.5: Product: Bedien-Anzeige-Terminal USB-BAT  
   >usb 1-1.5: Manufacturer: ELV Elektronik AG  
   >hid-generic 0003:18EF:E01A.0005: hiddev0,hidraw0: USB HID v1.01 Device [ELV Elektronik AG Bedien-Anzeige-Terminal USB-BAT] on usb-0000:00:1a.0-1.5/input0
  
  
*  collecting info  
        > lsusb -v -d 18ef:e01a  
        > ls -al /dev/usb/hiddev*  
        > ls -al /dev/hidraw*  
        > udevadm info -a --name /dev/hidraw0  
        
        >root@hal:# udevadm info -a --name /dev/usb/hiddev1  
        >
        >Udevadm info starts with the device specified by the devpath and then  
        >walks up the chain of parent devices. It prints for every device  
        >found, all possible attributes in the udev rules key format.  
        >A rule to match, can be composed by the attributes of the device  
        >and the attributes from one single parent device.  
        >  
        >  looking at device '/devices/pci0000:00/0000:00:1a.0/usb3/3-1/3-1.2/3-1.2:1.0/usbmisc/hiddev1':  
        >    KERNEL=="hiddev1"  
        >    SUBSYSTEM=="usbmisc"  
        >    DRIVER==""  
        >  
        >  looking at parent device '/devices/pci0000:00/0000:00:1a.0/usb3/3-1/3-1.2/3-1.2:1.0':  
        >    KERNELS=="3-1.2:1.0"  
        >    SUBSYSTEMS=="usb"  
        >    DRIVERS=="usbhid"  
        >    ATTRS{bInterfaceClass}=="03"			<===== means HID-Device  
        >    ATTRS{bInterfaceSubClass}=="00"		<===== means no Boot Interface Subclass  
        >    ATTRS{bInterfaceProtocol}=="00"  
        >    ATTRS{bNumEndpoints}=="02"  
        >    ATTRS{supports_autosuspend}=="1"  
        >    ATTRS{bAlternateSetting}==" 0"  
        >    ATTRS{bInterfaceNumber}=="00"  
        >  
        >  looking at parent device '/devices/pci0000:00/0000:00:1a.0/usb3/3-1/3-1.2': 
        >    KERNELS=="3-1.2"  
        >    SUBSYSTEMS=="usb"  
        >    DRIVERS=="usb"  
        >    ATTRS{bDeviceSubClass}=="00"  
        >    ATTRS{bDeviceProtocol}=="00"  
        >    ATTRS{devpath}=="1.2"  
        >    ATTRS{idVendor}=="18ef"  
        >    ATTRS{speed}=="12"									<===== means Full Speed - 12Mbits/s  
        >    ATTRS{bNumInterfaces}==" 1"  
        >    ATTRS{bConfigurationValue}=="1"  
        >    ATTRS{bMaxPacketSize0}=="64"  
        >    ATTRS{busnum}=="3"  
        >    ATTRS{devnum}=="4"  
        >    ATTRS{configuration}==""  
        >    ATTRS{bMaxPower}=="64mA"  
        >    ATTRS{authorized}=="1"  
        >    ATTRS{bmAttributes}=="80"  
        >    ATTRS{bNumConfigurations}=="1"  
        >    ATTRS{maxchild}=="0"  
        >    ATTRS{bcdDevice}=="0000"  
        >    ATTRS{avoid_reset_quirk}=="0"  
        >    ATTRS{quirks}=="0x0"  
        >    ATTRS{version}==" 1.10"  
        >    ATTRS{urbnum}=="12"  
        >    ATTRS{ltm_capable}=="no"  
        >    ATTRS{manufacturer}=="ELV Elektronik AG"  
        >    ATTRS{removable}=="unknown"  
        >    ATTRS{idProduct}=="e01a"  
        >    ATTRS{bDeviceClass}=="00"  
        >    ATTRS{product}=="Bedien-Anzeige-Terminal USB-BAT"  
 
 
*  test  
 				udevadm monitor  
 				udevadm test /class/usbmisc/hiddev1  
       

*  adding udev-rule which makes a symlink "/dev/usb-bat" to hidraw device "/dev/hidraw?"  
        # /etc/udev/rules.d/10-local.rules (note: one liner!)  
        KERNEL=="hidraw[0-9]*",ATTRS{idVendor}=="18ef",ATTRS{idProduct}=="e01a", SYMLINK+="usb-bat",MODE="777"  


*  sending command from shell (caution: some shells have buildin echo command)  
        # beep  
        /bin/echo -en '\x01\x02\xf3\x7f' | dd ibs=64 conv=sync of=/dev/usb-bat  
        # display on after keypress for 5 secs  
        /bin/echo -en '\x01\x02\xf2\x01' | dd ibs=64 conv=sync of=/dev/usb-bat  


*  sending short text message to line 2 of display  
         usblcd text2 '    hello world!    '  
