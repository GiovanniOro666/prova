#ifndef IO_H
#define IO_H

#include "types.h"

// Leggi file accelerazioni
int read_acceleration_file(const char *filename, float *data, 
                           int max_samples, float unit_conversion);

// Scrivi risultati
void write_results(const char *filename, SignalData *top, SignalData *base,
                   float *drift, float *drift_norm, int n,
                   float dt, int alarm_idx);

// Stampa statistiche input
void print_input_statistics(int n_samples, float dt, 
                            float pga_top, float pga_base);

// Stampa report finale
void print_final_report(AnalysisResults *results, AlarmThreshold *threshold);

#endif