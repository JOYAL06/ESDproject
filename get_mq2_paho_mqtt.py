
from pushbullet import Pushbullet
import paho.mqtt.client as mqtt
from time import sleep
import threading

int
MQTT_ADDRESS='192.168.77.127'
MQTT_USER='admin'
MQTT_PASSWORD='admin'
MQTT_TOPIC='home/+/+'
co_f= 0.0
ch4_f = 0.0
counter = 0
c = 0.0
flag = False

exitFlag = 0
criticalcount = 0


mqtt_client=mqtt.Client()
mqtt_client.username_pw_set(MQTT_USER,MQTT_PASSWORD)


def on_connect(client,userdata,flags,rc):
 print('Connected with MQ'+str(rc))
 client.subscribe(MQTT_TOPIC)

def on_message(client,userdata,msg):
 global co_f
 global ch4_f 
 global counter 
 global flag
 global criticalcount
 global c
 
 print("topic: ", str(msg.topic))
 if (str(msg.topic) == 'home/pro/ppm'):
	sData = str(msg.payload).split(',')
	counter = int(sData[2])
	co_f = float(sData[1])
	ch4_f = float(sData[0])
	
	print('C0: ', co_f, 'CH4:', ch4_f,"Count",counter)
	if (co_f > 2 or ch4_f >2):
		flag = True
		criticalcount += 1
		if(criticalcount == 4):
			criticalcount = 0
			#mqtt_client.publish ("inTopic", "action")
			if((counter%2) == 0):
				mqtt_client.publish ("inTopic", "action")
			
		
	else:
		flag = False
		if (criticalcount != 0):
			criticalcount -= 1
		
 print('message: '+str(msg.payload))
 print()
 print()




mqtt_client.on_connect=on_connect
mqtt_client.on_message=on_message

def main():
 global mqtt_client
 mqtt_client.connect(MQTT_ADDRESS,'1883')
 mqtt_client.loop_forever()




def thread1():
	global flag
	while(True):
		sleep(10)
		if (flag == True):
			mqtt_client.publish("ALERT", "cric")
		

if __name__=='__main__':
 t1 = threading.Thread(target=thread1)
 t1.daemon = True
 t1.start()
 print('MQTT')
 main()
 
 
