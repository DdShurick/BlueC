/*Based on a simple systray applet example by Rodrigo De Castro, 2007
GPL license /usr/share/doc/legal/gpl-2.0.txt.
Bluez-tray GPL v2, DdShutick 18.05.2016 */

#include <string.h>
#include <stdio.h>
#include <locale.h> 
#include <libintl.h> 
#include <stdlib.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
/* bluez-5.50_DEV */
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#define _(STRING)    gettext(STRING)

GtkStatusIcon *tray_icon;
unsigned int interval = 1000; /*update interval in milliseconds*/
FILE *fp;
char statefile[42], hardfile[42], softfile[42], *btdev, *st, infomsg[72], cmd[33], label_on[24], label_off[24];
int state, ctl, dev_id, i = 0;
static struct hci_dev_info di;

gboolean Update(gpointer ptr) {
	
	char addr[18], *name;
	bdaddr_t bdaddr;
	
/*check status bluetooth soft blocked*/	
	
	if ((fp = fopen(statefile,"r"))==NULL) {
		perror("Can't open statefile");
		exit(1);
	}
	state = fgetc(fp);
	fclose(fp);
	
/* Infomsg */

	/* Get device info */
	di.dev_id = dev_id; //??
	if (ioctl(ctl, HCIGETDEVINFO, (void *) &di)) {
		perror("Can't get device info");
		exit(1);
	}
	ba2str(&di.bdaddr, addr); //
	st = hci_dflagstostr(di.flags);
	infomsg[0]=0;
	strcat(infomsg,"  BTdevice:  ");
	strcat(infomsg,di.name);
	
/* update icon...*/
/* -> */
	if (gtk_status_icon_get_blinking(tray_icon)==TRUE) {
		
		exit(1);
		} /* <- */
	if (state=='0') {
		gtk_status_icon_set_from_file(tray_icon,"/usr/share/pixmaps/bluetooth_off.png");
		
/*check status bluetooth blocked*/

		if ((fp = fopen(hardfile,"r"))==NULL) {
			perror("Can't open hardfile");
			exit(1);
		}
		if (fgetc(fp)=='1') strcat(infomsg,_("\n Locked hardware \n")); //"\n Блокирован аппаратно \n"
		fclose(fp);
		if ((fp = fopen(softfile,"r"))==NULL) {
			perror("Can't open softfile");
			exit(1);
		}
		if (fgetc(fp)=='1') strcat(infomsg,_("\n Blocked software \n")); //"\n Блокирован программно \n"
		fclose(fp);
	}
	else if (state=='1') {
    	if (strstr(st,"DOWN")) gtk_status_icon_set_from_file(tray_icon,"/usr/share/pixmaps/bluetooth_off.png");
    	else if (strstr(st,"SCAN")) { 
			gtk_status_icon_set_from_file(tray_icon,"/usr/share/pixmaps/bluetooth_on.png");
		}
    	else {
			gtk_status_icon_set_from_file(tray_icon,"/usr/share/pixmaps/bluetooth.png");
		}

	strcat(infomsg,"\n ");
	strcat(infomsg,addr);
	strcat(infomsg," \n ");
	strcat(infomsg,st);
	
	}

//update tooltip...
	gtk_status_icon_set_tooltip(tray_icon,infomsg);
	return TRUE;
}

static void cmd_scan(int ctl, int dev_id, char *opt)
{
	struct hci_dev_req dr;

	dr.dev_id  = dev_id;
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
						dev_id, strerror(errno), errno);
		exit(1);
	}
}

void  view_popup_menu_onIscan (GtkWidget *menuitem, gpointer userdata)
	{
		cmd_scan(ctl, dev_id, "iscan");
	}

void  view_popup_menu_onPscan (GtkWidget *menuitem, gpointer userdata)
	{
		cmd_scan(ctl, dev_id, "pscan");
	}

void  view_popup_menu_onPiscan (GtkWidget *menuitem, gpointer userdata)
	{
		cmd_scan(ctl, dev_id, "piscan");
	}

void  view_popup_menu_onNoscan (GtkWidget *menuitem, gpointer userdata)
	{
		cmd_scan(ctl, dev_id, "noscan");
	}

void  view_popup_menu_About (GtkWidget *menuitem, gpointer userdata)
	{
	
		GtkWidget *window, *button;

		window = gtk_window_new(GTK_WINDOW_POPUP);
		gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
		gtk_window_set_default_size(GTK_WINDOW(window), 200, 130);
		gtk_container_set_border_width (GTK_CONTAINER(window), 4);
		
		button = gtk_button_new_with_label("\t \"Bluez-tray-0.3\" \n\nGUI for local bluetooth interface\n\n\tDdShurick, GPL v2 ");
		g_signal_connect_swapped(G_OBJECT(button),"clicked",G_CALLBACK(gtk_widget_destroy),G_OBJECT(window));
		gtk_container_add(GTK_CONTAINER(window), button);
/*		g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_widget_destroy), NULL);
		GdkColor color;
    	gdk_color_parse("#00ffff", &color);
    	gtk_widget_modify_bg(GTK_WIDGET(window), GTK_STATE_NORMAL, &color);*/
		gtk_widget_show_all(window);

	}

void  view_popup_menu_Disconnect (GtkWidget *menuitem, gpointer userdata)
	{ 
/* Stop HCI device */
		if (ioctl(ctl, HCIDEVDOWN, dev_id) < 0) {
			fprintf(stderr, "Can't down device hci%d: %s (%d)\n",
						dev_id, strerror(errno), errno);
			exit(1);
		}
    }

void btdevup() {
/* Start HCI device */
	if (ioctl(ctl, HCIDEVUP, dev_id) < 0) {
		if (errno == EALREADY)
		return;
		gtk_status_icon_set_blinking(tray_icon,TRUE); /* */
		fprintf(stderr, "Can't init device hci%d: %s (%d)\n", dev_id, strerror(errno), errno);
	}
}

void  view_popup_menu_Connect (GtkWidget *menuitem, gpointer userdata)
{ 
/* Unblock if blocked */
	if ((fp = fopen(softfile,"r"))==NULL) {
		perror("Can't open softfile");
		exit(1);
	}
	if (fgetc(fp)=='1') system("/usr/sbin/rfkill unblock bluetooth"); /* */
	fclose(fp);
	btdevup();
}

void tray_icon_on_click(GtkStatusIcon *status_icon, gpointer user_data)
{	
	if (strstr(st,"DOWN")) btdevup();
	if (gtk_status_icon_get_blinking(tray_icon)==FALSE) {
		if ((system("/usr/local/bin/defaultbtmanager &")) == 0) system(cmd);
	}
}

void tray_icon_on_menu(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data)
{
	GtkWidget *menu, *menuitem;
    menu = gtk_menu_new();
    menuitem = gtk_menu_item_new_with_label(_("About program")); //"О программе"
    g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_About, status_icon);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    if (strstr(st,"UP")) {
    	if (strstr(st,"SCAN")) {
			menuitem = gtk_menu_item_new_with_label(_("Disable visibility")); //"Отключить видимость"
			g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_onNoscan, status_icon);
    		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
		}
		else {
			menuitem = gtk_menu_item_new_with_label(_("Scan on piscan")); //"Включить piscan"
			g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_onPiscan, status_icon);
    		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    		menuitem = gtk_menu_item_new_with_label(_("Scan on pscan")); //Включить pscan"
			g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_onPscan, status_icon);
    		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    		menuitem = gtk_menu_item_new_with_label(_("Scan on iscan")); //"Включить iscan"
			g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_onIscan, status_icon);
    		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
		}
		menuitem = gtk_menu_item_new_with_label(label_off);
		g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_Disconnect, status_icon);
    	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	} 
    if (strstr(st,"DOWN")) {
		menuitem = gtk_menu_item_new_with_label(label_on);
		g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_Connect, status_icon);
    	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	}
	
	gtk_widget_show_all(menu);
//    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, gdk_event_get_time(NULL));
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button,activate_time);
}

static GtkStatusIcon *create_tray_icon() {

    tray_icon = gtk_status_icon_new();
    gtk_status_icon_set_visible(tray_icon, TRUE);
    g_signal_connect(G_OBJECT(tray_icon), "activate", G_CALLBACK(tray_icon_on_click), NULL);
    g_signal_connect(G_OBJECT(tray_icon), "popup-menu", G_CALLBACK(tray_icon_on_menu), NULL);
    
    return tray_icon;
}

int main(int argc, char **argv) {
	 
	if (argc != 3) {
		printf ("%s\n","Usage: bluez-tray hci0 rfkill0\n\tor bluez-tray hci0 up");
		exit(1);
	}
	gtk_init(&argc, &argv);
	btdev = argv[1];
	dev_id = atoi(btdev + 3);
	
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
	
	cmd[0]=0;
	strcat(cmd,"/usr/bin/puppybt ");
	strcat(cmd,argv[1]);
	strcat(cmd," ");
	strcat(cmd,argv[2]);
	strcat(cmd," &");
	
	setlocale( LC_ALL, "" );
	bindtextdomain( "bluez-tray", "/usr/share/locale" );
	textdomain( "bluez-tray" );
	
	label_on[0]=0;
	strcat(label_on,_("Power on ")); //"Включить "
	strcat(label_on,argv[1]);

	label_off[0]=0;
	strcat(label_off,_("Power off ")); //"Отключить "
	strcat(label_off,argv[1]);
	
/* Open HCI socket  */
	if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
		perror("Can't open HCI socket.");
		exit(1);
	}
    if (strcmp(argv[2],"up") == 0) { btdevup(); exit(0); }
    
    tray_icon = create_tray_icon();
      
    gtk_timeout_add(interval, Update, NULL);
    Update(NULL);

    gtk_main();
	close(ctl);
    return 0;
}
