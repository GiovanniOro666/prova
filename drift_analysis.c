#include "drift_analysis.h"
#include "signal_processing.h"
#include "config.h"
#include <math.h>
#include <stdio.h>

extern const AlarmThreshold thresholds[];
extern const int NUM_THRESHOLDS;

int get_alarm_thresholds(BuildingType type, DamageState state,
                         float *drift_limit, float *prob_threshold) {
    for (int i = 0; i < NUM_THRESHOLDS; i++) {
        if (thresholds[i].type == type && thresholds[i].state == state) {
            *drift_limit = thresholds[i].drift_limit;
            *prob_threshold = thresholds[i].prob_threshold / 100.0f;
            return 1;
        }
    }
    return 0;
}

float calculate_exceedance_probability(float pgd_base, float drift_limit) {
    if (pgd_base < 1e-9f) return 0.0f;
    
    float log10_pgd = log10f(pgd_base);
    float mean_log10_drift = REG_INTERCEPT + REG_SLOPE * log10_pgd;
    float log10_drift_limit = log10f(drift_limit);
    float z = (log10_drift_limit - mean_log10_drift) / 
              (PRED_STD_DEV_LOG10 * sqrtf(2.0f));
    float prob_not_exceeding = 0.5f * (1.0f + erff(z));
    
    return 1.0f - prob_not_exceeding;
}

void perform_drift_analysis(SignalData *top, SignalData *base,
                            TriggerParams *trigger, FilterConfig *filter,
                            float ptm_len_s, float building_height,
                            AlarmThreshold *threshold,
                            AnalysisResults *results,
                            const char *debug_file) {
    
    int start = trigger->trigger_idx;
    int ptm_len = (int)(ptm_len_s * filter->fs);
    int end = start + ptm_len;
    if (end > top->n_samples) end = top->n_samples;
    
    // Inizializza risultati
    results->pgd_base = 0.0f;
    results->max_drift_abs = 0.0f;
    results->max_drift_norm = 0.0f;
    results->max_prob = 0.0f;
    results->alarm_triggered = 0;
    results->alarm_idx = -1;
    
    // Altezza normalizzazione (2/3 per sensori solo top)
    float norm_height = (2.0f / 3.0f) * building_height;
    
    FILE *fdebug = fopen(debug_file, "w");
    if (fdebug) {
        fprintf(fdebug, "# t(s), PGD_base(m), Drift_abs(m), Drift_norm(mm/m), Prob(%%)\n");
    }
    
    int report_every = filter->fs;
    int next_report = start + report_every;
    
    printf("ANALISI POST-TRIGGER...\n");
    
    // ========== INTEGRAZIONE CORRETTA - COME NEL FORTRAN ==========
    // Inizializza al trigger
    top->vel_unf[start] = 0.0f;
    top->vel_filt[start] = 0.0f;
    top->disp[start] = 0.0f;
    base->vel_unf[start] = 0.0f;
    base->vel_filt[start] = 0.0f;
    base->disp[start] = 0.0f;
    
    // Loop di integrazione sequenziale - NON chiamare funzioni che resettano!
    for (int i = start + 1; i < end && !results->alarm_triggered; i++) {
        
        // ===== INTEGRAZIONE TOP =====
        // Integrazione trapezoidale acc -> velocità non filtrata
        top->vel_unf[i] = top->vel_unf[i-1] + 
                          (top->acc_hp[i-1] + top->acc_hp[i]) * 0.5f * filter->dt;
        
        // High-pass sulla velocità (rimuove offset)
        top->vel_filt[i] = top->vel_unf[i] * filter->hp_b - 
                           top->vel_unf[i-1] * filter->hp_b + 
                           filter->hp_a * top->vel_filt[i-1];
        
        // Integrazione trapezoidale velocità filtrata -> spostamento
        top->disp[i] = top->disp[i-1] + 
                       (top->vel_filt[i-1] + top->vel_filt[i]) * 0.5f * filter->dt;
        
        // ===== INTEGRAZIONE BASE =====
        // Integrazione trapezoidale acc -> velocità non filtrata
        base->vel_unf[i] = base->vel_unf[i-1] + 
                           (base->acc_hp[i-1] + base->acc_hp[i]) * 0.5f * filter->dt;
        
        // High-pass sulla velocità
        base->vel_filt[i] = base->vel_unf[i] * filter->hp_b - 
                            base->vel_unf[i-1] * filter->hp_b + 
                            filter->hp_a * base->vel_filt[i-1];
        
        // Integrazione trapezoidale velocità filtrata -> spostamento
        base->disp[i] = base->disp[i-1] + 
                        (base->vel_filt[i-1] + base->vel_filt[i]) * 0.5f * filter->dt;
        
        // ===== CALCOLO DRIFT E ANALISI =====
        float drift_abs = top->disp[i] - base->disp[i];
        float drift_norm = drift_abs / norm_height;
        
        // Aggiorna massimi
        float abs_disp_base = fabsf(base->disp[i]);
        if (abs_disp_base > results->pgd_base) {
            results->pgd_base = abs_disp_base;
        }
        
        if (fabsf(drift_abs) > results->max_drift_abs) {
            results->max_drift_abs = fabsf(drift_abs);
        }
        
        if (fabsf(drift_norm) > results->max_drift_norm) {
            results->max_drift_norm = fabsf(drift_norm);
        }
        
        // Calcola probabilità
        float prob = calculate_exceedance_probability(results->pgd_base, 
                                                      threshold->drift_limit);
        
        if (prob > results->max_prob) {
            results->max_prob = prob;
        }
        
        // Log debug
        if (fdebug) {
            fprintf(fdebug, "%.3f, %.6e, %.6e, %.2f, %.2f\n",
                    i * filter->dt, results->pgd_base, 
                    results->max_drift_abs, results->max_drift_norm * 1000,
                    prob * 100.0f);
        }
        
        // Report periodico
        if (i >= next_report) {
            printf("  T+%.1fs: PGD=%.5fm, Drift=%.2f mm/m, P=%.2f%%\n",
                   (i - start) * filter->dt, results->pgd_base,
                   results->max_drift_norm * 1000, prob * 100.0f);
            next_report += report_every;
        }
        
        // Check allarme
        if (prob > threshold->prob_threshold) {
            results->alarm_triggered = 1;
            results->alarm_idx = i;
            
            printf("\n*** ALLARME ROSSO! ***\n");
            printf("Tempo: %.3f s dopo trigger\n", (i - start) * filter->dt);
            printf("PGD base: %.5f m\n", results->pgd_base);
            printf("Drift normalizzato: %.2f mm/m\n", 
                   results->max_drift_norm * 1000);
            printf("Probabilità: %.2f%% > %.2f%%\n",
                   prob * 100.0f, threshold->prob_threshold * 100.0f);
            break;
        }
    }
    
    if (fdebug) fclose(fdebug);
}