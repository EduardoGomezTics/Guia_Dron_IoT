// Soporte Wi-Fi

#if WIFI_ENABLED

#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiUdp.h>
#include "Preferences.h"

extern Preferences storage; // Reutilizar el almacenamiento principal

const int W_DISABLED = 0, W_AP = 1, W_STA = 2;
int wifiMode = W_AP;
int udpLocalPort = 14550;
int udpRemotePort = 14550;
IPAddress udpRemoteIP = "255.255.255.255";

WiFiUDP udp;

// --- Nuevos parámetros de robustez ---
static const int AP_MAX_CLIENTS = 8;                            // >4 evita rebotes cuando Chrome abre varias conexiones
static const int AP_CHANNEL     = 1;                            // canal fijo, evita re-escaneo
static const unsigned long WIFI_MAINTENANCE_MS = 15000;        // mantenimiento cada 15 s

void setupWiFi() {
    print("Configurando Wi-Fi\n");

    // 1) No persistir en flash: evita wear y bloqueos por escrituras repetidas
    WiFi.persistent(false);
    // 2) No auto-reconectar en STA: nosotros controlamos la lógica
    WiFi.setAutoReconnect(false);
    // 3) Modo mixto no hace falta en AP puro
    WiFi.mode(wifiMode == W_AP ? WIFI_MODE_AP : (wifiMode == W_STA ? WIFI_MODE_STA : WIFI_MODE_NULL));

    if (wifiMode == W_AP) {
        // IP fija del AP: evita renegociación DHCP y bloqueos
        IPAddress apIP(192, 168, 4, 1);
        IPAddress apGateway(192, 168, 4, 1);
        IPAddress apSubnet(255, 255, 255, 0);
        WiFi.softAPConfig(apIP, apGateway, apSubnet);

        String ssid = storage.getString("WIFI_AP_SSID", "XW_Drone_WiFi");
        String pass = storage.getString("WIFI_AP_PASS", "12345678");

        // max_conn explícito = 8, canal fijo = 1, SSID visible
        bool ok = WiFi.softAP(ssid.c_str(), pass.c_str(), AP_CHANNEL, 0, AP_MAX_CLIENTS);
        if (!ok) {
            print("⚠ Fallo al iniciar AP, reintentando en modo abierto\n");
            WiFi.softAP(ssid.c_str(), NULL, AP_CHANNEL, 0, AP_MAX_CLIENTS);
        }
        print("AP SSID: %s\n", ssid.c_str());
        print("AP IP:   %s\n", WiFi.softAPIP().toString().c_str());
        print("AP max clientes: %d\n", AP_MAX_CLIENTS);
    } else if (wifiMode == W_STA) {
        WiFi.begin(
            storage.getString("WIFI_STA_SSID", "").c_str(),
            storage.getString("WIFI_STA_PASS", "").c_str()
        );
    }

    // 4) Desactivar power-save: reduce latencia y evita micro-cortes
    WiFi.setSleep(false);

    // 5) Iniciar UDP
    udp.begin(udpLocalPort);

    print("Wi-Fi listo\n");
}

// --- Mantenimiento periódico: libera clientes zombies y reporta estado ---
void maintainWiFi() {
    static unsigned long lastCheck = 0;
    unsigned long now = millis();
    if (now - lastCheck < WIFI_MAINTENANCE_MS) return;
    lastCheck = now;

    if (wifiMode == W_AP) {
        int stations = WiFi.softAPgetStationNum();
        // Log útil para ver si se acumulan clientes
        print("WiFi AP: %d/%d estaciones\n", stations, AP_MAX_CLIENTS);

        // Si el AP llegó al tope y no hay tráfico reciente, forzamos un ciclo
        // que libera slots. Esto evita el "se bloquea tras varias conexiones".
        if (stations >= AP_MAX_CLIENTS) {
            print("WiFi AP: tope alcanzado, reiniciando AP para liberar clientes\n");
            WiFi.softAPdisconnect(false);   // false = no borrar config
            delay(200);
            String ssid = storage.getString("WIFI_AP_SSID", "XW_Drone_WiFi");
            String pass = storage.getString("WIFI_AP_PASS", "12345678");
            WiFi.softAP(ssid.c_str(), pass.c_str(), AP_CHANNEL, 0, AP_MAX_CLIENTS);
        }
    } else if (wifiMode == W_STA) {
        // Reconexión simple si se cae (sin bloquear el loop)
        static unsigned long lastReconnect = 0;
        if (!WiFi.isConnected() && now - lastReconnect > 10000) {
            lastReconnect = now;
            print("WiFi STA: reconectando...\n");
            WiFi.reconnect();
        }
    }
}

void sendWiFi(const uint8_t *buf, int len) {
    if (WiFi.softAPgetStationNum() == 0 && !WiFi.isConnected()) return;
    udp.beginPacket(udpRemoteIP, udpRemotePort);
    udp.write(buf, len);
    udp.endPacket();
}

int receiveWiFi(uint8_t *buf, int len) {
    int sz = udp.parsePacket();
    if (sz > 0) {
        if (udp.remoteIP()) udpRemoteIP = udp.remoteIP();
        return udp.read(buf, len);
    }
    return 0;
}

void printWiFiInfo() {
    if (WiFi.getMode() == WIFI_MODE_AP) {
        print("Modo: Access Point (AP)\n");
        print("MAC: %s\n", WiFi.softAPmacAddress().c_str());
        print("SSID: %s\n", WiFi.softAPSSID().c_str());
        print("Pass: ***\n");
        print("Clientes: %d/%d\n", WiFi.softAPgetStationNum(), AP_MAX_CLIENTS);
        print("IP: %s\n", WiFi.softAPIP().toString().c_str());
    } else if (WiFi.getMode() == WIFI_MODE_STA) {
        print("Modo: Cliente (STA)\n");
        print("Conectado: %d\n", WiFi.isConnected());
        print("MAC: %s\n", WiFi.macAddress().c_str());
        print("SSID: %s\n", WiFi.SSID().c_str());
        print("Pass: ***\n");
        print("IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        print("Modo: Deshabilitado\n");
        return;
    }
    print("IP remota: %s\n", udpRemoteIP.toString().c_str());
    print("MAVLink conectado: %d\n", mavlinkConnected);
}

void configWiFi(bool ap, const char *ssid, const char *password) {
    if (ap) {
        storage.putString("WIFI_AP_SSID", ssid);
        storage.putString("WIFI_AP_PASS", password);
    } else {
        storage.putString("WIFI_STA_SSID", ssid);
        storage.putString("WIFI_STA_PASS", password);
    }
    print("✓ Reinicia para aplicar los nuevos ajustes\n");
}

#endif