#ifndef DRIFT_ANALYSIS_H
#define DRIFT_ANALYSIS_H

#include "types.h"

// Trova soglie per tipo edificio e danno
int get_alarm_thresholds(BuildingType type, DamageState state,
                         float *drift_limit, float *prob_threshold);

// Calcola probabilità di superamento
float calculate_exceedance_probability(float pgd_base, float drift_limit);

// Analisi post-trigger completa
void perform_drift_analysis(SignalData *top, SignalData *base,
                            TriggerParams *trigger, FilterConfig *filter,
                            float ptm_len_s, float building_height,
                            AlarmThreshold *threshold,
                            AnalysisResults *results,
                            const char *debug_file);

#endif
