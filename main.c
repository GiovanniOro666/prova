#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "types.h"
#include "filters.h"
#include "signal_processing.h"
#include "trigger.h"
#include "drift_analysis.h"
#include "io.h"

// Definizioni costanti
const float BUILDING_HEIGHT_M = 10.0f;
const int INPUT_UNIT_IS_G = 1;
const float G_TO_MS2 = 9.81f;
const float REG_INTERCEPT = -1.01f;
const float REG_SLOPE = 0.59f;
const float PRED_STD_DEV_LOG10 = 0.25f;

const AlarmThreshold thresholds[] = {
    {RC_LOW_RISE, MODERATE,  0.0184f, 20.86f},
    {RC_LOW_RISE, EXTENSIVE, 0.0301f, 15.53f},
    {RC_LOW_RISE, COMPLETE,  0.0451f, 16.52f},
    {RC_MID_RISE, MODERATE,  0.0223f, 17.20f},
    {RC_MID_RISE, EXTENSIVE, 0.0449f, 16.60f},
    {RC_MID_RISE, COMPLETE,  0.0674f, 10.46f},
    {URM_REG_LOW_RISE, MODERATE,  0.0028f, 13.11f},
    {URM_REG_LOW_RISE, EXTENSIVE, 0.0138f, 12.63f},
    {URM_REG_LOW_RISE, COMPLETE,  0.0236f, 19.70f},
    {URM_REG_MID_RISE, MODERATE,  0.0062f, 26.46f},
    {URM_REG_MID_RISE, EXTENSIVE, 0.0219f, 17.28f},
    {URM_REG_MID_RISE, COMPLETE,  0.0350f, 13.55f},
    {URM_SS_LOW_RISE, MODERATE,   0.0019f, 17.75f},
    {URM_SS_LOW_RISE, EXTENSIVE,  0.0085f, 20.27f},
    {URM_SS_LOW_RISE, COMPLETE,   0.0140f, 12.82f},
    {URM_SS_MID_RISE, MODERATE,   0.0042f, 36.82f},
    {URM_SS_MID_RISE, EXTENSIVE,  0.0135f, 12.36f},
    {URM_SS_MID_RISE, COMPLETE,   0.0210f, 18.30f}
};
const int NUM_THRESHOLDS = sizeof(thresholds) / sizeof(AlarmThreshold);

// Nomi per output
const char* building_names[] = {
    "RC Low-Rise (cemento armato, 1-3 piani)",
    "RC Mid-Rise (cemento armato, 4-7 piani)",
    "URM Regular Low-Rise (muratura regolare, 1-3 piani)",
    "URM Regular Mid-Rise (muratura regolare, 4-7 piani)",
    "URM Soft-Story Low-Rise (muratura piano morbido, 1-3 piani)",
    "URM Soft-Story Mid-Rise (muratura piano morbido, 4-7 piani)"
};

const char* damage_names[] = {
    "Moderate (danno moderato)",
    "Extensive (danno esteso)",
    "Complete (collasso completo)"
};

// Funzioni per menu interattivo
void print_building_types() {
    printf("\n========== TIPOLOGIE EDIFICIO ==========\n");
    for (int i = 0; i < 6; i++) {
        printf("  %d - %s\n", i, building_names[i]);
    }
}

void print_damage_states() {
    printf("\n========== STATI DI DANNO ==========\n");
    for (int i = 0; i < 3; i++) {
        printf("  %d - %s\n", i, damage_names[i]);
    }
}

BuildingType get_building_type() {
    int choice;
    print_building_types();
    printf("\nSeleziona tipo edificio (0-5): ");
    if (scanf("%d", &choice) != 1 || choice < 0 || choice > 5) {
        printf("⚠ Scelta non valida, uso default: RC Low-Rise\n");
        // Pulisci buffer input
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        return RC_LOW_RISE;
    }
    return (BuildingType)choice;
}

DamageState get_damage_state() {
    int choice;
    print_damage_states();
    printf("\nSeleziona stato danno (0-2): ");
    if (scanf("%d", &choice) != 1 || choice < 0 || choice > 2) {
        printf("⚠ Scelta non valida, uso default: Extensive\n");
        // Pulisci buffer input
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        return EXTENSIVE;
    }
    return (DamageState)choice;
}

float get_building_height() {
    float height;
    printf("\n========== ALTEZZA EDIFICIO ==========\n");
    printf("Inserisci altezza edificio in metri (es. 10.0): ");
    if (scanf("%f", &height) != 1 || height <= 0 || height > 200) {
        printf("⚠ Valore non valido, uso default: 10.0 m\n");
        // Pulisci buffer input
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        return 10.0f;
    }
    return height;
}

int get_input_unit() {
    int choice;
    printf("\n========== UNITÀ DI MISURA INPUT ==========\n");
    printf("  0 - m/s² (metri al secondo quadrato)\n");
    printf("  1 - g (accelerazione di gravità, 1g = 9.81 m/s²)\n");
    printf("\nSeleziona unità di misura (0-1): ");
    if (scanf("%d", &choice) != 1 || (choice != 0 && choice != 1)) {
        printf("⚠ Scelta non valida, uso default: g\n");
        // Pulisci buffer input
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        return 1;
    }
    return choice;
}

int get_sampling_frequency() {
    int fs;
    int choice;
    
    printf("\n========== FREQUENZA CAMPIONAMENTO ==========\n");
    printf("  1 - Frequenza predefinita (100, 128, 200 Hz)\n");
    printf("  2 - Frequenza personalizzata\n");
    printf("\nSeleziona opzione (1-2): ");
    
    if (scanf("%d", &choice) != 1 || (choice != 1 && choice != 2)) {
        printf("⚠ Scelta non valida, uso default: 128 Hz\n");
        // Pulisci buffer input
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        return 128;
    }
    
    if (choice == 1) {
        // Frequenze predefinite (con coefficienti ottimizzati)
        printf("\nFrequenze predefinite:\n");
        printf("  1 - 100 Hz\n");
        printf("  2 - 128 Hz (default)\n");
        printf("  3 - 200 Hz\n");
        printf("\nSeleziona (1-3): ");
        
        int freq_choice;
        if (scanf("%d", &freq_choice) != 1) {
            printf("⚠ Scelta non valida, uso default: 128 Hz\n");
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            return 128;
        }
        
        switch(freq_choice) {
            case 1: return 100;
            case 2: return 128;
            case 3: return 200;
            default:
                printf("⚠ Scelta non valida, uso default: 128 Hz\n");
                return 128;
        }
    } else {
        // Frequenza personalizzata
        printf("\n🎯 FREQUENZA PERSONALIZZATA\n");
        printf("Inserisci frequenza in Hz (10-1000, es. 250): ");
        
        if (scanf("%d", &fs) != 1 || fs < 10 || fs > 1000) {
            printf("⚠ Valore non valido (deve essere 10-1000 Hz)\n");
            printf("  Uso default: 128 Hz\n");
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            return 128;
        }
        
        printf("✓ Frequenza impostata: %d Hz\n", fs);
        return fs;
    }
}

void print_configuration_summary(BuildingType building_type, DamageState damage_state,
                                float building_height, int input_is_g, int fs,
                                float drift_limit, float prob_threshold) {
    printf("\n");
    printf("==========================================================\n");
    printf("               RIEPILOGO CONFIGURAZIONE\n");
    printf("==========================================================\n");
    printf("Parametri Edificio:\n");
    printf("  • Tipologia: %s\n", building_names[building_type]);
    printf("  • Altezza: %.1f m\n", building_height);
    printf("  • Stato danno monitorato: %s\n", damage_names[damage_state]);
    printf("\nParametri Acquisizione:\n");
    printf("  • Frequenza campionamento: %d Hz\n", fs);
    printf("  • Unità input: %s\n", input_is_g ? "g (gravità)" : "m/s²");
    printf("\nSoglie Allarme:\n");
    printf("  • Drift limite: %.4f (%.2f mm/m)\n", drift_limit, drift_limit * 1000);
    printf("  • Probabilità soglia: %.2f%%\n", prob_threshold * 100.0f);
    printf("==========================================================\n\n");
}

int main() {
    char filein_top[256], filein_base[256];
    char fileout_csv[256], fileout_debug[256];
    
    printf("==========================================================\n");
    printf("  DOSEWS - Sistema di Allerta Sismica per Edifici\n");
    printf("  Damage-based On-Site Early Warning System\n");
    printf("==========================================================\n\n");
    
    // CONFIGURAZIONE INTERATTIVA
    printf("Inizia la configurazione del sistema...\n");
    
    BuildingType building_type = get_building_type();
    DamageState damage_state = get_damage_state();
    float building_height = get_building_height();
    int input_is_g = get_input_unit();
    int fs = get_sampling_frequency();
    
    // Parametri fissi (possono essere resi configurabili se necessario)
    float sta_s = 0.5f;   // Short Term Average window
    float lta_s = 6.0f;   // Long Term Average window
    float ptm_s = 10.0f;  // Post-Trigger Monitoring window
    
    // Trova soglie per la configurazione scelta
    float drift_limit, prob_threshold;
    if (!get_alarm_thresholds(building_type, damage_state,
                              &drift_limit, &prob_threshold)) {
        printf("\n❌ ERRORE: Soglie non trovate per la configurazione selezionata\n");
        return 1;
    }
    
    // Mostra riepilogo configurazione
    print_configuration_summary(building_type, damage_state, building_height,
                               input_is_g, fs, drift_limit, prob_threshold);
    
    printf("Premere INVIO per continuare...");
    getchar(); // Consuma il newline lasciato da scanf
    getchar(); // Aspetta INVIO
    
    // Inizializza filtri
    FilterConfig filter;
    init_filter_config(&filter, fs);
    
    if (filter.hp_a == 0.0f) {
        printf("❌ ERRORE: Frequenza non supportata (%d Hz)\n", fs);
        return 1;
    }
    
    // Alloca dati
    SignalData *top = create_signal_data(MAX_SAMPLES);
    SignalData *base = create_signal_data(MAX_SAMPLES);
    
    if (!top || !base) {
        printf("❌ ERRORE: Impossibile allocare memoria\n");
        return 1;
    }
    
    // Leggi file
    float unit_conv = input_is_g ? G_TO_MS2 : 1.0f;
    
    printf("\n========== CARICAMENTO DATI ==========\n");
    printf("File accelerazioni TOP (tetto/sommità edificio): ");
    if (scanf("%255s", filein_top) != 1) {
        printf("❌ ERRORE di input\n");
        free_signal_data(top);
        free_signal_data(base);
        cleanup_filter_config(&filter);
        return 1;
    }
    
    int n_top = read_acceleration_file(filein_top, top->acc, 
                                       MAX_SAMPLES, unit_conv);
    if (n_top < 0) {
        printf("❌ Impossibile leggere il file TOP\n");
        free_signal_data(top);
        free_signal_data(base);
        cleanup_filter_config(&filter);
        return 1;
    }
    
    printf("File accelerazioni BASE (fondazione/base edificio): ");
    if (scanf("%255s", filein_base) != 1) {
        printf("❌ ERRORE di input\n");
        free_signal_data(top);
        free_signal_data(base);
        cleanup_filter_config(&filter);
        return 1;
    }
    
    int n_base = read_acceleration_file(filein_base, base->acc,
                                        MAX_SAMPLES, unit_conv);
    if (n_base < 0) {
        printf("❌ Impossibile leggere il file BASE\n");
        free_signal_data(top);
        free_signal_data(base);
        cleanup_filter_config(&filter);
        return 1;
    }
    
    if (n_top != n_base) {
        printf("⚠ ATTENZIONE: File con lunghezze diverse (TOP=%d, BASE=%d)\n", 
               n_top, n_base);
    }
    
    int n = (n_top < n_base) ? n_top : n_base;
    top->n_samples = base->n_samples = n;
    
    // Statistiche
    float pga_top = calculate_pga(top->acc, n);
    float pga_base = calculate_pga(base->acc, n);
    print_input_statistics(n, filter.dt, pga_top, pga_base);
    
    // Prepara nomi file output
    snprintf(fileout_csv, sizeof(fileout_csv), "%s_results.csv", filein_top);
    snprintf(fileout_debug, sizeof(fileout_debug), "%s_debug.txt", filein_top);
    
    // Elaborazione segnali
    printf("\n========== ELABORAZIONE SEGNALI ==========\n");
    printf("Applicazione filtri high-pass e FIR...\n");
    
    apply_highpass_filter(top->acc, top->acc_hp, n, filter.hp_a, filter.hp_b);
    apply_fir_filter(top->acc_hp, top->acc_fir, n, filter.kernel, filter.filter_len);
    
    apply_highpass_filter(base->acc, base->acc_hp, n, filter.hp_a, filter.hp_b);
    apply_fir_filter(base->acc_hp, base->acc_fir, n, filter.kernel, filter.filter_len);
    
    printf("✓ Filtri applicati con successo\n");
    
    // Prepara struttura soglie allarme
    AlarmThreshold alarm_threshold;
    alarm_threshold.type = building_type;
    alarm_threshold.state = damage_state;
    alarm_threshold.drift_limit = drift_limit;
    alarm_threshold.prob_threshold = prob_threshold;
    
    // Trigger
    printf("\n========== RICERCA TRIGGER ==========\n");
    printf("Parametri: STA=%.1fs, LTA=%.1fs, Soglia=4.0\n", sta_s, lta_s);
    
    TriggerParams trigger;
    init_trigger_params(&trigger, sta_s, lta_s);
    
    if (!find_trigger(top->acc_fir, n, &trigger, &filter)) {
        printf("\n========== RISULTATO ==========\n");
        printf("⚪ Nessun evento sismico rilevato\n");
        printf("   (Rapporto STA/LTA non supera la soglia di trigger)\n");
        goto cleanup;
    }
    
    // Analisi drift post-trigger
    printf("\n========== ANALISI DRIFT POST-TRIGGER ==========\n");
    printf("Finestra analisi: %.1f secondi\n", ptm_s);
    printf("Altezza normalizzazione: %.2f m (2/3 di %.1f m)\n", 
           (2.0f/3.0f) * building_height, building_height);
    
    AnalysisResults results;
    perform_drift_analysis(top, base, &trigger, &filter, ptm_s,
                          building_height, &alarm_threshold,
                          &results, fileout_debug);
    
    // Report finale
    print_final_report(&results, &alarm_threshold);
    
    // Salva risultati completi
    printf("\n========== SALVATAGGIO RISULTATI ==========\n");
    
    float *drift = (float*)malloc(n * sizeof(float));
    float *drift_norm = (float*)malloc(n * sizeof(float));
    
    if (drift && drift_norm) {
        float norm_height = (2.0f / 3.0f) * building_height;
        for (int i = 0; i < n; i++) {
            drift[i] = top->disp[i] - base->disp[i];
            drift_norm[i] = drift[i] / norm_height;
        }
        
        write_results(fileout_csv, top, base, drift, drift_norm, n,
                      filter.dt, results.alarm_idx);
        
        printf("✓ File risultati: %s\n", fileout_csv);
        printf("✓ File debug: %s\n", fileout_debug);
        
        free(drift);
        free(drift_norm);
    }
    
    printf("\n==========================================================\n");
    if (results.alarm_triggered) {
        printf("  🔴 STATO FINALE: ALLARME ROSSO ATTIVATO\n");
    } else {
        printf("  🟢 STATO FINALE: NESSUN ALLARME\n");
    }
    printf("==========================================================\n");
    
cleanup:
    free_signal_data(top);
    free_signal_data(base);
    cleanup_filter_config(&filter);
    
    return 0;
}