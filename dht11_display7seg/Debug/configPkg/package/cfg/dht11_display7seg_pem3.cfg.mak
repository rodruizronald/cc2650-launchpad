# invoke SourceDir generated makefile for dht11_display7seg.pem3
dht11_display7seg.pem3: .libraries,dht11_display7seg.pem3
.libraries,dht11_display7seg.pem3: package/cfg/dht11_display7seg_pem3.xdl
	$(MAKE) -f C:\TI_RTOS\Workspace\CC2650\Drivers\Dht11_Display7Seg/src/makefile.libs

clean::
	$(MAKE) -f C:\TI_RTOS\Workspace\CC2650\Drivers\Dht11_Display7Seg/src/makefile.libs clean

