/* Algoritmo VAD (Voice Activity Detection) - Riccardo Fila */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#define SAMPLES 160 // Numero di campioni in un pacchetto
#define CHECKS 8 // Numero di campioni da controllare in ogni pacchetto (8 vuol dire uno ogni 20), dev'essere un sottomultiplo di SAMPLES
#define MINMEAN 35 // Valore minimo (fino a 255) di un campione con contenuto vocale

uint8_t zeros[SAMPLES], packet[SAMPLES]; // Dichiaro due array globali (quindi inizializzati a 0)

int main(int argc, char **argv) {
	// Apertura dei file
	if (argc != 2) {
		printf("Inserire come parametro il numero del file da analizzare.\n");
		return -1;
	}
	FILE *datai, *datao, *res;
	char nf1[20], nf2[20], nf3[20];
	strcpy(nf1, "inputaudio");
	strcat(nf1, argv[1]);
	strcat(nf1, ".data");
	strcpy(nf2, "outputVAD");
	strcat(nf2, argv[1]);
	strcpy(nf3, nf2);
	strcat(nf2, ".data");
	strcat(nf3, ".txt");
	if ((datai = fopen(nf1, "rb")) == NULL) {
		printf("Errore di lettura del file %s\n", nf1);
		return -1;
	}
	datao = fopen(nf2, "wb");
	res = fopen(nf3, "w");
	
	uint8_t sample; // Campione analizzato singolarmente
	int i = 0; // Contatore dei campioni all'interno di un pacchetto
	int sum = 0; // Somma dei valori quantizzati dei soli pacchetti analizzati
	
	// Scorrimento del file di ingresso finché tutti i pacchetti non sono stati analizzati
	fseek(datai, SAMPLES/CHECKS - 1, SEEK_CUR);
	while (fread(&sample, 1, 1, datai)) {
		sum += sample;
		i++;
		if (i == CHECKS) { // Se è arrivato alla fine del pacchetto
			if (sum/CHECKS < MINMEAN) {
				fprintf(res, "%d", 0);
				fwrite(zeros, 1, SAMPLES, datao);
			} else {
				fprintf(res, "%d", 1);
				fseek(datai, -SAMPLES, SEEK_CUR); // Riposiziona il puntatore all'inizio del pacchetto per rileggerlo e ricopiarlo in output
				fread(packet, 1, SAMPLES, datai);
				fwrite(packet, 1, SAMPLES, datao);
			}
			i = 0; // Prepara per il pacchetto seguente
			sum = 0;
		}
		fseek(datai, SAMPLES/CHECKS - 1, SEEK_CUR);
	}
	
	// Chiusura dei file
	fclose(datai);
	fclose(datao);
	fclose(res);
}
