#include <Arduino.h>
#include <Homie.h>

#include <WebSocketsClient.h>
#include <Hash.h>


WebSocketsClient webSocket;

#define HEARTBEAT_INTERVAL 25000

#define LEFT_FORWARD D1
#define LEFT_REVERSE 2
#define RIGHT_FORWARD D2
#define RIGHT_REVERSE 4

uint64_t heartbeatTimestamp = 0;
bool isConnected = false;

void setData(float force, float angle) {
    if(force == 0) {
        analogWrite(LEFT_FORWARD, 0);
        analogWrite(RIGHT_FORWARD, 0);
        return;
    }
    Serial.print("Force: ");
    Serial.print(force);
    Serial.print(" Angle: ");
    Serial.print(angle);
    double forward_momentum = sin(angle)*force;
    double left_right = cos(angle)*force;
    Serial.print(" Forward: ");
    Serial.print(forward_momentum);
        Serial.print(" LR: ");
    Serial.print(left_right);
    Serial.println();
    
    long forward = forward_momentum* 1024;
    long lr = left_right * 1024;
    if(forward_momentum > 0) 
    { //We can only go forward for now....
        int left = constrain(forward+lr, 0, 1024);
        int right = constrain(forward-lr, 0, 1024);
            Serial.print("L: ");
            Serial.print(left);
            Serial.print(" R: ");
            Serial.println(right);
            analogWrite(LEFT_FORWARD, left);
            analogWrite(RIGHT_FORWARD, right);
            
    } else {
        analogWrite(LEFT_FORWARD, 0);
        analogWrite(RIGHT_FORWARD, 0);
    }
    
}
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    String msg;
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.printf("[WSc] Disconnected!\n");
            isConnected = false;
            break;
        case WStype_CONNECTED:
        {
            Serial.printf("[WSc] Connected to url: %s\n", payload);
            isConnected = true;

            // send message to server when Connected
            // socket.io upgrade confirmation message (required)
            webSocket.sendTXT("5");
        }
            break;
        case WStype_TEXT:
            msg = String((char*) payload);
            if (msg.startsWith("42")) {
                Serial.printf("[WSc] get text: %s\n", payload + 2);

                StaticJsonBuffer<200> jsonBuffer;
                JsonArray& root = jsonBuffer.parseArray(payload + 2);
                if (!root.success()) {
                    Serial.println("Parsing failed");
                } else {
                    if(root[0] == "data") {
                        setData(root[1],root[2]);
                    }
                    if(root[0] == "stop") {
                        setData(0,0);
                    }
                }
            } else if (msg.startsWith("2")) {
                Serial.println("Heartbeat");
                webSocket.sendTXT("3");
            } else if (msg.startsWith("40")) {
                Serial.println("Connected");
            } else if (msg.startsWith("41")) {
                Serial.println("Disconnected");
            }
            break;
    }

}

void setupHandler() {
    webSocket.beginSocketIO("pro3-robot.herokuapp.com", 80);
    webSocket.onEvent(webSocketEvent);
}

void loopHandler() {

    webSocket.loop();

    if (isConnected) {

        uint64_t now = millis();

        if ((now - heartbeatTimestamp) > HEARTBEAT_INTERVAL) {
            heartbeatTimestamp = now;
            // socket.io heartbeat message
            webSocket.sendTXT("2");
        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial << endl << endl;

    Homie_setFirmware("PRO3", "1.0.0");
    Homie.setSetupFunction(setupHandler);
    Homie.setLoopFunction(loopHandler);
    Homie.setup();
}

void loop() {
    Homie.loop();
}