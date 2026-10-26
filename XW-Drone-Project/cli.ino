// Implementación de la interfaz de línea de comandos

#include "pid.h"
#include "vector.h"
#include "util.h"
#include "lpf.h"

extern LowPassFilter<Vector> gyroBiasFilter;

#if WEB_RC_ENABLED
extern bool webConsoleEnabled;
extern void webLog(const char* msg);
#endif

extern const int MOTOR_REAR_LEFT, MOTOR_REAR_RIGHT, MOTOR_FRONT_RIGHT, MOTOR_FRONT_LEFT;
extern const int RAW, ACRO, STAB, AUTO;
extern float t, dt, loopRate;
extern float controlTime;
extern uint16_t channels[16];
extern float controlRoll, controlPitch, controlThrottle, controlYaw, controlMode;
extern float motors[4];
extern int mode;
extern bool armed;

const char* motd =
"Escriba un comando y pulse Enter:\n"
"help - Ayuda\n"
"p - Mostrar todos los parámetros\n"
"p <nombre> - Mostrar un parámetro específico\n"
"p <nombre> <valor> - Establecer un parámetro\n"
"p MOT_PIN_FL 14 - Ejemplo, pin del motor delantero izquierdo a 14\n"
"preset - Reiniciar el almacenamiento de parámetros, ejecutar tras ajustar\n"
"mfr, mfl, mrr, mrl - Probar motores (¡los motores girarán a tope, por seguridad NO montar hélices!)\n"
"ca - Calibrar giroscopio y acelerómetro\n"
"ps - Mostrar actitud pitch/roll/yaw\n"
"cr - Calibrar emisora RC\n"
"rc - Mostrar datos del RC\n"
"wifi - Mostrar información Wi-Fi\n"
"ap <ssid> <contraseña> - Configurar SSID y contraseña en modo AP\n"
"sta <ssid> <contraseña> - Configurar modo cliente STA\n"
"raw/stab/acro/auto - Establecer modo de vuelo\n"
"arm - Armar dron\n"
"disarm - Desarmar dron\n"
"psq - Mostrar cuaternión de actitud\n"
"imu - Mostrar datos del IMU\n"
"time - Mostrar información de tiempo\n"
"mot - Mostrar salida de motores\n"
"sys - Mostrar información del sistema\n"
"log [dump] - Imprimir log\n"
"reboot - Reiniciar el dron\n"
"reset - Reiniciar el dron\n";

void print(const char* format, ...) {
	char buf[1000];
	va_list args;
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	Serial.print(buf);
#if WIFI_ENABLED
	mavlinkPrint(buf);
#endif
#if WEB_RC_ENABLED
	if (webConsoleEnabled) webLog(buf);
#endif
}

void pause(float duration) {
	float start = t;
	while (t - start < duration) {
		step();
		handleInput();
#if WIFI_ENABLED
		processMavlink();
#endif
#if WEB_RC_ENABLED
		readWebRC(); // Mantener el servidor HTTP respondiendo durante comandos largos (ca/cr)
#endif
		delay(50);
	}
}

void doCommand(String str, bool echo = false) {
	// analizar comando
	String command, arg0, arg1;
	splitString(str, command, arg0, arg1);
	if (command.isEmpty()) return;

	// eco del comando
	if (echo) {
		print("> %s\n", str.c_str());
	}

	command.toLowerCase();

	// ejecutar comando
	if (command == "help" || command == "motd") {
		print("%s\n", motd);
	} else if (command == "p" && arg0 == "") {
		printParameters();
	} else if (command == "p" && arg0 != "" && arg1 == "") {
		print("%s = %g\n", arg0.c_str(), getParameter(arg0.c_str()));
	} else if (command == "p") {
		bool success = setParameter(arg0.c_str(), arg1.toFloat());
		if (success) {
			print("%s = %g\n", arg0.c_str(), getParameter(arg0.c_str()));
		} else {
			print("Parámetro no encontrado: %s\n", arg0.c_str());
		}
	} else if (command == "preset") {
		resetParameters();
	} else if (command == "time") {
		print("Tiempo: %f\n", t);
		print("Frecuencia del bucle: %.0f\n", loopRate);
		print("dt: %f\n", dt);
	} else if (command == "ps") {
		Vector a = attitude.toEuler();
		print("roll: %f pitch: %f yaw: %f\n", degrees(a.x), degrees(a.y), degrees(a.z));
	} else if (command == "psq") {
		print("qw: %f qx: %f qy: %f qz: %f\n", attitude.w, attitude.x, attitude.y, attitude.z);
	} else if (command == "imu") {
		printIMUInfo();
		printIMUCalibration();
		print("en tierra: %d\n", landed);
	} else if (command == "arm") {
		armed = true;
	} else if (command == "disarm") {
		armed = false;
	} else if (command == "raw") {
		mode = RAW;
	} else if (command == "stab") {
		mode = STAB;
	} else if (command == "acro") {
		mode = ACRO;
	} else if (command == "auto") {
		mode = AUTO;
	} else if (command == "rc") {
		print("canales: ");
		for (int i = 0; i < 16; i++) {
			print("%u ", channels[i]);
		}
		print("\nroll: %g pitch: %g yaw: %g gas: %g modo: %g\n",
			controlRoll, controlPitch, controlYaw, controlThrottle, controlMode);
		print("tiempo: %.1f\n", controlTime);
		print("modo: %s\n", getModeName());
		print("armado: %d\n", armed);
	} else if (command == "wifi") {
#if WIFI_ENABLED
		printWiFiInfo();
#endif
	} else if (command == "ap") {
#if WIFI_ENABLED
		configWiFi(true, arg0.c_str(), arg1.c_str());
#endif
	} else if (command == "sta") {
#if WIFI_ENABLED
		configWiFi(false, arg0.c_str(), arg1.c_str());
#endif
	} else if (command == "mot") {
		print("frontal-derecho %g frontal-izquierdo %g trasero-derecho %g trasero-izquierdo %g\n",
			motors[MOTOR_FRONT_RIGHT], motors[MOTOR_FRONT_LEFT], motors[MOTOR_REAR_RIGHT], motors[MOTOR_REAR_LEFT]);
	} else if (command == "log") {
		printLogHeader();
		if (arg0 == "dump") printLogData();
	} else if (command == "cr") {
		calibrateRC();
	} else if (command == "ca") {
		calibrateAccel();
	} else if (command == "mfr") {
		testMotor(MOTOR_FRONT_RIGHT);
	} else if (command == "mfl") {
		testMotor(MOTOR_FRONT_LEFT);
	} else if (command == "mrr") {
		testMotor(MOTOR_REAR_RIGHT);
	} else if (command == "mrl") {
		testMotor(MOTOR_REAR_LEFT);
	} else if (command == "sys") {
#ifdef ESP32
		print("Chip: %s\n", ESP.getChipModel());
		print("Temperatura: %.1f °C\n", temperatureRead());
		print("Heap libre: %d\n", ESP.getFreeHeap());
		// Imprimir tabla de tareas
		print("Num  Tarea               Pila   Prio  Núcleo CPU%%\n");
		int taskCount = uxTaskGetNumberOfTasks();
		TaskStatus_t *systemState = new TaskStatus_t[taskCount];
		uint32_t totalRunTime;
		uxTaskGetSystemState(systemState, taskCount, &totalRunTime);
		for (int i = 0; i < taskCount; i++) {
			String core = systemState[i].xCoreID == tskNO_AFFINITY ? "*" : String(systemState[i].xCoreID);
			int cpuPercentage = systemState[i].ulRunTimeCounter / (totalRunTime / 100);
			print("%-5d%-20s%-7d%-6d%-6s%d\n",systemState[i].xTaskNumber, systemState[i].pcTaskName,
				systemState[i].usStackHighWaterMark, systemState[i].uxCurrentPriority, core.c_str(), cpuPercentage);
		}
		delete[] systemState;
#endif
	} else if (command == "reset") {
		attitude = Quaternion();
		gyroBiasFilter.reset();
	} else if (command == "reboot") {
		ESP.restart();
	} else {
		print("Comando inválido: %s\n", command.c_str());
	}
}

void handleInput() {
	static bool showMotd = true;
	static String input;

	if (showMotd) {
		print("%s\n", motd);
		showMotd = false;
	}

	while (Serial.available()) {
		char c = Serial.read();
		if (c == '\n') {
			doCommand(input);
			input.clear();
		} else {
			input += c;
		}
	}
}