#define ANALOG_PIN                   (A0)
#define RL_VALUE                     (5)
#define RO_CLEAN_AIR_FACTOR          (9.83)
#define CALIBARAION_SAMPLE_TIMES     (50)
#define CALIBRATION_SAMPLE_INTERVAL  (500)
#define READ_SAMPLE_INTERVAL         (50)
#define READ_SAMPLE_TIMES            (5)
#define GAS_CH4                      (0)
#define GAS_CO                       (1)

float           CH4Curve[3]  =  {2.3, 0.60, -0.48};
float           COCurve[3]  =   {2.3, 0.72, -0.34};
float           Ro           =  10;
int inputPin = 5; // choose input pin (for Infrared sensor)
int counter = 0;
int val = 0;
int ledPin = 13;



#include <ESP8266WiFi.h>
#include <PubSubClient.h>


// Update these with values suitable for your network.

const char* ssid = "POCO X3";
const char* password = "kuttujoyal";
const char* mqtt_server = "raspberrypi.local";


WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[1024];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  if ((char)payload[0] == 'a'){
      digitalWrite(14, HIGH);// set the LED on
      delay(2000);
      digitalWrite(14, LOW);// set the LED on
    }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void counterincrease()
{
  long state = digitalRead(5);
  if(state == LOW) 
  {
    counter+=1;
    //Serial.println(counter);
  }
}

void setup() {
  
  Serial.begin(115200);
  pinMode(14,HIGH);
  pinMode(inputPin, INPUT); // declare Infrared sensor as input
  setup_wifi();
  Serial.print("Calibrating...\n");
  Ro = MQCalibration(ANALOG_PIN);
  Serial.print("Calibration is done...\n");
  Serial.print("Ro=");
  Serial.print(Ro);
  Serial.print("kohm");
  Serial.print("\n");
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  counterincrease();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  Serial.setTimeout(2000);
  Serial.print("CH4:");
  float ppm_ch4 = MQGetGasPercentage(MQRead(ANALOG_PIN) / Ro, GAS_CH4);
  Serial.println(ppm_ch4);
  Serial.print("CO:");
  float ppm_co =  MQGetGasPercentage(MQRead(ANALOG_PIN) / Ro, GAS_CO);
  Serial.println(ppm_co);

  long now = millis();
  if (now - lastMsg > 500) {
    lastMsg = now;
    ++value;
    snprintf (msg, 1024,"%f,%f,%d",ppm_ch4,ppm_co,counter);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("home/pro/ppm", msg);
  }
  
}
float MQResistanceCalculation(int raw_adc)
{
  return ( ((float)RL_VALUE * (1023 - raw_adc) / raw_adc));
}

float MQCalibration(int ANALOG_PIN)
{
  int i;
  float val = 0;

  for (i = 0; i < CALIBARAION_SAMPLE_TIMES; i++) {
    val += MQResistanceCalculation(analogRead(ANALOG_PIN));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }

  val = val / CALIBARAION_SAMPLE_TIMES;
  val = val / RO_CLEAN_AIR_FACTOR;
  return val;
}

float MQRead(int mq_pin)
{
  int i;
  float rs = 0;

  for (i = 0; i < READ_SAMPLE_TIMES; i++) {
    rs += MQResistanceCalculation(analogRead(ANALOG_PIN));
    delay(READ_SAMPLE_INTERVAL);
  }

  rs = rs / READ_SAMPLE_TIMES;

  return rs;
}

int MQGetGasPercentage(float rs_ro_ratio, int gas_id)
{
  if ( gas_id == GAS_CH4 )
  {
    return MQGetPercentage(rs_ro_ratio, CH4Curve);
  }

  else if ( gas_id == GAS_CO )
  {
    return MQGetPercentage(rs_ro_ratio, COCurve);
  }
  return 0;
}

int  MQGetPercentage(float rs_ro_ratio, float *pcurve)
{
  return (pow(10, ( ((log(rs_ro_ratio) - pcurve[1]) / pcurve[2]) + pcurve[0])));
}
