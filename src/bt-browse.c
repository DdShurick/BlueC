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

/* ID of the service attribute.
 * Most attributes after 0x200 are defined based on the service, so
 * we need to find what is the service (which is messy) - Jean II */
#define SERVICE_ATTR	0x1

/* Definition of the optional arguments in protocol list */
static struct member_def protocol_members[] = {
	{ "Protocol"		},
	{ "Channel/Port"	},
	{ "Version"		},
};

/* Definition of the optional arguments in profile list */
static struct member_def profile_members[] = {
	{ "Profile"	},
	{ "Version"	},
};

/* Definition of the optional arguments in Language list */
static struct member_def language_members[] = {
	{ "Code ISO639"		},
	{ "Encoding"		},
	{ "Base Offset"		},
};

/* Name of the various common attributes. See BT assigned numbers */
static struct attrib_def attrib_names[] = {
	{ 0x0, "ServiceRecordHandle", NULL, 0 },
	{ 0x1, "ServiceClassIDList", NULL, 0 },
	{ 0x2, "ServiceRecordState", NULL, 0 },
	{ 0x3, "ServiceID", NULL, 0 },
	{ 0x4, "ProtocolDescriptorList",
		protocol_members, N_ELEMENTS(protocol_members) },
	{ 0x5, "BrowseGroupList", NULL, 0 },
	{ 0x6, "LanguageBaseAttributeIDList",
		language_members, N_ELEMENTS(language_members) },
	{ 0x7, "ServiceInfoTimeToLive", NULL, 0 },
	{ 0x8, "ServiceAvailability", NULL, 0 },
	{ 0x9, "BluetoothProfileDescriptorList",
		profile_members, N_ELEMENTS(profile_members) },
	{ 0xA, "DocumentationURL", NULL, 0 },
	{ 0xB, "ClientExecutableURL", NULL, 0 },
	{ 0xC, "IconURL", NULL, 0 },
	{ 0xD, "AdditionalProtocolDescriptorLists", NULL, 0 },
	/* Definitions after that are tricky (per profile or offset) */
};

const int attrib_max = N_ELEMENTS(attrib_names);

/* Name of the various SPD attributes. See BT assigned numbers */
static struct attrib_def sdp_attrib_names[] = {
	{ 0x200, "VersionNumberList", NULL, 0 },
	{ 0x201, "ServiceDatabaseState", NULL, 0 },
};

/* Name of the various SPD attributes. See BT assigned numbers */
static struct attrib_def browse_attrib_names[] = {
	{ 0x200, "GroupID", NULL, 0 },
};

/* Name of the various Device ID attributes. See Device Id spec. */
static struct attrib_def did_attrib_names[] = {
	{ 0x200, "SpecificationID", NULL, 0 },
	{ 0x201, "VendorID", NULL, 0 },
	{ 0x202, "ProductID", NULL, 0 },
	{ 0x203, "Version", NULL, 0 },
	{ 0x204, "PrimaryRecord", NULL, 0 },
	{ 0x205, "VendorIDSource", NULL, 0 },
};

/* Name of the various HID attributes. See HID spec. */
static struct attrib_def hid_attrib_names[] = {
	{ 0x200, "DeviceReleaseNum", NULL, 0 },
	{ 0x201, "ParserVersion", NULL, 0 },
	{ 0x202, "DeviceSubclass", NULL, 0 },
	{ 0x203, "CountryCode", NULL, 0 },
	{ 0x204, "VirtualCable", NULL, 0 },
	{ 0x205, "ReconnectInitiate", NULL, 0 },
	{ 0x206, "DescriptorList", NULL, 0 },
	{ 0x207, "LangIDBaseList", NULL, 0 },
	{ 0x208, "SDPDisable", NULL, 0 },
	{ 0x209, "BatteryPower", NULL, 0 },
	{ 0x20a, "RemoteWakeup", NULL, 0 },
	{ 0x20b, "ProfileVersion", NULL, 0 },
	{ 0x20c, "SupervisionTimeout", NULL, 0 },
	{ 0x20d, "NormallyConnectable", NULL, 0 },
	{ 0x20e, "BootDevice", NULL, 0 },
};

/* Name of the various PAN attributes. See BT assigned numbers */
/* Note : those need to be double checked - Jean II */
static struct attrib_def pan_attrib_names[] = {
	{ 0x200, "IpSubnet", NULL, 0 },		/* Obsolete ??? */
	{ 0x30A, "SecurityDescription", NULL, 0 },
	{ 0x30B, "NetAccessType", NULL, 0 },
	{ 0x30C, "MaxNetAccessrate", NULL, 0 },
	{ 0x30D, "IPv4Subnet", NULL, 0 },
	{ 0x30E, "IPv6Subnet", NULL, 0 },
};

/* Name of the various Generic-Audio attributes. See BT assigned numbers */
/* Note : totally untested - Jean II */
static struct attrib_def audio_attrib_names[] = {
	{ 0x302, "Remote audio volume control", NULL, 0 },
};

/* Name of the various IrMCSync attributes. See BT assigned numbers */
static struct attrib_def irmc_attrib_names[] = {
	{ 0x0301, "SupportedDataStoresList", NULL, 0 },
};

/* Name of the various GOEP attributes. See BT assigned numbers */
static struct attrib_def goep_attrib_names[] = {
	{ 0x200, "GoepL2capPsm", NULL, 0 },
};

/* Name of the various PBAP attributes. See BT assigned numbers */
static struct attrib_def pbap_attrib_names[] = {
	{ 0x0314, "SupportedRepositories", NULL, 0 },
	{ 0x0317, "PbapSupportedFeatures", NULL, 0 },
};

/* Name of the various MAS attributes. See BT assigned numbers */
static struct attrib_def mas_attrib_names[] = {
	{ 0x0315, "MASInstanceID", NULL, 0 },
	{ 0x0316, "SupportedMessageTypes", NULL, 0 },
	{ 0x0317, "MapSupportedFeatures", NULL, 0 },
};

/* Name of the various MNS attributes. See BT assigned numbers */
static struct attrib_def mns_attrib_names[] = {
	{ 0x0317, "MapSupportedFeatures", NULL, 0 },
};

/* Same for the UUIDs. See BT assigned numbers */
static struct uuid_def uuid16_names[] = {
	/* -- Protocols -- */
	{ 0x0001, "SDP", NULL, 0 },
	{ 0x0002, "UDP", NULL, 0 },
	{ 0x0003, "RFCOMM", NULL, 0 },
	{ 0x0004, "TCP", NULL, 0 },
	{ 0x0005, "TCS-BIN", NULL, 0 },
	{ 0x0006, "TCS-AT", NULL, 0 },
	{ 0x0008, "OBEX", NULL, 0 },
	{ 0x0009, "IP", NULL, 0 },
	{ 0x000a, "FTP", NULL, 0 },
	{ 0x000c, "HTTP", NULL, 0 },
	{ 0x000e, "WSP", NULL, 0 },
	{ 0x000f, "BNEP", NULL, 0 },
	{ 0x0010, "UPnP/ESDP", NULL, 0 },
	{ 0x0011, "HIDP", NULL, 0 },
	{ 0x0012, "HardcopyControlChannel", NULL, 0 },
	{ 0x0014, "HardcopyDataChannel", NULL, 0 },
	{ 0x0016, "HardcopyNotification", NULL, 0 },
	{ 0x0017, "AVCTP", NULL, 0 },
	{ 0x0019, "AVDTP", NULL, 0 },
	{ 0x001b, "CMTP", NULL, 0 },
	{ 0x001d, "UDI_C-Plane", NULL, 0 },
	{ 0x0100, "L2CAP", NULL, 0 },
	/* -- Services -- */
	{ 0x1000, "ServiceDiscoveryServerServiceClassID",
		sdp_attrib_names, N_ELEMENTS(sdp_attrib_names) },
	{ 0x1001, "BrowseGroupDescriptorServiceClassID",
		browse_attrib_names, N_ELEMENTS(browse_attrib_names) },
	{ 0x1002, "PublicBrowseGroup", NULL, 0 },
	{ 0x1101, "SerialPort", NULL, 0 },
	{ 0x1102, "LANAccessUsingPPP", NULL, 0 },
	{ 0x1103, "DialupNetworking (DUN)", NULL, 0 },
	{ 0x1104, "IrMCSync",
		irmc_attrib_names, N_ELEMENTS(irmc_attrib_names) },
	{ 0x1105, "OBEXObjectPush",
		goep_attrib_names, N_ELEMENTS(goep_attrib_names) },
	{ 0x1106, "OBEXFileTransfer",
		goep_attrib_names, N_ELEMENTS(goep_attrib_names) },
	{ 0x1107, "IrMCSyncCommand", NULL, 0 },
	{ 0x1108, "Headset",
		audio_attrib_names, N_ELEMENTS(audio_attrib_names) },
	{ 0x1109, "CordlessTelephony", NULL, 0 },
	{ 0x110a, "AudioSource", NULL, 0 },
	{ 0x110b, "AudioSink", NULL, 0 },
	{ 0x110c, "RemoteControlTarget", NULL, 0 },
	{ 0x110d, "AdvancedAudio", NULL, 0 },
	{ 0x110e, "RemoteControl", NULL, 0 },
	{ 0x110f, "RemoteControlController", NULL, 0 },
	{ 0x1110, "Intercom", NULL, 0 },
	{ 0x1111, "Fax", NULL, 0 },
	{ 0x1112, "HeadsetAudioGateway", NULL, 0 },
	{ 0x1113, "WAP", NULL, 0 },
	{ 0x1114, "WAP Client", NULL, 0 },
	{ 0x1115, "PANU (PAN/BNEP)",
		pan_attrib_names, N_ELEMENTS(pan_attrib_names) },
	{ 0x1116, "NAP (PAN/BNEP)",
		pan_attrib_names, N_ELEMENTS(pan_attrib_names) },
	{ 0x1117, "GN (PAN/BNEP)",
		pan_attrib_names, N_ELEMENTS(pan_attrib_names) },
	{ 0x1118, "DirectPrinting (BPP)", NULL, 0 },
	{ 0x1119, "ReferencePrinting (BPP)", NULL, 0 },
	{ 0x111a, "Imaging (BIP)", NULL, 0 },
	{ 0x111b, "ImagingResponder (BIP)", NULL, 0 },
	{ 0x111c, "ImagingAutomaticArchive (BIP)", NULL, 0 },
	{ 0x111d, "ImagingReferencedObjects (BIP)", NULL, 0 },
	{ 0x111e, "Handsfree", NULL, 0 },
	{ 0x111f, "HandsfreeAudioGateway", NULL, 0 },
	{ 0x1120, "DirectPrintingReferenceObjectsService (BPP)", NULL, 0 },
	{ 0x1121, "ReflectedUI (BPP)", NULL, 0 },
	{ 0x1122, "BasicPrinting (BPP)", NULL, 0 },
	{ 0x1123, "PrintingStatus (BPP)", NULL, 0 },
	{ 0x1124, "HumanInterfaceDeviceService (HID)",
		hid_attrib_names, N_ELEMENTS(hid_attrib_names) },
	{ 0x1125, "HardcopyCableReplacement (HCR)", NULL, 0 },
	{ 0x1126, "HCR_Print (HCR)", NULL, 0 },
	{ 0x1127, "HCR_Scan (HCR)", NULL, 0 },
	{ 0x1128, "Common ISDN Access (CIP)", NULL, 0 },
	{ 0x112a, "UDI-MT", NULL, 0 },
	{ 0x112b, "UDI-TA", NULL, 0 },
	{ 0x112c, "Audio/Video", NULL, 0 },
	{ 0x112d, "SIM Access (SAP)", NULL, 0 },
	{ 0x112e, "Phonebook Access (PBAP) - PCE", NULL, 0 },
	{ 0x112f, "Phonebook Access (PBAP) - PSE",
		pbap_attrib_names, N_ELEMENTS(pbap_attrib_names) },
	{ 0x1130, "Phonebook Access (PBAP)", NULL, 0 },
	{ 0x1131, "Headset (HSP)", NULL, 0 },
	{ 0x1132, "Message Access (MAP) - MAS",
		mas_attrib_names, N_ELEMENTS(mas_attrib_names) },
	{ 0x1133, "Message Access (MAP) - MNS",
		mns_attrib_names, N_ELEMENTS(mns_attrib_names) },
	{ 0x1134, "Message Access (MAP)", NULL, 0 },
	/* ... */
	{ 0x1200, "PnPInformation",
		did_attrib_names, N_ELEMENTS(did_attrib_names) },
	{ 0x1201, "GenericNetworking", NULL, 0 },
	{ 0x1202, "GenericFileTransfer", NULL, 0 },
	{ 0x1203, "GenericAudio",
		audio_attrib_names, N_ELEMENTS(audio_attrib_names) },
	{ 0x1204, "GenericTelephony", NULL, 0 },
	/* ... */
	{ 0x1303, "VideoSource", NULL, 0 },
	{ 0x1304, "VideoSink", NULL, 0 },
	{ 0x1305, "VideoDistribution", NULL, 0 },
	{ 0x1400, "HDP", NULL, 0 },
	{ 0x1401, "HDPSource", NULL, 0 },
	{ 0x1402, "HDPSink", NULL, 0 },
	{ 0x2112, "AppleAgent", NULL, 0 },
};

static const int uuid16_max = N_ELEMENTS(uuid16_names);

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
			printf("  \"%s\" (0x%s)\n", str, UUID_str);
			break;
		case SDP_UINT8:
			if (proto == RFCOMM_UUID)
				printf("    Channel: %d\n", p->val.uint8);
			else
				printf("    uint8: 0x%02x\n", p->val.uint8);
			break;
		case SDP_UINT16:
			if (proto == L2CAP_UUID) {
				if (i == 1)
					printf("    PSM: %d\n", p->val.uint16);
				else
					printf("    Version: 0x%04x\n", p->val.uint16);
			} else if (proto == BNEP_UUID)
				if (i == 1)
					printf("    Version: 0x%04x\n", p->val.uint16);
				else
					printf("    uint16: 0x%04x\n", p->val.uint16);
			else
				printf("    uint16: 0x%04x\n", p->val.uint16);
			break;
		case SDP_SEQ16:
			printf("    SEQ16:");
			for (s = p->val.dataseq; s; s = s->next)
				printf(" %x", s->val.uint16);
			printf("\n");
			break;
		case SDP_SEQ8:
			printf("    SEQ8:");
			for (s = p->val.dataseq; s; s = s->next)
				printf(" %x", s->val.uint8);
			printf("\n");
			break;
		default:
			printf("    FIXME: dtd=0%x\n", p->dtd);
			break;
		}
	}
}
/*
static void print_lang_attr(void *value, void *user)
{
	sdp_lang_attr_t *lang = (sdp_lang_attr_t *)value;
	printf("  code_ISO639: 0x%02x\n", lang->code_ISO639);
	printf("  encoding:    0x%02x\n", lang->encoding);
	printf("  base_offset: 0x%02x\n", lang->base_offset);
}
*/
static void print_access_protos(void *value, void *userData)
{
	sdp_list_t *protDescSeq = (sdp_list_t *)value;
	sdp_list_foreach(protDescSeq, print_service_desc, 0);
}

static void print_profile_desc(void *value, void *userData)
{
	sdp_profile_desc_t *desc = (sdp_profile_desc_t *)value;
	char str[MAX_LEN_PROFILEDESCRIPTOR_UUID_STR];

	sdp_uuid2strn(&desc->uuid, UUID_str, MAX_LEN_UUID_STR);
	sdp_profile_uuid2strn(&desc->uuid, str, MAX_LEN_PROFILEDESCRIPTOR_UUID_STR);

	printf("  \"%s\" (0x%s)\n", str, UUID_str);
	if (desc->version)
		printf("    Version: 0x%04x\n", desc->version);
}

/*
 * Parse a SDP record in user friendly form.
 */
static void print_service_attr(sdp_record_t *rec)
{
	sdp_list_t *list = 0, *proto = 0;

	sdp_record_print(rec);

//	printf("Service RecHandle: 0x%x\n", rec->handle);

	if (sdp_get_service_classes(rec, &list) == 0) {
		printf("Service Class ID List:\n");
		sdp_list_foreach(list, print_service_class, 0);
		sdp_list_free(list, free);
	}
	if (sdp_get_access_protos(rec, &proto) == 0) {
		printf("Protocol Descriptor List:\n");
		sdp_list_foreach(proto, print_access_protos, 0);
		sdp_list_foreach(proto, (sdp_list_func_t)sdp_list_free, 0);
		sdp_list_free(proto, 0);
	}
/*	if (sdp_get_lang_attr(rec, &list) == 0) {
		printf("Language Base Attr List:\n");
		sdp_list_foreach(list, print_lang_attr, 0);
		sdp_list_free(list, free);
	}*/
	if (sdp_get_profile_descs(rec, &list) == 0) {
		printf("Profile Descriptor List:\n");
		sdp_list_foreach(list, print_profile_desc, 0);
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

//	if (context->view != RAW_VIEW) {
		if (context->svc)
			printf("Searching for %s on %s ...\n", context->svc, str);
		else
			printf("Browsing %s ...\n", str);
//	}

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
		printf("\n");

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
/*
 * Browse the full SDP database (i.e. list all services starting from the
 * root/top-level).
 */
static int cmd_browse(int argc, char **argv)
{
	struct search_context context;
	int opt, num;

	/* Initialise context */
	memset(&context, '\0', sizeof(struct search_context));
	/* We want to browse the top-level/root */
	sdp_uuid16_create(&context.group, PUBLIC_BROWSE_GROUP);

	if (argc >= 1) {
		bdaddr_t bdaddr;
		str2ba(argv[0], &bdaddr);
		return do_search(&bdaddr, &context);
	}

	return do_search(NULL, &context);
}

static void usage(void) {
	

	printf("bt-browse - part of sdptool v5.50\n");
	printf("Usage:\n"
		"\tbt-browse [bdaddr]\n");
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

	cmd_browse(argc, argv);

	return 1;
}
