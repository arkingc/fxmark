# verbose flag
BUILD_VERBOSE = $(V)
ifeq ($(BUILD_VERBOSE),1)
  Q =
else
  Q = @
endif

# directory
SRC = src
BIN = bin

# cflags and source code
CFLAGS += $(DEFS) -Wall -g -O3 -D_GNU_SOURCE
LDFLAGS +=
LIBS    = $(SRC)/bench.c $(SRC)/util.c
TC      = $(SRC)/MWCM.c $(SRC)/MWCL.c \
		  $(SRC)/DWAL.c $(SRC)/DWOL.c \
		  $(SRC)/DWSL.c $(SRC)/MWRM.c \
		  $(SRC)/DWOM.c $(SRC)/MWRL.c \
		  $(SRC)/DRBL.c $(SRC)/DRBM.c \
		  $(SRC)/DRBH.c $(SRC)/MRDL.c \
		  $(SRC)/MRDM.c $(SRC)/MRPM.c \
		  $(SRC)/MRDL_bg.c $(SRC)/DRBL_bg.c \
		  $(SRC)/DRBH_bg.c $(SRC)/MRDM_bg.c \
		  $(SRC)/DRBM_bg.c $(SRC)/MRPM_bg.c \
		  $(SRC)/MWUM.c $(SRC)/MWUL.c \
		  $(SRC)/DWTL.c $(SRC)/MRPH.c \
		  $(SRC)/MRPL.c $(SRC)/MOLH.c \
		  $(SRC)/MOLC.c $(SRC)/MOLI.c \
		  $(SRC)/MULH.c $(SRC)/MULC.c \
		  $(SRC)/MULI.c $(SRC)/MCLH.c \
		  $(SRC)/MCLC.c $(SRC)/MCLI.c \
		  $(SRC)/MRLH.c $(SRC)/MRLC.c \
		  $(SRC)/MRLI.c $(SRC)/DALH.c \
		  $(SRC)/DALC.c $(SRC)/DALI.c \
		  $(SRC)/DTLH.c $(SRC)/DTLC.c \
		  $(SRC)/DTLI.c $(SRC)/DRHH.c \
		  $(SRC)/DRHI.c $(SRC)/DRLH.c \
		  $(SRC)/DWLH.c $(SRC)/MOLCDM.c \
		  $(SRC)/MOLIDM.c $(SRC)/MCLCDM.c \
		  $(SRC)/MCLIDM.c $(SRC)/MULCDM.c \
		  $(SRC)/MULIDM.c $(SRC)/MRLCDM.c \
		  $(SRC)/MRLIDM.c $(SRC)/DRLCDM.c \
		  $(SRC)/DRLIDM.c $(SRC)/DWLCDM.c \
		  $(SRC)/DWLIDM.c $(SRC)/DALCDM.c \
		  $(SRC)/DALIDM.c $(SRC)/DTLCDM.c \
		  $(SRC)/DTLIDM.c $(SRC)/MOLCBT.c \
		  $(SRC)/MOLIBT.c $(SRC)/MCLCBT.c \
		  $(SRC)/MCLIBT.c $(SRC)/MULCBT.c \
		  $(SRC)/MULIBT.c $(SRC)/MRLCBT.c \
		  $(SRC)/MRLIBT.c $(SRC)/DRLCBT.c \
		  $(SRC)/DRLIBT.c $(SRC)/DWLCBT.c \
		  $(SRC)/DWLIBT.c $(SRC)/DALCBT.c \
		  $(SRC)/DALIBT.c $(SRC)/DTLCBT.c \
		  $(SRC)/DTLIBT.c $(SRC)/CMR.c \
		  $(SRC)/IMU.c $(SRC)/IMC.c \
		  $(SRC)/IDW.c $(SRC)/MOLCAU.c \
		  $(SRC)/MOLIAU.c
DEPS	= $(wildcard $(SRC)/*.h) $(LIBS) $(TC)
BINS	= $(BIN)/fxmark
CPUPOLS = $(SRC)/cpuinfo $(SRC)/cpupol.h $(BIN)/cpupol.py

# tool
CPU_POLICY = $(BIN)/cpu-sequences | $(BIN)/gen_corepolicy

# target
all: $(BINS) $(LIBS) $(TC)

$(BIN)/%: $(SRC)/%.c $(DEPS) $(SRC)/cpupol.h $(BIN)/cpupol.py
	@echo "CC	$@"
	$(Q)$(CC) $< $(CFLAGS) -o $@ $(LIBS) $(TC) $(LDFLAGS)

$(SRC)/cpuinfo:
	$(Q)sudo $(BIN)/set-cpus all > /dev/null
	- $(Q)cp /proc/cpuinfo $@ 

$(SRC)/cpupol.h: $(SRC)/cpuinfo $(BIN)/cpu-sequences $(BIN)/gen_corepolicy
	- $(Q)cat $(SRC)/cpuinfo | $(CPU_POLICY) c  > $@ 2>&1

$(BIN)/cpupol.py: $(SRC)/cpuinfo $(BIN)/cpu-sequences $(BIN)/gen_corepolicy
	- $(Q)cat $(SRC)/cpuinfo | $(CPU_POLICY) py > $@ 2>&1

clean:
	@echo "CLEAN"
	$(Q) rm -f $(BIN)/*.o $(BINS) $(CPUPOLS)

.PHONY: all clean
