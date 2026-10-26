// Protecciones de seguridad
// Funciones fail-safe

bool isInverted = false;  // ¿Está el cuerpo invertido actualmente? (cos Z < INVERTED_COS_THRESHOLD)

float rcLossTimeout = 1;        // Tiempo de espera de pérdida de RC (segundos), configurable con SF_RC_LOSS_TIME
float descendTime = 10;         // Tiempo de descenso hasta parada (segundos), configurable con SF_DESCEND_TIME
#define WEB_RC_LOSS_TIMEOUT_MS 8000UL  // Umbral de pérdida del control web (ms), debe ser mayor que el intervalo de latido de 2000 ms

// Parámetros de protección contra inversión
#define INVERTED_COS_THRESHOLD -0.7f   // cos(134°), inclinación superior a 134° se considera invertido (margen para deriva del giroscopio)
#define INVERTED_TIMEOUT        1.5f   // Invertido continuo superior a 1.5 s activa la parada

extern float controlTime;
extern float controlRoll, controlPitch, controlThrottle, controlYaw;

#if WEB_RC_ENABLED
// Declaraciones de variables del Web RC
extern bool webRCEnabled;
extern bool useWebRC;
extern unsigned long webRCLastUpdate;
bool isUsingWebRC();
#endif

extern bool armed;
extern int mode;
extern float dt;
extern float thrustTarget;
extern float t;
extern float batteryVoltage;  // battery.ino
extern Quaternion attitudeTarget;
extern Quaternion attitude;

void failsafe() {
	rcLossFailsafe();
#if WEB_RC_ENABLED
	webRCLossFailsafe();
#endif
	autoFailsafe();
	invertedFailsafe();
	batteryFailsafe();
}

// Fail-safe por pérdida de RC
void rcLossFailsafe() {
	if (controlTime == 0) return; // sin RC en absoluto
	if (!armed) return;
#if WEB_RC_ENABLED
	if (isUsingWebRC()) return; // WebRC gestiona su propio timeout (webRCLossFailsafe)
#endif
	if (t - controlTime > rcLossTimeout) {
		descend();
	}
}

// Descenso suave al perder el RC
void descend() {
	mode = AUTO;
	attitudeTarget = Quaternion();
	thrustTarget -= dt / descendTime;
	if (thrustTarget < 0) {
		thrustTarget = 0;
		armed = false;
	}
}

// Permitir al piloto interrumpir el vuelo automático
void autoFailsafe() {
	static float roll, pitch, yaw, throttle;
	
	// control* ya unifica las entradas de SBUS/MAVLink/WebRC, basta con comprobar
	if (roll != controlRoll || pitch != controlPitch || yaw != controlYaw || abs(throttle - controlThrottle) > 0.05) {
		if (mode == AUTO) mode = STAB;
	}
	
	roll = controlRoll;
	pitch = controlPitch;
	yaw = controlYaw;
	throttle = controlThrottle;
}

#if WEB_RC_ENABLED
// Protección por pérdida del control web
void webRCLossFailsafe() {
	if (!webRCEnabled || !useWebRC) return;
	if (!armed) return;

	// Comparación directa en milisegundos, evita el error máximo de 1 s que introduce la división entera
	if (millis() - webRCLastUpdate > WEB_RC_LOSS_TIMEOUT_MS) {
		print("Conexión Web RC perdida, iniciando descenso\n");
		descend();
		webRCEnabled = false;
		useWebRC = false;
	}
}
#endif

// Protección contra inversión: si el ángulo entre el eje Z del cuerpo y el Z del mundo supera 120° durante 1.5 s, se apaga
void invertedFailsafe() {
	if (!armed) {
		isInverted = false;
		return;
	}

	// Componente Z del eje del cuerpo en el sistema mundial: ≈+1 en posición normal, ≈-1 invertido
	Vector worldUp = Quaternion::rotateVector(Vector(0, 0, 1), attitude);

	static float invertedStartTime = 0;
	if (worldUp.z < INVERTED_COS_THRESHOLD) {
		isInverted = true;
		if (invertedStartTime == 0) invertedStartTime = t;
		if (t - invertedStartTime > INVERTED_TIMEOUT) {
			armed = false;
			thrustTarget = 0.0f;
			invertedStartTime = 0;
			print("Protección invertido: motores detenidos\n");
		}
	} else {
		isInverted = false;
		invertedStartTime = 0;
	}
}

// Protección por voltaje de batería
// L1 (3.4V): prohíbe armar sin desarmar (gestionado en control.ino), autodesarme en ralentí
// L2 (2.8V): solo LED parpadeando rápido en vuelo
// L3 (2.6V): aterrizaje automático en vuelo (usa descend(), velocidad configurada por SF_DESCEND_TIME)
void batteryFailsafe() {
	static bool l3Latched = false;
	static float lowSince = 0.0f;
	static float l3LastNotify = 0.0f;

	if (batteryVoltage < VBAT_ABSENT_THRESHOLD) return; // sin batería, ignorar
	if (!armed) {
		l3Latched = false;
		lowSince = 0.0f;
		l3LastNotify = 0.0f;
		return;
	}

	// Tras disparar L3 mantener el descenso hasta desarmar, evita oscilar alrededor del umbral
	if (l3Latched) {
		if (t - l3LastNotify >= 2.0f) {
			print("Batería crítica (%.2fV), descenso continuo, empuje %.0f%%\n",
			      batteryVoltage, thrustTarget * 100.0f);
#if WEB_RC_ENABLED
			char warnBuf[64];
			snprintf(warnBuf, sizeof(warnBuf),
			         "Batería crítica (%.2fV) descenso continuo empuje %.0f%%",
			         batteryVoltage, thrustTarget * 100.0f);
			setWebRCWarn(warnBuf);
#endif
			l3LastNotify = t;
		}
		descend();
		return;
	}

	bool flying = thrustTarget >= BATTERY_FLYING_THRUST_MIN;

	bool actionCondition = false;
	bool criticalAction = false;

	// En vuelo: solo L3 dispara acción fuerte; L2 sigue avisando por LED
	if (flying) {
		if (batteryVoltage < VBAT_CRITICAL_THRESHOLD) {
			actionCondition = true;
			criticalAction = true;
		}
	} else {
		// Armado en ralentí: L1 dispara autodesarme
		if (batteryVoltage < VBAT_WARN_THRESHOLD) {
			actionCondition = true;
		}
	}

	if (!actionCondition) {
		lowSince = 0.0f;
		return;
	}

	if (lowSince == 0.0f) lowSince = t;
	if (t - lowSince < BATTERY_ACTION_DEBOUNCE_TIME) return;

	lowSince = 0.0f;
	if (criticalAction) {
		l3Latched = true;
		l3LastNotify = 0.0f;
		print("Batería crítica (%.2fV), iniciando aterrizaje automático\n", batteryVoltage);
#if WEB_RC_ENABLED
		char warnBuf[64];
		snprintf(warnBuf, sizeof(warnBuf),
		         "Batería crítica (%.2fV) iniciando aterrizaje automático",
		         batteryVoltage);
		setWebRCWarn(warnBuf);
#endif
		descend();
		return;
	}

	armed = false;
	thrustTarget = 0.0f;
	print("Batería baja (%.2fV), autodesarmado\n", batteryVoltage);
#if WEB_RC_ENABLED
	char warnBuf[64];
	snprintf(warnBuf, sizeof(warnBuf), "Batería baja (%.2fV) autodesarmado", batteryVoltage);
	setWebRCWarn(warnBuf);
#endif
}