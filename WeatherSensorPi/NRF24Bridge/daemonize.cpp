#include <syslog.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h> //umask
#include <unistd.h> //setsid
#include <stdio.h> //perror
#include <stdlib.h>
#include <signal.h> //sidaction
#include <string.h> 
#include <errno.h> 
#include <sys/file.h>
#include <sys/stat.h>

#include "daemonize.h"

#define LOCKDIR "/var/run/WeatherSensorPi"
#define LOCKFILE "/var/run/WeatherSensorPi/WeatherSensorPi.pid"
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) 

int fds_in, fds_out, fds_err;

typedef struct stat Stat;

static int do_mkdir(const char* path, mode_t mode)
{
	Stat            st;
	int             status = 0;

	if (stat(path, &st) != 0)
	{
		/* Directory does not exist. EEXIST for race condition */
		if (mkdir(path, mode) != 0 && errno != EEXIST)
			status = -1;
	}
	else if (!S_ISDIR(st.st_mode))
	{
		errno = ENOTDIR;
		status = -1;
	}

	return(status);
}


int already_running(void)
{
	syslog(LOG_ERR, "Checking for process copies...");

	int fd;
	char buf[16];

	do_mkdir(LOCKDIR, 0777);

	fd = open(LOCKFILE, O_RDWR | O_CREAT, LOCKMODE);

	if (fd < 0)
	{
		syslog(LOG_ERR, "Error in opening %s: %s!", LOCKFILE, strerror(errno));
		exit(1);
	}

	syslog(LOG_WARNING, "Lock-file is opened!");

	flock(fd, LOCK_EX | LOCK_NB);

	if (errno == EWOULDBLOCK)
	{
		syslog(LOG_ERR, "Error in locking %s: %s!", LOCKFILE, strerror(errno));
		exit(1);
	}

	syslog(LOG_WARNING, "Writing PID...");
	ftruncate(fd, 0);

	sprintf(buf, "%ld", (long)getpid());

	write(fd, buf, strlen(buf) + 1);

	syslog(LOG_WARNING, "PID was writter successfully.");

	return 0;
}

void daemonize(const char* cmd)
{
	int fd0, fd1, fd2;
	pid_t pid;
	struct sigaction sa;

	// Step 1. forking to become group leader
	pid = fork();
	if (pid == -1)
	{
		perror("fork error\n");
	}
	else if (pid != 0)
		exit(0);

	// Step 2. setsid()
	setsid();

	// Step 3. forking to become session leader.
	pid = fork();
	if (pid == -1)
	{
		perror("fork error\n");
	}
	else if (pid != 0)
		exit(0);

	// Step 4. chdir("/")
	chdir("/");

	// Step 5. umask(0)
	umask(0);

	// Step 6. Closing all fds
	int max = sysconf(_SC_OPEN_MAX);
	for (int i = 0; i < max; i++)
	{
		close(i);
	}

	// Step 7. Establishing stdin, stdout, stderr
	fds_in = open("/dev/null", O_RDWR);
	//fds_out = dup(stdin);
	//fds_err = dup(stderr);

	// Initializing log file
	openlog(cmd, LOG_CONS, LOG_DAEMON);
	if (fds_in == -1 || fds_out == -1 || fds_err == -1)
	{
		syslog(LOG_ERR, "Error in some descriptors: in: %d, out: %d, err: %d", fds_in, fds_out, fds_err);
		exit(1);
	}
}