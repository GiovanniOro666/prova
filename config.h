#ifndef CONFIG_H
#define CONFIG_H

#define MAX_SAMPLES 500000
#define MAX_KERNEL 400

// Configurazione edificio
extern const float BUILDING_HEIGHT_M;
extern const int INPUT_UNIT_IS_G;

// Costanti fisiche
extern const float G_TO_MS2;

// Parametri regressione
extern const float REG_INTERCEPT;
extern const float REG_SLOPE;
extern const float PRED_STD_DEV_LOG10;

#endif