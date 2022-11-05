#
# OMNeT++/OMNEST Makefile for $(LIB_PREFIX)rdp
#
# This file was generated with the command:
#  opp_makemake -M release --make-so -f --deep -KECMP_PROJ=.=/home/luca/ecmp -KINET4_PROJ=/home/luca/inet -DINET_IMPORT -I/home/luca/inet/src -L/home/luca/ecmp/src -L/home/luca/inet/src -lecmp -lINET
#

# Name of target to be created (-o option)
TARGET = $(LIB_PREFIX)rdp$(D)$(SHARED_LIB_SUFFIX)
TARGET_DIR = .

# C++ include paths (with -I)
INCLUDE_PATH = -I/home/luca/inet/src

# Additional object and library files to link with
EXTRA_OBJS =

# Additional libraries (-L, -l options)
LIBS = $(LDFLAG_LIBPATH)/home/luca/ecmp/src $(LDFLAG_LIBPATH)/home/luca/inet/src  -lecmp -lINET

# Output directory
PROJECT_OUTPUT_DIR = ../out
PROJECTRELATIVE_PATH = src
O = $(PROJECT_OUTPUT_DIR)/$(CONFIGNAME)/$(PROJECTRELATIVE_PATH)

# Object files for local .cc, .msg and .sm files
OBJS = \
    $O/application/rdpapp/RdpAppBase.o \
    $O/application/rdpapp/RdpBasicClientApp.o \
    $O/application/rdpapp/RdpSinkApp.o \
    $O/common/ResultFiltersThroughput.o \
    $O/common/TopologyEcmp.o \
    $O/common/queue/RdpSwitchQueue.o \
    $O/common/scheduler/CentralSchedulerRdp.o \
    $O/transportlayer/common/L4ToolsRdp.o \
    $O/transportlayer/contract/rdp/RdpSocket.o \
    $O/transportlayer/rdp/Rdp.o \
    $O/transportlayer/rdp/RdpConnectionBase.o \
    $O/transportlayer/rdp/RdpConnectionEventProc.o \
    $O/transportlayer/rdp/RdpConnectionRcvSegment.o \
    $O/transportlayer/rdp/RdpConnectionUtil.o \
    $O/transportlayer/rdp/RdpReceiveQueue.o \
    $O/transportlayer/rdp/RdpSendQueue.o \
    $O/transportlayer/rdp/flavours/DumbRdp.o \
    $O/transportlayer/rdp/flavours/RdpAIMD.o \
    $O/transportlayer/rdp/rdp_common/RdpHeader.o \
    $O/application/rdpapp/GenericAppMsgRdp_m.o \
    $O/transportlayer/contract/rdp/RdpCommand_m.o \
    $O/transportlayer/rdp/RdpConnection_m.o \
    $O/transportlayer/rdp/rdp_common/RdpHeader_m.o

# Message files
MSGFILES = \
    application/rdpapp/GenericAppMsgRdp.msg \
    transportlayer/contract/rdp/RdpCommand.msg \
    transportlayer/rdp/RdpConnection.msg \
    transportlayer/rdp/rdp_common/RdpHeader.msg

# SM files
SMFILES =

# Default mode (-M option); can be overridden with make MODE=debug (or =release)
ifndef MODE
MODE = release
endif

# Other makefile variables (-K)
ECMP_PROJ=.=/home/luca/ecmp
INET4_PROJ=/home/luca/inet

#------------------------------------------------------------------------------

# Pull in OMNeT++ configuration (Makefile.inc)

ifneq ("$(OMNETPP_CONFIGFILE)","")
CONFIGFILE = $(OMNETPP_CONFIGFILE)
else
ifneq ("$(OMNETPP_ROOT)","")
CONFIGFILE = $(OMNETPP_ROOT)/Makefile.inc
else
CONFIGFILE = $(shell opp_configfilepath)
endif
endif

ifeq ("$(wildcard $(CONFIGFILE))","")
$(error Config file '$(CONFIGFILE)' does not exist -- add the OMNeT++ bin directory to the path so that opp_configfilepath can be found, or set the OMNETPP_CONFIGFILE variable to point to Makefile.inc)
endif

include $(CONFIGFILE)

# Simulation kernel and user interface libraries
OMNETPP_LIBS = -loppenvir$D $(KERNEL_LIBS) $(SYS_LIBS)
ifneq ($(TOOLCHAIN_NAME),clangc2)
LIBS += -Wl,-rpath,$(abspath /home/luca/ecmp/src) -Wl,-rpath,$(abspath /home/luca/inet/src)
endif

COPTS = $(CFLAGS) $(IMPORT_DEFINES) -DINET_IMPORT $(INCLUDE_PATH) -I$(OMNETPP_INCL_DIR)
MSGCOPTS = $(INCLUDE_PATH)
SMCOPTS =

# we want to recompile everything if COPTS changes,
# so we store COPTS into $COPTS_FILE and have object
# files depend on it (except when "make depend" was called)
COPTS_FILE = $O/.last-copts
ifneq ("$(COPTS)","$(shell cat $(COPTS_FILE) 2>/dev/null || echo '')")
$(shell $(MKPATH) "$O" && echo "$(COPTS)" >$(COPTS_FILE))
endif

#------------------------------------------------------------------------------
# User-supplied makefile fragment(s)
# >>>
# inserted from file 'makefrag':
MSGC:=$(MSGC) --msg6

# <<<
#------------------------------------------------------------------------------

# Main target
all: $(TARGET_DIR)/$(TARGET)

$(TARGET_DIR)/% :: $O/%
	@mkdir -p $(TARGET_DIR)
	$(Q)$(LN) $< $@
ifeq ($(TOOLCHAIN_NAME),clangc2)
	$(Q)-$(LN) $(<:%.dll=%.lib) $(@:%.dll=%.lib)
endif

$O/$(TARGET): $(OBJS)  $(wildcard $(EXTRA_OBJS)) Makefile $(CONFIGFILE)
	@$(MKPATH) $O
	@echo Creating shared library: $@
	$(Q)$(SHLIB_LD) -o $O/$(TARGET) $(OBJS) $(EXTRA_OBJS) $(AS_NEEDED_OFF) $(WHOLE_ARCHIVE_ON) $(LIBS) $(WHOLE_ARCHIVE_OFF) $(OMNETPP_LIBS) $(LDFLAGS)
	$(Q)$(SHLIB_POSTPROCESS) $O/$(TARGET)

.PHONY: all clean cleanall depend msgheaders smheaders

.SUFFIXES: .cc

$O/%.o: %.cc $(COPTS_FILE) | msgheaders smheaders
	@$(MKPATH) $(dir $@)
	$(qecho) "$<"
	$(Q)$(CXX) -c $(CXXFLAGS) $(COPTS) -o $@ $<

%_m.cc %_m.h: %.msg
	$(qecho) MSGC: $<
	$(Q)$(MSGC) -s _m.cc -MD -MP -MF $O/$(basename $<)_m.h.d $(MSGCOPTS) $?

%_sm.cc %_sm.h: %.sm
	$(qecho) SMC: $<
	$(Q)$(SMC) -c++ -suffix cc $(SMCOPTS) $?

msgheaders: $(MSGFILES:.msg=_m.h)

smheaders: $(SMFILES:.sm=_sm.h)

clean:
	$(qecho) Cleaning $(TARGET)
	$(Q)-rm -rf $O
	$(Q)-rm -f $(TARGET_DIR)/$(TARGET)
	$(Q)-rm -f $(TARGET_DIR)/$(TARGET:%.dll=%.lib)
	$(Q)-rm -f $(call opp_rwildcard, . , *_m.cc *_m.h *_sm.cc *_sm.h)

cleanall:
	$(Q)$(MAKE) -s clean MODE=release
	$(Q)$(MAKE) -s clean MODE=debug
	$(Q)-rm -rf $(PROJECT_OUTPUT_DIR)

# include all dependencies
-include $(OBJS:%=%.d) $(MSGFILES:%.msg=$O/%_m.h.d)
