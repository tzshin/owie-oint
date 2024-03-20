#include <Arduino.h>

#include <tuple>
#include <string>

#include <WiFi.h>
#include <HTTPClient.h>


#define LED_PIN      8
#define LED_PAT_WAIT std::make_tuple(75, 75)
#define LED_PAT_OKAY std::make_tuple(100, 0)
#define LED_PAT_FAIL std::make_tuple(75, 600)
#define LED_PAT_TRST std::make_tuple(0, 100)

#define WIFI_SSID    "Whellow"
#define WIFI_PSWD    "20020220"

#define RESTART_S    8

#define S_TO_MILLIS  1000


class LedBlinker {
public:
    LedBlinker(int pin) {
        this->pin = pin;
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    void set_pattern(std::tuple<int, int> pat) {
        std::tie(this->duration.on, this->duration.off) = pat;
        
        if (this->duration.on == 0 || this->duration.off == 0) {
            this->is_solid = true;
        }
        else {
            this->is_solid = false;
        }
    }

    void pause_blinking(bool pause, int duration = -1) {
        if (pause) {
            this->is_led_on = false;
            this->is_paused = true;
            this->duration.pause = duration;
            this->timestamp = millis();
            digitalWrite(this->pin, HIGH);
        }
        else {
            this->is_paused = false;
            this->duration.pause = -1;
            this->timestamp = millis();
        }
    }

    void tic() {
        if (this->is_paused) {
            if (this->duration.pause == -1 || millis() - this->timestamp < this->duration.pause) {
                return;
            }
            else {
                pause_blinking(false);
            }
        }

        if (this->is_solid) {
            if (duration.on != 0) {
                this->is_led_on = true;
                this->timestamp = millis();
                digitalWrite(this->pin, LOW);
            }
            else {
                this->is_led_on = false;
                this->timestamp = millis();
                digitalWrite(this->pin, HIGH);
            }
        }
        else if (this->is_led_on) {
            if (millis() - this->timestamp > this->duration.on) {
                this->is_led_on = false;
                this->timestamp = millis();
                digitalWrite(this->pin, HIGH);
            }
        }
        else {
            if (millis() - this->timestamp > this->duration.off) {
                this->is_led_on = true;
                this->timestamp = millis();
                digitalWrite(this->pin, LOW);
            }
        }
    }
    
private:
    int pin = 0;
    struct {
        int on = 0;
        int off = 0;
        int pause = 0;
    } duration;
    bool is_led_on = false;
    bool is_solid = false;
    bool is_paused = false;
    unsigned long timestamp = 0;
};


LedBlinker led_blinker(LED_PIN);
bool tried_unlock = false;


bool unlock_owie() {
    IPAddress gatewayIp = WiFi.gatewayIP();
    HTTPClient http;
    bool unlocked = false;

    http.begin("http://" + gatewayIp.toString() + "/lock?unlock=true");
    if (http.GET() == 200) {
        unlocked = true;
    }
    http.end();

    return unlocked;
}


void setup() {
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PSWD);

    led_blinker.set_pattern(LED_PAT_WAIT);
}


void loop() {
    led_blinker.tic();

    if (WiFi.status() == WL_CONNECTED && !tried_unlock) {
        if (unlock_owie()) {
            led_blinker.set_pattern(LED_PAT_OKAY);
            Serial.println("+----------------+");
            Serial.println("| OWIE Unlocked! |");
            Serial.println("| Stay Floating! |");
            Serial.println("+----------------+");
        }
        else {
            led_blinker.set_pattern(LED_PAT_FAIL);
        }
        tried_unlock = true;
        WiFi.disconnect();
    }

    if (millis() > RESTART_S * S_TO_MILLIS && !tried_unlock) {
        led_blinker.set_pattern(LED_PAT_TRST);
        tried_unlock = true;
        WiFi.disconnect();
        ESP.restart();
    }
}