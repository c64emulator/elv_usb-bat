## [glcd-usbbat.c:] Driver for USB-BAT with LCDproc on UNIX/LINUX/DEBIAN  
  
  
* USB Message Frames:  
  
```       
     | HID-Report-ID |  Byte-Count 1) |     CMD-ID     |   CMD/Data 2)  |
     +---------------+----------------+----------------+----------------+
     |     (out:0x01)|       0x01-0x3E|                | 61 Byte        |
     | 1 Byte        | 1 Byte         | 1 Byte         |                |
     |      (in:0x02)|       0x01-0x06|                |  5 Byte        |
     +---------------+----------------+----------------+----------------+
   
       1) only CMD-ID and Datafields count
       2) unused bytes are filled with zeroes (0x00)
```       
   
* Framebuffer
  
The USB-BAT uses a LCD module of 122x32 pixel (DM12232-05YT)
This display area is tiled/paged over 2 controllers (PT6520) with 4 tiles/pages
each. Each tile has 8x61 pixel, each data byte representing
a 1-bit wide vertical line/column of the tile.

The display can be updated at a tile granularity.
```       
              Chip 1           Chip 2        
        |0     E0      60|61    E1     121|
      --+----------------+----------------+
       0|                |                |
        |  Tile0/Page0   |  Tile0/Page1   |
       7|                |                |
      --+----------------+----------------+
        |  Tile1/Page2   |  Tile1/Page3   |
        +----------------+----------------+
        |  Tile2/Page4   |  Tile2/Page5   |                                     .
        +----------------+----------------+
      31|  Tile3/Page6   |  Tile3/Page7   |
      --+----------------+----------------+
``` 

#### Note: This program will eat some of your CPU power! CPU use will be between 1% and 10% while simply polling the device for keypress!
