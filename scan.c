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

static char *get_minor_device_name(int major, int minor)
{
	switch (major) {
	case 0:	/* misc */
		return "";
	case 1:	/* computer */
		switch (minor) {
		case 0:
			return "Uncategorized";
		case 1:
			return "Desktop workstation";
		case 2:
			return "Server";
		case 3:
			return "Laptop";
		case 4:
			return "Handheld";
		case 5:
			return "Palm";
		case 6:
			return "Wearable";
		}
		break;
	case 2:	/* phone */
		switch (minor) {
		case 0:
			return "Uncategorized";
		case 1:
			return "Cellular";
		case 2:
			return "Cordless";
		case 3:
			return "Smart phone";
		case 4:
			return "Wired modem or voice gateway";
		case 5:
			return "Common ISDN Access";
		case 6:
			return "Sim Card Reader";
		}
		break;
	case 3:	/* lan access */
		if (minor == 0)
			return "Uncategorized";
		switch (minor / 8) {
		case 0:
			return "Fully available";
		case 1:
			return "1-17% utilized";
		case 2:
			return "17-33% utilized";
		case 3:
			return "33-50% utilized";
		case 4:
			return "50-67% utilized";
		case 5:
			return "67-83% utilized";
		case 6:
			return "83-99% utilized";
		case 7:
			return "No service available";
		}
		break;
	case 4:	/* audio/video */
		switch (minor) {
		case 0:
			return "Uncategorized";
		case 1:
			return "Device conforms to the Headset profile";
		case 2:
			return "Hands-free";
			/* 3 is reserved */
		case 4:
			return "Microphone";
		case 5:
			return "Loudspeaker";
		case 6:
			return "Headphones";
		case 7:
			return "Portable Audio";
		case 8:
			return "Car Audio";
		case 9:
			return "Set-top box";
		case 10:
			return "HiFi Audio Device";
		case 11:
			return "VCR";
		case 12:
			return "Video Camera";
		case 13:
			return "Camcorder";
		case 14:
			return "Video Monitor";
		case 15:
			return "Video Display and Loudspeaker";
		case 16:
			return "Video Conferencing";
			/* 17 is reserved */
		case 18:
			return "Gaming/Toy";
		}
		break;
	case 5:	/* peripheral */ {
		static char cls_str[48]; cls_str[0] = 0;

		switch (minor & 48) {
		case 16:
			strncpy(cls_str, "Keyboard", sizeof(cls_str));
			break;
		case 32:
			strncpy(cls_str, "Pointing device", sizeof(cls_str));
			break;
		case 48:
			strncpy(cls_str, "Combo keyboard/pointing device", sizeof(cls_str));
			break;
		}
		if ((minor & 15) && (strlen(cls_str) > 0))
			strcat(cls_str, "/");

		switch (minor & 15) {
		case 0:
			break;
		case 1:
			strncat(cls_str, "Joystick",
					sizeof(cls_str) - strlen(cls_str) - 1);
			break;
		case 2:
			strncat(cls_str, "Gamepad",
					sizeof(cls_str) - strlen(cls_str) - 1);
			break;
		case 3:
			strncat(cls_str, "Remote control",
					sizeof(cls_str) - strlen(cls_str) - 1);
			break;
		case 4:
			strncat(cls_str, "Sensing device",
					sizeof(cls_str) - strlen(cls_str) - 1);
			break;
		case 5:
			strncat(cls_str, "Digitizer tablet",
					sizeof(cls_str) - strlen(cls_str) - 1);
			break;
		case 6:
			strncat(cls_str, "Card reader",
					sizeof(cls_str) - strlen(cls_str) - 1);
			break;
		default:
			strncat(cls_str, "(reserved)",
					sizeof(cls_str) - strlen(cls_str) - 1);
			break;
		}
		if (strlen(cls_str) > 0)
			return cls_str;
		break;
	}
	case 6:	/* imaging */
		if (minor & 4)
			return "Display";
		if (minor & 8)
			return "Camera";
		if (minor & 16)
			return "Scanner";
		if (minor & 32)
			return "Printer";
		break;
	case 7: /* wearable */
		switch (minor) {
		case 1:
			return "Wrist Watch";
		case 2:
			return "Pager";
		case 3:
			return "Jacket";
		case 4:
			return "Helmet";
		case 5:
			return "Glasses";
		}
		break;
	case 8: /* toy */
		switch (minor) {
		case 1:
			return "Robot";
		case 2:
			return "Vehicle";
		case 3:
			return "Doll / Action Figure";
		case 4:
			return "Controller";
		case 5:
			return "Game";
		}
		break;
	case 63:	/* uncategorised */
		return "";
	}
	return "Unknown (reserved) minor device class";
}

static char *major_classes[] = {
	"Miscellaneous", "Computer", "Phone", "LAN Access",
	"Audio/Video", "Peripheral", "Imaging", "Uncategorized"
};

/* Device scanning */

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

	for_each_opt(opt, scan_options, NULL) {
		switch (opt) {
		case 'l':
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

		case 'A':
			extcls = 1;
			extinf = 1;
			extoui = 1;
			break;

		default:
			printf("%s", scan_help);
			return;
		}
	}
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

			printf("\t%s\t%s\n", addr, name);
			continue;
		}

		ba2str(&(info+i)->bdaddr, addr);
		printf("BD Address:\t%s [mode %d, clkoffset 0x%4.4x]\n", addr,
			(info+i)->pscan_rep_mode, btohs((info+i)->clock_offset));

		if (extoui) {
			comp = batocomp(&(info+i)->bdaddr);
			if (comp) {
				char oui[9];
				ba2oui(&(info+i)->bdaddr, oui);
				printf("OUI company:\t%s (%s)\n", comp, oui);
				free(comp);
			}
		}

		cc = 0;

		if (extinf) {
			cr = malloc(sizeof(*cr) + sizeof(struct hci_conn_info));
			if (cr) {
				bacpy(&cr->bdaddr, &(info+i)->bdaddr);
				cr->type = ACL_LINK;
				if (ioctl(dd, HCIGETCONNINFO, (unsigned long) cr) < 0) {
					handle = 0;
					cc = 1;
				} else {
					handle = htobs(cr->conn_info->handle);
					cc = 0;
				}
				free(cr);
			}

			if (cc) {
				if (hci_create_connection(dd, &(info+i)->bdaddr,
						htobs(di.pkt_type & ACL_PTYPE_MASK),
						(info+i)->clock_offset | 0x8000,
						0x01, &handle, 25000) < 0) {
					handle = 0;
					cc = 0;
				}
			}
		}

		if (hci_read_remote_name_with_clock_offset(dd,
					&(info+i)->bdaddr,
					(info+i)->pscan_rep_mode,
					(info+i)->clock_offset | 0x8000,
					sizeof(name), name, 100000) < 0) {
		} else {
			for (n = 0; n < 248 && name[n]; n++) {
				if ((unsigned char) name[i] < 32 || name[i] == 127)
					name[i] = '.';
			}

			name[248] = '\0';
		}

		if (strlen(name) > 0)
			printf("Device name:\t%s\n", name);

		if (extcls) {
			memcpy(cls, (info+i)->dev_class, 3);
			printf("Device class:\t");
			if ((cls[1] & 0x1f) > sizeof(major_classes) / sizeof(char *))
				printf("Invalid");
			else
				printf("%s, %s", major_classes[cls[1] & 0x1f],
					get_minor_device_name(cls[1] & 0x1f, cls[0] >> 2));
			printf(" (0x%2.2x%2.2x%2.2x)\n", cls[2], cls[1], cls[0]);
		}

		if (extinf && handle > 0) {
			if (hci_read_remote_version(dd, handle, &version, 20000) == 0) {
				char *ver = lmp_vertostr(version.lmp_ver);
				printf("Manufacturer:\t%s (%d)\n",
					bt_compidtostr(version.manufacturer),
					version.manufacturer);
				printf("LMP version:\t%s (0x%x) [subver 0x%x]\n",
					ver ? ver : "n/a",
					version.lmp_ver, version.lmp_subver);
				if (ver)
					bt_free(ver);
			}

			if (hci_read_remote_features(dd, handle, features, 20000) == 0) {
				char *tmp = lmp_featurestostr(features, "\t\t", 63);
				printf("LMP features:\t0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x"
					" 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x\n",
					features[0], features[1],
					features[2], features[3],
					features[4], features[5],
					features[6], features[7]);
				printf("%s\n", tmp);
				bt_free(tmp);
			}

			if (cc) {
				usleep(10000);
				hci_disconnect(dd, handle, HCI_OE_USER_ENDED_CONNECTION, 10000);
			}
		}

		printf("\n");
	}
}

int main(int argc, char *argv[]) {
	
	int ctl;
	static struct hci_dev_info di;
	
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

