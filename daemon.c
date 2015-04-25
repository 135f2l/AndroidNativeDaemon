#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/stat.h>
#define BUFFER_LENGTH 1024
#define FLAG_FILE "daemon.lock"
#define ROOT_PATH "/data/data/"
#define SLEEP_INTERVEL 60  // every x seconds to check if process is running
#define VERSION "v100"

int isProcessExist(char* processName);
void runProcess(char* packageName, char* serviceName);
int isAppUninstalled(char* packagePath);
void log2file(const char* format, ...);
void initDaemon();

int isEnableLog = 0;
char packagePath[256] = ROOT_PATH;
char logFilePath[512] = "";

/**
 * main
 * @param  argc [6 parameters]
 * @param  argv
 * [packagename,processname,ServiceNameOrActivityName,isEnableLog,logFilePath]
 * e.g. packagename: "com.yourapp";
 * processname:"com.yourapp:servicetag" ;
 * servicename: "com.yourapp.someService"; 
 * isEnableLog: "1";
 * logFilePath: "";
 * @return      [description]
 */
int main(int argc, char** argv) {
  if (argc < 6) {
    fprintf(stderr,
            "Usage:%s <packagename> <processname> <ServiceNameOrActivityName> "
            "<isEnableLog> <logFilePath>\n",
            argv[0]);
    return -1;
  }

  initDaemon();

  char* packageName = argv[1];
  char* processName = argv[2];
  char* serviceName = argv[3];
  char* enableLogFlag = argv[4];
  char* logFilePara = argv[5];

  strcat(logFilePath, logFilePara);

  if (strcmp(enableLogFlag, "1") == 0) {
    isEnableLog = 1;
  } else {
    isEnableLog = 0;
  }


  // packagePath = ROOT_PATH;
  // packagePath ==> /data/data/ctrip.android.view:pushsdk.v1
  strcat(packagePath, packageName);
  log2file(
      "packagename is: %s.\nprocessname is: %s.\nservicename is: "
      "%s\nisEnableLog: %s\npackagePath is: %s\nlogFilePath is: %s\n",
      packageName, processName, serviceName, enableLogFlag, packagePath, logFilePath);

  // check if the app has been uninstalled or the path is invalid
  if (isAppUninstalled(packagePath) == -1) {
    log2file("app has been uninstalled. exit.\n");
    exit(-1);
  }

  char flagFilePath[256] = "";
  // flagFilePath ==> /data/data/ctrip.android.view:pushsdk.v1
  strcat(flagFilePath, packagePath);
  // flagFilePath ==> /data/data/ctrip.android.view:pushsdk.v1/
  strcat(flagFilePath, "/");
  // flagFilePath ==> /data/data/ctrip.android.view:pushsdk.v1/daemon.lock
  strcat(flagFilePath, FLAG_FILE);
  log2file("flagFilePath is: %s\n", flagFilePath);

  int ret = -1;
  FILE* g_lockfile = NULL;

  //check if self is already running
  g_lockfile = fopen(flagFilePath, "a+");
  if (g_lockfile == NULL) {
    fprintf(stderr, "fopen() failed:%s!\n", strerror(errno));
    return -1;
  }

  log2file("g_lockfile opened.\n");

  ret = 0;
  ret = flock(fileno(g_lockfile), LOCK_EX | LOCK_NB);
  if (ret != 0) {
    fprintf(stderr, "flock() failed:%s!\n", strerror(errno));
    log2file("daemon already running. exit.\n");
    return -1;
  }

  // check android app/service is alive
  while (1) {
    if (isAppUninstalled(packagePath) == -1) {
      log2file("app has been uninstalled. exit.\n");
      exit(-1);
    }
    if (!isProcessExist(processName)) {
      runProcess(packageName, serviceName);
    }

    log2file("sleep for %d seconds...\n************************\n", SLEEP_INTERVEL);
    sleep(SLEEP_INTERVEL);
  }

  return 0;
}

/**
 * init the daemon, all the magic in it.
 */
void initDaemon(){

  if(fork() != 0){
    exit(0);
  }

  //setpgrp();// is equivalent to setpgid(0,0)
  setsid(); //its critical

  signal(SIGINT, SIG_IGN);
  signal(SIGHUP, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGCHLD, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  signal(SIGKILL, SIG_IGN);
  
  if(fork() != 0){
    exit(0);
  }
  //chdir("/");
  //umask(0);
}

/**
 * check if app's already been uninstalled
 * @param  packagePath [description]
 * @return             [description]
 */
int isAppUninstalled(char* packagePath) {
  if (access(packagePath, 0) == -1) {
    printf("app uninstalled. file:'%s' not exits.\n", packagePath);
    return -1;
  }
  return 1;
}

/**
 * check if the process is running. by using ps command.
 * @param  processName [description]
 * @return             [description]
 */
int isProcessExist(char* processName) {
  char buf[BUFFER_LENGTH];
  char command[BUFFER_LENGTH];
  FILE* fp;
  int ret = 0;
  sprintf(command, "ps -c | grep %s", processName);

  //log2file("command is: %s\n", command);

  if ((fp = popen(command, "r")) == NULL) {
    printf("popen failed\n");
    exit(1);
  }

  if ((fgets(buf, BUFFER_LENGTH, fp)) != NULL) {
    ret = 1;
    log2file("ps info:%s\n", buf);
  } else {
    log2file("process: %s not running.\n", processName);
  }

  pclose(fp);
  return ret;
}

/**
 * actully run the android app/service
 * @param packageName [description]
 * @param serviceName [description]
 */
void runProcess(char* packageName, char* serviceName) {
  FILE* fp;
  char service_name[BUFFER_LENGTH];
  sprintf(service_name, "%s/%s", packageName, serviceName);
  
  char command[BUFFER_LENGTH];
  sprintf(command, "am startservice --user 0 -n %s", service_name);
  // sprintf(command, "am startservice -n %s/%s", packageName, serviceName);
  log2file("run cmd: %s\n", command);


  //execlp("am","am","startservice","--user",uid,"-n",service_name, (char *)NULL);

  if ((fp = popen(command, "r")) == NULL) {
    log2file("popen failed\n");
    exit(1);
  }

  pclose(fp);
}

/**
 * log to file
 */
void log2file(const char* format, ...) {

  if (!isEnableLog) {
    return;
  }

  char logContent[BUFFER_LENGTH] = "";
  va_list args;
  va_start(args, format);
  vsprintf(logContent, format, args);
  va_end(args);

  //use date as the log file name
  time_t currentTime = time(NULL);
  char logFileName[20] = "";
  strftime(logFileName, sizeof(logFileName), "%F",
           localtime(&currentTime));  // yyyy-mm-dd
  char logTime[20] = "";
  strftime(logTime, sizeof(logTime), "%T",
           localtime(&currentTime));  // H%:M%:S%

  char logContentWithTimestamp[BUFFER_LENGTH] = "";
  sprintf(logContentWithTimestamp, "%s : %s", logTime, logContent);
  printf("%s\n", logContentWithTimestamp);

  char logFileFullName[256] = "";
  //printf("log file path is: %s/%s.log\n", packagePath, logFileName);
  sprintf(logFileFullName, "%s/daemon-%s-%s.log", logFilePath, VERSION, logFileName);

  FILE* logfile = fopen(logFileFullName, "a+");
  if (logfile == NULL) {
    //fprintf(stderr, "fopen() failed:%s!\n", strerror(errno));
    logfile = fopen(logFileFullName, "w+");
  }
  if (logfile == NULL) {
    fprintf(stderr, "fopen() failed:%s!\n", strerror(errno));
    return;
  }
  fprintf(logfile, "%s\n", logContentWithTimestamp);
  fclose(logfile);
}

