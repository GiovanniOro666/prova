#ifndef TRIGGER_H
#define TRIGGER_H

#include "types.h"

// Inizializza parametri trigger
void init_trigger_params(TriggerParams *params, float sta_s, float lta_s);

// Cerca trigger STA/LTA
int find_trigger(float *signal, int n, TriggerParams *params, 
                 FilterConfig *filter_cfg);

#endif