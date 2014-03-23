/*
 * main.c
 *
 *  Created on: 15 mar 2014
 *      Author: Filip
 */
/*
    C socket server example
*/

#include<stdio.h>
#include <stdlib.h>
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include <sys/param.h>

#include "main_osc.h"
#include "fpga_osc.h"
#include "version.h"

float t_params[PARAMS_NUM] = { 0, 1e6, 0, 0, 0, 0, 0, 0, 0, 0 };

int main(int argc , char *argv[])
{
    /* Initialization of Oscilloscope application */
    if(rp_app_init() < 0) {
        fprintf(stderr, "rp_app_init() failed!\n");
        return -1;
    }

    /* Setting of parameters in Oscilloscope main module */
    if(rp_set_params((float *)&t_params, PARAMS_NUM) < 0) {
        fprintf(stderr, "rp_set_params() failed!\n");
        return -1;
    }

    int socket_desc , client_sock , c , read_size;
    struct sockaddr_in server , client;
    char client_message[2000];

    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");

    //Listen
    listen(socket_desc , 3);

    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    //accept connection from an incoming client
    client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
    puts("Connection accepted");

    // Allocate variables
	float **s;
	int sig_num, sig_len;
	int i;
	int ret_val;

	int retries;

	s = (float **)malloc(SIGNALS_NUM * sizeof(float *));
	for(i = 0; i < SIGNALS_NUM; i++) {
		s[i] = (float *)malloc(SIGNAL_LENGTH * sizeof(float));
	}
	char *sString;

    uint32_t size = 8192;
    char* command;
    char* cmdData;
    int channel;
    //Receive a message from client
    while( (read_size = recv(client_sock , client_message , 2000 , 0)) > 0 )
    {
    	client_message[read_size] = 0;
    	cmdData = strtok(client_message, ":");
    	if (cmdData != NULL)
    	{
    		command = cmdData;
    		cmdData = strtok(NULL, ":");
    	}
//    	printf("Read size: %7d\n", read_size);
//    	printf("%c\n",client_message[read_size]);
//    	printf("Raw message: ");
//    	printf(client_message);
//    	printf("\n");
//    	printf("Command: ");
//    	printf(command);
//    	printf("\n");
//    	printf("Command data: ");
//    	printf(cmdData);
//    	printf("\n");
    	if (strcmp(command,"getWaveform")==0)
    	{
//    		printf("Sending waveform\n");
    		channel = atoi(cmdData);
    		retries = 1;
			while(retries >= 0) {
				if((ret_val = rp_get_signals(&s, &sig_num, &sig_len)) >= 0) {
					/* Signals acquired in s[][]:
					 * s[0][i] - TODO
					 * s[1][i] - Channel ADC1 raw signal
					 * s[2][i] - Channel ADC2 raw signal
					 */

//    	                for(i = 0; i < MIN(size, sig_len); i++) {
//    	                    printf("%7d %7d\n", (int)s[1][i], (int)s[2][i]);
//    	                }
					sString = (char *) &s[channel+1][0];
//					printf("%7d\n", (int)sizeof(float));
					write(client_sock , sString , 4*size);
					break;
				}

				if(retries-- == 0) {
					//sString = (char *) &s[channel+1][0];
					write(client_sock , "not triggered" , 13);
					break;
				}
				usleep(1000);
			}
    	}
    	else if (strcmp(command,"setRecordlength")==0)
    	{
    		printf("Setting record length %7d\n", atoi(cmdData));
    		size = atoi(cmdData);
    		write(client_sock , "OK" , 2);
    	}
    	else if (strcmp(command,"setTriggerSource")==0)
    	{
    		printf("Setting trigger source %s\n", cmdData);
    		if (strcmp(cmdData,"CH1")==0)
    		{
    			t_params[3] = 0;
    		}
    		else if (strcmp(cmdData,"CH2")==0)
    		{
    			t_params[3] = 1;
    		}
    		else
    		{
    			t_params[3] = 2;
    		}
    	    /* Setting of parameters in Oscilloscope main module */
    	    if(rp_set_params((float *)&t_params, PARAMS_NUM) < 0) {
    	        fprintf(stderr, "rp_set_params() failed!\n");
    	        write(client_sock , "Error" , 2);
    	    }
    	    else
    	    {
    	    	write(client_sock , "OK" , 2);
    	    }
    	}
    	else if (strcmp(command,"setDecimation")==0)
    	{
			printf("Setting decimation factor %7d\n", atoi(cmdData));
			t_params[8] = atoi(cmdData);
    	    /* Setting of parameters in Oscilloscope main module */
    	    if(rp_set_params((float *)&t_params, PARAMS_NUM) < 0) {
    	        fprintf(stderr, "rp_set_params() failed!\n");
    	        write(client_sock , "Error" , 2);
			}
			else
			{
				write(client_sock , "OK" , 2);
			}

		}
    	else if (strcmp(command,"setTriggerLevel")==0)
    	{
			printf("Setting trigger level %f\n", atof(cmdData));
			t_params[6] = atof(cmdData);
    	    /* Setting of parameters in Oscilloscope main module */
    	    if(rp_set_params((float *)&t_params, PARAMS_NUM) < 0) {
    	        fprintf(stderr, "rp_set_params() failed!\n");
    	        write(client_sock , "Error" , 2);
			}
			else
			{
				write(client_sock , "OK" , 2);
			}

		}
    	else if (strcmp(command,"setTriggerMode")==0)
    	{
    		printf("Setting trigger mode %s\n", cmdData);
			if (strcmp(cmdData,"AUTO")==0)
			{
				t_params[2] = 0;
			}
			else if (strcmp(cmdData,"NORMAL")==0)
			{
				t_params[2] = 1;
			}
			else	// single
			{
				t_params[2] = 2;
			}
    	    /* Setting of parameters in Oscilloscope main module */
    	    if(rp_set_params((float *)&t_params, PARAMS_NUM) < 0) {
    	        fprintf(stderr, "rp_set_params() failed!\n");
    	        write(client_sock , "Error" , 2);
			}
			else
			{
				write(client_sock , "OK" , 2);
			}

		}
    	else if (strcmp(command,"setTriggerDelay")==0)
		{
			printf("Setting trigger delay %7d\n", atoi(cmdData));
			t_params[5] = atoi(cmdData);
			/* Setting of parameters in Oscilloscope main module */
			if(rp_set_params((float *)&t_params, PARAMS_NUM) < 0) {
				fprintf(stderr, "rp_set_params() failed!\n");
				write(client_sock , "Error" , 2);
			}
			else
			{
				write(client_sock , "OK" , 2);
			}

		}
    	else if (strcmp(command,"setTriggerEdge")==0)
		{
			printf("Setting trigger edge %7d\n", atoi(cmdData));
			t_params[4] = atoi(cmdData);
			/* Setting of parameters in Oscilloscope main module */
			if(rp_set_params((float *)&t_params, PARAMS_NUM) < 0) {
				fprintf(stderr, "rp_set_params() failed!\n");
				write(client_sock , "Error" , 2);
			}
			else
			{
				write(client_sock , "OK" , 2);
			}

		}
    	else
    	{
    		printf("Unknown command %s\n", command);
    		write(client_sock, "unknown command", 15);

    	}
    }

    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }

    if(rp_app_exit() < 0) {
        fprintf(stderr, "rp_app_exit() failed!\n");
        return -1;
    }

    return 0;
}

