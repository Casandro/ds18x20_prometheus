#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <threads.h>
#include <stdatomic.h>
#include <netdb.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <dirent.h>

#define T_ERR (-300000)

typedef struct sensor_s {
	struct sensor_s *next;
	char *id;
	char *label;
	atomic_int status; // 0: no value yet, <0: locked, 1: valid data
	double temperature;
} sensor_t;

sensor_t *sensor=NULL;

void add_sensor(const char *id, const char *label)
{
	sensor_t *s=malloc(sizeof(sensor_t));
	if (s==NULL) {
		printf("add_sensor %s, malloc failed\n", id);
		return;
	}
	if (label==NULL) 
		printf("Adding sensor %s\n", id);
		else printf("Adding sensor %s with label %s\n", id, label);
	s->status=0;
	s->temperature=0;
	s->id=strdup(id);
	s->label=NULL;
	if (label!=NULL) s->label=strdup(label);
	s->next=sensor;
	sensor=s;
}

sensor_t *find_sensor_(sensor_t *s, const char *id)
{
	if (s==NULL) return s;
	if (strcmp(s->id, id)==0) return s;
	return find_sensor_(s->next, id);
}

sensor_t *find_sensor(const char *id)
{
	return find_sensor_(sensor, id);
}

int64_t get_sensor_data(const char *sname)
{
	if (sname==NULL) return T_ERR;
	if (strlen(sname)>30) return T_ERR;
	char fname[64];
	memset(fname, 0, sizeof(fname));
	snprintf(fname, sizeof(fname)-1, "/sys/bus/w1/devices/%s/temperature", sname);

	FILE *f=fopen(fname, "r");
	if (f==NULL) return T_ERR;
	int64_t t;
	if (fscanf(f, "%lld", &t)!=1) return T_ERR;
	fclose(f);
	return t;
}

void enumerate_sensors()
{
	DIR *dp;
	struct dirent *ep;
	dp=opendir("/sys/bus/w1/devices/");
	if (dp==NULL) return;
	while ((ep=readdir(dp))) {
		if (ep->d_name[0]=='.') continue;
		sensor_t *s=find_sensor(ep->d_name);
		if (s!=NULL) continue;
		int64_t res=get_sensor_data(ep->d_name);
		if (res==T_ERR) continue;
		add_sensor(ep->d_name,ep->d_name);
	}
	closedir(dp);
}

void print_sensor_metrics(int fd, sensor_t *s)
{
	if (s==NULL) return;
	if (s->status>0){
		char mline[128];
		s->status=-1; //Locked
		snprintf(mline, sizeof(mline)-1, "temperature{sensor_serial=\"%s\",sensor=\"%s\"} %0.3lf\n", s->id, s->label, s->temperature);
		mline[sizeof(mline)-1]='\0';
		s->status=2; //Unlock
		send(fd, mline, strlen(mline), 0);
		printf("%s\n", mline);
	}
	print_sensor_metrics(fd, s->next);
}

void update_sensor_data(sensor_t *s) {
	if (s==NULL) return;
	usleep(100000);
	int status=s->status;
	if (s->status<0) goto final;
	if (s->status==1) goto final; //Unread data
	int64_t sd=get_sensor_data(s->id);
	if ((sd==T_ERR) || (sd<=-50000) || (sd>=85000) ) {
		status=0;
		goto final;
	}
	if ( (sd<=-50000) || (sd>=85000)) goto final;
	s->status=-2; //Locked by update sensor_data
	s->temperature=((double)sd)/1000;
	status=1;
final:
	s->status=status;
	update_sensor_data(s->next);

}

int sensorthread(void *thr_data) {
	(void) thr_data;
	while(1==1) {
		update_sensor_data(sensor);
		sleep(1);
	}
}


void send_string(const int fd, const char *s)
{
	send(fd, s, strlen(s), 0);
}

void webserver(const int port)
{
	int sockfs=socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server;
	memset(&server, 0, sizeof(server));
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=htonl(INADDR_ANY);
	server.sin_port=htons(port);
	if (bind(sockfs, (struct sockaddr*)&server, sizeof(server)) <0) {
		printf("Bind failed\n");
		return;
	}
	if(listen(sockfs, 6)==-1) {
		printf("Listen failed\n");
		return;
	}
	printf("Starting Webserver\n");
	for (;;) {
		struct sockaddr_in client;
		socklen_t len=sizeof(client);
		int sock2=accept(sockfs, (struct sockaddr*)&client, &len);
		if (sock2<0) {
			printf("Accept failed %d\n", sock2);
			return;
		}
		send_string(sock2, "HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=UTF-8\r\nConnection: close\r\n\r\n");
		print_sensor_metrics(sock2, sensor);
		shutdown(sock2, SHUT_WR);
		close(sock2);
	}
}

int main(int argc, char *argv[])
{
	if (argc<3) {
		printf("Usage: %s <port> <sensor-serial>=<label>...\n", argv[0]);
		return 1;
	}
	int port=atoi(argv[1]);
	if ((port<=0) || (port>=65536)) {
		printf("Invalid port number %d\n", port);
		return 1;
	}
	for (int n=2; n<argc; n++) {
		char *s=strdup(argv[n]);
		char *eq=strchr(s,'=');
		if (eq!=NULL) {
			*eq='\0';
			add_sensor(s, eq+1);
		}
		if (s!=NULL) free(s);
	}
	enumerate_sensors();
	thrd_t thread;
	thrd_create(&thread, sensorthread, NULL);
	webserver(port);
	return 0;
}
