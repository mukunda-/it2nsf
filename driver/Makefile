#
# IT-NES DRIVER MAKEFILE
#

.SUFFIXES:

#---------------------------------------------------------------------------------

CC65		:= /c/sneskit/cc65/bin
AS			:= $(CC65)/ca65
LD			:= $(CC65)/ld65

#---------------------------------------------------------------------------------

TARGET		:=	driver.nsf
BUILD		:=	build
SOURCES		:=	source
INCLUDES	:=	include

#---------------------------------------------------------------------------------

ASFLAGS		:=	--cpu 6502 -s -D HIROM
INCLUDE		:=	$(foreach dir,$(INCLUDES),-I $(CURDIR)/$(dir)) -I $(CURDIR)/$(BUILD)

OUTPUT		:=	$(CURDIR)/$(TARGET)
LDFLAGS		:=	-C $(CURDIR)/config.ld
VPATH		:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) 
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.asm)))
OFILES		:=	$(SFILES:.asm=.o)
 
.PHONY: clean
 
all	: $(TARGET)

#---------------------------------------------------------------------------------

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).smc $(TARGET).map
	
#---------------------------------------------------------------------------------

%.nsf: $(OFILES)
	@echo LINKING
	@$(LD) $(LDFLAGS) $(OFILES) -o $@ -m $(@:.nsf=.map)
	@echo Okay! :: $(notdir $@)
	
#---------------------------------------------------------------------------------

%.o : %.asm
	@echo Assembling :: $(notdir $<)
	@$(AS) $(ASFLAGS) $(INCLUDE) $< -o $@
