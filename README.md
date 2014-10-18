#elv_usb-bat  
  
All about the Bedien-Anzeige-Terminal from ELV.de (USB-BAT, order# 92448)  
  
##Other Repos:  
 - https://github.com/Vindaomeo/elv_usb_bat: Python class to access the ELV USB-Bat Terminal  
 - ... (tbd)  
  
##Other Ressources  
 - Seller: http://www.elv.de/bedien-anzeige-terminal-fuer-fs20-hausautomation-komplettbausatz.html  
 - Wakebox-Wiki: http://www.wakebox.de/wiki/index.php?title=Anleitung:_Konfiguration_des_USB-BAT_von_ELV_mit_wakeboX  
 - LCDproc (Multi-platform LCD display driver): http://www.lcdproc.org/  
 - similiar product: picoLCD 256x64 Sideshow for CD-Bay (http://www.mini-box.com/picoLCD)  
 - ... (tbd)  
  
##see more info at the wiki!  
  
##Meine Wunschliste f�r USB-BAT v2.0:
   *  Seriennummer (bzw. als ID in einem device-eeprom abspeicherbar), um mehrere USB-BAT eindeutig unterscheiden zu k�nnen
   *  custom character in einem eeprom des devices abspeicherbar
   *  Der Zeichensatz des USB-BAT sollte um Icons und Bar-Graph-Extensions erweitert werden, �hnlich wie unter <http://mmdolze.users.sourceforge.net/lcdproc-fonts.html> zu sehen ist.
   *  Kontrast des Displays und Helligkeit der Beleuchtung per Soft-Befehl einstellbar
   *  Einschaltdauer der Beleuchtung sollte nicht in 10ms(!) Schritten, sondern in Minuten (0 bis 255 min) konfigurierbar sein
   *  Toggle-Befehl, um die Beleuchtung umzuschalten
   *  Einschaltdauer der Beleuchtung bei Tastendruck (momentan fix auf 5 sec) sollte konfigurierbar sein (0 bis 255 sec)
   *  Energie-Spar-Modus (momentan: mit Beleuchtung: 420mW, ohne Beleuchtung: 113mW), der per Softbefehl eingeschaltet und per Knopfdruck deaktiviert werden kann
   *  debug modus, in dem die erhaltenen Daten/Kommandos als Hexdump angezeigt werden (64-Byte-Messages: 4 Zeilen � 16 Zeichen)
   *  Buzzer sollte PC-Speaker/Beep-konform sein, dh. verschiedene Tonh�hen/Frequenzen unterst�tzen
   *  andere Display-Farben(z.B. blau/wei�)
   *  OLED-Display
   *  beleuchtete (dimmbar!) farbige Tasten
   *  SMD-Bauteile nicht so nah an den L�tstellen/Pads f�r die Selbstbest�ckung
   *  mit geringem Aufwand in einen 5.25"-Einschub montierbar
   *  Einsatz von Schrauben, die notfalls auch im Baumarkt nachgekauft werden k�nnen. Das gilt insbesondere f�r die Kunststoffschrauben 2.5x12 (Bossard B2.5X12/BN82428)
      