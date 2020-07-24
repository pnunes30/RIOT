Program Ciot25 chip info for RIOT

Important: Don't forget to comment/uncomment the LINKER_SCRIPT line inside cortus_aps/Makefile.include 

/*****************************************************/
/******************** SRAM ***************************/
/*****************************************************/

Linker script: lscripts/elf32aps.ld
Eclispe Debug config: 

  Main tab:
      Provide gdbinit file in include/Bsp/ciot25_gdbinit file.
      Check box: - Start on startup at: main,
                 - Breakpoint on exit
                 - Generic 'load' command (according to the gdbinit file)

  OpenOCD params tab:
      CPU family --> cortus-apsv2
      Platform   --> CIoT25
      Cable      --> Cortus FPGA Board
      Other flags : /home/user/cortus-ide/bin/openocd_nikada  -f target/cortus-apsv2.cfg  -f interface/cortus-ft2232.cfg  -f board/cortus-ciot25.cfg
      Check box: - Lunch OpenOCD
      Pattern: ${FLAG}


/*****************************************************/
/******************** FLASH **************************/
/****************** FULL BANK ************************/
/*****************************************************/

Linker script: lscripts/elf32aps_flash.x
Eclispe Debug config: 

  Main tab:
      Provide gdbinit file in include/Bsp/ciot25_gdbinit_flash file.
      Check box: - Start on startup at: main,
                 - Breakpoint on exit
                 - Generic 'load' command (according to the gdbinit file)

  OpenOCD params tab:
      CPU family --> cortus-apsv2
      Platform   --> CIoT25
      Cable      --> Cortus FPGA Board
      Other flags : /home/user/cortus-ide/bin/openocd_nikada  -f target/cortus-apsv2.cfg  -f interface/cortus-ft2232.cfg  -f board/cortus-ciot25-sstflash.cfg
      Check box: - Lunch OpenOCD
      Pattern: ${FLAG}

NOTE: use cortus-ciot25-sstflash.cfg only for flashing when code has changed, cortus-ciot25.cfg otherwise


/*****************************************************/
/******************** FLASH **************************/
/****************** 1st  BANK ************************/
/*****************************************************/

We need to have a specific linker script (or flash driver)
for Bank0, otherwise when flashing using elf32aps_flash.x 
it will erase the 2nd bank !!!!
Until then, flash first the bank0 then bank1.


/*****************************************************/
/******************** FLASH **************************/
/****************** 2nd  BANK ************************/
/*****************************************************/

Linker script: lscripts/elf32aps_flash_bank1.x
Eclispe Debug config: 

  Main tab:
      Provide gdbinit file in include/Bsp/ciot25_gdbinit_flash file.
      Check box: - Start on startup at: main,
                 - Breakpoint on exit
                 - Generic 'load' command (according to the gdbinit file)

  OpenOCD params tab:
      CPU family --> cortus-apsv2
      Platform   --> CIoT25
      Cable      --> Cortus FPGA Board
      Other flags : /home/user/cortus-ide/bin/openocd_nikada  -f target/cortus-apsv2.cfg  -f interface/cortus-ft2232.cfg  -f board/cortus-ciot25-sstflash.cfg
      Check box: - Lunch OpenOCD
      Pattern: ${FLAG}

NOTE: use cortus-ciot25-sstflash.cfg only for flashing when code has changed, cortus-ciot25.cfg otherwise
In order to flash the chip at the right address, open the cortus-ciot25-sstflash.cfg file
Comment this line : flash bank flash ciot25sst 0x80000000 0x3fe00 4 4 cpu
And uncomment this one : flash bank flash ciot25sst 0x8003fe00 0x3fe00 4 4 cpu


/*****************************************************/
/******************** TESTING ************************/
/*****************************************************/

1. Flash your program (simple 'hello from bank 0') inside bank0 (no need to run/debug it)
2. Flash your program (simple 'Hello from bank 1') insise bank1 (no need to run/debug it)
3. Lunch openOCD (openocd_nikada -f target/cortus-apsv2.cfg  -f interface/cortus-ft2232.cfg  -f board/cortus-ciot25.cfg)
4. Lunch GDB (aps-gdb) without an .exe
5. Set bootkey with odd value of 1 thus selecting the 2nd bank next time it'll boot:
In gdb, enter : 
    set remotetimeout 600
    tar remote:3333
    set *(0xa0000000) = 0x20000       # 1 << 17 => write_en in ctrreg flash_ctl
    set *(0xa0000004) = 0x7           # tACC flash_ctl
    set *(0x8007FC00) = 0xfffffffe    # write one 0 to FLASH_BOOT_KEY
    set *(0xa0000000) = 0x0           # put write_en = 0 in ctrreg flash_ctl
    x 0x8007FC00                      # verify value
    
6. Hit c to run the 2nd bank flashed program and see your printf.
If not, quit openOCD and GDB and press the reset button on the demo board or unplug/plug the board.

Do not try to rewrite the bootkey with 0xffffffff, it won't work.
The flash can only write after an erase (at least 1 sector) OR to set value from 1b to 0b 
(e.g: if initial value is 1 then it can be write down to 0, if initial value 
0 then it can't be written back to 1 without an erase first)


/* End of text file */