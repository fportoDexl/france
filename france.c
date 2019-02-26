//  __  __   __        __  __   FRAgmeNtador de Catalogos Espaciais
// |__ |__| |__| |\ | |   |__   (Space Catalogues Fragmenter)
// |   |\   |  | | \| |__ |__   Daniel Gaspar - daniel@gaspar.ws
//                              DEXL Lab - LNCC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 200

void error(int value, char* msg) {
	fprintf(stderr, "Error: %s (#%d)\n", msg, value);
	exit(value);
}

unsigned int countTotal(FILE * inputFile) {
	unsigned int qtyLines = 0;
	char line[200];

	fseek(inputFile, 0, SEEK_SET);

	while (fgets(line, sizeof(line), inputFile) != NULL) {
		qtyLines++;
	}

	return qtyLines;
}

void countBetween2(FILE * inputFile, double startRAL, double endRAL,
		double startRAR, double endRAR, unsigned int *qtyIntervalL,
		unsigned int *qtyIntervalR) {
	double currRA;
	(*qtyIntervalL) = 0;
	(*qtyIntervalR) = 0;
	char line[200], *tok, *id;

	fseek(inputFile, 0, SEEK_SET);

	while (fgets(line, sizeof(line), inputFile) != NULL) {
		if((id = strtok(line, ";,")) == NULL)
			error(3, "Invalid input file.");

		if((tok = strtok(NULL, ";,")) == NULL)
			error(4, "Invalid input file.");
		currRA = atof(tok);
		if (currRA > startRAL && currRA <= endRAL)
			(*qtyIntervalL)++;
		if (currRA < startRAR && currRA >= endRAR)
			(*qtyIntervalR)++;

	}
}

unsigned int countBetween(FILE * inputFile, double startRAL, double endRAL) {
	double currRA;
	unsigned int qtyObjs = 0;
	char line[200], *tok, *id;

	fseek(inputFile, 0, SEEK_SET);

	while (fgets(line, sizeof(line), inputFile) != NULL) {
		if((id = strtok(line, ";,")) == NULL)
			error(3, "Invalid input file.");

		if((tok = strtok(NULL, ";,")) == NULL)
			error(4, "Invalid input file.");
		currRA = atof(tok);
		if (currRA > startRAL && currRA <= endRAL) {
			qtyObjs++;
		}
	}
	return qtyObjs;
}

void calculate_border(char *msg, double *border, double start, double end, double neighbor_limit) {
	int count = 0;
	double border_limit = neighbor_limit / 3600;

	if (start == 0) {
		border[0] = 360 - border_limit;
		border[1] = 360;
	} else {
		if (start - border_limit <= 0) {
			border[0] = 360 - (border_limit + (0 - start));
			border[1] = 360;
			border[2] = 0;
			border[3] = start;
			count = 1;
		} else {
			border[0] = start - border_limit;
			border[1] = start;
		}
	}

	if (end == 360) {
		border[4] = 0;
		border[5] = 0 + border_limit;

	} else {
		if (end + border_limit >= 360) {
			border[4] = end;
			border[5] = 360;
			border[6] = 0;
			border[7] = 0 + (border_limit - (360 - end));
			count = count + 10;
		} else {
			border[4] = end;
			border[5] = end + border_limit;
		}
	}

	switch (count) {
	case 1:
		sprintf(msg, "%.5f,%.5f;%.5f,%.5f;[%.5f,%.5f];%.5f,%.5f\n", border[0],
				border[1], border[2], border[3], start, end, border[4],
				border[5]);
		break;

	case 10:
		sprintf(msg, "%.5f,%.5f;[%.5f,%.5f];%.5f,%.5f;%.5f,%.5f\n", border[0],
				border[1], start, end, border[4], border[5], border[6],
				border[7]);
		break;

	case 11:
		sprintf(msg, "%.5f,%.5f;%.5f,%.5f;[%.5f,%.5f];%.5f,%.5f;%.5f,%.5f\n",
				border[0], border[1], border[2], border[3], start, end,
				border[4], border[5], border[6], border[7]);
		break;

	default:
		sprintf(msg, "%.5f,%.5f;[%.5f,%.5f];%.5f,%.5f\n", border[0], border[1],
				start, end, border[4], border[5]);
	}
}

int main(int argc, char** argv) {
	FILE * inputFile;
	FILE * outputFile;
	unsigned int qtyObjs, qtyIntervalL, qtyIntervalR, objsPerBucket, epsilon,
			bucket, sumObjsL, sumObjsR;
	int qtyBuckets, qtyInter, offsetL, offsetR, verbose = 0, maxInter, errorL,
			errorR, i;
	double startRAL, endRAL, startRAR, endRAR, diffL, diffR;
	double border_ral[8] = { 0.0 };
	double border_rar[8] = { 0.0 };
	double *limits;

	// Chech for parameters
	if (argc < 5) {
		printf(
				"  __  __   __        __  __   FRAgmeNtador de Catalogos Espaciais\n");
		printf(" |__ |__| |__| |\\ | |   |__  (Space Catalogues Fragmenter)\n");
		printf(
				" |   |\\  |  | | \\| |__ |__  Daniel Gaspar - daniel@gaspar.ws\n");
		printf("                              DEXL Lab - LNCC\n");
		printf(" USAGE:\n\n %s INPUT_FILE QTY_OF_BUCKETS [-v]\n\n", argv[0]);
		return -1;
	}
	qtyBuckets = atoi(argv[2]);
	if (qtyBuckets <= 0)
		error(2, "Not valid quantity of buckets.");

	if (argc > 5 && (strcmp(argv[3], "-v") == 0))
		verbose = 1;

	if (verbose)
		printf("Verbose mode selected\n");

	limits = (double*) malloc((qtyBuckets - 1) * sizeof(double));
	if (limits == NULL)
		error(6, "Fail to allocate memory.");

	// Open input file
	if ((inputFile = fopen(argv[1], "r")) == NULL)
		error(1, "Fail to open input file");

	// Open input file
	if ((outputFile = fopen(argv[4], "w")) == NULL)
		error(1, "Fail to open output file");

	double neighbor_limit = atoi(argv[5]);

	qtyObjs = countTotal(inputFile);

	if (verbose)
		printf("Total space objects: %d\n", qtyObjs);
	if (qtyBuckets > qtyObjs)
		error(3, "There are more buckets than objects.");

	objsPerBucket = qtyObjs / qtyBuckets;
	if (verbose)
		printf("Average objects per bucket: %d\n", objsPerBucket);

	// Dynamic error
	epsilon = objsPerBucket * 0.005; // Let's say that 0,5% of the objsPerBucket is acceptable
	if (verbose)
		printf("Epsilon: %d\n", epsilon);

	maxInter = 50;  // Just to make sure we don't overrun

	endRAL = 0.0;
	endRAR = 360.0;
	offsetL = 0;
	offsetR = 0;
	sumObjsL = 0;
	sumObjsR = 0;

	char msg[qtyBuckets * MAX];
	for (bucket = 1; bucket < ((float) qtyBuckets / 2.0); bucket++) { // While there is at least one bucket to do
		if (verbose)
			fprintf(stderr, "\rRunning FRANCE over partitions... [%d%%]",
					(int) ((bucket - 1) * 100 / (int) (qtyBuckets / 2)));
		startRAL = endRAL;
		startRAR = endRAR;
		endRAL = endRAR = startRAL + (startRAR - startRAL) / 2;
		diffL = endRAL - startRAL;
		diffR = startRAR - endRAR;

		qtyInter = 1;
		countBetween2(inputFile, startRAL, endRAL, startRAR, endRAR,
				&qtyIntervalL, &qtyIntervalR);
		errorL = abs(qtyIntervalL - (objsPerBucket - offsetL));
		errorR = abs(qtyIntervalR - (objsPerBucket - offsetR));

		while (errorL > epsilon || errorR > epsilon) {
			diffL /= 2;
			diffR /= 2;
			if (qtyIntervalL < objsPerBucket)
				endRAL += diffL;
			else
				endRAL -= diffL;
			if (qtyIntervalR < objsPerBucket)
				endRAR -= diffR;
			else
				endRAR += diffR;
			qtyInter++;
			if (qtyInter >= maxInter)
				break;
			countBetween2(inputFile, startRAL, endRAL, startRAR, endRAR,
					&qtyIntervalL, &qtyIntervalR);
			errorL = abs(qtyIntervalL - (objsPerBucket - offsetL));
			errorR = abs(qtyIntervalR - (objsPerBucket - offsetR));
		}

		if (verbose) {
			printf(
					"startRAL: %.5f  endRAL: %.5f  qtyIntervalL: %d  qtyInter: %d  desired: %d\n",
					startRAL, endRAL, qtyIntervalL, qtyInter,
					objsPerBucket - offsetL);
			printf(
					"startRAR: %.5f  endRAR: %.5f  qtyIntervalR: %d  qtyInter: %d  desired: %d\n",
					startRAR, endRAR, qtyIntervalR, qtyInter,
					objsPerBucket - offsetR);
			char localMsg1[MAX], localMsg2[MAX];

			calculate_border(localMsg1, border_ral, startRAL, endRAL,
					neighbor_limit);

			if (startRAR > endRAR) {
				calculate_border(localMsg2, border_rar, endRAR, startRAR,
						neighbor_limit);
			} else {
				calculate_border(localMsg2, border_rar, startRAR, endRAR,
						neighbor_limit);
			}

			strcat(msg, localMsg1);
			strcat(msg, localMsg2);
		}

		limits[bucket - 1] = endRAL;
		limits[qtyBuckets - bucket - 1] = endRAR;

		sumObjsL += qtyIntervalL;
		sumObjsR += qtyIntervalR;
		offsetL = sumObjsL - bucket * objsPerBucket; // dynamic offset correction - avoid error propagation
		offsetR = sumObjsR - bucket * objsPerBucket;
	}

	if (qtyBuckets % 2 == 0) { // if qtyBuckets is even, there is two buckets left - let's FRANCE the first
		if (verbose)
			fprintf(stderr, "\rRunning FRANCE over partitions... [%d%%]",
					(int) ((bucket - 1) * 100 / (int) (qtyBuckets / 2)));
		startRAL = endRAL;
		endRAL = endRAR;
		diffL = endRAL - startRAL;

		qtyInter = 1;
		errorL = abs(
				(qtyIntervalL = countBetween(inputFile, startRAL, endRAL))
						- (objsPerBucket - offsetL));
		while (errorL > epsilon) {
			diffL /= 2;
			if (qtyIntervalL < objsPerBucket)
				endRAL += diffL;
			else
				endRAL -= diffL;
			qtyInter++;
			if (qtyInter >= maxInter)
				break;
			errorL = abs(
					(qtyIntervalL = countBetween(inputFile, startRAL, endRAL))
							- (objsPerBucket - offsetL));
		}

		if (verbose) {
			printf(
					"startRA: %.5f  endRA: %.5f  qtyInterval: %d  qtyInter: %d  desired: %d\n",
					startRAL, endRAL, qtyIntervalL, qtyInter,
					objsPerBucket - offsetL);
			char localMsg1[MAX];
			calculate_border(localMsg1, border_ral, startRAL, endRAL,
					neighbor_limit);
			strcat(msg, localMsg1);
		}

		limits[bucket - 1] = endRAL;

		sumObjsL += qtyIntervalL;
		offsetL = sumObjsL - bucket * objsPerBucket; // dynamic offset correction - avoid error propagation
	}

	if (verbose) {
		startRAL = endRAL;
		endRAL = endRAR;
		qtyIntervalL = countBetween(inputFile, startRAL, endRAL);
		printf("startRA: %.5f  endRA: %.5f  qtyInterval: %d \n", startRAL,
				endRAL, qtyIntervalL);
		char localMsg1[MAX];
		calculate_border(localMsg1, border_ral, startRAL, endRAL,
				neighbor_limit);
		strcat(msg, localMsg1);
	}

	if (!verbose) {
		fprintf(stderr, "\rRunning FRANCE over partitions... [done]\n");
		for (i = 0; i < qtyBuckets - 1; i++) {
			printf("%.5f\n", limits[i]);
		}

	}

	fprintf(outputFile, "%s", msg);

	fclose(inputFile);
	fclose(outputFile);

	return 0;
}
