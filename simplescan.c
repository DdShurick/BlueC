/*Copyright © 2005-2008 Albert Huang. Example 4-1.*/
/* */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

FILE	*fd;

static void usage(void)
{
	int i;

	printf("bt-scan - scan remote bluetooth device\n");
	printf("Usage:\n"
		"\tbt-scan [options]\n");
	printf("Options:\n"
		"\t-h, --help\tDisplay help\n"
		"\t-i, --device dev\tHCI device\n");
	printf("The result is written in /tmp/btscan.lst\n");
}

static struct option main_options[] = {
	{ "help",	0, 0, 'h' },
	{ "version",0, 0, 'v' },
	{ "device",	1, 0, 'i' },
	{ 0, 0, 0, 0 }
};

int main(int argc, char **argv)
{
    inquiry_info *info = NULL;
    int max_rsp, num_rsp;
    int dev_id, ctl, len, flags;
    int i, opt;
    char addr[19] = { 0 };
    char name[248] = { 0 };
    
	while ((opt=getopt_long(argc, argv, "+i:hv", main_options, NULL)) != -1) {
		switch (opt) {
		case 'i':
			dev_id = hci_devid(optarg);
			if (dev_id < 0) {
				perror("Invalid device");
				exit(1);
			}
			break;
		case 'v':
			printf("\tbt-scan version 002.\n");
			exit(0);
		case 'h':
		default:
			usage();
			exit(0);
		}
	}
	
    dev_id = hci_get_route(NULL);
    ctl = hci_open_dev(dev_id);
    if (dev_id < 0 || ctl < 0) {
        perror("opening socket");
        exit(1);
    }
    
	printf("Scanning ...\n");
	
    len  = 8;
    max_rsp = 255;
    flags = IREQ_CACHE_FLUSH;
    info = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));
    
    num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &info, flags);
    if( num_rsp < 0 ) perror("hci_inquiry");
	
	if ((fd = fopen("/tmp/btscan.lst","w"))==NULL) {
		perror("Can't open");
		exit(1);
	}
	
    for (i = 0; i < num_rsp; i++) {
        ba2str(&(info+i)->bdaddr, addr);
        memset(name, 0, sizeof(name));
        if (hci_read_remote_name(ctl, &(info+i)->bdaddr, sizeof(name), name, 0) < 0) strcpy(name, "n/a");
        printf("%s  %s\n", addr, name);
        fprintf(fd,"%s %s\n", addr, name);
    }
	fclose(fd);
    free(info);
    close(ctl);
    return 0;
}

