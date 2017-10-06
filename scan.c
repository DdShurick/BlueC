/*The part of hcitool-5.46*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

FILE	*fd;

#define ENT(e) (sizeof(e)/sizeof(char*))
static char *majors[] = {"Misc", "Computer", "Phone", "Net Access", "Audio/Video",\
                        "Peripheral", "Imaging", "Wearable", "Toy"};

static char* computers[] = {"Misc", "Desktop", "Server", "Laptop", "Handheld",\
                                "Palm", "Wearable"};

static char* phones[] = {"Misc", "Cell", "Cordless", "Smart", "Wired",\
                        "ISDN", "Sim Card Reader For "};

static char* av[] = {"Misc", "Headset", "Handsfree", "Reserved", "Microphone", "Loudspeaker",\
                "Headphones", "Portable Audio", "Car Audio", "Set-Top Box",\
                "HiFi Audio", "Video Tape Recorder", "Video Camera",\
                "Camcorder", "Video Display and Loudspeaker", "Video Conferencing", "Reserved",\
                "Game / Toy"};

static char* peripheral[] = {"Misc", "Joystick", "Gamepad", "Remote control", "Sensing device",\
                "Digitiser Tablet", "Card Reader"};

static char* wearable[] = {"Misc", "Wrist Watch", "Pager", "Jacket", "Helmet", "Glasses"};

static char* toys[] = {"Misc", "Robot", "Vehicle", "Character", "Controller", "Game"};
/* end of device descriptions */

/* Decode device class */
 
static void classinfo(uint8_t dev_class[3]) {
	int flags = dev_class[2];
	int major = dev_class[1];
	int minor = dev_class[0] >> 2;

	printf("[ ");
	if (flags & 0x1) printf("Position ");
	if (flags & 0x2) printf("Net ");
	if (flags & 0x4) printf("Render ");
	if (flags & 0x8) printf("Capture ");
	if (flags & 0x10) printf("Obex ");
	if (flags & 0x20) printf("Audio ");
	if (flags & 0x40) printf("Phone ");
	if (flags & 0x80) printf("Info ");
	printf("]");

	if (major > ENT(majors)) {
		if (major == 63)
			printf(" Unclassified device\n");
		return;
	}
	switch (major) {
	case 1:
		if (minor <= ENT(computers)) printf(" %s", computers[minor]);
		break;
	case 2:
		if (minor <= ENT(phones)) printf(" %s", phones[minor]);
		break;
	case 3:
		printf(" Usage %d/56", minor);
		break;
	case 4:
		if (minor <= ENT(av)) printf(" %s", av[minor]);
		break;
	case 5:
		if ((minor & 0xF) <= ENT(peripheral)) printf(" %s", peripheral[(minor & 0xF)]);
		if (minor & 0x10) printf(" with keyboard");
		if (minor & 0x20) printf(" with pointing device");
		break;
	case 6:
		if (minor & 0x2) printf(" with display");
		if (minor & 0x4) printf(" with camera");
		if (minor & 0x8) printf(" with scanner");
		if (minor & 0x10) printf(" with printer");
		break;
	case 7:
		if (minor <= ENT(wearable)) printf(" %s", wearable[minor]);
		break;
	case 8:
		if (minor <= ENT(toys)) printf(" %s", toys[minor]);
		break;
	}
	printf(" %s\n", majors[major]);
}

static void cmd_scan(int dev_id, int argc, char **argv)
{
	inquiry_info *info = NULL;
	uint8_t lap[3] = { 0x33, 0x8b, 0x9e };
	int num_rsp, length, flags;
//	uint8_t cls[3], features[8];
	char addr[18], name[249]; //, *comp;
	struct hci_dev_info di;
/*	struct hci_conn_info_req *cr; */
	int i, n, opt, dd; // l, cc;

	length  = 8;	/* ~10 seconds */
	num_rsp = 0;
	flags   = 0;

	if (dev_id < 0) {
		dev_id = hci_get_route(NULL);
		if (dev_id < 0) {
			perror("Device is not available");
			exit(1);
		}
	}

	if (hci_devinfo(dev_id, &di) < 0) {
		perror("Can't get device info");
		exit(1);
	}

	printf("Scanning ...\n");
	num_rsp = hci_inquiry(dev_id, length, num_rsp, lap, &info, flags);
	if (num_rsp < 0) {
		perror("Inquiry failed");
		exit(1);
	}

	dd = hci_open_dev(dev_id);
	if (dd < 0) {
		perror("HCI device open failed");
		free(info);
		exit(1);
	}

	if ((fd = fopen("/tmp/btscan.lst","w"))==NULL) {
		perror("Can't open");
		exit(1);
	}
	for (i = 0; i < num_rsp; i++) {
		uint16_t handle = 0;

		ba2str(&(info+i)->bdaddr, addr);

		if (hci_read_remote_name_with_clock_offset(dd,
			&(info+i)->bdaddr,
			(info+i)->pscan_rep_mode,
			(info+i)->clock_offset | 0x8000,
			sizeof(name), name, 100000) < 0)
			strcpy(name, "n/a");

		for (n = 0; n < 248 && name[n]; n++) {
			if ((unsigned char) name[i] < 32 || name[i] == 127) name[i] = '.';
		}

		name[248] = '\0';
		fprintf(fd,"%s %s\n", addr, name);
		printf("%s %s\n", addr, name);
		printf("%x %x %x %x\n",(info+i)->pscan_rep_mode,(info+i)->pscan_mode,(info+i)->pscan_period_mode);
		classinfo((info+i)->dev_class);
		continue;
	}
	fclose(fd);
}

static void usage(void)
{
	printf("bt-scan - scan remote bluetooth device\n");
	printf("this is a part HCI Tool ver 5.46\n");
	printf("Usage:\n"
		"\tbt-scan [options]\n");
	printf("Options:\n"
		"\t-h, --help\tDisplay help\n"
		"\t-i, --device dev\tHCI device\n");
	printf("The result is written in /tmp/btscan.lst");
}

static struct option main_options[] = {
	{ "help",	0, 0, 'h' },
	{ "device",	1, 0, 'i' },
	{ 0, 0, 0, 0 }
};

int main(int argc, char *argv[]) {
	
	int ctl, opt, i, dev_id = -1;
	static struct hci_dev_info di;

	while ((opt=getopt_long(argc, argv, "+i:h", main_options, NULL)) != -1) {
		switch (opt) {
		case 'i':
			dev_id = hci_devid(optarg);
			if (dev_id < 0) {
				perror("Invalid device");
				exit(1);
			}
			break;

		case 'h':
		default:
			usage();
			exit(0);
		}
	}
	
	if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
		perror("Can't open HCI socket.");
		exit(1);
	}
	
	if (ioctl(ctl, HCIGETDEVINFO, (void *) &di) < 0) {
		perror("Can't get device info");
		exit(1);
	}
	
	cmd_scan(di.dev_id , 0, NULL);
	close(ctl);
	return 0;
}

