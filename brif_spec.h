/* 
 * brif_spec.h
 *
 * Brif file format specification.
 * Field names and lengths
 *
 * Copyright (C) 2006-2010 Andrew Clayton <andrew@pccl.info>
 *
 * Released under the General Public License (GPL) version 2
 * See COPYING
 */

/* Main record line definitions */
static const char *mrn[] = { "Record Processed", "Record Type", 
		"Transaction Type", "Cont/Aux Following", "Live/Test", 
		"Transaction Date", "Transaction Time", "Merchant ID", 
		"Terminal ID", "Capture Method", "PAN", "Expiry Date", 
		"Start Date", "Issue No.", "Amount", "Cash Back Amount",
		"Card Track 2", "Originators Trans Ref", "Currency Code", 
		"Sort Code", "Auth Method", "User ID", "RESERVED", 
		"Card Scheme Code", "Network Terminal No.",
		"Transaction Number", "Status", "Auth Code", "Error No.", 
		"Error/Auth Message", "CRLF" };
static const int mrl[31][2] = { {0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1},
		{5, 8}, {13, 6}, {19, 15}, {34, 8}, {42, 1},
		{43, 19}, {62, 4}, {66, 4}, {70, 2}, {72, 12},
		{84, 12}, {96, 40}, {136, 25}, {161, 3}, {164, 1},
		{165, 1}, {166, 8}, {174, 9}, {183, 3}, {186, 4},
		{190, 11}, {201, 1}, {202, 8}, {210, 4}, {214, 84}, {298, 2} };

/* Purchasing card line definitions */
static const char *pcln[] = { "Record Processed", "Record Type", "Aux Type",
		"Continuation", "Page No.", "VAT Transaction Amount",
		"Customer VAT Reg No.", "Supplier Order Ref",
		"Order Date", "Discount Amount", "Freight Amount",
		"Dest Post Code", "Ship From Post Code",
		"Dest Country Code", "Freight VAT Rate",
		"Transaction VAT Status", "Customer Ref",
		"Customer Account No.", "Invoice No.",
		"Original Invoice No.", "Cost Centre",
		"Total Items Amount", "Filler", "CRLF" };
static const int pcl[24][2] = { {0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1},
		{5, 12}, {17, 13}, {30, 12}, {42, 8}, {50, 12},
		{62, 12}, {74, 10}, {84, 10}, {94, 3}, {97, 4},
		{101, 1}, {102, 20}, {122, 12}, {134, 15}, {149, 15},
		{164, 20}, {184, 12}, {196, 102}, {298, 2} };

/* Purchasing card line item definitions */
static const char *pciln[] = { "Record Processed", "Record Type", "Aux Type",
		"Continuation", "Page No.", "Item No.", "Unit Cost",
		"Unit of Measure", "Commodity Code", "Item Description",
		"Quantity", "VAT Rate", "Line Discount Amount",
		"Product Code", "Line VAT Amount", "Line Total Amount",
		"VAT Rate Type", "Original Line Total Amount",
		"Debit/Credit", "Filler", "CRLF" };
static const int pcil[21][2] = { {0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1},
		{5, 3}, {8, 12}, {20, 12}, {32, 15}, {47, 40},
		{87, 12}, {99, 4}, {103, 12}, {115, 12}, {127, 12},
		{139, 12}, {151, 1}, {152, 12}, {164, 1}, {165, 133},
		{298, 2} };
