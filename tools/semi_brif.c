/*
 * semi_brif.c	Program to display the fields of a pre-brif'd file.
 *
 * Andrew Clayton <andrew@pccl.info>, 2012
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BRIF_LINE_LEN		300

#define BRIF_MR_F3_S		2
#define BRIF_MR_F3_L		1
#define BRIF_MR_F5_S		4
#define BRIF_MR_F5_L		1
#define BRIF_MR_F15_S		72
#define BRIF_MR_F15_L		12
#define BRIF_MR_F18_S		136
#define BRIF_MR_F18_L		BRIF_LINE_LEN

#define BRIF_PC_F6_S		5
#define BRIF_PC_F6_L		12
#define BRIF_PC_F8_S		30
#define BRIF_PC_F8_L		12
#define BRIF_PC_F9_S		42
#define BRIF_PC_F9_L		8
#define BRIF_PC_F10_S		50
#define BRIF_PC_F10_L		12
#define BRIF_PC_F11_S		62
#define BRIF_PC_F11_L		12
#define BRIF_PC_F15_S		97
#define BRIF_PC_F15_L		4
#define BRIF_PC_F17_S		102
#define BRIF_PC_F17_L		20
#define BRIF_PC_F19_S		134
#define BRIF_PC_F19_L		15
#define BRIF_PC_F22_S		184
#define BRIF_PC_F22_L		12

#define BRIF_PI_F4_S		3
#define BRIF_PI_F4_L		1
#define BRIF_PI_F6_S		5
#define BRIF_PI_F6_L		3
#define BRIF_PI_F7_S		8
#define BRIF_PI_F7_L		12
#define BRIF_PI_F8_S		20
#define BRIF_PI_F8_L		12
#define BRIF_PI_F10_S		47
#define BRIF_PI_F10_L		40
#define BRIF_PI_F11_S		87
#define BRIF_PI_F11_L		12
#define BRIF_PI_F12_S		99
#define BRIF_PI_F12_L		4
#define BRIF_PI_F13_S		103
#define BRIF_PI_F13_L		12
#define BRIF_PI_F14_S		115
#define BRIF_PI_F14_L		12
#define BRIF_PI_F15_S		127
#define BRIF_PI_F15_L		12
#define BRIF_PI_F16_S		139
#define BRIF_PI_F16_L		12
#define BRIF_PI_F18_S		152
#define BRIF_PI_F18_L		12

static void print_field(const char *line, const char *field, int start,
			int len)
{
	char data[BRIF_LINE_LEN + 1];

	snprintf(data, len + 1, "%s", line + start);
	/* Remove trailing \n from end of line */
	if (strstr(data, "\n"))
		data[strlen(data) - 1] = '\0';
	printf("\t%-3s = %s\n", field, data);
}

static void do_main_record(const char *line)
{
	printf("Main Record Line\n\n");

	print_field(line, "F3", BRIF_MR_F3_S, BRIF_MR_F3_L);
	print_field(line, "F5", BRIF_MR_F5_S, BRIF_MR_F5_L);
	print_field(line, "F15", BRIF_MR_F15_S, BRIF_MR_F15_L);
	print_field(line, "F18", BRIF_MR_F18_S, BRIF_MR_F18_L);

	printf("\n");
}

static void do_purchasing_card(const char *line)
{
	printf("Purchasing Card Line\n\n");

	print_field(line, "F6", BRIF_PC_F6_S, BRIF_PC_F6_L);
	print_field(line, "F8", BRIF_PC_F8_S, BRIF_PC_F8_L);
	print_field(line, "F9", BRIF_PC_F9_S, BRIF_PC_F9_L);
	print_field(line, "F10", BRIF_PC_F10_S, BRIF_PC_F10_L);
	print_field(line, "F11", BRIF_PC_F11_S, BRIF_PC_F11_L);
	print_field(line, "f15", BRIF_PC_F15_S, BRIF_PC_F15_L);
	print_field(line, "F17", BRIF_PC_F17_S, BRIF_PC_F17_L);
	print_field(line, "F19", BRIF_PC_F19_S, BRIF_PC_F19_L);
	print_field(line, "F22", BRIF_PC_F22_S, BRIF_PC_F22_L);

	printf("\n");
}

static void do_purchasing_card_item(const char *line)
{
	printf("Purchasing Card Item Line\n\n");

	print_field(line, "F4", BRIF_PI_F4_S, BRIF_PI_F4_L);
	print_field(line, "F6", BRIF_PI_F6_S, BRIF_PI_F6_L);
	print_field(line, "F7", BRIF_PI_F7_S, BRIF_PI_F7_L);
	print_field(line, "F8", BRIF_PI_F8_S, BRIF_PI_F8_L);
	print_field(line, "F10", BRIF_PI_F10_S, BRIF_PI_F10_L);
	print_field(line, "F11", BRIF_PI_F11_S, BRIF_PI_F11_L);
	print_field(line, "F12", BRIF_PI_F12_S, BRIF_PI_F12_L);
	print_field(line, "F13", BRIF_PI_F13_S, BRIF_PI_F13_L);
	print_field(line, "F14", BRIF_PI_F14_S, BRIF_PI_F14_L);
	print_field(line, "F15", BRIF_PI_F15_S, BRIF_PI_F15_L);
	print_field(line, "F16", BRIF_PI_F16_S, BRIF_PI_F16_L);
	print_field(line, "F18", BRIF_PI_F18_S, BRIF_PI_F18_L);

	printf("\n");
}

int main(int argc, char **argv)
{
	char line[BRIF_LINE_LEN + 1];
	FILE *fp;

	if (argc < 2)
		exit(EXIT_FAILURE);

	fp = fopen(argv[1], "r");
	while (!feof(fp)) {
		fgets(line, BRIF_LINE_LEN, fp);
		if (line[2] == 'S' || line[2] == 'R')
			do_main_record(line);
		else if (line[3] == ' ')
			do_purchasing_card(line);
		else if (line[3] == '0' || line[3] == '1')
			do_purchasing_card_item(line);
	}
	fclose(fp);

	exit(EXIT_SUCCESS);
}
