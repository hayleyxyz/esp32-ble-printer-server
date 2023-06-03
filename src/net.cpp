#include <WiFi.h>
#include "net.h"
#include "secrets.h"

namespace wifi {
    void connect() {
        WiFi.mode(WIFI_STA);
        WiFi.begin(secrets::wifi::ssid, secrets::wifi::password);

        while (WiFi.status() != WL_CONNECTED) {
            printf(".");
            delay(500);
        }

        printf("WiFi connected\n");
        printf("IP address: %s\n", WiFi.localIP().toString().c_str());

        WiFi.setHostname("MX05");
    }
};
