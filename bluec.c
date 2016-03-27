/*Based on a simple systray applet example by Rodrigo De Castro, 2007
GPL license /usr/share/doc/legal/gpl-2.0.txt.
BlueC GPL v2, DdShutick 27.03.2016 */

//#include <string.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
//#include <glib/gstdio.h>

//#define _(STRING)    gettext(STRING)

GtkStatusIcon *tray_icon;
unsigned int interval = 10000; /*update interval in milliseconds*/
FILE *fp;

GdkPixbuf *bluetooth_pixbuf;
//GdkPixbuf *on_pixbuf;
//GdkPixbuf *off_pixbuf;

gboolean Update(gpointer ptr) {

//    fp = (FILE *)popen("df -m `awk -F '=' '/rw/ {print $1}' /sys/fs/aufs/*/br[0-9]*` | sed -n 2p | tr -s ' ' | cut -f 2,4 -d ' '","r");
//    fgets(meminfo,sizeof meminfo,fp);
//    pclose(fp);

    //update icon... (sizefree,sizetotal are in MB)
    if (gtk_status_icon_get_blinking(tray_icon)==TRUE) gtk_status_icon_set_blinking(tray_icon,FALSE);
/*    percentfree=(sizefree*100)/sizetotal;
    if (sizefree < 20) {
        gtk_status_icon_set_from_pixbuf(tray_icon,critical_pixbuf);
        gtk_status_icon_set_blinking(tray_icon,TRUE);
    }
    else if (sizefree < 50) {
        gtk_status_icon_set_from_pixbuf(tray_icon,critical_pixbuf);
    }
    else if (percentfree < 20) {
        gtk_status_icon_set_from_pixbuf(tray_icon,critical_pixbuf);
    }
    else if (percentfree < 45) {
        gtk_status_icon_set_from_pixbuf(tray_icon,ok_pixbuf);
	}
    else if (percentfree < 70) {
        gtk_status_icon_set_from_pixbuf(tray_icon,good_pixbuf);
	}
    else {*/
    gtk_status_icon_set_from_file(tray_icon, "/usr/share/icons/hicolor/48x48/apps/bluetooth.png");
//    blueman_pixbuf=gdk_pixbuf_new_from_xpm_data((const char**)blueman_xpm);
//    gtk_status_icon_set_from_pixbuf(tray_icon,blueman_pixbuf);
//        gtk_status_icon_set_from_pixbuf(tray_icon,bluetooth_pixbuf);
//	}
	return TRUE;
}

void tray_icon_on_click(GtkStatusIcon *status_icon, gpointer user_data)
{
    //printf("Clicked on tray icon\n");
    //system("partview &"); ###sfs
    system("echo click &");
}

void tray_icon_on_menu(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data)
{
    //printf("Popup menu\n"); ###sfs
    system("echo menu &");
}

static GtkStatusIcon *create_tray_icon() {

    tray_icon = gtk_status_icon_new();
    g_signal_connect(G_OBJECT(tray_icon), "activate", G_CALLBACK(tray_icon_on_click), NULL);
    g_signal_connect(G_OBJECT(tray_icon), "popup-menu", G_CALLBACK(tray_icon_on_menu), NULL);
    gtk_status_icon_set_visible(tray_icon, TRUE);

    return tray_icon;
}

int main(int argc, char **argv) {
    //int len;
    //char strpupmode[8];
    GtkStatusIcon *tray_icon;

//    setlocale( LC_ALL, "" );
//    bindtextdomain( "freememapplet_tray", "/usr/share/locale" );
//    textdomain( "freememapplet_tray" );

    gtk_init(&argc, &argv);
    tray_icon = create_tray_icon();
        
    gtk_timeout_add(interval, Update, NULL);
    Update(NULL);

    gtk_main();

    return 0;
}
