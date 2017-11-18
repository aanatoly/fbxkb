#########################
## Custom Settings     ##
#########################

# Note 1: PWD will be that of configure, not scripts directory, so to run script
# from this directory refer it as 'scripts/name'. For example
#add_var endianess "detect endianess (big or little)" '`scripts/endianess.sh`'

# Note 2: values are evaluated in the same order they were added, so
# if you want libdir's default value to be '$eprefix/lib', add it after prefix

# Usefull stuff
#add_var os "detect OS flavour" '`uname -s | tr [:lower:] [:upper:]`'

# Root File System for embedded or cross-compiler builds
# It's used internaly by add_pkg_vars function. pkg-config, used to get package
# variables, will look for its config files in that directory only.
#add_var rfs "embedded RFS (root file system)" /nfsboot/rfs


add_yesno_var dependency "generate dependency Makefiles" no

add_var xkb_list_file "XKB list file" /usr/share/X11/xkb/rules/base.lst
add_pkg_vars glib glib-2.0
add_pkg_vars gtk  gtk+-2.0 "--atleast-version=2.12"

# These vars are special - their values will be added to CFLAGS and LDFLAGS
add_var cflagsx "C flags" '$glib_cflags $gtk_cflags -fPIC $alsa_cflags'
add_var ldflagsx "linker flags" '$glib_libs $gtk_libs $alsa_libs'

