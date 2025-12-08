# Copyright (c) 2016-2025 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

DTDIR := lib/firmware/device-tree
SRCDIR := $(DTDIR)/overlays-src
OUTDIR := $(DTDIR)/overlays-available

SOURCES := $(wildcard $(SRCDIR)/*.dts)
OUTPUTS := $(patsubst $(SRCDIR)/%.dts,$(OUTDIR)/%.dtbo,$(SOURCES))

.PHONY: all clean

all: $(OUTPUTS)

$(OUTDIR)/%.dtbo: $(SRCDIR)/%.dts | $(OUTDIR)
	dtc -b 0 -@ -O dtb $< -o $@

$(OUTDIR):
	mkdir -p $@

clean:
	rm -f $(OUTDIR)/*.dtbo
