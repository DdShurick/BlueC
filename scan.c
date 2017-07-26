#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <signal.h>

#include "bluetooth.h"
#include "hci.h"
#include "hci_lib.h"
#include "oui.h"

#define for_each_opt(opt, long, short) while ((opt=getopt_long(argc, argv, short ? short:"+", long, NULL)) != -1)

FILE	*fd;

#ifdef HAVE_UDEV_HWDB_NEW
#include <libudev.h>

char *batocomp(const bdaddr_t *ba)
{
	struct udev *udev;
	struct udev_hwdb *hwdb;
	struct udev_list_entry *head, *entry;
	char modalias[11], *comp = NULL;

	sprintf(modalias, "OUI:%2.2X%2.2X%2.2X", ba->b[5], ba->b[4], ba->b[3]);

	udev = udev_new();
	if (!udev)
		return NULL;

	hwdb = udev_hwdb_new(udev);
	if (!hwdb)
		goto done;

	head = udev_hwdb_get_properties_list_entry(hwdb, modalias, 0);

	udev_list_entry_foreach(entry, head) {
		const char *name = udev_list_entry_get_name(entry);

		if (name && !strcmp(name, "ID_OUI_FROM_DATABASE")) {
			comp = strdup(udev_list_entry_get_value(entry));
			break;
		}
	}

	hwdb = udev_hwdb_unref(hwdb);

done:
	udev = udev_unref(udev);

	return comp;
}
#else
char *batocomp(const bdaddr_t *ba)
{
	return NULL;
}
#endif

/* Device scanning */
/*
static struct option scan_options[] = {
	{ "help",	0, 0, 'h' },
	{ "length",	1, 0, 'l' },
	{ "numrsp",	1, 0, 'n' },
	{ "iac",	1, 0, 'i' },
	{ "flush",	0, 0, 'f' },
	{ "class",	0, 0, 'C' },
	{ "info",	0, 0, 'I' },
	{ "oui",	0, 0, 'O' },
	{ "all",	0, 0, 'A' },
	{ "ext",	0, 0, 'A' },
	{ 0, 0, 0, 0 }
};

static const char *scan_help =
	"Usage:\n"
	"\tscan [--length=N] [--numrsp=N] [--iac=lap] [--flush] [--class] [--info] [--oui] [--refresh]\n";
*/
static void cmd_scan(int dev_id, int argc, char **argv)
{
	inquiry_info *info = NULL;
	uint8_t lap[3] = { 0x33, 0x8b, 0x9e };
	int num_rsp, length, flags;
	uint8_t cls[3], features[8];
	char addr[18], name[249], *comp;
	struct hci_version version;
	struct hci_dev_info di;
	struct hci_conn_info_req *cr;
	int extcls = 0, extinf = 0, extoui = 0;
	int i, n, l, opt, dd, cc;

	length  = 8;	/* ~10 seconds */
	num_rsp = 0;
	flags   = 0;
 
/*	for_each_opt(opt, scan_options, NULL) { 
		switch (opt) {
/*		case 'l':
			length = atoi(optarg);
			break;

		case 'n':
			num_rsp = atoi(optarg);
			break;

		case 'i':
			l = strtoul(optarg, 0, 16);
			if (!strcasecmp(optarg, "giac")) {
				l = 0x9e8b33;
			} else if (!strcasecmp(optarg, "liac")) {
				l = 0x9e8b00;
			} else if (l < 0x9e8b00 || l > 0x9e8b3f) {
				printf("Invalid access code 0x%x\n", l);
				exit(1);
			}
			lap[0] = (l & 0xff);
			lap[1] = (l >> 8) & 0xff;
			lap[2] = (l >> 16) & 0xff;
			break;

		case 'f':
			flags |= IREQ_CACHE_FLUSH;
			break;

		case 'C':
			extcls = 1;
			break;

		case 'I':
			extinf = 1;
			break;

		case 'O':
			extoui = 1;
			break;
*/
/*		case 'A': 
			extcls = 1;
			extinf = 1;
			extoui = 1;
	 		break;

		default:
			printf("%s", scan_help);
			return;
		} 
	}*/
//	helper_arg(0, 0, &argc, &argv, scan_help);

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

	if (extcls || extinf || extoui)
		printf("\n");

	if ((fd = fopen("/tmp/btscan.lst","w"))==NULL) {
		perror("Can't open");
		exit(1);
	}
	for (i = 0; i < num_rsp; i++) {
		uint16_t handle = 0;

		if (!extcls && !extinf && !extoui) {
			ba2str(&(info+i)->bdaddr, addr);

			if (hci_read_remote_name_with_clock_offset(dd,
					&(info+i)->bdaddr,
					(info+i)->pscan_rep_mode,
					(info+i)->clock_offset | 0x8000,
					sizeof(name), name, 100000) < 0)
				strcpy(name, "n/a");

			for (n = 0; n < 248 && name[n]; n++) {
				if ((unsigned char) name[i] < 32 || name[i] == 127)
					name[i] = '.';
			}

			name[248] = '\0';
			fprintf(fd,"%s\t%s\n", addr, name);
			printf("%s\t%s\n", addr, name);
			continue;
		}
	fclose(fd);
	}
}

static void usage(void)
{
	int i;

	printf("bt-scan - part HCI Tool ver 5.46\n");
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
//надо посмотреть	
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
//.	
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

