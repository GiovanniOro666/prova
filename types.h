#ifndef TYPES_H
#define TYPES_H

typedef enum {
    RC_LOW_RISE, 
    RC_MID_RISE,
    URM_REG_LOW_RISE, 
    URM_REG_MID_RISE,
    URM_SS_LOW_RISE, 
    URM_SS_MID_RISE
} BuildingType;

typedef enum {
    MODERATE, 
    EXTENSIVE, 
    COMPLETE
} DamageState;

typedef struct {
    BuildingType type;
    DamageState state;
    float drift_limit;       
    float prob_threshold;    
} AlarmThreshold;

typedef struct {
    float *acc;           // Accelerazione
    float *acc_hp;        // High-pass
    float *acc_fir;       // FIR filtered
    float *vel_unf;       // Velocità unfilt
    float *vel_filt;      // Velocità filtrata
    float *disp;          // Spostamento
    int n_samples;
} SignalData;

typedef struct {
    int fs;               // Frequenza campionamento
    float dt;             // Intervallo temporale
    float hp_a, hp_b;     // Coefficienti HP filter
    int filter_len;       // Lunghezza kernel
    float *kernel;        // Kernel FIR
} FilterConfig;

typedef struct {
    float STA_len_s;      // Finestra STA
    float LTA_len_s;      // Finestra LTA
    float threshold;      // Soglia STA/LTA
    int trigger_idx;      // Indice trigger
    int triggered;        // Flag trigger
} TriggerParams;

typedef struct {
    float pgd_base;           // PGD massimo base
    float max_drift_abs;      // Drift assoluto max
    float max_drift_norm;     // Drift normalizzato max
    float max_prob;           // Probabilità massima
    int alarm_triggered;      // Flag allarme
    int alarm_idx;            // Indice allarme
} AnalysisResults;

#endif