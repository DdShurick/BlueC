/*Based on a simple systray applet example by Rodrigo De Castro, 2007
GPL license /usr/share/doc/legal/gpl-2.0.txt.
BlueZ_tray GPL v2, DdShutick 07.04.2016 */

#include <string.h>
//#include <libintl.h>
//#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
//#include <gdk/gdkkeysyms.h>
//#include <glib/gstdio.h>
//#include <dirent.h>
//#include <errno.h>

//#define _(STRING)    gettext(STRING)

GtkStatusIcon *tray_icon;
unsigned int interval = 10000; /*update interval in milliseconds*/
FILE *fp;
char statusfile[42], *bldev, status, infomsg[32], cmd[32], label_on[24], label_off[24];

gboolean Update(gpointer ptr) {
//check status bluetooth
	if ((fp = fopen(statusfile,"r"))==NULL) exit(1);
	status = fgetc(fp);
	fclose(fp);
	infomsg[0]=0;
	strcat(infomsg," Bluetooth \n    ");
	strcat(infomsg,bldev);
//update icon...
//    if (gtk_status_icon_get_blinking(tray_icon)==TRUE) gtk_status_icon_set_blinking(tray_icon,FALSE);
	
    if (status=='0') gtk_status_icon_set_from_file(tray_icon,"/usr/share/icons/hicolor/48x48/apps/bluetooth_off.png");
//        gtk_status_icon_set_blinking(tray_icon,TRUE);
    else if (status=='1') gtk_status_icon_set_from_file(tray_icon, "/usr/share/icons/hicolor/48x48/apps/bluetooth.png");
//update tooltip...
	gtk_status_icon_set_tooltip(tray_icon, infomsg);
	return TRUE;
}

//void  view_popup_menu_onInfo (GtkWidget *menuitem, gpointer userdata)
//	{ 
//		system("echo Info");
//    }

//void  view_popup_menu_onFindDevices (GtkWidget *menuitem, gpointer userdata)
//	{
//		system("echo FindDevices");
//	}

//void  view_popup_menu_onPushFile (GtkWidget *menuitem, gpointer userdata)
//	{
//		system("echo PushFile");
//	}

void  view_popup_menu_onAbout (GtkWidget *menuitem, gpointer userdata)
	{
		system("echo \"Bluez-tray-0,1\"");
	}

void  view_popup_menu_onDisconnect (GtkWidget *menuitem, gpointer userdata)
	{ 
		if ((fp = fopen(statusfile,"w"))==NULL) exit(1);
		status = fputc('0',fp);
		fclose(fp);
//		system("/usr/bin/rfkill block bluetooth");
    }

void  view_popup_menu_onReconnect (GtkWidget *menuitem, gpointer userdata)
	{ 
		if ((fp = fopen(statusfile,"w"))==NULL) exit(1);
		status = fputc('1',fp);
		fclose(fp);
//		system("/usr/bin/rfkill unblock bluetooth");
    }

void tray_icon_on_click(GtkStatusIcon *status_icon, gpointer user_data)
{
    system(cmd);
}

void tray_icon_on_menu(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data)
{
	GtkWidget *menu, *menuitem;
    menu = gtk_menu_new();
//    menuitem = gtk_menu_item_new_with_label("Информация");
//    g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_onInfo, status_icon);
//    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
//    menuitem = gtk_menu_item_new_with_label("Поиск устройств");
//    g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_onFindDevices, status_icon);
//    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
//    menuitem = gtk_menu_item_new_with_label("Передать файлы");
//    g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_onPushFile, status_icon);
//    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    menuitem = gtk_menu_item_new_with_label("О программе");
    g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_onAbout, status_icon);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    if (status=='1') {
		menuitem = gtk_menu_item_new_with_label(label_off);
		g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_onDisconnect, status_icon);
    	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	}
    if (status=='0') {
		menuitem = gtk_menu_item_new_with_label(label_on);
		g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_onReconnect, status_icon);
    	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	} 
	gtk_widget_show_all(menu);
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, gdk_event_get_time(NULL));
}

static GtkStatusIcon *create_tray_icon() {

    tray_icon = gtk_status_icon_new();
    g_signal_connect(G_OBJECT(tray_icon), "activate", G_CALLBACK(tray_icon_on_click), NULL);
    g_signal_connect(G_OBJECT(tray_icon), "popup-menu", G_CALLBACK(tray_icon_on_menu), NULL);
    gtk_status_icon_set_visible(tray_icon, TRUE);

    return tray_icon;
}

int main(int argc, char **argv) {
	
	if (argc != 3) { printf ("%s\n","Usage: bluez-tray hci0 rfkill0"); exit(1); }
	
	gtk_init(&argc, &argv);

	statusfile[0]=0;
	strcat(statusfile,"/sys/class/bluetooth/");
	strcat(statusfile,argv[1]);
	strcat(statusfile,"/");
	strcat(statusfile,argv[2]);
	strcat(statusfile,"/state");
	bldev = argv[1];
	cmd[0]=0;
	strcat(cmd,"/usr/bin/bt-connect");
	strcat(cmd," -i ");
	strcat(cmd,argv[2]);
	label_on[0]=0;
	strcat(label_on,"Включить ");
	strcat(label_on,argv[1]);
	label_off[0]=0;
	strcat(label_off,"Отключить ");
	strcat(label_off,argv[1]);
//    GtkStatusIcon *tray_icon;
	
//    setlocale( LC_ALL, "" );
//    bindtextdomain( "bluez_tray", "/usr/share/locale" );
//    textdomain( "bluez_tray" );

    
    tray_icon = create_tray_icon();
      
    gtk_timeout_add(interval, Update, NULL);
    Update(NULL);

    gtk_main();

    return 0;
}
