/* * * The part of sdptool-5.50 * * */
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include "bluetooth/sdp.h"
#include "bluetooth/sdp_lib.h"
/* 
#ifndef APPLE_AGENT_SVCLASS_ID
#define APPLE_AGENT_SVCLASS_ID 0x2112
#endif
*/
FILE	*fi;
#define for_each_opt(opt, long, short) while ((opt=getopt_long(argc, argv, short ? short:"+", long, 0)) != -1)
#define N_ELEMENTS(x) (sizeof(x) / sizeof((x)[0]))
#define DEFAULT_VIEW 0	/* Display only known attribute */

/* Pass args to the inquiry/search handler */
struct search_context {
	char		*svc;		/* Service */
	uuid_t		group;		/* Browse group */
	int		view;		/* View mode */
	uint32_t	handle;		/* Service record handle */
};

typedef int (*handler_t)(bdaddr_t *bdaddr, struct search_context *arg);
int	st;
static char UUID_str[MAX_LEN_UUID_STR];
static bdaddr_t interface;

/* Definition of attribute members */
struct member_def {
	char *name;
};

/* Definition of an attribute */
struct attrib_def {
	int			num;		/* Numeric ID - 16 bits */
	char			*name;		/* User readable name */
	struct member_def	*members;	/* Definition of attribute args */
	int			member_max;	/* Max of attribute arg definitions */
};

/* Definition of a service or protocol */
struct uuid_def {
	int			num;		/* Numeric ID - 16 bits */
	char			*name;		/* User readable name */
	struct attrib_def	*attribs;	/* Specific attribute definitions */
	int			attrib_max;	/* Max of attribute definitions */
};

/* Context information about current attribute */
struct attrib_context {
	struct uuid_def		*service;	/* Service UUID, if known */
	struct attrib_def	*attrib;	/* Description of the attribute */
	int			member_index;	/* Index of current attribute member */
};

/* Context information about the whole service */
struct service_context {
	struct uuid_def		*service;	/* Service UUID, if known */
};

/* Allow us to do nice formatting of the lists */
static char *indent_spaces = "                                         ";

#define SERVICE_ATTR	0x1

//static const int uuid16_max = N_ELEMENTS(uuid16_names);

static void print_service_class(void *value, void *userData)
{
	char ServiceClassUUID_str[MAX_LEN_SERVICECLASS_UUID_STR];
	uuid_t *uuid = (uuid_t *)value;

	sdp_uuid2strn(uuid, UUID_str, MAX_LEN_UUID_STR);
	sdp_svclass_uuid2strn(uuid, ServiceClassUUID_str, MAX_LEN_SERVICECLASS_UUID_STR);
	if (st == 1) {
		if (uuid->type != SDP_UUID128) {
			printf("\"%s\" %s\n", ServiceClassUUID_str, UUID_str); //
			fprintf(fi, "%s\n", UUID_str);
			st = 0;
		}
		else {
			printf("UUID 128: %s\n", UUID_str);
			fprintf(fi, "UUID 128: %s\n", UUID_str);
		}
	}	
}

static void print_service_desc(void *value, void *user)
{
	char str[MAX_LEN_PROTOCOL_UUID_STR];
	sdp_data_t *p = (sdp_data_t *)value, *s;
	int i = 0, proto = 0;

	for (; p; p = p->next, i++) {
		switch (p->dtd) {
		case SDP_UUID16:
		case SDP_UUID32:
		case SDP_UUID128:
			sdp_uuid2strn(&p->val.uuid, UUID_str, MAX_LEN_UUID_STR);
			sdp_proto_uuid2strn(&p->val.uuid, str, sizeof(str));
			proto = sdp_uuid_to_proto(&p->val.uuid);
//			printf("  \"%s\" (0x%s)\n", str, UUID_str);
			break;
		case SDP_UINT8:
			if (proto == RFCOMM_UUID) {
				printf("Channel %d ", p->val.uint8);
				fprintf(fi, "%d ", p->val.uint8);
				st = 1;
				}
			else {
				printf("uint8: 0x%02x ", p->val.uint8);
				fprintf(fi, "uint8: 0x%02x ", p->val.uint8);
				}
			break;
		case SDP_UINT16:
		default:
			st = 0;
			break;
		}
	}
}

static void print_access_protos(void *value, void *userData)
{
	sdp_list_t *protDescSeq = (sdp_list_t *)value;
	sdp_list_foreach(protDescSeq, print_service_desc, 0);
}


 /* Parse a SDP record in user friendly form.*/
 
static void print_service_attr(sdp_record_t *rec)
{
	sdp_list_t *list = 0, *proto = 0;
	
	if (sdp_get_access_protos(rec, &proto) == 0) {
		sdp_list_foreach(proto, print_access_protos, 0);
		sdp_list_foreach(proto, (sdp_list_func_t)sdp_list_free, 0);
		sdp_list_free(proto, 0);
	}
	if (sdp_get_service_classes(rec, &list) == 0) {
		sdp_list_foreach(list, print_service_class, 0);
		sdp_list_free(list, free);
	}
}

/* Perform an inquiry and search/browse all peer found. */

typedef int (*handler_t)(bdaddr_t *bdaddr, struct search_context *arg);

static char UUID_str[MAX_LEN_UUID_STR];
static bdaddr_t interface;

static void inquiry(handler_t handler, void *arg)
{
	inquiry_info ii[20];
	uint8_t count = 0;
	int i;

	fprintf(stderr,"Inquiring ...\n");
	if (sdp_general_inquiry(ii, 20, 8, &count) < 0) {
		fprintf(stderr,"Inquiry failed\n");
		return;
	}

	for (i = 0; i < count; i++)
		handler(&ii[i].bdaddr, arg);
}

 /* Search for a specific SDP service */

static int do_search(bdaddr_t *bdaddr, struct search_context *context)
{
	sdp_list_t *attrid, *search, *seq, *next;
	uint32_t range = 0x0000ffff;
	char str[20], file[35] = "/tmp/";
	sdp_session_t *sess;
	
	if (!bdaddr) {
		inquiry(do_search, context);
		return 0;
	}

	sess = sdp_connect(&interface, bdaddr, SDP_RETRY_IF_BUSY);
	ba2str(bdaddr, str);
	if (!sess) {
		fprintf(stderr,"Failed to connect to SDP server on %s: %s\n", str, strerror(errno));
		return -1;
	}
	
	if (context->svc)
		fprintf(stderr,"Searching for %s on %s ...\n", context->svc, str);
	else
		fprintf(stderr,"Browsing %s ...\n", str);

	attrid = sdp_list_append(0, &range);
	search = sdp_list_append(0, &context->group);
	if (sdp_service_search_attr_req(sess, search, SDP_ATTR_REQ_RANGE, attrid, &seq)) {
		fprintf(stderr,"Service Search failed: %s\n", strerror(errno));
		sdp_list_free(attrid, 0);
		sdp_list_free(search, 0);
		sdp_close(sess);
		return -1;
	}
	sdp_list_free(attrid, 0);
	sdp_list_free(search, 0);

	strcat(file, str);
	strcat(file,".info.lst");
	fi = fopen(file, "w");

	for (; seq; seq = next) {
		sdp_record_t *rec = (sdp_record_t *) seq->data;
		struct search_context sub_context;
		
		print_service_attr(rec);
		
		/* Set the subcontext for browsing the sub tree */
		memcpy(&sub_context, context, sizeof(struct search_context));

		if (sdp_get_group_id(rec, &sub_context.group) != -1) {
			/* Browse the next level down if not done */
			if (sub_context.group.value.uuid16 != context->group.value.uuid16)
				do_search(bdaddr, &sub_context);
		}
		next = seq->next;
		free(seq);
		sdp_record_free(rec);
	}
	
	fclose(fi);
	sdp_close(sess);
	return 0;
}

static void usage(void) {
	

	printf("bt-browse - part of sdptool v5.50\n");
	printf("Usage:\n"
		"\tbt-browse [-i hci0] [bdaddr]\n");
	printf("Options:\n"
		"\t-h\t\tDisplay help\n"
		"\t-i\t\tSpecify source interface\n"
		"\tif no bdaddr, browse all.\n");

}

static struct option main_options[] = {
	{ "help",	0, 0, 'h' },
	{ "device",	1, 0, 'i' },
	{ 0, 0, 0, 0 }
};

int main(int argc, char *argv[])
{
	int i, opt;
	struct search_context context;
	
	bacpy(&interface, BDADDR_ANY);

	while ((opt=getopt_long(argc, argv, "+i:h", main_options, NULL)) != -1) {
		switch(opt) {
		case 'i':
			if (!strncmp(optarg, "hci", 3))
				hci_devba(atoi(optarg + 3), &interface);
			else
				str2ba(optarg, &interface);
			break;

		case 'h':
			usage();
			exit(0);

		default:
			exit(1);
		}
	}

	argc -= optind;
	argv += optind;
	optind = 0;

/* Initialise context */
	memset(&context, '\0', sizeof(struct search_context));
	/* We want to browse the top-level/root */
	sdp_uuid16_create(&context.group, PUBLIC_BROWSE_GROUP);

	if (argc >= 1) {
		bdaddr_t bdaddr;
		str2ba(argv[0], &bdaddr);
		do_search(&bdaddr, &context);
	}
	else
	do_search(NULL, &context);
	return 0;
}
