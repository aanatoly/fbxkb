#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <ctype.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"

// #define DEBUG
#include "dbg.h"

typedef struct {
    gchar sym[3];
    gchar* name;
    GdkPixbuf* flag;
} kbd_group_t;

static int hide_default;
static int display_version;
static int print_on;
static char* print_fmt = "%n:%c:%N";
static int icon_size = 20;

static GOptionEntry entries[] = {
    { "version", 0, 0, G_OPTION_ARG_NONE, &display_version, "Display the version and exit", NULL },
    { "hide-default", 0, 0, G_OPTION_ARG_NONE, &hide_default,
     "Hide flag when default keyboard is active", NULL },
    { "print", 'p', 0, G_OPTION_ARG_NONE, &print_on, "Print XKB group switches to stdout", NULL },
    { "fmt", 0, 0, G_OPTION_ARG_STRING, &print_fmt,
     "Format string for XKB switch messages, default is '%n:%c:%N' (%n=number, %c=sym, %N=name)", "FORMAT" },
    { "size", 's', 0, G_OPTION_ARG_INT, &icon_size, "Set the size of flag icons (default 20)", "SIZE" },
    { NULL }
};

static int cur_group;
static int ngroups;
static int xkb_event_type;
static Display* dpy;
static kbd_group_t group[XkbNumKbdGroups];
static GdkPixbuf* default_flag;
static GtkStatusIcon* icon;
static GtkWidget* menu;

static void Xerror_handler(Display* d, XErrorEvent* ev);
static GdkFilterReturn filter(XEvent* xev, GdkEvent* event, gpointer data);
static void print_formatted_switch(int group_num, kbd_group_t* grp, const char* format);

static void menu_about(GtkWidget* widget, gpointer data)
{
    gchar* authors[] = { "Anatoly Asviyan", NULL };
    ENTER;
    gtk_show_about_dialog(NULL, "authors", authors, "comments", "X11 keyboard language switcher",
        "license", "GPLv2", "version", PROJECT_VERSION, "website", "https://github.com/aanatoly/fbxkb",
        "logo-icon-name", "preferences-desktop-keyboard", NULL);
    RET();
}

static void menu_exit(GtkWidget* widget, gpointer data)
{
    ENTER;
    exit(0);
    RET();
}

static void menu_activated(GtkWidget* widget, gpointer data)
{
    ENTER;
    DBG("asking %d group\n", GPOINTER_TO_INT(data));
    XkbLockGroup(dpy, XkbUseCoreKbd, GPOINTER_TO_INT(data));
    RET();
}

static void menu_create()
{
    int i;
    GtkWidget *mi, *img;

    ENTER;
    menu = gtk_menu_new();
    /* flags */
    for (i = 0; i < ngroups; i++) {
        mi = gtk_image_menu_item_new_with_label(group[i].name);
        g_signal_connect(G_OBJECT(mi), "activate", (GCallback)menu_activated, GINT_TO_POINTER(i));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
        gtk_widget_show(mi);
        img = gtk_image_new_from_pixbuf(group[i].flag);
        gtk_widget_show(img);
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(mi), img);
    }
    /* separator */
    mi = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    gtk_widget_show(mi);
    /* about */
    mi = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
    g_signal_connect(G_OBJECT(mi), "activate", (GCallback)menu_about, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    gtk_widget_show(mi);
    /* exit */
    mi = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
    g_signal_connect(G_OBJECT(mi), "activate", (GCallback)menu_exit, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    gtk_widget_show(mi);

    RET();
}

static gboolean clicked(GtkStatusIcon* status_icon, GdkEventButton* event, gpointer data)
{
    ENTER;
    if (event->button == 1) {
        XkbLockGroup(dpy, XkbUseCoreKbd, (cur_group + 1) % ngroups);
    } else {
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL, gtk_status_icon_position_menu, icon,
            event->button, event->time);
    }
    RET(FALSE);
}

static void gui_extra_rebuild()
{
    ENTER;
    if (menu)
        gtk_widget_destroy(menu);
    menu_create();
    RET();
}

static void gui_update()
{
    ENTER;
    DBG("group=%d name=%s flag=%p\n", cur_group, group[cur_group].name, group[cur_group].flag);
    gtk_status_icon_set_from_pixbuf(icon, group[cur_group].flag);
    if (hide_default)
        gtk_status_icon_set_visible(icon, cur_group);
    if (print_on)
        print_formatted_switch(cur_group, &group[cur_group], print_fmt);
    RET();
}

static void gui_create()
{
    ENTER;
    icon = gtk_status_icon_new();
    g_signal_connect(G_OBJECT(icon), "button-press-event", G_CALLBACK(clicked), NULL);
    g_object_set(gtk_settings_get_default(), "gtk-tooltip-timeout", 1500, NULL);
    RET();
}

/* loads flag image for the @country_code */
static GdkPixbuf* get_flag(char* country_code)
{
    char file[] = "tt.png";

    ENTER;
    DBG("country_code=%s\n", country_code);
    if (strlen(country_code) != 2)
        RET(NULL);

    file[0] = country_code[0];
    file[1] = country_code[1];
    GdkPixbuf* original = gdk_pixbuf_new_from_file(file, NULL);
    if (!original)
        RET(NULL);
    int orig_w = gdk_pixbuf_get_width(original);
    int orig_h = gdk_pixbuf_get_height(original);
    int square_size = (orig_w > orig_h) ? orig_w : orig_h;
    GdkPixbuf* square = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, square_size, square_size);
    gdk_pixbuf_fill(square, 0x00000000); // transparent
    int x_off = (square_size - orig_w) / 2;
    int y_off = (square_size - orig_h) / 2;
    gdk_pixbuf_copy_area(original, 0, 0, orig_w, orig_h, square, x_off, y_off);
    g_object_unref(original);
    GdkPixbuf* scaled = gdk_pixbuf_scale_simple(square, icon_size, icon_size, GDK_INTERP_BILINEAR);
    g_object_unref(square);
    RET(scaled);
}

/* looks up corrsct flag image for every language group and replaces
 * default_flag image when it is found.
 * Flag is derived from xkb symbolic name, that looks something like that
 *    pc(pc105)+us+ru(phonetic):2+il(phonetic):3+group(shifts_toggle)+group(switch)
 *
 * Run 'xlsatoms | grep pc' to see your value.
 */
static void get_group_flags(XkbDescRec* kbd_desc_ptr)
{
    char *symbols, *tmp, *tok;
    GdkPixbuf* flag;
    int no;

    ENTER;
    if (XkbGetNames(dpy, XkbSymbolsNameMask, kbd_desc_ptr) != Success) {
        ERR("XkbGetNames failed.\n");
        RET();
    }
    if (kbd_desc_ptr->names->symbols == None
        || (symbols = XGetAtomName(dpy, kbd_desc_ptr->names->symbols)) == NULL) {
        ERR("Can't get group symbol names\n");
        RET();
    }
    DBG("symbols=%s\n", symbols);
    for (tok = strtok(symbols, "+"); tok; tok = strtok(NULL, "+")) {
        DBG("tok=%s\n", tok);

        /* find group symbolic name (like en, us or ru) and group number */
        tmp = strchr(tok, ':');
        if (tmp) {
            if (sscanf(tmp + 1, "%d", &no) != 1) {
                ERR("can't read group number in <%s> token\n", tok);
                goto out;
            }
            no--;
        } else {
            no = 0;
        }
        if (no < 0 || no >= ngroups) {
            ERR("Group number %d is out of range 1..%d in token <%s>\n", no, ngroups, tok);
            goto out;
        }
        for (tmp = tok; isalpha(*tmp); tmp++)
            ;
        *tmp = 0;
        /* if we have flag with same name, then replace default image.
         * otherwise do nothing */
        if ((flag = get_flag(tok))) {
            group[no].flag = flag;
            strncpy(group[no].sym, tok, 2);
            group[no].sym[2] = '\0';
        }
        DBG("sym %s flag %sfound \n", tok, flag ? "" : "NOT ");
    }

out:
    XFree(symbols);
}

static void free_group_info()
{
    int i;

    for (i = 0; i < XkbNumKbdGroups; i++) {
        if (group[i].name)
            XFree(group[i].name);
        if (group[i].flag && group[i].flag != default_flag)
            g_object_unref(group[i].flag);
    }
    bzero(group, sizeof(group));
}

/* gets vital info to switch xkb language groups */
static void get_group_info()
{
    XkbDescRec* kbd_desc_ptr;
    XkbStateRec xkb_state;
    int i;

    ENTER;
    kbd_desc_ptr = XkbAllocKeyboard();
    if (!kbd_desc_ptr) {
        ERR("can't alloc kbd info\n");
        exit(1);
    }
    // kbd_desc_ptr->dpy = GDK_DISPLAY();
    if (XkbGetControls(dpy, XkbAllControlsMask, kbd_desc_ptr) != Success) {
        ERR("can't get Xkb controls\n");
        goto out;
    }
    ngroups = kbd_desc_ptr->ctrls->num_groups;
    if (ngroups < 1) {
        ERR("No keyboard group found\n");
        goto out;
    }
    if (XkbGetState(dpy, XkbUseCoreKbd, &xkb_state) != Success) {
        ERR("can't get Xkb state\n");
        goto out;
    }
    cur_group = xkb_state.group;
    DBG("cur_group = %d ngroups = %d\n", cur_group, ngroups);
    if (XkbGetNames(dpy, XkbGroupNamesMask, kbd_desc_ptr) != Success) {
        ERR("Can't get group names\n");
        goto out;
    }
    for (i = 0; i < ngroups; i++) {
        if (!(group[i].name = XGetAtomName(dpy, kbd_desc_ptr->names->groups[i]))) {
            ERR("Can't get name of group #%d\n", i);
            goto out;
        }
        group[i].flag = default_flag;
        DBG("group[%d].name=%s\n", i, group[i].name);
    }

    get_group_flags(kbd_desc_ptr);

out:
    XkbFreeKeyboard(kbd_desc_ptr, 0, True);
}

static void print_formatted_switch(int group_num, kbd_group_t* grp, const char* format)
{
    ENTER;
    DBG("group: %d, fmt: '%s'\n", group_num, format);
    if (!format)
        return;
    char buffer[1024];
    int buf_pos = 0;
    for (const char* p = format; *p && buf_pos < sizeof(buffer) - 1; p++) {
        if (*p != '%') {
            buffer[buf_pos++] = *p;
            continue;
        }
        p++;
        if (*p == 'n') {
            int len = snprintf(buffer + buf_pos, sizeof(buffer) - buf_pos, "%d", group_num);
            if (len > 0)
                buf_pos += len;
        } else if (*p == 'c') {
            int len = snprintf(buffer + buf_pos, sizeof(buffer) - buf_pos, "%s", grp->sym);
            if (len > 0)
                buf_pos += len;
        } else if (*p == 'N') {
            int len = snprintf(buffer + buf_pos, sizeof(buffer) - buf_pos, "%s", grp->name);
            if (len > 0)
                buf_pos += len;
        } else if (*p == '%') {
            buffer[buf_pos++] = '%';
        } else {
            buffer[buf_pos++] = '%';
            buffer[buf_pos++] = *p;
        }
    }
    buffer[buf_pos] = '\0';
    printf("%s\n", buffer);
}

static GdkFilterReturn filter(XEvent* xev, GdkEvent* event, gpointer data)
{
    XkbEvent* xkbev;

    ENTER;
    if (xev->type != xkb_event_type)
        RET(GDK_FILTER_CONTINUE);

    xkbev = (XkbEvent*)xev;
    DBG("XkbTypeEvent %d \n", xkbev->any.xkb_type);
    if (xkbev->any.xkb_type == XkbStateNotify) {
        DBG("XkbStateNotify: group=%d\n", xkbev->state.group);
        cur_group = xkbev->state.group;
        if (cur_group >= ngroups) {
            ERR("current group is bigger then total group number");
            cur_group = 0;
        }
        gui_update();
    } else if (xkbev->any.xkb_type == XkbNewKeyboardNotify) {
        DBG("XkbNewKeyboardNotify\n");
        free_group_info();
        get_group_info();
        gui_update();
        gui_extra_rebuild();
    }
    RET(GDK_FILTER_REMOVE);
}

void Xerror_handler(Display* d, XErrorEvent* ev)
{
    char buf[256];

    ENTER;
    XGetErrorText(GDK_DISPLAY(), ev->error_code, buf, 256);
    ERR("fbxkb : X error: %s\n", buf);
    RET();
}

static void init()
{
    int dummy;

    ENTER;
    if (!XkbQueryExtension(GDK_DISPLAY(), &dummy, &xkb_event_type, &dummy, &dummy, &dummy)) {
        ERR("no XKB extension\n");
        exit(1);
    }
    XSetLocaleModifiers("");
    XSetErrorHandler((XErrorHandler)Xerror_handler);
    dpy = GDK_DISPLAY();
    if (chdir(DATA_DIR)) {
        ERR("can't chdir to %s\n", DATA_DIR);
        exit(1);
    }
    if (!(default_flag = get_flag("zz"))) {
        ERR("can't load default flag image\n");
        exit(1);
    }
    XkbSelectEventDetails(
        dpy, XkbUseCoreKbd, XkbStateNotify, XkbAllStateComponentsMask, XkbGroupStateMask);
    gdk_window_add_filter(NULL, (GdkFilterFunc)filter, NULL);
    RET();
}

static void create_all()
{
    ENTER;
    get_group_info();
    gui_create();
    gui_update();
    gui_extra_rebuild();
    RET();
}

int main(int argc, char* argv[])
{
    GOptionContext* context;
    GError* error = NULL;

    ENTER;
    gtk_set_locale();
    context = g_option_context_new("- X11 keyboard indicator and switcher");
    g_option_context_add_main_entries(context, entries, NULL);
    g_option_context_add_group(context, gtk_get_option_group(TRUE));
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        g_print("%s\n", error->message);
        exit(1);
    }
    gtk_init(&argc, &argv);
    if (argc > 1) {
        g_print("Unknown option %s.\nRun '%s --help' for description\n", argv[1], g_get_prgname());
        exit(1);
    }
    if (display_version) {
        printf("%s %s\n", g_get_prgname(), PROJECT_VERSION);
        exit(0);
    }
    init();
    create_all();
    gtk_main();
    RET(0);
}
