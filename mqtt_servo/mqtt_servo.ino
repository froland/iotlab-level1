#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>

#define MSG_BUFFER_SIZE 50
#define KEEP_ALIVE_PERIOD_MS 60000
#define SERVO_MAX 180
#define SERVO_STEP_DEGREE 5
#define SERVO_STEP_DELAY_MS 10

const char* ssid = "...your WiFi SSID...";
const char* password = "...your WiFi key...";
const char* mqtt_server = "...your MQTT hostname or IP...";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
char msg[MSG_BUFFER_SIZE];
int value = 0;
Servo myservo;
int currentServoPosition = 0;
int targetServoPosition = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  char* mqttMessageAsString = (char *)malloc(length + 1);
  memcpy(mqttMessageAsString, payload, length);
  mqttMessageAsString[length] = '\0';
  int mqttMessageAsInt = atoi(mqttMessageAsString);
  targetServoPosition = constrain(mqttMessageAsInt, 0, SERVO_MAX);
  Serial.print("Message received from MQTT: ");
  Serial.println(mqttMessageAsInt, DEC);
  free(mqttMessageAsString);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.publish("esp8266/debug", "connected");
      client.subscribe("esp8266/servo");
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  myservo.attach(D6);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (currentServoPosition != targetServoPosition) {
    int positionDelta = targetServoPosition - currentServoPosition;
    if (positionDelta > SERVO_STEP_DEGREE) {
      positionDelta = SERVO_STEP_DEGREE;
    } else if (positionDelta < -SERVO_STEP_DEGREE) {
      positionDelta = -SERVO_STEP_DEGREE;
    }
    currentServoPosition = currentServoPosition + positionDelta;
    myservo.write(currentServoPosition);
    Serial.print("Moving servo to ");
    Serial.print(currentServoPosition, DEC);
    Serial.println("°");
    delay(SERVO_STEP_DELAY_MS);
  }

  unsigned long now = millis();
  if (now - lastMsg > KEEP_ALIVE_PERIOD_MS) {
    lastMsg = now;
    ++value;
    snprintf(msg, MSG_BUFFER_SIZE, "hello #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("outTopic", msg);
  }
}
