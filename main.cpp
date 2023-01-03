#include "mbed.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"
#include "bbcar.h"


Ticker servo_ticker;
Ticker servo_feedback_ticker;

WiFiInterface *wifi;
volatile int message_num = 0;
volatile int arrivedcount = 0;
volatile bool closed = false;
   float val;
const char* topic = "Mbed";

Thread mqtt_thread(osPriorityHigh);
EventQueue mqtt_queue;
/**********************/
Thread t;
double elapsedTime=0;
int counter=0;

PwmOut pin9(D10), pin10(D9);
PwmIn pin11(D12), pin12(D11);
BBCar car(pin9, pin11, pin10, pin12, servo_ticker, servo_feedback_ticker);
BusInOut qti_pin(D4,D5,D6,D7);

BufferedSerial pc(USBTX, USBRX);
DigitalInOut ping(D12);
Timer t1;

double ia0 , c , a, pre, dd;

void messageArrived(MQTT::MessageData& md) {
    MQTT::Message &message = md.message;
    char msg[300];
    sprintf(msg, "Message arrived: QoS%d, retained %d, dup %d, packetID %d\r\n", message.qos, message.retained, message.dup, message.id);
    printf(msg);
    ThisThread::sleep_for(100ms);
    char payload[300];
    sprintf(payload, "Payload %.*s\r\n", message.payloadlen, (char*)message.payload);
    printf(payload);
    ++arrivedcount;
}

void publish_message_gyro(MQTT::Client<MQTTNetwork, Countdown>* client) {
    message_num++;
    MQTT::Message message;
    char buff[100];

    sprintf(buff, "%f\n", dd);
    
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*) buff;
    message.payloadlen = strlen(buff) + 1;
    int rc = client->publish(topic, message);

    printf("rc:  %d\r\n", rc);
    printf("Puslish message: %s\r\n", buff);
}

void close_mqtt() {
    closed = true;
}


int main() {


   pc.set_baud(9600);


   parallax_qti qti1(qti_pin);
   int pattern;
   int a = 0;
   double d0,d1,ia0,ia1;

   car.goStraight(40);
   car.feedbackWheel();
   ia0 = car.servo0.angle;
   ia1 = car.servo1.angle;

    wifi = WiFiInterface::get_default_instance();
    if (!wifi) {
            printf("ERROR: No WiFiInterface found.\r\n");
            return -1;
    }


    printf("\nConnecting to %s...\r\n", MBED_CONF_APP_WIFI_SSID);
    int ret = wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
    if (ret != 0) {
            printf("\nConnection error: %d\r\n", ret);
            return -1;
    }


    NetworkInterface* net = wifi;
    MQTTNetwork mqttNetwork(net);
    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);

    //TODO: revise host to your IP
    const char* host = "172.20.10.2";
    const int port=1883;
    printf("Connecting to TCP network...\r\n");
    printf("address is %s/%d\r\n", host, port);

    int rc = mqttNetwork.connect(host, port);//(host, 1883);
    if (rc != 0) {
            printf("Connection error.");
            return -1;
    }
    printf("Successfully connected!\r\n");

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "Mbed";

    if ((rc = client.connect(data)) != 0){
            printf("Fail to connect MQTT\r\n");
    }
    if (client.subscribe(topic, MQTT::QOS0, messageArrived) != 0){
            printf("Fail to subscribe\r\n");
    }

    mqtt_thread.start(callback(&mqtt_queue, &EventQueue::dispatch_forever));

    int num = 0;
    while (num != 5) {
            client.yield(100);
            ++num;
    }
    while (1) {
        if (closed) break;
        client.yield(500);
        publish_message_gyro(&client);

      pattern = (int)qti1;
      //printf("%d\n",pattern);
       ping.output();
      ping = 0; wait_us(200);
      ping = 1; wait_us(5);
      ping = 0; wait_us(5);

      ping.input();
      while(ping.read() == 0);
      t1.start();
      while(ping.read() == 1);
      val = t1.read();

      //printf("Ping = %lf\r\n", val*17700.4f);
      printf("Laser Ping = %lf\r\n", val*17150.0f); //Laser Ping's distance
      car.turn(45, 0.1);
      pre = val;

      t1.stop();
      t1.reset();

      switch (pattern) {
         case 0b1000: car.turn(40, 0.1); dd = 1100.0; a=0; break;
         case 0b1100: car.turn(40, 0.5); dd = 1100.0; a=0; break;
         case 0b0100: car.turn(40, 0.7); dd = 1100.0; a=0; break;
         case 0b1101: car.turn(40, 0.7); dd = 1100.0; a=0; break;
         case 0b0110: car.goStraight(40); a=0; dd = 1100.0; break;
         case 0b1011: car.turn(42, -0.7); a=0; dd = 1100.0; break;
         case 0b0010: car.turn(42, -0.7); a=0; dd = 1100.0; break;
         case 0b0011: car.turn(42, -0.5); a=0; dd = 1100.0; break;
         case 0b0001: car.turn(42, -0.1); a=0; dd = 1100.0; break;
         case 0b0000: car.goStraight(-40); a=0; dd = 1110.0; break;
         case 0b1111: 
            if (a==0) {
                car.stop(); 
                a++;
                ThisThread::sleep_for(1s);
                car.feedbackWheel();
                d0 = 2*3.14*3*(abs(car.servo0.angle-ia0)/360);
                d1 = 2*3.14*3*(abs(car.servo1.angle-ia1)/360);
                dd = (d0+d1)/2;
                printf("distance = %lf\n", (d0+d1)/2);
 
                break;
            } else {
                if (val*17150.0f < 8) {
                    dd = 111000.0;
                    printf("turn left\n");
                    car.turn(40, 0.7);
                } else {
                    car.goStraight(40);
                }
                ThisThread::sleep_for(1s);
                car.feedbackWheel();
                ia0 = car.servo0.angle;
                ia1 = car.servo1.angle;
                break;
            }
         default: car.goStraight(40);
      }
            ThisThread::sleep_for(100ms);
    }

    printf("Ready to close MQTT Network......\n");

    if ((rc = client.unsubscribe(topic)) != 0) {
            printf("Failed: rc from unsubscribe was %d\n", rc);
    }
    if ((rc = client.disconnect()) != 0) {
    printf("Failed: rc from disconnect was %d\n", rc);
    }

    mqttNetwork.disconnect();
    printf("Successfully closed!\n");

    return 0;
}