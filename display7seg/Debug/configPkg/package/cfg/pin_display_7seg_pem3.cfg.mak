# invoke SourceDir generated makefile for pin_display_7seg.pem3
pin_display_7seg.pem3: .libraries,pin_display_7seg.pem3
.libraries,pin_display_7seg.pem3: package/cfg/pin_display_7seg_pem3.xdl
	$(MAKE) -f C:\TI_RTOS\Workspace\CC2650\Drivers\PIN_DISPLAY_7SEG/src/makefile.libs

clean::
	$(MAKE) -f C:\TI_RTOS\Workspace\CC2650\Drivers\PIN_DISPLAY_7SEG/src/makefile.libs clean

