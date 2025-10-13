#include "filters.h"
#include <stdlib.h>
#include <math.h>
#include <omp.h>

void init_filter_config(FilterConfig *config, int fs) {
    config->fs = fs;
    config->dt = 1.0f / fs;
    
    // Coefficienti HP filter
    switch (fs) {
        case 128: 
            config->hp_b = 0.9981626f; 
            config->hp_a = 0.99632521f; 
            break;
        case 100: 
            config->hp_b = 0.99764934f; 
            config->hp_a = 0.99529868f; 
            break;
        case 200: 
            config->hp_b = 0.99882329f; 
            config->hp_a = 0.99764658f; 
            break;
        default: 
            config->hp_a = config->hp_b = 0.0f;
            return;
    }
    
    // Alloca e crea kernel
    config->filter_len = 2 * fs;
    config->kernel = (float*)malloc(config->filter_len * sizeof(float));
    
    float sum;
    create_gaussian_kernel(config->kernel, config->filter_len, &sum);
}

void create_gaussian_kernel(float *kernel, int len, float *sum) {
    float temp_sum = 0.0f;  // ← Variabile locale
    
    #pragma omp parallel for reduction(+:temp_sum)
    for (int k = 0; k < len; k++) {
        float gg = expf(-powf((float)(len - 1 - k) / (0.050f * len), 2));
        kernel[k] = gg;
        temp_sum += gg;
    }
    
    *sum = temp_sum;  // ← Assegna il risultato
    
    // Normalizza
    #pragma omp parallel for
    for (int k = 0; k < len; k++) {
        kernel[k] /= temp_sum;
    }
}

void apply_highpass_filter(float *input, float *output, int n, 
                           float hp_a, float hp_b) {
    output[0] = 0.0f;
    for (int i = 1; i < n; i++) {
        output[i] = input[i] * hp_b - input[i-1] * hp_b + hp_a * output[i-1];
    }
}

void apply_fir_filter(float *input, float *output, int n, 
                      float *kernel, int kernel_len) {
    #pragma omp parallel for
    for (int i = kernel_len; i < n; i++) {
        float accu = 0.0f;
        for (int k = 0; k < kernel_len; k++) {
            accu += input[i-k] * kernel[k];
        }
        output[i] = accu;
    }
}

void cleanup_filter_config(FilterConfig *config) {
    if (config->kernel) {
        free(config->kernel);
        config->kernel = NULL;
    }
}