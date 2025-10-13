#ifndef SIGNAL_PROCESSING_H
#define SIGNAL_PROCESSING_H

#include "types.h"

// Alloca dati segnale
SignalData* create_signal_data(int n_samples);

// Libera dati segnale
void free_signal_data(SignalData *data);

// Inizializza array a zero
void init_signal_arrays(SignalData *data);

// Integra accelerazione per ottenere velocità
void integrate_to_velocity(float *acc, float *vel_unf, float *vel_filt,
                           int start, int end, float dt, 
                           float hp_a, float hp_b);

// Integra velocità per ottenere spostamento
void integrate_to_displacement(float *vel, float *disp,
                               int start, int end, float dt);

// Calcola PGA
float calculate_pga(float *data, int n);

#endif