# invoke SourceDir generated makefile for dht11.pem3
dht11.pem3: .libraries,dht11.pem3
.libraries,dht11.pem3: package/cfg/dht11_pem3.xdl
	$(MAKE) -f C:\TI_RTOS\Workspace\CC2650\Drivers\Dht11/src/makefile.libs

clean::
	$(MAKE) -f C:\TI_RTOS\Workspace\CC2650\Drivers\Dht11/src/makefile.libs clean

