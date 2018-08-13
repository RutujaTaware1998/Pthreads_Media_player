#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include <string.h>
#include <unistd.h>

#define BUF_SIZE 10

struct frame
{
	int f1;
	int f2;

};

int in_buffer[BUF_SIZE];
struct frame out_buffer[BUF_SIZE];

int in_add = 0;
int in_rem = 0;
int in_num =0;

int out_add = 0;
int out_rem = 0;
int out_num = 0;


pthread_mutex_t in_m = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t out_m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  c_net = PTHREAD_COND_INITIALIZER;
pthread_cond_t  c_dec_in = PTHREAD_COND_INITIALIZER;
pthread_cond_t  c_dec_out = PTHREAD_COND_INITIALIZER;
pthread_cond_t  c_rend = PTHREAD_COND_INITIALIZER;

void* listen_to_port(int portno);
void* decompress(void* param);
void* renderer(void* param);
void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc , char* argv[])
{	
	
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
    int portno;
    portno = atoi(argv[1]);
     





	pthread_t tid1 ,tid2 ,tid3;
	int i;

	pthread_create(&tid1 , NULL , (void*(*)(void*))listen_to_port , (void*)portno);
	pthread_create(&tid2  , NULL ,decompress , NULL);
	pthread_create(&tid3 , NULL , renderer , NULL);

	pthread_join(tid1 ,NULL);
	pthread_join(tid2,NULL);

	pthread_exit(NULL);
}

void* listen_to_port(int portno)
{	
	int sockfd, newsockfd, clilen;
    
     

    struct sockaddr_in serv_addr, cli_addr;
    int n;
    
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     listen(sockfd,10);
     clilen = sizeof(cli_addr);
     



	int  i;
	while(1)
	{	

		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	    if (newsockfd < 0) 
	         error("ERROR on accept");
	   
	    int n;
	    n = read(newsockfd,&i,sizeof(int));
	    if (n < 0) error("ERROR reading from socket");
	    printf("\nHere is the message: %d\n",i);
	    n = write(newsockfd,"I got your message",18);
	    if (n < 0) error("ERROR writing to socket");
	    



	   // printf("\nAdding to buffer");
		pthread_mutex_lock(&in_m);
	//	printf("\nAdding to buffer");
		while(in_num == BUF_SIZE)
		{
			pthread_cond_wait(&c_net , &in_m);
		}
		printf("\nAdding to in_buffer");
		in_buffer[in_add] = i;
		in_add = (in_add+1)%BUF_SIZE;
		in_num++;
		pthread_mutex_unlock(&in_m);
		pthread_cond_signal(&c_dec_in);
		printf("\n inserted %d in in_buffer" ,i);
	}

	printf("\nNetwork thread quitting");

}



void* decompress(void* param)
{
	int i;
	int k=0;
	struct  frame f;
	while(1)
	{
		pthread_mutex_lock(&in_m);
		printf("\nDecompressing");
		while(in_num ==0)
		{
			pthread_cond_wait(&c_dec_in , &in_m);
		}

		i = in_buffer[in_rem];
		in_rem= (in_rem+1)%BUF_SIZE;
		in_num--;
		pthread_mutex_unlock(&in_m);
		pthread_cond_signal(&c_net);
		printf("\nDecompress value %d" , i );


		k= (k+1)%2;

		if(k==1)
		{
			f.f1 = i;
		}

		else if(k==0)
		{
			f.f2 = i;
		
		}



		if(k ==0 )
		{
			pthread_mutex_lock(&out_m);
		//	printf("\nAdding to buffer");
			while(out_num == BUF_SIZE)
			{
				pthread_cond_wait(&c_dec_out , &out_m);
			}
			printf("\nAdding to out_buffer");
			out_buffer[out_add] = f;
			out_add = (out_add+1)%BUF_SIZE;
			out_num++;
			pthread_mutex_unlock(&out_m);
			pthread_cond_signal(&c_rend);
			printf("\n inserted %d , %d in out_buffer frame" ,f.f1 , f.f2);
		}




	}	
}


void* renderer(void* param)
{	
	struct frame f;
	while(1)
	{
		pthread_mutex_lock(&out_m);
		printf("\nDisplaying frame");
		while(out_num ==0)
		{
			pthread_cond_wait(&c_rend , &out_m);
		}

		f = out_buffer[out_rem];
		out_rem= (out_rem+1)%BUF_SIZE;
		out_num--;
		pthread_mutex_unlock(&out_m);
		pthread_cond_signal(&c_dec_out);
		printf("\nFrame  values %d , %d" , f.f1 , f.f2 );
	}		
}










