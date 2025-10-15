#include "filters.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <omp.h>

// Calcola coefficienti filtro passa-alto per qualsiasi frequenza
static void calculate_highpass_coefficients(int fs, float *hp_a, float *hp_b) {
    const float fc = 0.075f;  // Frequenza di taglio in Hz
    const float PI = 3.14159265358979323846f;
    
    // Calcolo usando trasformata bilineare
    float rc = 1.0f / (2.0f * PI * fc);  // Costante di tempo RC
    float dt = 1.0f / (float)fs;          // Periodo di campionamento
    float alpha = rc / (rc + dt);         // Coefficiente alpha
    
    *hp_a = alpha;                        // Coefficiente feedback (y[n-1])
    *hp_b = (1.0f + alpha) / 2.0f;       // Coefficiente feedforward (x[n] e x[n-1])
}

void init_filter_config(FilterConfig *config, int fs) {
    config->fs = fs;
    config->dt = 1.0f / fs;
    
    // Verifica range frequenza
    if (fs < 10 || fs > 1000) {
        printf("⚠ Frequenza fuori range (10-1000 Hz)\n");
        config->hp_a = config->hp_b = 0.0f;
        return;
    }
    
    // Usa coefficienti predefiniti per le frequenze standard (più accurati)
    // altrimenti calcola automaticamente
    switch (fs) {
        case 128: 
            config->hp_b = 0.9981626f; 
            config->hp_a = 0.99632521f;
            printf("✓ Usando coefficienti predefiniti per 128 Hz\n");
            break;
        case 100: 
            config->hp_b = 0.99764934f; 
            config->hp_a = 0.99529868f;
            printf("✓ Usando coefficienti predefiniti per 100 Hz\n");
            break;
        case 200: 
            config->hp_b = 0.99882329f; 
            config->hp_a = 0.99764658f;
            printf("✓ Usando coefficienti predefiniti per 200 Hz\n");
            break;
        default:
            // Calcola coefficienti per frequenza custom
            calculate_highpass_coefficients(fs, &config->hp_a, &config->hp_b);
            printf("✓ Calcolati coefficienti per %d Hz (custom)\n", fs);
            printf("  hp_a = %.8f\n", config->hp_a);
            printf("  hp_b = %.8f\n", config->hp_b);
            break;
    }
    
    // Alloca e crea kernel
    config->filter_len = 2 * fs;
    config->kernel = (float*)malloc(config->filter_len * sizeof(float));
    
    float sum;
    create_gaussian_kernel(config->kernel, config->filter_len, &sum);
    
    printf("  Dimensione kernel: %d campioni (2 secondi)\n", config->filter_len);
}

void create_gaussian_kernel(float *kernel, int len, float *sum) {
    float temp_sum = 0.0f;
    
    #pragma omp parallel for reduction(+:temp_sum)
    for (int k = 0; k < len; k++) {
        float gg = expf(-powf((float)(len - 1 - k) / (0.050f * len), 2));
        kernel[k] = gg;
        temp_sum += gg;
    }
    
    *sum = temp_sum;
    
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