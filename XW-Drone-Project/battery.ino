// Medición de voltaje de batería
// Battery voltage monitoring

#include "board_config.h"

#define VBAT_ADC_PIN       BOARD_VBAT_ADC_PIN  // Pin de muestreo del voltaje de batería
#define VBAT_ADC_SAMPLES   16                  // Promedio de varias muestras ADC, más alto = menos ruido pero respuesta más lenta
#define VBAT_DIVIDER       (43.0f / 33.0f)     // Relación del divisor: R arriba 10kΩ + R abajo 33kΩ, fórmula Vbat = Vadc × 43/33
#define VBAT_WARN_THRESHOLD     3.5f           // L1: umbral de aviso en reposo/ralentí (V)
#define VBAT_LOW_THRESHOLD      3.3f           // L2: umbral de aviso de batería baja en vuelo (V)
#define VBAT_CRITICAL_THRESHOLD 3.0f           // L3: umbral crítico en vuelo, aterrizaje automático (V)
#define VBAT_ABSENT_THRESHOLD   0.5f           // Por debajo de este valor se considera sin batería, ignorar todos los avisos

#define BATTERY_FLYING_THRUST_MIN    0.15f     // Empuje ≥ a este valor se considera en vuelo, L1 no aplica
#define BATTERY_ACTION_DEBOUNCE_TIME  0.9f     // Tiempo anti-rebote de detección continua de bajo voltaje (segundos)

extern float t;
float batteryVoltage = 0.0f;  // Último voltaje global, actualizado periódicamente por updateBatteryVoltage()

void updateBatteryVoltage() {
	static float lastCheck = 0;
	if (t - lastCheck < 0.5f) return;  // Muestrear cada 0.5 segundos
	lastCheck = t;
	batteryVoltage = readBatteryVoltage();
}

float readBatteryVoltage() {
	long sum = 0;
	for (int i = 0; i < VBAT_ADC_SAMPLES; i++) sum += analogReadMilliVolts(VBAT_ADC_PIN);
	return (sum / (float)VBAT_ADC_SAMPLES / 1000.0f) * VBAT_DIVIDER;
}