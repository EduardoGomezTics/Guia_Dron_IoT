// Servidor de control remoto web

#if WEB_RC_ENABLED

#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include "web_rc_html.h"

// Variables de control unificadas del dron (escritas por la capa de adaptación de protocolo,
// compartidas con SBUS/MAVLink)
extern float t;
extern float controlTime;
extern float controlRoll, controlPitch, controlYaw, controlThrottle, controlMode;

// ==================== Constantes de configuración ====================
#define WEB_RC_TIMEOUT_MS 10000  // Tiempo de espera de conexión: si el último paquete supera este tiempo (ms), se considera desconectado; debe ser mayor que el intervalo de latido
// VBAT_ADC_PIN / VBAT_ADC_SAMPLES / VBAT_DIVIDER se movieron a battery.ino

// ==================== Banderas de estado de conexión ====================
bool webRCEnabled = false;          // Web RC tiene una conexión válida actualmente (actualizado cada frame por readWebRC())
bool useWebRC = false;              // Actualmente se está usando Web RC para control (sincronizado con webRCEnabled)
bool webRCUpdated = false;          // Se ha recibido al menos un paquete de joystick (false antes de la primera conexión)
bool webConsoleEnabled = false;     // Interruptor de la consola de depuración web: POST /console/enable lo activa; cuando está activo, print() escribe en el búfer
static char webRCWarnMsg[64] = "";  // Mensaje de advertencia pendiente de enviar al frontend, se borra tras un envío

// ==================== Valores temporales del joystick (para el endpoint de estado) ====================
// Unidades: gas 0~100 (%), ejes de actitud ±30 (°), pulsado=1/soltado=0
float webRCRoll = 0.0f;
float webRCPitch = 0.0f;
float webRCYaw = 0.0f;
float webRCThrottle = 0.0f;
uint16_t webRCButtons = 0;          // Máscara de 16 bits de botones, bit0=armar bit1=desarmar bit2=parada bit6=STAB bit7=ACRO bit8=ALTHOLD
unsigned long webRCLastUpdate = 0;  // Marca de tiempo millis() del último paquete recibido

// ==================== Escalado de sensibilidad ====================
// Salida final = ángulo procesado × scale / STICK_MAX, resultado escrito en control* ([-1,1])
// Reducir scale → respuesta más suave del dron; aumentar → más sensible
float webRCThrottleScale = 1.0f;  // Límite de potencia máxima del gas, 1.0=100%, 0.8=máximo solo se puede empujar al 80% del gas
float webRCStickScale = 0.85f;    // Sensibilidad de alabeo/cabeceo, rango recomendado 0.5~1.0
float webRCYawScale = 0.68f;      // Sensibilidad de guiñada, la inercia de guiñada es menor por lo que es independiente y más baja, rango recomendado 0.3~0.8

// ==================== Zona muerta ====================
// Espacio normalizado (proporción 0~1), el error de centrado del joystick dentro de la zona muerta produce salida 0
// Aumentar → zona neutral más ancha, menos toques accidentales; reducir → respuesta más sensible pero propensa a deriva
float stickDeadzone = 0.06f;     // Compartido por alabeo/cabeceo/guiñada, 6% de zona muerta normalizada
float throttleDeadzone = 0.06f;  // Zona muerta del extremo inferior del gas, empujar por debajo de esta proporción produce 0% (anti-rebote)

// ==================== Rangos de entrada (no modificar sin cuidado, debe coincidir con el frontend) ====================
static const float THROTTLE_MIN = 0.0f;    // Límite inferior de salida de gas (%)
static const float THROTTLE_MAX = 100.0f;  // Límite superior de salida de gas (%)
static const float STICK_MAX = 30.0f;      // Ángulo máximo de ejes de actitud (°), corresponde al tope del frontend; modificar requiere ajustar sincronizadamente PID TILT_MAX
static const float RAW_MAX = 100.0f;       // Valor de tope de normalización del joystick del frontend, JS del frontend emite fijo ±100

// ==================== Estado interno (runtime, no modificar manualmente) ====================
static float lastValidThrottle = 0.0f;       // Último valor de gas validado, usado como reserva ante anomalías
static float lastValidRoll = 0.0f;           // Último valor de alabeo validado
static float lastValidPitch = 0.0f;          // Último valor de cabeceo validado
static float lastValidYaw = 0.0f;            // Último valor de guiñada validado
static unsigned long lastDataErrorTime = 0;  // Última marca de tiempo de anomalía de datos, usada para limitar frecuencia de logs de error

// ==================== Servidor web ====================
WebServer webRCServer(8080);

// ==================== Búfer de log de la consola ====================
#define CONSOLE_LINES 30
#define CONSOLE_LINE_LEN 240
static char consoleBuf[CONSOLE_LINES][CONSOLE_LINE_LEN];
static int consoleTail = 0;
static int consoleFilled = 0;
static int consoleTotal = 0;  // Número total de líneas que solo crece, usado para extracción incremental

#define CONSOLE_CMD_QUEUE_SIZE 4
#define CONSOLE_CMD_LEN 64
static char consoleCmdQueue[CONSOLE_CMD_QUEUE_SIZE][CONSOLE_CMD_LEN];
static int consoleCmdHead = 0;
static int consoleCmdTail = 0;
static int consoleCmdCount = 0;

bool enqueueConsoleCmd(const char* cmd) {
  if (!cmd || !*cmd) return false;
  if (consoleCmdCount >= CONSOLE_CMD_QUEUE_SIZE) return false;
  strncpy(consoleCmdQueue[consoleCmdTail], cmd, CONSOLE_CMD_LEN - 1);
  consoleCmdQueue[consoleCmdTail][CONSOLE_CMD_LEN - 1] = '\0';
  consoleCmdTail = (consoleCmdTail + 1) % CONSOLE_CMD_QUEUE_SIZE;
  consoleCmdCount++;
  return true;
}

void processConsoleCommandQueue() {
  if (consoleCmdCount <= 0) return;

  String cmd = consoleCmdQueue[consoleCmdHead];
  consoleCmdHead = (consoleCmdHead + 1) % CONSOLE_CMD_QUEUE_SIZE;
  consoleCmdCount--;
  doCommand(cmd, false);
}

void webLog(const char* msg) {
  // Dividir por \n para evitar contaminar el JSON con saltos de línea
  const char* start = msg;
  while (*start) {
    const char* end = strchr(start, '\n');
    int len = end ? (int)(end - start) : (int)strlen(start);
    if (len > 0) {
      int copy = (len < CONSOLE_LINE_LEN - 1) ? len : (CONSOLE_LINE_LEN - 1);
      strncpy(consoleBuf[consoleTail], start, copy);
      consoleBuf[consoleTail][copy] = '\0';
      consoleTail = (consoleTail + 1) % CONSOLE_LINES;
      if (consoleFilled < CONSOLE_LINES) consoleFilled++;
      consoleTotal++;
    }
    if (!end) break;
    start = end + 1;
  }
}

// ==================== Procesamiento del joystick ====================

// Zona muerta en espacio normalizado (entrada/salida ambas en [-1, 1])
// La parte fuera de la zona muerta se reasigna linealmente al rango completo,
// joystick a tope = salida a tope
float applyDeadzone(float norm, float deadzone) {
  if (fabsf(norm) < deadzone) return 0.0f;
  float sign = (norm > 0.0f) ? 1.0f : -1.0f;
  return sign * (fabsf(norm) - deadzone) / (1.0f - deadzone);
}

// Procesar eje de actitud (alabeo/cabeceo/guiñada)
// Entrada raw ∈ [-100, +100], salida ∈ [-STICK_MAX, +STICK_MAX] (°)
float processAxis(float raw, float& lastValid) {
  if (isnan(raw) || isinf(raw) || fabsf(raw) > 1000.0f) {
    if (millis() - lastDataErrorTime > 2000) lastDataErrorTime = millis();
    return lastValid;
  }
  float norm = constrain(raw, -RAW_MAX, RAW_MAX) / RAW_MAX;
  norm = applyDeadzone(norm, stickDeadzone);
  lastValid = norm * STICK_MAX;
  return lastValid;
}

// Procesar eje de gas
// Frontend: eje Y del joystick izquierdo, abajo=-100, arriba=+100
// Salida: pct ∈ [0, 100]
float processThrottle(float raw) {
  if (isnan(raw) || isinf(raw) || fabsf(raw) > 1000.0f) return lastValidThrottle;
  raw = constrain(raw, -RAW_MAX, RAW_MAX);
  // Mapeo lineal: -100→0%, 0→50%, +100→100%
  float pct = (raw + RAW_MAX) / (2.0f * RAW_MAX) * THROTTLE_MAX;
  if (pct < throttleDeadzone * THROTTLE_MAX) pct = 0.0f;
  pct = constrain(pct * webRCThrottleScale, THROTTLE_MIN, THROTTLE_MAX);
  lastValidThrottle = pct;
  return pct;
}

// ==================== Procesamiento central de datos ====================

void setWebRCInput(float roll, float pitch, float yaw, float throttle, uint16_t buttons) {
  float pThrottle = processThrottle(throttle);
  float pYaw = processAxis(yaw, lastValidYaw);
  float pPitch = processAxis(pitch, lastValidPitch);
  float pRoll = processAxis(roll, lastValidRoll);

  // Guardar valores procesados (para lectura desde el endpoint de estado)
  webRCThrottle = pThrottle;
  webRCYaw = pYaw;
  webRCPitch = pPitch;
  webRCRoll = pRoll;
  webRCButtons = buttons;
  webRCLastUpdate = millis();
  webRCUpdated = true;

  // Escribir en variables de control unificadas (misma ruta que SBUS/MAVLink)
  controlRoll = constrain(pRoll * webRCStickScale / STICK_MAX, -1.0f, 1.0f);
  controlPitch = constrain(pPitch * webRCStickScale / STICK_MAX, -1.0f, 1.0f);
  controlYaw = constrain(pYaw * webRCYawScale / STICK_MAX, -1.0f, 1.0f);
  controlThrottle = pThrottle / THROTTLE_MAX;
  controlMode = NAN;
  controlTime = t;

  static float lastPrintedThrottle = -1.0f;
  if (fabsf(pThrottle - lastPrintedThrottle) > 5.0f) {
    print("WebRC G=%.0f%% A=%.1f C=%.1f Gu=%.1f Btn=0x%04X\n",
          pThrottle, pRoll, pPitch, pYaw, buttons);
    lastPrintedThrottle = pThrottle;
  }
}

// ==================== Manejador del protocolo JSON ====================

const char* findJsonValue(const char* json, const char* key) {
  const char* p = strstr(json, key);
  if (!p) return nullptr;
  p = strchr(p, ':');
  if (!p) return nullptr;
  p++;
  while (*p == ' ' || *p == '"') p++;
  return p;
}

bool handleJSONProtocol(String& body) {
  if (body.length() == 0 || body.indexOf('{') == -1) return false;
  const char* json = body.c_str();
  const char* typePos = strstr(json, "\"t\":");
  if (!typePos) return false;
  int type = atoi(typePos + 4);
  const char* v;

  switch (type) {
    case 1:
      {  // Datos del joystick
        float th = 0, r = 0, p = 0, y = 0;
        uint32_t ts = 0;
        if ((v = findJsonValue(json, "\"th\""))) th = atof(v);
        if ((v = findJsonValue(json, "\"r\""))) r = atof(v);
        if ((v = findJsonValue(json, "\"p\""))) p = atof(v);
        if ((v = findJsonValue(json, "\"y\""))) y = atof(v);
        if ((v = findJsonValue(json, "\"ts\""))) ts = atol(v);
        setWebRCInput(r, p, y, th, webRCButtons);
        break;
      }
    case 2:
      {  // Evento de botón
        int idx = 0, state = 0;
        uint32_t ts = 0;
        if ((v = findJsonValue(json, "\"b\""))) idx = atoi(v);
        if ((v = findJsonValue(json, "\"s\""))) state = atoi(v);
        if ((v = findJsonValue(json, "\"ts\""))) ts = atol(v);
        if (idx >= 0 && idx < 16) {
          if (state) webRCButtons |= (1 << idx);
          else webRCButtons &= ~(1 << idx);
          webRCLastUpdate = millis();
          webRCUpdated = true;
        }
        break;
      }
    case 4:  // Latido
      webRCLastUpdate = millis();
      webRCUpdated = true;
      break;
  }
  return true;
}

void setWebRCWarn(const char* msg) {
  strncpy(webRCWarnMsg, msg, sizeof(webRCWarnMsg) - 1);
  webRCWarnMsg[sizeof(webRCWarnMsg) - 1] = '\0';
}

// ==================== Manejo de peticiones HTTP ====================

void handleWebRCRequest() {
  if (!webRCServer.hasArg("plain")) {
    webRCServer.send(400, "application/json", "{\"e\":\"no data\"}");
    return;
  }
  String body = webRCServer.arg("plain");
  if (handleJSONProtocol(body)) {
    char resp[256];
    if (webRCWarnMsg[0]) {
      snprintf(resp, sizeof(resp),
               "{\"s\":\"ok\",\"m\":%d,\"arm\":%d,\"warn\":\"%s\"}",
               mode, (int)armed, webRCWarnMsg);
      webRCWarnMsg[0] = '\0';  // Limpiar inmediatamente tras enviar
    } else {
      snprintf(resp, sizeof(resp),
               "{\"s\":\"ok\",\"m\":%d,\"arm\":%d}",
               mode, (int)armed);
    }
    webRCServer.send(200, "application/json", resp);
  } else {
    webRCServer.send(400, "application/json", "{\"e\":\"parse failed\"}");
  }
}

// ==================== Voltaje de batería ====================
// readBatteryVoltage() se movió a battery.ino, aquí simplemente se llama

// ==================== Estado de conexión ====================

bool isWebRCEnabled() {
  return webRCUpdated && (millis() - webRCLastUpdate < WEB_RC_TIMEOUT_MS);
}

bool isUsingWebRC() {
  return useWebRC && isWebRCEnabled();
}

// ==================== Función principal de configuración ====================

void setupWebRC() {
  lastValidThrottle = THROTTLE_MIN;
  lastValidRoll = lastValidPitch = lastValidYaw = 0.0f;

  webRCServer.on("/", HTTP_GET, []() {
    webRCServer.send(200, "text/html", webRCIndexHtml);
  });
  webRCServer.on("/web_rc", HTTP_POST, handleWebRCRequest);
  webRCServer.on("/web_rc/heartbeat", HTTP_POST, handleWebRCRequest);

  webRCServer.on("/console", HTTP_GET, []() {
    int since = -1;
    int limit = 20;
    if (webRCServer.hasArg("since")) since = webRCServer.arg("since").toInt();
    if (webRCServer.hasArg("limit")) limit = webRCServer.arg("limit").toInt();
    if (limit <= 0) limit = 20;
    if (limit > CONSOLE_LINES) limit = CONSOLE_LINES;

    int availableFrom = max(0, consoleTotal - consoleFilled);
    int sendFrom = (since >= 0) ? since : availableFrom;
    if (sendFrom < availableFrom) sendFrom = availableFrom;
    if (sendFrom > consoleTotal) sendFrom = consoleTotal;
    int sendTo = min(consoleTotal, sendFrom + limit);

    String json = "{\"total\":";
    json += consoleTotal;
    json += ",\"next\":";
    json += sendTo;
    json += ",\"has_more\":";
    json += (sendTo < consoleTotal) ? "true" : "false";
    json += ",\"lines\":[";

    bool first = true;
    for (int i = sendFrom; i < sendTo; i++) {
      int idx = i % CONSOLE_LINES;
      if (!first) json += ",";
      first = false;
      json += "\"";
      String line = consoleBuf[idx];
      line.replace("\\", "\\\\");  // \ → \\
      line.replace("\"", "\\\"");  // " → \"
      line.replace("\n", "\\n");   // Reserva: salto de línea → \n
      line.replace("\r", "\\r");   // Reserva: CR → \r
      json += line + "\"";
    }
    json += "]}";
    webRCServer.send(200, "application/json", json);
  });

  webRCServer.on("/console/cmd", HTTP_POST, []() {
    String cmd = webRCServer.arg("plain");
    cmd.trim();
    if (cmd.length() == 0) {
      webRCServer.send(400, "application/json", "{\"ok\":0,\"e\":\"empty command\"}");
      return;
    }

    char buf[CONSOLE_LINE_LEN];
    snprintf(buf, sizeof(buf), "> %s", cmd.c_str());
    webLog(buf);

    if (!enqueueConsoleCmd(cmd.c_str())) {
      webLog("! la cola de comandos está llena");
      webRCServer.send(503, "application/json", "{\"ok\":0,\"e\":\"queue full\"}");
      return;
    }

    webRCServer.send(200, "application/json", "{\"ok\":1,\"queued\":1}");
  });

  webRCServer.on("/console/enable", HTTP_POST, []() {
    webConsoleEnabled = true;
    webRCServer.send(200, "application/json", "{\"ok\":1}");
  });

  webRCServer.on("/console/disable", HTTP_POST, []() {
    webConsoleEnabled = false;
    webRCServer.send(200, "application/json", "{\"ok\":1}");
  });

  webRCServer.on("/web_rc/status", HTTP_GET, []() {
    float vbat = readBatteryVoltage();
    if (isnan(vbat) || vbat < 0.0f) vbat = 0.0f;
    char json[384];
    snprintf(json, sizeof(json),
             "{\"enabled\":%s,\"active\":%s,"
             "\"voltage\":%.2f,"
             "\"throttle\":%.1f,\"roll\":%.1f,\"pitch\":%.1f,\"yaw\":%.1f}",
             isWebRCEnabled() ? "true" : "false",
             isUsingWebRC() ? "true" : "false",
             vbat,
             webRCThrottle, webRCRoll, webRCPitch, webRCYaw);
    webRCServer.send(200, "application/json", json);
  });

  webRCServer.begin();
  print("✓ Web RC iniciado: http://192.168.4.1:8080\n");
  print("  Zona muerta joystick=%.0f%% gas=%.0f%% | Escala joystick=%.2f guiñada=%.2f\n",
        stickDeadzone * 100.0f, throttleDeadzone * 100.0f,
        webRCStickScale, webRCYawScale);
}

// ==================== Función del bucle principal ====================

void readWebRC() {
  webRCServer.handleClient();
  if (isWebRCEnabled()) {
    webRCEnabled = useWebRC = true;
  } else {
    webRCEnabled = useWebRC = false;
  }
}

#else
void setupWebRC() {
  print("Web RC deshabilitado\n");
}
void readWebRC() {}
void processConsoleCommandQueue() {}
#endif