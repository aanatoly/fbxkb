###############################################
#  environment checks                         #
###############################################

$(TOPDIR)/output/config.mk:
	@echo Please run $(TOPDIR)/configure
	@echo
	@false
include $(TOPDIR)/output/config.mk
CFLAGS += -O3 -I$(TOPDIR)/output


###############################################
#  recurion rules                             #
###############################################

RGOALS = all clean install svnignore
.PHONY : $(RGOALS) $(SUBDIRS)
$(RGOALS) : $(SUBDIRS)

$(SUBDIRS):
	$Q$(MAKE) -S -C $@ $(MAKECMDGOALS)
unexport SUBDIRS

$(RGOALS) $(SUBDIRS) : FORCE
FORCE:
	@#

.DEFAULT_GOAL := all
CONF = $(TOPDIR)/output/config.h $(TOPDIR)/output/config.mk

define obj_rules
CLEANLIST += $(1).o
$(1).o : $(1).c $(CONF)
	$(CC) $(CFLAGS) $($(2)_cflags) -c -o $$@ $$<
endef

define bin_rules
all : $(1)
CLEANLIST += $(1)

$(foreach o,$($(1)_src:%.c=%),$(eval $(call obj_rules,$(o),$(1))))

$(1) : $($(1)_src:%.c=%.o)
	$(CC) $($(1)_src:%.c=%.o) $(LDFLAGS) $($(1)_libs) -o $$@

ifeq ($($(1)_install),)
install : $(1)_install
$(1)_install :
	install -m 755 -d $(DESTDIR)$(BIN_DIR)
	install -m 755 $(1) $(DESTDIR)$(BIN_DIR)
endif
endef

targets = $(filter %_type,$(.VARIABLES))
$(foreach t,$(targets),$(eval $(call $($(t))_rules,$(t:_type=))))


###############################################
#  clean                                      #
###############################################

clean:
ifneq (,$(CLEANLIST))
	rm -rf $(CLEANLIST)
endif

distclean : clean
ifneq (,$(DISTCLEANLIST))
	rm -rf $(DISTCLEANLIST)
endif
