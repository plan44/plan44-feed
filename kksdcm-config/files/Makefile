# Copyright (c) 2023 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

.PHONY: all

# list the dtbo's required here. Sources must be in dtoverlays
overlays = spi-custom.dtbo


all: $(overlays)

clean:
	rm $(overlays)

$(overlays) : %.dtbo: dtoverlays/%.dts
	$(LINUX_DIR)/scripts/dtc/dtc -b 0 -@ -O dtb $< -o $@

