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

/* Pass args to the inquiry/search handler */
struct search_context {
	char		*svc;		/* Service */
	uuid_t		group;		/* Browse group */
	int		view;		/* View mode */
	uint32_t	handle;		/* Service record handle */
};

typedef int (*handler_t)(bdaddr_t *bdaddr, struct search_context *arg);

static char UUID_str[MAX_LEN_UUID_STR];
static bdaddr_t interface;

static void print_service_class(void *value, void *userData)
{
	char ServiceClassUUID_str[MAX_LEN_SERVICECLASS_UUID_STR];
	uuid_t *uuid = (uuid_t *)value;

	sdp_uuid2strn(uuid, UUID_str, MAX_LEN_UUID_STR);
	sdp_svclass_uuid2strn(uuid, ServiceClassUUID_str, MAX_LEN_SERVICECLASS_UUID_STR);
	if (uuid->type != SDP_UUID128)
		printf("  \"%s\" (0x%s)\n", ServiceClassUUID_str, UUID_str);
	else
		printf("  UUID 128: %s\n", UUID_str);
}

/*
 * Parse a SDP record in user friendly form.
 */
static void print_service_attr(sdp_record_t *rec)
{
	sdp_list_t *list = 0, *proto = 0;

	if (sdp_get_service_classes(rec, &list) == 0) {
//		printf("Service Class ID List:\n");
		sdp_list_foreach(list, print_service_class, 0);
		sdp_list_free(list, free);
	}
}

/*
 * Perform an inquiry and search/browse all peer found.
 */
 
typedef int (*handler_t)(bdaddr_t *bdaddr, struct search_context *arg);

static char UUID_str[MAX_LEN_UUID_STR];
static bdaddr_t interface;

static void inquiry(handler_t handler, void *arg)
{
	inquiry_info ii[20];
	uint8_t count = 0;
	int i;

	printf("Inquiring ...\n");
	if (sdp_general_inquiry(ii, 20, 8, &count) < 0) {
		printf("Inquiry failed\n");
		return;
	}

	for (i = 0; i < count; i++)
		handler(&ii[i].bdaddr, arg);
}

/*
 * Search for a specific SDP service
 */
static int do_search(bdaddr_t *bdaddr, struct search_context *context)
{
	sdp_list_t *attrid, *search, *seq, *next;
	uint32_t range = 0x0000ffff;
	char str[20];
	sdp_session_t *sess;

	if (!bdaddr) {
		inquiry(do_search, context);
		return 0;
	}

	sess = sdp_connect(&interface, bdaddr, SDP_RETRY_IF_BUSY);
	ba2str(bdaddr, str);
	if (!sess) {
		printf("Failed to connect to SDP server on %s: %s\n", str, strerror(errno));
		return -1;
	}
	
	if (context->svc)
		printf("Searching for %s on %s ...\n", context->svc, str);
	else
		printf("Browsing %s ...\n", str);

	attrid = sdp_list_append(0, &range);
	search = sdp_list_append(0, &context->group);
	if (sdp_service_search_attr_req(sess, search, SDP_ATTR_REQ_RANGE, attrid, &seq)) {
		printf("Service Search failed: %s\n", strerror(errno));
		sdp_list_free(attrid, 0);
		sdp_list_free(search, 0);
		sdp_close(sess);
		return -1;
	}
	sdp_list_free(attrid, 0);
	sdp_list_free(search, 0);

	for (; seq; seq = next) {
		sdp_record_t *rec = (sdp_record_t *) seq->data;
		struct search_context sub_context;

		print_service_attr(rec);
//		printf("\n");

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

	sdp_close(sess);
	return 0;
}

static void usage(void) {
	
	printf("bt-browse - part of sdptool v5.50\n");
	printf("Usage:\n"
		"\tbt-browse [-i hci0] [bdaddr]\n");
	printf("Options:\n"
		"\t-h\t\tDisplay help\n"
		"\t-i\t\tSpecify source interface\n");

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
