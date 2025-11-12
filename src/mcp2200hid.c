/*
 * mcp2200cli.c
 *
 *  Created on: 2025.11.12
 *      Author: papergion
 */
#define PROGNAME "MCP2200HID"
#include <stdio.h>
#include <sys/types.h>

#include "mcp2200.h"
#include <getopt.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

// ===================================================================================
uint8_t params = 0; //  0x01:direction    0x02:alternate    0x04:output   0x08:option
uint8_t operation = 0; //  c/i
uint8_t readwrite = 0; //  r/w/p/f
uint8_t direction = 0; 
uint8_t alternate = 0; 
uint8_t output = 0; 
uint8_t option = 0; 
uint8_t data [32];
uint8_t nowdirection = 0; 
uint8_t nowalternate = 0; 
uint8_t nowoutput = 0; 
uint8_t nowoption = 0; 
uint8_t nowgpio = 0; 
uint8_t baudH = 0; 
uint8_t baudL = 0; 
// =============================================================================================
char axTOchar(char * aData)
{
char *ptr;
long ret;
    ret = strtoul(aData, &ptr, 16);
    return (char) ret;
}
// ===================================================================================
static void print_usage(const char *prog)	// NOT USED
{
	printf("Usage: %s [-cvfmBupwDdCRKSNTUtl]\n", prog);  // utilizzo completo:  -B -D -C -K -T
	puts("  -c --config [r/w] : configuration read/write \n"
		 "  -i --i/o [r/w/p/f]    : gpio read/write/pulse/fall \n"
		 "  -d --direction [0xnn] : direction of gpio (0=out, 1=in)\n"
		 "  -a --alternate [0xnn] : alternate use of gpio (0=i/o, 1=alternate)\n"
		 "  -o --output    [0xnn] : default / output value of gpio\n"
		 "       non-specified items remains unchanged"
		 );
}// ===================================================================================
static char parse_opts(int argc, char *argv[])	
{
	if ((argc < 2) || (argc > 6))
	{
		print_usage(PROGNAME);
		return 3;
	}
	while (1) {
		static const struct option lopts[] = {
//------------longname---optarg---short--      0=no optarg    1=optarg obbligatorio     2=optarg facoltativo
			{ "config",     1, 0, 'c' },	// -c[r/w]
			{ "i/o",     	1, 0, 'i' },	// -i[r/w/p/f]
			{ "direction",  1, 0, 'd' },	// -d[0xnn]
			{ "alternate",  1, 0, 'a' },	// -a[0xnn]
			{ "output",     1, 0, 'o' },	// -o[0xnn]
			{ "options",    1, 0, 'p' },	// -p[0xnn]
			{ "help",		0, 0, '?' },
			{ NULL, 0, 0, 0 }
		};	
		int c;
		c = getopt_long(argc, argv, "c:i:d:a:o:p:?h", lopts, NULL);

		if (c == -1)
			return 0;

		switch (c) 
		{
		case 'h':
			print_usage(PROGNAME);
			return 2;
			break;
		case 'c':
		    operation = 'c';
			readwrite = optarg[0];
			if (readwrite == 'r')
				printf("\n-config read");
			else
			if (readwrite == 'w')
				printf("\n-config write");
			else
			{
				fprintf(stderr,"-c [r/w] !\n");
				print_usage(PROGNAME);
				return 2;
			}
			break;

		case 'i':
		    operation = 'i';
			readwrite = optarg[0];
			if (readwrite == 'r')
				printf("\n-input read");
			else
			if (readwrite == 'w')
				printf("\n-output write");
			else
			if (readwrite == 'p')
				printf("\n-output pulse");
			else
			if (readwrite == 'f')
				printf("\n-output fall");
			else
			{
				fprintf(stderr,"-i [r/w/p/f] !\n");
				print_usage(PROGNAME);
				return 2;
			}
			break;
			 
		case 'd':
			direction = axTOchar(optarg);
			printf("\n-direction 0x%02X",direction);
			params |= 0x01;
			break;

		case 'a':
			alternate = axTOchar(optarg);
			printf("\n-alternate 0x%02X",alternate);
			params |= 0x02;
			break;

		case 'o':
			output = axTOchar(optarg);
			printf("\n-output 0x%02X",output);
			params |= 0x04;
			break;

		case 'p':
			option = axTOchar(optarg);
			printf("\n-option 0x%02X",option);
			params |= 0x08;
			break;

		case '?':
			return 2;		

		default:
			print_usage(argv[0]);
			break;
		}
	}
	return 0;
}
// ===================================================================================
void mSleep(int millisec) {
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = 1000000L;
	while (millisec--)
	{
    	nanosleep(&req, (struct timespec *)NULL);
	}
}
// ===================================================================================



// ===================================================================================
int main(int argc, char** argv){
// ===================================================================================
	printf("\n******** MCP2200 ********\n");
	if (parse_opts(argc, argv))
		return 0;

/* ===================================================================================
coerenza
unsigned char operation = 0; //  c/i
unsigned char readwrite = 0; //  r/w/p/f
unsigned char direction = 0; 
unsigned char alternate = 0; 
unsigned char output = 0; 
unsigned char option = 0; 
// =================================================================================*/
	if (operation == 0) 
	{
        fprintf(stderr,"\nplease specify config or i/o \n");
		return 2;		
	}
	else
		printf("\n*************************\n");

	int r = mcp2200_init();
	if (r < 0)
		return r;

	int cnt = mcp2200_list_devices(MCP2200_VENDOR_ID, MCP2200_PRODUCT_ID);
	if (cnt < 0) {
		printf("List_devices error...!");
		return cnt;
	}

	if (cnt == 0){
		printf("No device found!");
		return 0;
	}

	if (cnt > 1)
	{
		printf("Multiple devices, couldn't choose..");
	}

//	if (cnt == 1)
	{
		int address = mcp2200_get_address(0);
		printf("Opening at address 0x%x\n", address);

		int connectionID = mcp2200_connect(0,MCP2200_HID_CLASS);

		if (connectionID < 0){
			printf("Connection failed! Error code: %d\n", connectionID);
			return 0;
		}
		r = mcp2200_hid_read_io(connectionID, data);
		if (r == 0)
			printf("read OK\n");
		else
		{
			printf("read error: %d\n", r);
			return 0;
		}
		printf("\nactual values:");
		nowdirection = data[4]; 
		nowalternate = data[5]; 
		nowoutput = data[6]; 
		nowoption = data[7];
		baudH = data[8];
		baudL = data[9];
		nowgpio = data[10];
		
		
		if (operation == 'c')
			printf("\ni/o dir: 0x%02X  - altern: 0x%02x  - defout: 0x%02x  - opts: 0x%02x  - baud: 0x%02X%02X\n",nowdirection,nowalternate,nowoutput,nowoption,baudH,baudL);
		if (operation == 'i')
			printf("\ni/o pin: 0x%02X \n",nowgpio);

		if (params & 0x01)
			nowdirection = direction;
		if (params & 0x02)
			nowalternate = alternate;
		if (params & 0x04)
			nowoutput = output;
		if (params & 0x08)
			nowoption = option;

		if (params & 0x04)
			nowgpio = output;

		if (readwrite == 'w')
		{
			printf("\nnew values:");
			if (operation == 'c')
			{
				printf("\ni/o dir: 0x%02X  - altern: 0x%02x  - defout: 0x%02x  - opts: 0x%02x  - baud: 0x%02X%02X\n",nowdirection,nowalternate,nowoutput,nowoption,baudH,baudL);
				r = mcp2200_hid_configure(connectionID, nowdirection, nowalternate, nowoutput, nowoutput, baudH, baudL);
			}
			if (operation == 'i')
			{
				printf("\ni/o pin: 0x%02X \n",nowgpio);
				r = mcp2200_hid_set_clear_output(connectionID, nowgpio/*bits to set*/, (nowgpio^0xFF)/*bits to clear*/);
			}
		}
		else
		if (readwrite == 'p')
		{
			printf("\ni/o pulse pin: 0x%02X \n",nowgpio);
			r = mcp2200_hid_set_clear_output(connectionID, nowgpio/*bits to set*/, 0/*bits to clear*/);
			mSleep(300);
			r = mcp2200_hid_set_clear_output(connectionID, 0/*bits to set*/, nowgpio/*bits to clear*/);
		}
		else
		if (readwrite == 'f')
		{
			printf("\ni/o fall pin: 0x%02X \n",nowgpio);
			r = mcp2200_hid_set_clear_output(connectionID, 0/*bits to set*/, nowgpio/*bits to clear*/);
			mSleep(300);
			r = mcp2200_hid_set_clear_output(connectionID, nowgpio/*bits to set*/, 0/*bits to clear*/);
		}
		if (r == 0)
			printf("OK...\n");
		else
		{
			printf("write error: %d\n", r);
			return 0;
		}
		mcp2200_disconnect(connectionID);
	}
	mcp2200_close();
	return 0;
}
