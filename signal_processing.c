#include "signal_processing.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <omp.h>

SignalData* create_signal_data(int n_samples) {
    SignalData *data = (SignalData*)malloc(sizeof(SignalData));
    
    data->acc = (float*)calloc(n_samples, sizeof(float));
    data->acc_hp = (float*)calloc(n_samples, sizeof(float));
    data->acc_fir = (float*)calloc(n_samples, sizeof(float));
    data->vel_unf = (float*)calloc(n_samples, sizeof(float));
    data->vel_filt = (float*)calloc(n_samples, sizeof(float));
    data->disp = (float*)calloc(n_samples, sizeof(float));
    data->n_samples = n_samples;
    
    return data;
}

void free_signal_data(SignalData *data) {
    if (data) {
        free(data->acc);
        free(data->acc_hp);
        free(data->acc_fir);
        free(data->vel_unf);
        free(data->vel_filt);
        free(data->disp);
        free(data);
    }
}

void init_signal_arrays(SignalData *data) {
    #pragma omp parallel for
    for (int i = 0; i < data->n_samples; i++) {
        data->acc_hp[i] = 0.0f;
        data->acc_fir[i] = 0.0f;
        data->vel_unf[i] = 0.0f;
        data->vel_filt[i] = 0.0f;
        data->disp[i] = 0.0f;
    }
}

void integrate_to_velocity(float *acc, float *vel_unf, float *vel_filt,
                           int start, int end, float dt, 
                           float hp_a, float hp_b) {
    vel_unf[start] = 0.0f;
    vel_filt[start] = 0.0f;
    
    for (int i = start + 1; i < end; i++) {
        // Integrazione trapezoidale
        vel_unf[i] = vel_unf[i-1] + (acc[i-1] + acc[i]) * 0.5f * dt;
        
        // High-pass sulla velocità
        vel_filt[i] = vel_unf[i] * hp_b - vel_unf[i-1] * hp_b + 
                      hp_a * vel_filt[i-1];
    }
}

void integrate_to_displacement(float *vel, float *disp,
                               int start, int end, float dt) {
    disp[start] = 0.0f;
    
    for (int i = start + 1; i < end; i++) {
        // Integrazione trapezoidale
        disp[i] = disp[i-1] + (vel[i-1] + vel[i]) * 0.5f * dt;
    }
}

float calculate_pga(float *data, int n) {
    float pga = 0.0f;
    for (int i = 0; i < n; i++) {
        if (fabsf(data[i]) > pga) {
            pga = fabsf(data[i]);
        }
    }
    return pga;
}