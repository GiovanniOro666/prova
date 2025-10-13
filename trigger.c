// trigger.c
#include "trigger.h"
#include <math.h>
#include <stdio.h>

void init_trigger_params(TriggerParams *params, float sta_s, float lta_s) {
    params->STA_len_s = sta_s;
    params->LTA_len_s = lta_s;
    params->threshold = 4.0f;
    params->trigger_idx = -1;
    params->triggered = 0;
}

int find_trigger(float *signal, int n, TriggerParams *params, 
                 FilterConfig *filter_cfg) {
    int sta_len = (int)(params->STA_len_s * filter_cfg->fs);
    int lta_len = (int)(params->LTA_len_s * filter_cfg->fs);
    int start_idx = filter_cfg->filter_len + lta_len - 1;
    
    float sta_sum = 0.0f;
    float lta_sum = 0.0f;
    
    // Inizializza finestre
    for (int i = start_idx - lta_len + 1; i <= start_idx; i++) {
        lta_sum += fabsf(signal[i]);
        if (i >= start_idx - sta_len + 1) {
            sta_sum += fabsf(signal[i]);
        }
    }
    
    // Cerca trigger
    for (int i = start_idx; i < n; i++) {
        if (i > start_idx) {
            sta_sum += fabsf(signal[i]) - fabsf(signal[i - sta_len]);
            lta_sum += fabsf(signal[i]) - fabsf(signal[i - lta_len]);
        }
        
        float sta_avg = sta_sum / sta_len;
        float lta_avg = lta_sum / lta_len;
        float ratio = (lta_avg > 1e-9f) ? sta_avg / lta_avg : 0.0f;
        
        if (ratio > params->threshold) {
            params->trigger_idx = i;
            params->triggered = 1;
            printf("✓ TRIGGER: indice=%d, t=%.3fs, STA/LTA=%.2f\n\n", 
                   i, i * filter_cfg->dt, ratio);
            return 1;
        }
    }
    
    printf("✗ NESSUN TRIGGER\n");
    return 0;
}