
# Initialization. Create your vars here. Runs before command line processing.
def init():
    # standart autoconf options
    var_group_new('autoconf', 'Standart autoconf options')

    var_new("prefix", help = "install architecture-independent files",
        metavar='DIR', default = lambda : '/usr',
        group = 'autoconf')
    var_new("eprefix", help = "install architecture-dependent files",
        metavar='DIR', default = lambda : var('prefix'),
        group = 'autoconf')
    var_new("bindir", help = "user executables",
        metavar='DIR', default = lambda : var('eprefix') + '/bin',
        group = 'autoconf')
    var_new("sbindir", help = "system executables",
        metavar='DIR', default = lambda : var('eprefix') + '/sbin',
        group = 'autoconf')
    var_new("libexecdir", help = "program executables",
        metavar='DIR', default = lambda : var('eprefix') + '/libexec',
        group = 'autoconf')
    var_new("libdir", help = "object code libraries",
        metavar='DIR', default = lambda : var('eprefix') + '/lib',
        group = 'autoconf')
    var_new("sysconfdir", help = "read-only single-machine data",
        metavar='DIR', default = lambda : var('prefix') + '/etc',
        group = 'autoconf')
    var_new("datadir", help = "read-only architecture-independent data",
        metavar='DIR', default = lambda : var('prefix') + '/share',
        group = 'autoconf')
    var_new("localedir", help = "languagetranslation files",
        metavar='DIR', default = lambda : var('datadir') + '/locale',
        group = 'autoconf')
    var_new("includedir", help = "C header files",
        metavar='DIR', default = lambda : var('prefix') + '/include',
        group = 'autoconf')
    var_new("mandir", help = "man documentation",
        metavar='DIR', default = lambda : var('prefix') + '/man',
        group = 'autoconf')
    var_new("infodir", help = "info documentation",
        metavar='DIR', default = lambda : var('prefix') + '/info',
        group = 'autoconf')
    var_new("localstatedir", help = "modifiable single-machine data in DIR",
        metavar='DIR', default = lambda : var('prefix') + '/var',
        group = 'autoconf')

    var_new("project_name", help = "Project name",
        metavar='NAME', default = lambda : detect_project_name())
    # var_new("volume", help = "Enable volume plugin",
    #     action = ToggleAction, default = False)


# Logic and stuff that does not require human interaction. Runs after
# command line processing.
# Here you can do anything you want. You can create vars, delete vars, modify
# their values. Just bear in mind: whatever you do here will end up in
# config.h.
def resolve():

    # if var('volume'):
    #     var_new_from_pkg('alsa', 'alsa', '--atleast-version=1.0.10')

    var_new_from_pkg('gtk2', 'gtk+-2.0')

def detect_project_name():
    # Hardcode here the name of your project
    # ret = "projectname"

    # Alternatively, take top dir name as a project name
    ret = os.getcwd().split('/')[-1]

    return ret

