#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <mosquitto.h>

int exit_all=0;
void *print_message_function( void *ptr );
void *print_led_state(void* ptr);
void mqtt_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);
void mqtt_connect_callback(struct mosquitto *mosq, void *userdata, int result);
void mqtt_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos);
void mqtt_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str);
void mqtt_disconnect_callback(struct mosquitto *mosq, void *userdata, int result);
void mqtt_publish_callback(struct mosquitto *mosq, void *userdata, int result);
void mqtt_unsubscribe_callback(struct mosquitto *mosq, void *userdata, int result);
int main()
{
     pthread_t thread1, thread2;
     char *message1 = "Thread 1";
     char *message2 = "Thread 2";
     int  iret1, iret2;

    /* Create independent threads each of which will execute function */

     iret1 = pthread_create( &thread1, NULL, print_message_function, (void*) message1);
     iret2 = pthread_create( &thread1, NULL, print_message_function, (void*) message2);

     /* Wait till threads are complete before main continues. Unless we  */
     /* wait we run the risk of executing an exit which will terminate   */
     /* the process and all threads before the threads have completed.   */

     //pthread_join( thread1, NULL);
     //pthread_join( thread2, NULL); 

     printf("Thread 1 returns: %d\n",iret1);
     printf("Thread 2 returns: %d\n",iret2);
     pthread_join( thread2, NULL);      
     int i;
	char *host = "192.168.0.109";
	int port = 9999;
	int keepalive = 60;
	bool clean_session = true;
	struct mosquitto *mosq = NULL;

	mosquitto_lib_init();
	mosq = mosquitto_new("thermostat", clean_session, NULL);
	if(!mosq){
		fprintf(stderr, "Error: Out of memory.\n");
		return 1;
	}
	mosquitto_disconnect_callback_set(mosq, mqtt_disconnect_callback);	
	mosquitto_publish_callback_set(mosq, mqtt_publish_callback);	
	mosquitto_unsubscribe_callback_set(mosq, mqtt_unsubscribe_callback);		
	mosquitto_log_callback_set(mosq, mqtt_log_callback);
	mosquitto_connect_callback_set(mosq, mqtt_connect_callback);
	mosquitto_message_callback_set(mosq, mqtt_message_callback);
	mosquitto_subscribe_callback_set(mosq, mqtt_subscribe_callback);

	if(mosquitto_connect(mosq, host, port, keepalive)){
		fprintf(stderr, "Unable to connect.\n");
		return 1;
	}    
	mosquitto_loop_forever(mosq, -1, 1);

	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
     return 1;
}
void *print_led_state(void* ptr)
{
     while(1)
     {
          if(exit_all)
               break;
     }
}
void *print_message_function( void *ptr )
{
     char *message;
     message = (char *) ptr;
     printf("%s \n", message);
}




void mqtt_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	if(message->payloadlen){
		printf("%s %s\n", message->topic, (char*)message->payload);
	}else{
		printf("%s (null)\n", (char*)message->topic);
	}
	fflush(stdout);
}

void mqtt_connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
	int i;
	if(!result){
		/* Subscribe to broker information topics on successful connect. */
	    mosquitto_subscribe(mosq, NULL, "testtopic", 2);
		mosquitto_subscribe(mosq, NULL, "#", 0) ;
	}else{
		fprintf(stderr, "Connect failed\n");
	}
}
void mqtt_disconnect_callback(struct mosquitto *mosq, void *userdata, int result)
{
	fprintf(stderr, "mosquitto disconnected\n");
}
void mqtt_publish_callback(struct mosquitto *mosq, void *userdata, int result)
{
	printf("mqtt message published  {info:%s, id:%d}\n",(char*)userdata, result);
}
void mqtt_unsubscribe_callback(struct mosquitto *mosq, void *userdata, int result)
{
	printf("mqtt unsubscribe {info:%s, id:%d}\n",(char*)userdata, result);
}

void mqtt_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
	int i;

	printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
	for(i=1; i<qos_count; i++){
		printf(", %d", granted_qos[i]);
	}
	printf("\n");
}

void mqtt_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
	/* Pring all log messages regardless of level. */
	printf("%s\n", str);
}
