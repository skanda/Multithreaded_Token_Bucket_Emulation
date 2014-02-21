#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include "my402list.h"

struct tbucket_parameters{
	int B;
	float r;
	long int num_pkts;
	int token_count;
	int tokens;
	float lambda, mu;
	int P;
	double iatime;
	double sertime;
	double avgpq1;
	double avgpq2;
	double avgps;
	double avgsys;
	double tot_emu_time;
	double std_dev;
	int pkt_dropped;
};

struct packet{
    double q1start_time, q2start_time, arrival_time; // in microseconds
    float lambda, mu;
    int P,p_num;
	//Keep a count for identifying packet count during sig handling
};

void* pktarrival(void *arg);
void* tokdeposit(void *arg);
void* pkttrans(void *arg);
void handler(int sig);
void Usage();
void ReadLine(char *data, float *lambda, float *mu, int *P);
int ReadCommandLine(int argc, char *argv[],struct tbucket_parameters *t);
void ptrholder(void *arg);

int fileread = 0;
My402List Q1, Q2;
int pkt_count=0;
int tot_tokens=0;
int flag = 0;
int control =0;
//Statistics variables

pthread_t t_pktarr, t_tokdep, t_server,t_handler;
pthread_mutex_t mutex;
pthread_cond_t cond;
FILE *fptr;
double init_time;
struct timeval ti; 

void init(struct tbucket_parameters *tbp, int B, float r, int num, float lambda, float mu, int P)
	{
		tbp->B=B;
		tbp->r=r;
		tbp->num_pkts=num;
		tbp->token_count=0;
		tbp->tokens=0;
		tbp->lambda=lambda;
		tbp->mu=mu;
		tbp->P=P;
		
		tbp->iatime=0;
		tbp->sertime=0;
		tbp->avgpq1=0;
		tbp->avgpq2=0;
		tbp->avgps=0;
		tbp->avgsys=0;
		tbp->std_dev=0;
		tbp->pkt_dropped=0;
	}

	void statistics(struct tbucket_parameters *tbparam)
	{ pthread_mutex_lock(&mutex);
	printf("\nStatistics:\n");
		printf("     average packet inter-arrival time = %0.8fs\n",tbparam->iatime/tbparam->num_pkts);
		
		if(tbparam->P > tbparam->B)
			printf("     average packet service time = There were no packets which are serviced out to give the value of this parameter\n\n");
		else
			printf("     average packet service time = %0.8fs\n\n",tbparam->sertime/tbparam->num_pkts);
		
		printf("     average number of packets in Q1 = %0.8f\n",tbparam->avgpq1/tbparam->tot_emu_time);
		printf("     average number of packets in Q2 = %0.8f\n",tbparam->avgpq2/tbparam->tot_emu_time);
		printf("     average number of packets in S = %0.8f\n\n",tbparam->avgps/tbparam->tot_emu_time);
		
		if(tbparam->P > tbparam->B)
			{printf("     average time a packet spent in system = There were no packets in the system to give the value for this parameter\n");
			printf("     standard deviation for time spent in system = The value for this parameter cannot be determined.\n\n"); }
		else
		{printf("     average time a packet spent in system = %0.8fs\n",tbparam->avgsys/tbparam->num_pkts);
		printf("     standard deviation for time spent in system = %0.8fs\n\n",tbparam->std_dev); }
		
		printf("     token drop probability = %0.8f\n",(float)tbparam->token_count/(tot_tokens-1));
		printf("     packet drop probability = %0.8f\n",(float)tbparam->pkt_dropped/tbparam->num_pkts);
		
	My402ListUnlinkAll(&Q1);
	My402ListUnlinkAll(&Q2);
	pthread_mutex_unlock(&mutex);
	exit(0);
	}
	
int main(int argc, char *argv[])
	{
		struct tbucket_parameters t;
		int a;
		init(&t,10, 1.5, 20, 0.5, 0.35, 3);
		struct tbucket_parameters *tbp = (struct tbucket_parameters *)malloc(sizeof(struct tbucket_parameters));
		a = tbp->B;
		
		(void) signal(SIGINT,handler);
		
		ReadCommandLine(argc, argv, &t);
		control =1;
		ptrholder((void *)&t); 
		My402ListInit(&Q1);
		My402ListInit(&Q2);
	
		pthread_mutex_init(&mutex,NULL);
		pthread_cond_init(&cond,0);
		
		printf("00000000.000ms: emulation begins\n");
		gettimeofday(&ti, NULL);
		init_time = ti.tv_sec+ti.tv_usec/1000000.0;

		pthread_create(&t_pktarr, NULL, pktarrival, (void*)&t);
		pthread_create(&t_tokdep, NULL, tokdeposit, (void*)&t);
		pthread_create(&t_server, NULL, pkttrans, (void*)&t);

		pthread_join(t_pktarr, 0);
		pthread_join(t_server, 0);
		
		exit(0);		
		return 0;
	}

void* pktarrival(void *arg)
	{
		struct tbucket_parameters *tb = (struct tbucket_parameters *)arg;
		int i,m;
		char data[30];
		double current_time,temp_time=0;
		char temp[20];
		char *lhs, *rhs;
		long int t_val;
		
		m = tb->num_pkts;
		for(i=1;i<=tb->num_pkts;i++)
		{
			if(fileread == 1)
			{
				if(fgets(data,30, fptr))
				{
					ReadLine(data, &tb->lambda, &tb->mu, &tb->P);
					m--;
					
			//		printf("After reading Line\n");
			//		printf("%f\t %f\t %f\t %d\t %d\t %d\n",tb->lambda, tb->mu, tb->r, tb->B, tb->P, j);
				}
				else
				{
					fclose(fptr);
				//	fprintf(stderr,"Done!\n");
				//	break;
				}
			}
			usleep(1000000.0/(tb->lambda));
			
			gettimeofday(&ti,NULL);
				current_time=ti.tv_sec+ti.tv_usec/1000000.0;
				sprintf(temp,"%0.3f",(current_time-init_time)*1000);
				lhs=strtok_r(temp,".",&rhs);
				t_val = strtoul(lhs,NULL,10);
				tb->iatime+= current_time-init_time-temp_time;
			if(tb->P > tb->B)
		{
			tb->pkt_dropped++;
			temp_time=current_time-init_time;
			gettimeofday(&ti,NULL);
				current_time=ti.tv_sec+ti.tv_usec/1000000.0;
				sprintf(temp,"%0.3f",(current_time-init_time)*1000);
				lhs=strtok_r(temp,".",&rhs);
				t_val = strtoul(lhs,NULL,10);
				pkt_count++;
				tb->tot_emu_time = current_time-init_time;
			//	pthread_testcancel();
			printf("%08ld.%sms: Packet p%d arrives, needs %d tokens, dropped\n",t_val,rhs,i,tb->P);
			if(tb->num_pkts == pkt_count)			
				pthread_cond_signal(&cond);	
				
			continue;
		}
			pthread_mutex_lock(&mutex);
				pthread_cleanup_push(pthread_mutex_unlock(&mutex), (void *)&mutex);
				
				struct packet  *p = (struct packet *)malloc(sizeof(struct packet));
				
				printf("%08ld.%sms: p%d arrives, needs %d tokens, inter-arrival time = %0.3fms\n",t_val,rhs, i,tb->P,(current_time-init_time-temp_time)*1000.0f);
				
				temp_time=current_time-init_time;
							
				p->mu=tb->mu;
				p->P =tb->P;
				p->p_num = i;
				p->arrival_time=(current_time-init_time);
				
				My402ListAppend(&Q1,(void*)p);
				gettimeofday(&ti,NULL);
				current_time=ti.tv_sec+ti.tv_usec/1000000.0;
				p->q1start_time=(current_time-init_time);
				sprintf(temp,"%0.3f",(current_time-init_time)*1000);
				lhs=strtok_r(temp,".",&rhs);
				t_val = strtoul(lhs,NULL,10);
				
				printf("%08ld.%sms: p%d enters Q1\n",t_val,rhs,p->p_num);
				
				if(My402ListLength(&Q1)==1)
					{
						if(tb->tokens > tb->P)
						{	
							tb->tokens = tb->tokens - tb->P;
							
							My402ListUnlink(&Q1,My402ListFirst(&Q1));
							gettimeofday(&ti,NULL);
							current_time=ti.tv_sec+ti.tv_usec/1000000.0;
							sprintf(temp,"%0.3f",(current_time-init_time)*1000);
							lhs=strtok_r(temp,".",&rhs);
							t_val = strtoul(lhs,NULL,10);
							
							tb->avgpq1+=current_time - init_time - p->q1start_time;
							printf("%08ld.%sms: p%d leaves Q1, time in Q1 = %0.3fms, token bucket now has %d tokens\n",t_val,rhs, i, (current_time-init_time-p->q1start_time)*1000.0f,tb->tokens);
										
							My402ListAppend(&Q2, (void*)p); //Should I calculate time here?
							gettimeofday(&ti,NULL);
							current_time=ti.tv_sec+ti.tv_usec/1000000.0;
							p->q2start_time=current_time-init_time;
							sprintf(temp,"%0.3f",(current_time-init_time)*1000);
							lhs=strtok_r(temp,".",&rhs);
							t_val = strtoul(lhs,NULL,10);
							pkt_count++;
							printf("%08ld.%sms: p%d enters Q2\n",t_val,rhs,p->p_num);
							
							if(My402ListLength(&Q2)==1) 
							pthread_cond_signal(&cond);	
							
						}
					}
			pthread_mutex_unlock(&mutex);
			pthread_cleanup_pop(0);
		}
		if(tb->P > tb->B)
		statistics(tb);
		
		return 0;
	}

void* tokdeposit(void *arg)
	{
		struct tbucket_parameters *tb = (struct tbucket_parameters *)arg;
		double current_time;
		char temp[20];
		char *lhs, *rhs;
		long int t_val;
		int j =1;
		
		while(1)
		{
			usleep(1000000.0/(tb->r));
			pthread_mutex_lock(&mutex);
				pthread_cleanup_push(pthread_mutex_unlock(&mutex), (void *)&mutex);
				if(pkt_count == tb->num_pkts )
						{
							pthread_mutex_unlock(&mutex);
							break;
						}
				//	tb->tokens++;
					
					if(tb->tokens >= tb->B)
					{
						printf("%08ld.%sms: token t%d arrives,dropped\n",t_val,rhs, j);
						tb->token_count++;
						if(tb->num_pkts == pkt_count)
						{
							pthread_mutex_unlock(&mutex);
							break;
						}
					}
					
					else
					{
						tb->tokens++;
						gettimeofday(&ti,NULL);
					current_time=ti.tv_sec+ti.tv_usec/1000000.0;
					sprintf(temp,"%0.3f",(current_time-init_time)*1000);
					lhs=strtok_r(temp,".",&rhs);
					t_val = strtoul(lhs,NULL,10);
					printf("%08ld.%sms: token t%d arrives, token bucket now has %d tokens\n",t_val,rhs, j,tb->tokens);
			
					}
					j++;
					tot_tokens = j;
									
					
					
			if(!(My402ListEmpty(&Q1)))
			{
				My402ListElem *elem = My402ListFirst(&Q1);
				struct packet *pkt  = (struct packet *)elem->obj;
				if(tb->tokens >= pkt->P)
				{
					tb->tokens = tb->tokens - pkt->P;
					My402ListUnlink(&Q1,elem);
					
					gettimeofday(&ti,NULL);
					current_time=ti.tv_sec+ti.tv_usec/1000000.0;
					sprintf(temp,"%0.3f",(current_time-init_time)*1000);
					lhs=strtok_r(temp,".",&rhs);
					t_val = strtoul(lhs,NULL,10);
					tb->avgpq1+=current_time - init_time - pkt->q1start_time;
					printf("%08ld.%sms: p%d leaves Q1, time in Q1 = %0.3fms, token bucket now has %d tokens\n",t_val,rhs, pkt->p_num, (current_time-init_time-pkt->q1start_time)*1000.0f,tb->tokens);
					
					My402ListAppend(&Q2,(void *)pkt);
					
					gettimeofday(&ti,NULL);
					current_time=ti.tv_sec+ti.tv_usec/1000000.0;
					pkt->q2start_time=current_time-init_time;
					sprintf(temp,"%0.3f",(current_time-init_time)*1000);
					lhs=strtok_r(temp,".",&rhs);
					t_val = strtoul(lhs,NULL,10);
					pkt_count++;
					printf("%08ld.%sms: p%d enters Q2\n",t_val,rhs,pkt->p_num);
					
					pthread_cond_signal(&cond);
					
				}
			}				
			
			pthread_mutex_unlock(&mutex);
			pthread_cleanup_pop(0);
			
		}
		return 0;
	}

void* pkttrans(void *arg)
	{
		struct tbucket_parameters *tb = (struct tbucket_parameters *)arg;
		int i;
		double current_time,timesys,temp_time=0;
		char temp[20];
		char *lhs, *rhs;
		long int t_val;
		struct packet *pkt;
		
		//Statistics variables
		double avg_sq = 0;
		for(i=0;i<tb->num_pkts;i++)
		{
		//wait for queue not empty to be signalled
		pthread_mutex_lock(&mutex);
			pthread_cleanup_push(pthread_mutex_unlock(&mutex), (void *)&mutex);
			while(My402ListEmpty(&Q2))
				pthread_cond_wait(&cond,&mutex);
		pthread_mutex_unlock(&mutex);
		flag = 1;
		pthread_mutex_lock(&mutex);
		
			My402ListElem *elem = My402ListFirst(&Q2);
			pkt  = (struct packet *)elem->obj;
			
			gettimeofday(&ti,NULL);
			current_time=ti.tv_sec+ti.tv_usec/1000000.0;
			temp_time=current_time-init_time;
			sprintf(temp,"%0.3f",(current_time-init_time)*1000);
			lhs=strtok_r(temp,".",&rhs);
			t_val = strtoul(lhs,NULL,10);
			
			tb->avgpq2+=current_time-init_time-pkt->q2start_time;
			printf("%08ld.%sms: p%d begin service at S, time in Q2 = %.3fms\n",t_val,rhs,pkt->p_num,(current_time-init_time-pkt->q2start_time)*1000.0f);
			
			My402ListUnlink(&Q2,elem);
		pthread_mutex_unlock(&mutex);
			usleep(1000000.0/pkt->mu);
		pthread_mutex_lock(&mutex);
			gettimeofday(&ti,NULL);
			current_time=ti.tv_sec+ti.tv_usec/1000000.0;
			sprintf(temp,"%0.3f",(current_time-init_time)*1000);
			lhs=strtok_r(temp,".",&rhs);
			t_val = strtoul(lhs,NULL,10);
			
			timesys = current_time - init_time - pkt->q1start_time;
			tb->sertime+=current_time-init_time-temp_time;
			tb->avgps+=current_time-init_time-temp_time;
			tb->avgsys+=current_time - init_time - pkt->q1start_time;
			tb->tot_emu_time = current_time-init_time;
			avg_sq += (timesys*timesys);
			tb->std_dev = sqrt((avg_sq/tb->num_pkts) - ((tb->avgsys/tb->num_pkts)*(tb->avgsys/tb->num_pkts)));
			
			printf("%08ld.%sms: p%d departs from S, service time = %.3fms time in system = %.3fms\n",t_val,rhs,pkt->p_num,current_time-init_time-temp_time,(current_time - init_time-pkt->q1start_time)*1000.0f);
		
			
			pthread_mutex_unlock(&mutex);
			flag=0;
		pthread_cleanup_pop(0);
		}
		statistics(tb);
		return 0;
	}

void Usage()
    {
        /* print out usage informaiton, i.e. commandline syntax */
		fprintf(stderr, "The syntax is : \nwarmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
        exit(1);
    }

void ReadLine(char *data, float *lambda, float *mu, int *P)
	{
		char *temp;
			temp = strtok(data, " \n\t");		
			*lambda = 1000.0f/(float)atoi(temp);
			temp = strtok(0, " \n\t");
			*P = atoi(temp);
			temp = strtok(0, " \n\t");
			*mu = 1000.0f/(float)atoi(temp);
	}

int ReadCommandLine(int argc, char *argv[],struct tbucket_parameters *t)
	{
		int i=0;
		
		if(argc ==1)
		{
				printf("\nEmulation Parameters:\n");
				printf("    lambda = %f\n",t->lambda);
				printf("    mu = %f\n",t->mu);
				printf("    r = %f\n",t->r);
				printf("    B = %d\n",t->B);
				printf("    P = %d\n",t->P);
				printf("    number to arrive = %ld\n\n",t->num_pkts);
				return 0;
		}
	
		if(argc % 2 == 0)
		{
			Usage();
		}
	
		for(i=1;i<argc;i=i+2)
		{
			if(*argv[i] != '-')
			{
				Usage();
			}
		
			if(strncmp(argv[i], "-t", 2)==0)
			{				
				fptr=fopen(argv[i+1],"r");
				if(fptr == NULL)
				{
					fprintf(stderr,"Input file not in correct format\n");
					exit(1);
				}
			
				else
				{
					char data[11];
					fgets(data,sizeof(data),fptr);
					data[strlen(data)-1]='\0';
					t->num_pkts = atoi(data);
					fileread = 1;
				
					if(t->num_pkts > 2147483647)
					{
						fprintf(stderr,"Number out of range\n");
						exit(1);
					}
					
					else if(t->num_pkts ==0)
					{
					printf("Statistics:\n");
		printf("     average packet inter-arrival time = There were no packets which are serviced out to give the value of this parameter\n");
		printf("     average packet service time = There were no packets which are serviced out to give the value of this parameter\n\n");
		printf("     average number of packets in Q1 = 0\n");
		printf("     average number of packets in Q2 = 0\n");
		printf("     average number of packets in S = 0\n\n");
		printf("     average time a packet spent in system = There were no packets which are serviced out to give the value of this parameter\n");
		printf("     standard deviation for time spent in system = The value for this parameter cannot be determined.\n\n");
		 
		printf("     token drop probability = 0\n");
		printf("     packet drop probability = 0\n");
					exit(0);
					}
				}
				printf("\nEmulation Parameters:\n");
				printf("    r = %f\n",t->r);
				printf("    B = %d\n",t->B);
				printf("    tsfile = %s\n\n",argv[i+1]);
				return 0;
			}
			
			else
			{
				if(strncmp(argv[i],"-lambda", 7)==0)
				{
					if(argv[i+1]!=NULL)
						t->lambda = (float)strtod(argv[i+1],0);
				
					if(t->lambda < 0.0)
						exit(0);
				
					if(1/(t->lambda) > 10.0)
						t->lambda = 1/10.0;
						
				}	
		
				else if(strncmp(argv[i], "-mu", 3)==0)
				{
					if(argv[i+1]!=NULL)
						t->mu = (float)strtod(argv[i+1], 0);
				
					if(t->mu < 0.0)
						exit(0);
				
					if(1/(t->mu) > 10.0)
						t->mu = 1/10.0;
				}
		
				else if(strncmp(argv[i], "-r",2)==0)
				{
					if(argv[i+1]!=NULL)
						t->r = (float)strtod(argv[i+1],0);
					
					if(1/(t->r)>10.0)
						t->r = 1/10.0;
				}

				else if(strncmp(argv[i], "-B",2)==0)
				{
					if(argv[i+1]!=NULL)
						t->B = atoi(argv[i+1]);

					if(t->B > 2147483647)
					{
						fprintf(stderr,"Number out of range\n");
						exit(1);
					}
				}
		
				else if(strncmp(argv[i], "-P",2)==0)
				{
					if(argv[i+1]!=NULL)
					t->P = atoi(argv[i+1]);

					if(t->P > 2147483647)
					{
						fprintf(stderr,"Number out of range\n");
						exit(1);
					}
				}
		
				else if(strncmp(argv[i], "-n",2)==0)
				{
					if(argv[i+1]!=NULL)
					t->num_pkts = atoi(argv[i+1]);
					if(t->num_pkts > 2147483647)
					{
						fprintf(stderr,"Number out of range\n");
						exit(1);
					}
				}
				
				else
				Usage();
				
				
			}
		}
				printf("\nEmulation Parameters:\n");
				printf("    lambda = %f\n",t->lambda);
				printf("    mu = %f\n",t->mu);
				printf("    r = %f\n",t->r);
				printf("    B = %d\n",t->B);
				printf("    P = %d\n",t->P);
				printf("    number to arrive = %ld\n\n",t->num_pkts);			
			return 0;
	}
	
/*Taken from Standard C Library Functions*/

	 void handler(int sig)
     {
		struct tbucket_parameters tbparam;
		pthread_cancel(t_pktarr);
	pthread_cancel(t_tokdep);
			while(flag==1);
			fprintf(stderr,"\nCtrl+C was pressed, terminating the program\n");
			ptrholder((void*)&tbparam);
     }
	 
 void ptrholder(void *arg) 
	 {
		struct tbucket_parameters *tbparam = (struct tbucket_parameters *)arg;
		if(control==0)
		statistics(tbparam);
		
		control = 0;
	 }

