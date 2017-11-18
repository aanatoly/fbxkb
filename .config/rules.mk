
###############################################
#  environment checks                         #
###############################################

$(TOPDIR)/config.mk:
	@echo Please run $(TOPDIR)/configure
	@echo
	@false
include $(TOPDIR)/config.mk

.DEFAULT_GOAL := all

define prnvar
$(warning $1='$($1)')
endef

ifeq ($(realpath $(TOPDIR)),$(CURDIR))
IS_TOPDIR := yes
else
IS_TOPDIR := no
endif

IS_ALL := $(filter all,$(if $(MAKECMDGOALS),$(MAKECMDGOALS),$(.DEFAULT_GOAL)))

ifeq ($(NV),)
ifeq ($(PROJECT_VERSION),)
PROJECT_VERSION := $(shell [ -f $(TOPDIR)/version ] \
	&& echo `< $(TOPDIR)/version`)
endif
ifeq ($(PROJECT_VERSION),)
PROJECT_VERSION := 1.0
endif
export PROJECT_VERSION
export NV = $(PROJECT_NAME)-$(PROJECT_VERSION)
endif

ifneq ($(IS_ALL),)
all $(SUBDIRS): $(TOPDIR)/version.h
$(TOPDIR)/version.h: $(TOPDIR)/version
	$(call summary,TEXT  ,$@)
	$Qecho > $@
	$Qecho "// Automaticaly generated. Do not edit" >> $@
	$Qecho "#define PROJECT_VERSION \"$(PROJECT_VERSION)\"" >> $@
endif


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


###############################################
#  help                                       #
###############################################

# Help targets allow to shed a light on what your makefile does.
# Text for targets and variables will be formated into nice columns,
# while help for examples will be printed as is.
#
# You can have any number of these, if you use '::' synatx
#
# The syntax is
# help_target ::
# 	echo "name - explanation, every"
# 	echo "    next line is indented"
#
# help_variable ::
# 	echo "name - explanation, every"
# 	echo "    next line is indented"
#
# help_example ::
# 	echo "@@"
# 	echo "Any multi-line text formated as desired"

help :
	$Q$(MAKE) -j1 help_target | $(TOPDIR)/.config/help target
	$Q$(MAKE) -j1 help_variable | $(TOPDIR)/.config/help variable
	$Q$(MAKE) -j1 help_example | $(TOPDIR)/.config/help example

help_target ::
	@echo "help    - print this help"


###############################################
#  output customization                       #
###############################################

help_variable ::
	@echo "V - verbose output, if non-null"

help_example ::
	@echo "@@"
	@echo "Verbose output"
	@echo "  make V=1"
	@echo "  make V=1 clean"
	@echo "  make V=1 tar"

ifeq ($(MAKELEVEL),0)
export STARTDIR:=$(CURDIR)
endif

# make V=1 - very verbose, prints all commands
# make V=0 - prints only titles [default]
ifeq ($V,1)
override Q :=
else
override Q := @
MAKEFLAGS += --no-print-directory
endif
summary = @echo " $(1)" $(subst $(STARTDIR)/,,$(CURDIR)/)$(2)
summary2 = @printf " %-5s %s\n" "$(1)" "$(2)"
export V


###############################################
#  build rules                                #
###############################################

help_target ::
	@echo "all    - build all target"
	@echo "install - install binaries"

help_variable ::
	@echo "DESTDIR - install under this dir rather "
	@echo "          then under /"

help_example ::
	@echo "@@"
	@echo "Installs stuff in a separate dir for easy packaging."
	@echo "  ./configure --prefix=/usr/local"
	@echo "  make"
	@echo "  make install DESTDIR=/tmp/tmp313231"

CFLAGS += -I$(TOPDIR)
define obj_rules
CLEANLIST += $(1).o
$(1).o : $(1).c  $(TOPDIR)/version.h
	$(call summary,CC    ,$$@)
	$Q$(CC) $(CFLAGS) $($(2)_cflags) -c -o $$@ $$<
endef

define bin_rules
all : $(1)
CLEANLIST += $(1)

$(foreach o,$($(1)_src:%.c=%),$(eval $(call obj_rules,$(o),$(1))))

$(1) : $($(1)_src:%.c=%.o)
	$(call summary,LD    ,$$@)
	$Q$(CC) $($(1)_src:%.c=%.o) $(LDFLAGS) $($(1)_libs) -o $$@

ifeq ($($(1)_install),)
install : $(1)_install
$(1)_install :
	$(call summary,INSTALL  ,$$@)
	$Qinstall -m 755 $(1) $(DESTDIR)$(BINDIR)
endif
endef


define lib_rules
all : lib$(1).so
CLEANLIST += lib$(1).so
$(eval $(1)_cflags += -fPIC)
$(foreach o,$($(1)_src:%.c=%),$(eval $(call obj_rules,$(o),$(1))))

lib$(1).so : $($(1)_src:%.c=%.o)
	$(call summary,LD    ,$$@)
	$Q$(CC) $$? $(LDFLAGS) $($(1)_libs) -shared -o $$@

ifeq ($($(1)_install),)
install : $(1)_install
$(1)_install :
	$(call summary,INSTALL  ,$$@)
	$Qinstall -m 755 $(1) $(DESTDIR)$(LIBDIR)
endif
endef

targets = $(filter %_type,$(.VARIABLES))
$(foreach t,$(targets),$(eval $(call $($(t))_rules,$(t:_type=))))


###############################################
#  clean                                      #
###############################################

help_target ::
	@echo "clean - clean build results"
	@echo "distclean - clean build and configure results"

ifeq ($(IS_TOPDIR),yes)
DISTCLEANLIST += config.mk config.h
CLEANLIST += version.h
endif

clean:
ifneq (,$(CLEANLIST))
	$(call summary,CLEAN  ,)
	$Qrm -rf $(CLEANLIST)
endif

distclean : clean
ifneq (,$(DISTCLEANLIST))
	$(call summary,DCLEAN  ,)
	$Qrm -rf $(DISTCLEANLIST)
endif


###############################################
#  tar                                        #
###############################################

ifeq ($(IS_TOPDIR),yes)
help_target ::
	@echo "tar - make tar archive of a project ocde"

tar :
	$Qif read -t 5 -p "Run 'make distclean' first [Y/n] ? " line; then \
		[ "$${line:-y}" == "y" ] && $(MAKE) distclean; \
		true; \
	fi
	$(call summary,TAR   ,$(NV))
	$Q$(TOPDIR)/.config/tar $(NV)
endif


###############################################
#  misc                                       #
###############################################

help_target ::
	@echo "svnignore - tell svn to ignore files in a cleanlist"

svnignore:
	@prop=prop-$$$$.txt; \
	for i in $(DISTCLEANLIST) $(CLEANLIST); do echo "$$i"; done > $$prop;  \
	cat $$prop; \
	svn propset svn:ignore --file $$prop .; \
	rm -f $$prop
