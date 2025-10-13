#ifndef FILTERS_H
#define FILTERS_H

#include "types.h"

// Inizializza configurazione filtri
void init_filter_config(FilterConfig *config, int fs);

// Crea kernel FIR gaussiano
void create_gaussian_kernel(float *kernel, int len, float *sum);

// Applica filtro high-pass
void apply_highpass_filter(float *input, float *output, int n, 
                           float hp_a, float hp_b);

// Applica filtro FIR
void apply_fir_filter(float *input, float *output, int n, 
                      float *kernel, int kernel_len);

// Libera risorse
void cleanup_filter_config(FilterConfig *config);

#endif