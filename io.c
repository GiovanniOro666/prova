#include "io.h"
#include "config.h"
#include <stdio.h>

int read_acceleration_file(const char *filename, float *data, 
                           int max_samples, float unit_conversion) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("ERRORE: Impossibile aprire %s\n", filename);
        return -1;
    }
    
    int count = 0;
    float temp;
    while (count < max_samples && fscanf(fp, "%f", &temp) == 1) {
        data[count] = temp * unit_conversion;
        count++;
    }
    
    fclose(fp);
    return count;
}

void write_results(const char *filename, SignalData *top, SignalData *base,
                   float *drift, float *drift_norm, int n,
                   float dt, int alarm_idx) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return;
    
    fprintf(fp, "# Indice, Tempo(s), Drift_abs(m), Drift_norm(mm/m), "
                "Disp_TOP(m), Disp_BASE(m), Allarme\n");
    
    for (int i = 0; i < n; i++) {
        char alarm_flag = (i == alarm_idx) ? 'R' : ' ';
        fprintf(fp, "%d, %.6f, %.8e, %.6f, %.8e, %.8e, %c\n",
                i+1, i*dt, drift[i], drift_norm[i] * 1000,
                top->disp[i], base->disp[i], alarm_flag);
    }
    
    fclose(fp);
}

void print_input_statistics(int n_samples, float dt, 
                            float pga_top, float pga_base) {
    printf("\nSTATISTICHE INPUT:\n");
    printf("  Campioni: %d (durata: %.1f s)\n", n_samples, n_samples * dt);
    printf("  PGA TOP:  %.3f g (%.3f m/s²)\n", pga_top / G_TO_MS2, pga_top);
    printf("  PGA BASE: %.3f g (%.3f m/s²)\n", pga_base / G_TO_MS2, pga_base);
}

void print_final_report(AnalysisResults *results, AlarmThreshold *threshold) {
    printf("\n========== REPORT FINALE ==========\n");
    printf("PGD massimo: %.5f m\n", results->pgd_base);
    printf("Drift assoluto: %.5f m\n", results->max_drift_abs);
    printf("Drift normalizzato: %.2f mm/m\n", results->max_drift_norm * 1000);
    printf("Probabilità massima: %.2f%%\n", results->max_prob * 100.0f);
    
    if (results->alarm_triggered) {
        printf("\n✓ ALLARME ATTIVATO\n");
    } else {
        printf("\n✗ NESSUN ALLARME\n");
        if (results->max_prob > threshold->prob_threshold * 0.7) {
            printf("  Probabilità vicina alla soglia\n");
        }
    }
}