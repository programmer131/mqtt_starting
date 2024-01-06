#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <mosquitto.h>
#include <sys/stat.h>
#include <errno.h>
#include <codec2.h>

int exit_all = 0;
void DumpHex(const void *data, size_t size);
void *print_message_function(void *ptr);
void *print_led_state(void *ptr);
void *codec2_thread(void *ptr);
void runCommand(char *cmd, char *buffer, int size);
void mqtt_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);
void mqtt_connect_callback(struct mosquitto *mosq, void *userdata, int result);
void mqtt_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos);
void mqtt_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str);
void mqtt_disconnect_callback(struct mosquitto *mosq, void *userdata, int result);
void mqtt_publish_callback(struct mosquitto *mosq, void *userdata, int result);
void mqtt_unsubscribe_callback(struct mosquitto *mosq, void *userdata, int result);
// br=1300; arecord -f S16_LE -c 1 -r 8000 | ./src/c2enc $br - -
int main()
{
	system("echo Hello, World!");
	pthread_t thread1, thread2;
	char *message1 = "Thread 1";
	char *message2 = "Thread 2";
	int iret1, iret2;

	/* Create independent threads each of which will execute function */

	iret1 = pthread_create(&thread1, NULL, codec2_thread, NULL);
	// iret1 = pthread_create(&thread1, NULL, print_message_function, (void *)message1);
	// iret2 = pthread_create(&thread1, NULL, print_message_function, (void *)message2);

	/* Wait till threads are complete before main continues. Unless we  */
	/* wait we run the risk of executing an exit which will terminate   */
	/* the process and all threads before the threads have completed.   */

	// pthread_join( thread1, NULL);
	// pthread_join( thread2, NULL);

	printf("Thread 1 returns: %d\n", iret1);
	printf("Thread 2 returns: %d\n", iret2);
	pthread_join(thread2, NULL);
	int i;
	char *host = "broker.emqx.io";
	int port = 1883;
	int keepalive = 60;
	bool clean_session = true;
	struct mosquitto *mosq = NULL;

	mosquitto_lib_init();
	mosq = mosquitto_new("thermostat", clean_session, NULL);
	if (!mosq)
	{
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

	if (mosquitto_connect(mosq, host, port, keepalive))
	{
		fprintf(stderr, "Unable to connect.\n");
		return 1;
	}
	mosquitto_loop_forever(mosq, -1, 1);

	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	return 1;
}
void *print_led_state(void *ptr)
{
	while (1)
	{
		if (exit_all)
			break;
	}
}
void *print_message_function(void *ptr)
{
	char *message;
	message = (char *)ptr;
	printf("%s \n", message);
}
void runCommand(char *cmd, char *buffer, int size)
{
	int c, i = 0;
	FILE *stream = popen(cmd, "r");
	while ((c = fgetc(stream)) != EOF && i < size - 1)
		buffer[i++] = c;
	buffer[i] = 0; // null terminate string
	pclose(stream);
}
long int findSize(char file_name[])
{
	// opening the file in read mode
	FILE *fp = fopen(file_name, "r");

	// checking if the file exist or not
	if (fp == NULL)
	{
		printf("File Not Found!\n");
		return -1;
	}

	fseek(fp, 0L, SEEK_END);

	// calculating the size of the file
	long int res = ftell(fp);

	// closing the file
	fclose(fp);

	return res;
}
int msleep(long msec)
{
	struct timespec ts;
	int res;

	if (msec < 0)
	{
		errno = EINVAL;
		return -1;
	}

	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * 1000000;

	do
	{
		res = nanosleep(&ts, &ts);
	} while (res && errno == EINTR);

	return res;
}
void *codec2_thread(void *ptr)
{
	char buf[2000];
	char *file_name = "/tmp/aud.bit";
	system("br=1300; arecord -f S16_LE -c 1 -r 8000 | c2enc $br - - > /tmp/aud.bit &");
	system("echo  ");
	struct stat st;
	do
	{
		stat(file_name, &st);
	} while (st.st_size < 100);
	printf("size of audio file = %ld", st.st_size);
	msleep(5);
	system("cat /tmp/aud.bit");
	FILE *filePtr = fopen(file_name, "rb"); // read write binary file
	long int offset = 0,sz;
	while (1)
	{
		fseek(filePtr, 0L, SEEK_END);
		sz = ftell(filePtr);
		printf("%ld",sz);
		//fseek(filePtr, 0, SEEK_SET);
	}

	// char c;
	// while ((c = fgetc(filePtr)) != EOF)
	// {
	// 	if (c == '\t')
	// 	{
	// 		fseek(filePtr, -1, SEEK_CUR);
	// 		fputc(' ', filePtr);
	// 		fseek(filePtr, 0, SEEK_CUR);
	// 	}
	// }

	fclose(filePtr);
}

void mqtt_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	if (message->payloadlen)
	{
		printf("%s %s\n", message->topic, (char *)message->payload);
	}
	else
	{
		printf("%s (null)\n", (char *)message->topic);
	}
	fflush(stdout);
}

void mqtt_connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
	int i;
	if (!result)
	{
		/* Subscribe to broker information topics on successful connect. */
		mosquitto_subscribe(mosq, NULL, "testtopic/#", 2);
		// mosquitto_subscribe(mosq, NULL, "#", 0) ;
	}
	else
	{
		fprintf(stderr, "Connect failed\n");
	}
}
void mqtt_disconnect_callback(struct mosquitto *mosq, void *userdata, int result)
{
	fprintf(stderr, "mosquitto disconnected\n");
}
void mqtt_publish_callback(struct mosquitto *mosq, void *userdata, int result)
{
	printf("mqtt message published  {info:%s, id:%d}\n", (char *)userdata, result);
}
void mqtt_unsubscribe_callback(struct mosquitto *mosq, void *userdata, int result)
{
	printf("mqtt unsubscribe {info:%s, id:%d}\n", (char *)userdata, result);
}

void mqtt_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
	int i;

	printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
	for (i = 1; i < qos_count; i++)
	{
		printf(", %d", granted_qos[i]);
	}
	printf("\n");
}

void mqtt_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
	/* Pring all log messages regardless of level. */
	printf("%s\n", str);
}

void DumpHex(const void *data, size_t size)
{
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i)
	{
		printf("%02X ", ((unsigned char *)data)[i]);
		if (((unsigned char *)data)[i] >= ' ' && ((unsigned char *)data)[i] <= '~')
		{
			ascii[i % 16] = ((unsigned char *)data)[i];
		}
		else
		{
			ascii[i % 16] = '.';
		}
		if ((i + 1) % 8 == 0 || i + 1 == size)
		{
			printf(" ");
			if ((i + 1) % 16 == 0)
			{
				printf("|  %s \n", ascii);
			}
			else if (i + 1 == size)
			{
				ascii[(i + 1) % 16] = '\0';
				if ((i + 1) % 16 <= 8)
				{
					printf(" ");
				}
				for (j = (i + 1) % 16; j < 16; ++j)
				{
					printf("   ");
				}
				printf("|  %s \n", ascii);
			}
		}
	}
}