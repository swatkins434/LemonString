### MUST BE COMPILED IN UNIX/LINUX ENVIRONMENT ###

OPTS=-O3 -I.
LIBS=-lm -lrt
BINDIR=../../bin
SRC=$(basename $(wildcard *.c))
OUTFILES=$(foreach f,$(basename $(SRC)),$(BINDIR)/$(f))

.DEFAULT_GOAL := default

FORCE:

DEFINES?=DESKTOP

$(addsuffix /compiler_flags,$(OUTFILES)): FORCE
	@mkdir -p $(dir $@)
	@echo "$(DEFINES)" | cmp -s - $@ || echo "$(DEFINES)" > $@

$(BINDIR)/%: %.c $(BINDIR)/%/compiler_flags
	@echo "Rebuilding $(notdir $@)"
	@mkdir -p $@
	@gcc -o $@/$(notdir $@) $(OPTS) $(LIBS) $(notdir $@).c $(addprefix -D ,$(DEFINES)) $(QUIET)

clean:
	@rm -rfv $(OUTFILES)

all: $(OUTFILES)
	@echo >/dev/null

default: clean all
	@echo >/dev/null

.PHONY: all clean default
