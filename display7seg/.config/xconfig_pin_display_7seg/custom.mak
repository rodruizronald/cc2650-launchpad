## THIS IS A GENERATED FILE -- DO NOT EDIT
.configuro: .libraries,em3 linker.cmd package/cfg/pin_display_7seg_pem3.oem3

# To simplify configuro usage in makefiles:
#     o create a generic linker command file name 
#     o set modification times of compiler.opt* files to be greater than
#       or equal to the generated config header
#
linker.cmd: package/cfg/pin_display_7seg_pem3.xdl
	$(SED) 's"^\"\(package/cfg/pin_display_7seg_pem3cfg.cmd\)\"$""\"C:/TI_RTOS/Workspace/CC2650/Drivers/PIN_DISPLAY_7SEG/.config/xconfig_pin_display_7seg/\1\""' package/cfg/pin_display_7seg_pem3.xdl > $@
	-$(SETDATE) -r:max package/cfg/pin_display_7seg_pem3.h compiler.opt compiler.opt.defs
