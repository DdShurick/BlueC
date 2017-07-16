/*Based on a simple systray applet example by Rodrigo De Castro, 2007
GPL license /usr/share/doc/legal/gpl-2.0.txt.
Bluez-tray GPL v2, DdShutick 18.05.2016 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
/*Эти добавить из bluez-5.45/lib/ */
#include "bluetooth.h"
#include "hci.h"
#include "hci_lib.h"

GtkStatusIcon *tray_icon;
unsigned int interval = 1000; /*update interval in milliseconds*/
FILE *fp;
char statefile[42], hardfile[42], softfile[42], *btdev, *st, infomsg[72], cmd[32], label_on[24], label_off[24], scancmd[32], hcicmd[25];
int state, ctl, hdev;
static struct hci_dev_info di;

gboolean Update(gpointer ptr) {
	
	char addr[19];
	bdaddr_t bdaddr;
	
/*check status bluetooth soft blocked*/	
	if ((fp = fopen(statefile,"r"))==NULL) exit(1);
	state = fgetc(fp);
	fclose(fp);
	
/* Infomsg */
/*	if ((fp = fopen(addressfile,"r"))==NULL) exit(1);
	fgets(addr,sizeof(addr),fp);
	fclose(fp);
	
	fp = popen("/usr/bin/hciconfig | /bin/egrep 'UP|DOWN' | tr -d '\t'","r");
	fgets(btupdown,sizeof btupdown,fp);
	pclose(fp);
	infomsg[0]=0;
	strcat(infomsg," Bluetooth: ");
	strcat(infomsg,btdev);
*/	
	
	/* Open HCI socket  
	if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
		perror("Can't open HCI socket.");
		exit(1);
	}*/
	/* Get device info */
	if (ioctl(ctl, HCIGETDEVINFO, (void *) &di)) {
		perror("Can't get device info");
		exit(1);
	}
	ba2str(&di.bdaddr, addr); 
	st = hci_dflagstostr(di.flags);
//	printf(" BTdevice %s:\n%s\n%s\n", di.name, addr, st);
	infomsg[0]=0;
	strcat(infomsg,"  BTdevice:  ");
	strcat(infomsg,di.name);
	
/* update icon...*/
//    if (gtk_status_icon_get_blinking(tray_icon)==TRUE) gtk_status_icon_set_blinking(tray_icon,FALSE);
	
	if (state=='0') {
		gtk_status_icon_set_from_file(tray_icon,"/usr/share/pixmaps/bluetooth_off.png");
/*check status bluetooth
hard blocked*/
		if ((fp = fopen(hardfile,"r"))==NULL) exit(1);
		if (fgetc(fp)=='1') strcat(infomsg,"\n Блоктрован аппаратно \n");
		fclose(fp);
		if ((fp = fopen(softfile,"r"))==NULL) exit(1);
		if (fgetc(fp)=='1') strcat(infomsg,"\n Блоктрован программно \n");
		fclose(fp);
	}
	else if (state=='1') {
    	if (strstr(st,"DOWN")) gtk_status_icon_set_from_file(tray_icon,"/usr/share/pixmaps/bluetooth_off.png");
    	else if (strstr(st,"SCAN")) { 
			gtk_status_icon_set_from_file(tray_icon,"/usr/share/pixmaps/bluetooth_on.png");
			scancmd[14]='\0';
			strcat (scancmd," pscan\n");
//			printf(scancmd);
		}
    	else {
			gtk_status_icon_set_from_file(tray_icon,"/usr/share/pixmaps/bluetooth.png");
			scancmd[14]='\0';
			strcat (scancmd," piscan\n");
//			printf(scancmd);
		}
/*	strcat(infomsg,"\n BD Address:\n ");
	strcat(infomsg,addr);
	strcat(infomsg,btupdown);*/
	strcat(infomsg,"\n");
	strcat(infomsg,addr);
	strcat(infomsg,"\n ");
	strcat(infomsg,st);
//	close(ctl);
	}

//update tooltip...
	gtk_status_icon_set_tooltip(tray_icon,infomsg);
	return TRUE;
}

static void cmd_scan(int ctl, int hdev, char *opt)
{
	struct hci_dev_req dr;

	dr.dev_id  = hdev;
	if (!strcmp(opt, "noscan"))
		dr.dev_opt = SCAN_DISABLED;
	else if (!strcmp(opt, "iscan"))
		dr.dev_opt = SCAN_INQUIRY;
	else if (!strcmp(opt, "pscan"))
		dr.dev_opt = SCAN_PAGE;
	else if (!strcmp(opt, "piscan"))
		dr.dev_opt = SCAN_PAGE | SCAN_INQUIRY;

	if (ioctl(ctl, HCISETSCAN, (unsigned long) &dr) < 0) {
		fprintf(stderr, "Can't set scan mode on hci%d: %s (%d)\n",
						hdev, strerror(errno), errno);
		exit(1);
	}
}

void  view_popup_menu_onIscan (GtkWidget *menuitem, gpointer userdata)
	{
//		system(scancmd); (unsigned long) &dr
		cmd_scan(ctl, hdev, "iscan");
	}

void  view_popup_menu_onPscan (GtkWidget *menuitem, gpointer userdata)
	{
//		system(scancmd); (unsigned long) &dr
		cmd_scan(ctl, hdev, "pscan");
	}

void  view_popup_menu_onPiscan (GtkWidget *menuitem, gpointer userdata)
	{
//		system(scancmd); (unsigned long) &dr
		cmd_scan(ctl, hdev, "piscan");
	}

void  view_popup_menu_onNoscan (GtkWidget *menuitem, gpointer userdata)
	{
//		system(scancmd); (unsigned long) &dr
		cmd_scan(ctl, hdev, "noscan");
	}

void  view_popup_menu_About (GtkWidget *menuitem, gpointer userdata)
	{
//		system("echo \"Bluez-tray-0,1\"");
		GtkWidget *window, *button;

		window = gtk_window_new(GTK_WINDOW_POPUP);
		gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
		gtk_window_set_default_size(GTK_WINDOW(window), 200, 130);
		gtk_container_set_border_width (GTK_CONTAINER(window), 4);
		
		button = gtk_button_new_with_label("\"Bluez-tray-0,2\"\n\n    GPL v2\n\n  DdShurick.");
		g_signal_connect_swapped(G_OBJECT(button),"clicked",G_CALLBACK(gtk_widget_destroy),G_OBJECT(window));
		gtk_container_add(GTK_CONTAINER(window), button);
//		g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_widget_destroy), NULL);
//		GdkColor color;
//    	gdk_color_parse("#00ffff", &color);
//    	gtk_widget_modify_bg(GTK_WIDGET(window), GTK_STATE_NORMAL, &color);
		gtk_widget_show_all(window);

	}

void  view_popup_menu_Disconnect (GtkWidget *menuitem, gpointer userdata)
	{ 
//		system(strcat(hcicmd," down"));
		/* Stop HCI device */
		if (ioctl(ctl, HCIDEVDOWN, hdev) < 0) {
			fprintf(stderr, "Can't down device hci%d: %s (%d)\n",
						hdev, strerror(errno), errno);
			exit(1);
		}
    }

void  view_popup_menu_Connect (GtkWidget *menuitem, gpointer userdata)
	{ 
//		system(strcat(hcicmd," up"));
		/* Unblock if blocked */
		if ((fp = fopen(softfile,"r"))==NULL) exit(1);
		if (fgetc(fp)=='1') system("/usr/sbin/rfkill unblock bluetooth");
		fclose(fp);
		/* Start HCI device */
		if (ioctl(ctl, HCIDEVUP, hdev) < 0) {
			if (errno == EALREADY)
			return;
			fprintf(stderr, "Can't init device hci%d: %s (%d)\n",
						hdev, strerror(errno), errno);
			exit(1);
		}
    }

void tray_icon_on_click(GtkStatusIcon *status_icon, gpointer user_data)
{
    if (system("urxvt -e /usr/bin/bluetoothctl &")) system("/usr/bin/puppybt");
}

void tray_icon_on_menu(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data)
{
	GtkWidget *menu, *menuitem;
    menu = gtk_menu_new();
/*    menuitem = gtk_menu_item_new_with_label("Информация");
      g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_onInfo, status_icon);
//    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
//    menuitem = gtk_menu_item_new_with_label("Поиск устройств");
//    g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_onFindDevices, status_icon);
//    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
//    menuitem = gtk_menu_item_new_with_label("Передать файлы");
//    g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_onPushFile, status_icon);
//    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem); */
    menuitem = gtk_menu_item_new_with_label("О программе");
    g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_About, status_icon);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    if (strstr(st,"UP")) {
		menuitem = gtk_menu_item_new_with_label(label_off);
		g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_Disconnect, status_icon);
    	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    	if (strstr(st,"SCAN")) {
			menuitem = gtk_menu_item_new_with_label("Отключить видимость");
			g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_onNoscan, status_icon);
    		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
		}
		else {
			menuitem = gtk_menu_item_new_with_label("Включить piscan");
			g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_onPiscan, status_icon);
    		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    		menuitem = gtk_menu_item_new_with_label("Включить pscan");
			g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_onPscan, status_icon);
    		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    		menuitem = gtk_menu_item_new_with_label("Включить iscan");
			g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_onIscan, status_icon);
    		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
		}
	} 
    if (strstr(st,"DOWN")) {
		menuitem = gtk_menu_item_new_with_label(label_on);
		g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_Connect, status_icon);
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
	btdev = argv[1];
	hdev = atoi(btdev + 3);
	
	hardfile[0]=0;
	strcat(hardfile,"/sys/class/bluetooth/");
	strcat(hardfile,argv[1]);
	strcat(hardfile,"/");
	strcat(hardfile,argv[2]);
	strcat(hardfile,"/hard");
	
	softfile[0]=0;
	strcat(softfile,"/sys/class/bluetooth/");
	strcat(softfile,argv[1]);
	strcat(softfile,"/");
	strcat(softfile,argv[2]);
	strcat(softfile,"/soft");

	statefile[0]=0;
	strcat(statefile,"/sys/class/bluetooth/");
	strcat(statefile,argv[1]);
	strcat(statefile,"/");
	strcat(statefile,argv[2]);
	strcat(statefile,"/state");
/*
	addressfile[0]=0;
	strcat(addressfile,"/sys/class/bluetooth/");
	strcat(addressfile,argv[1]);
	strcat(addressfile,"/address");
*/
	cmd[0]=0;
	strcat(cmd,"/usr/bin/bt-connect ");
	strcat(cmd,argv[1]);
	
	hcicmd[0]=0;
	strcat(hcicmd,"/usr/bin/hciconfig ");
	strcat(hcicmd,argv[1]);
	
	scancmd[0]=0;
	strcat(scancmd,"/usr/bin/hciconfig ");
	strcat(scancmd,argv[1]);

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
/* Open HCI socket  */
	if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
		perror("Can't open HCI socket.");
		exit(1);
	}
    
    tray_icon = create_tray_icon();
      
    gtk_timeout_add(interval, Update, NULL);
    Update(NULL);

    gtk_main();
	close(ctl);
    return 0;
}
