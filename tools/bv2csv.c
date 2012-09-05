#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#define BRIF_LINE_LEN	300

struct fields {
	char mr_3[32];
	char mr_8[32];
	double mr_15;
	char mr_18[32];

	double pc_10;
	double pc_11;
	double pc_22;

	double pci_7;
	double pci_11;
	double pci_13;
	double pci_15;
	double pci_16;
	double pci_18;
} fields;
struct fields fields;

/*
 * Adjust for credits (R) which should show as -ve values.
 * Default to +ve as that's the most common.
 */
double cra = 1.0;

/*
 * Takes a brif format amount, e.g 17520 and returns this value with
 * the decimal point added, i.e 175.20
 */
static double add_dp(const char *src, int dp)
{
	char amount[32];

	memset(amount, 0, sizeof(amount));
	strncpy(amount, src, strlen(src) - dp);
	strcat(amount, ".");
	strncat(amount, src + strlen(src) - dp, dp);

	return strtod(amount, NULL);
}

static void process_mr(const char *line, FILE *ofp)
{
	char field[32];

	/* Transaction Type. 3, 1 */
	snprintf(fields.mr_3, 2, "%s", line + 2);

	/* Merchant ID. 20, 15 */
	snprintf(fields.mr_8, 16, "%s", line + 19);

	/* Amount. 73, 12 */
	snprintf(field, 13, "%s", line + 72);
	fields.mr_15 = add_dp(field, 2);

	/* Originators Trans Ref, 137, 25 */
	snprintf(fields.mr_18, 26, "%s", line + 136);
}

static void process_pc(const char *line, FILE *ofp)
{
	char field[32];

	/* Discount Amount. 51, 12 */
	snprintf(field, 13, "%s", line + 50);
	fields.pc_10 = add_dp(field, 2);

	/* Freight Amount. 63, 12 */
	snprintf(field, 13, "%s", line + 62);
	fields.pc_11 = add_dp(field, 2);

	/* Total Items Amount. 185, 12 */
	snprintf(field, 13, "%s", line + 184);
	fields.pc_22 = add_dp(field, 2);
}

static void process_pci(const char *line, FILE *ofp)
{
	char field[32];

	/* Unit Cost. 9, 12 */
	snprintf(field, 13, "%s", line + 8);
	fields.pci_7 = add_dp(field, 4);

	/* Quantity. 88, 12 */
	snprintf(field, 13, "%s", line + 87);
	fields.pci_11 = add_dp(field, 4);

	/* Line Discount Amount. 104, 12 */
	snprintf(field, 13, "%s", line + 103);
	fields.pci_13 = add_dp(field, 2);

	/* Line VAT Amount. 128, 12 */
	snprintf(field, 13, "%s", line + 127);
	fields.pci_15 = add_dp(field, 2);

	/* Line Total Amount. 140, 12 */
	snprintf(field, 13, "%s", line + 139);
	fields.pci_16 = add_dp(field, 2);

	/* Original Line Total Amount. 153, 12 */
	snprintf(field, 13, "%s", line + 152);
	fields.pci_18 = add_dp(field, 2);
}

static void write_line(FILE *ofp)
{
	fprintf(ofp, "%s,%s,%s,%.2f,%.2f,%.2f,%.2f,%.4f,%.4f,%.2f,%.2f,%.2f,"
		"%.2f\n",
		fields.mr_18, fields.mr_3, fields.mr_8, fields.mr_15 * cra,
		fields.pc_10 * cra, fields.pc_11 * cra, fields.pc_22 * cra,
		fields.pci_7 * cra, fields.pci_11, fields.pci_13 * cra,
		fields.pci_15 * cra, fields.pci_16 * cra, fields.pci_18 * cra);
}

int main(int argc, char **argv)
{
	char line[BRIF_LINE_LEN + 1];
	char output_file[NAME_MAX + 1];
	FILE *ifp;
	FILE *ofp;
	bool done_pci = false;

	if (argc < 2)
		exit(EXIT_FAILURE);

	snprintf(output_file, sizeof(output_file), "%s.csv",
			argv[1] + strlen(argv[1]) - 12);
	ofp = fopen(output_file, "w");
	fprintf(ofp, "(MR) Orig. Trans Ref,(MR) Transaction Type,"
		"(MR) MID,(MR) Amount,(PC) Disc Amount,(PC) Freight Amount,"
		"(PC) Total Items Amount,(PCI) Unit Cost,(PCI) Quantity,"
		"(PCI) Line Discount Amount,(PCI) Line VAT Amount,"
		"(PCI) Line Total Amount,(PCI) Original Line Total Amount\n");

	ifp = fopen(argv[1], "r");
	while (fgets(line, sizeof(line), ifp) != NULL) {
		if (line[2] == 'S' || line[2] == 'R') {
			if (line[2] == 'R')
				cra = -1.0;
			else
				cra = 1.0;
			process_mr(line, ofp);
			done_pci = false;
		} else if (line[2] == 'P') {
			process_pc(line, ofp);
		} else if (line[2] == 'I') {
			if (done_pci) {
				/*
				 * Don't duplicate MR and PC values for each
				 * PCI line.
				 */
				fields.mr_15 = 0.0;
				fields.pc_10 = 0.0;
				fields.pc_11 = 0.0;
				fields.pc_22 = 0.0;
			}
			process_pci(line, ofp);
			write_line(ofp);
			done_pci = true;
		}
	}
	fclose(ifp);
	fclose(ofp);

	exit(EXIT_SUCCESS);
}
