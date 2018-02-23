#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <ifaddrs.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/statvfs.h>

#define TRUE 1
#define FALSE 0

#define CPULOAD  "/proc/loadavg"
#define CPUSTAT  "/proc/stat"
#define RAMUSE   "/proc/meminfo"

#define BINGB 1073741824
#define KBINGB 1048576

#ifdef LAPTOP
#define BATSTATE "/sys/class/power_supply/BAT0/status"
#define BATCAP   "/sys/class/power_supply/BAT0/capacity"
#define NETDEVWIFI "wlp2s0"
#define NETDEVLAN "enp0s20u"
#else
#define NETDEV "enp5s0"
#endif

#define DARKRED     "#d32f2f"
#define DARKGREEN   "#388e3c"
#define DARKYELLOW  "#fbc02c"
#define DARKBLUE    "#3949ab"
#define DARKMAGENTA "#c2185b"
#define DARKCYAN    "#0097a7"
#define LIGHTGREY   "#9e9e9e"
#define DARKGREY    "#424242"
#define RED         "#f44336"
#define GREEN       "#66bb6a"
#define YELLOW      "#ffeb3b"
#define BLUE        "#3f51b5"
#define MAGENTA     "#e91e63"
#define CYAN        "#00bcd4"
#define BLACK       "#212121"
#define GREY8       "#424242"
#define GREY7       "#616161"
#define GREY6       "#757575"
#define GREY5       "#9e9e9e"
#define GREY4       "#bdbdbd"
#define GREY3       "#e0e0e0"
#define GREY2       "#eeeeee"
#define GREY1       "#f5f5f5"
#define WHITE       "#fafafa"

// %{B#424242}%{F#9e9e9e}  %{B#9e9e9e}%{F#212121} LIGHTSEP
// %{B#9e9e9e}%{F#424242}  %{B#424242}%{F#eeeeee} DARKSEP

#define LIGHTSEP ""
#define DARKSEP ""

// For time handling
long   csec; //Centi-seconds i.e. 1/100th of a second
time_t sec;

struct timespec sleepTime, currentTime;

#define ClockFlag ((flags >> 5) & 1)
#define setClockFlag(b) (flags ^= (-b ^ flags) & (1 << 5))
timer_t clockTimerid;
struct sigevent clockSev;
struct itimerspec clockTrigger;
struct tm* tm;
time_t t;
char str_time[10];
char str_date[20];

size_t len = 0;

char flags = 0;

#define updateFlag (flags & 1)
#define setUpdateFlag(b) (flags ^= (-b ^ flags) & 1)

timer_t cpuTimerid;
struct sigevent cpuSev;
struct itimerspec cpuTrigger;
unsigned int totalJiffies1 = 0;
unsigned int totalJiffies2 = 0;
unsigned int totalJiffiesDiff = 0;
unsigned int workJiffies1 = 0;
unsigned int workJiffies2 = 0;
unsigned int workJiffiesDiff = 0;
long double cpuUsage;
long double currentLoad[3];

timer_t cpuTempTimerid;
struct sigevent cpuTempSev;
struct itimerspec cpuTempTrigger;
char cpuTempFilename[50];
unsigned int cpuTemp;

struct cpu {
    int user;
    int nice;
    int system;
    int idle;
    int iowait;
    int irq;
    int softirq;
};
struct cpu cpuStat;

timer_t ramTimerid;
struct sigevent ramSev;
struct itimerspec ramTrigger;
float totalRam;
float freeRam;
float usedRam;

#define WIFIFlag ((flags >> 6) & 1)
#define setWIFIFlag(b) (flags ^= (-b ^ flags) & (1 << 6))
timer_t ipTimerid;
struct sigevent ipSev;
struct itimerspec ipTrigger;
char* ipaddr;
struct ifaddrs* addrs;
struct ifaddrs* tmp;

timer_t diskTimerid;
struct sigevent diskSev;
struct itimerspec diskTrigger;
float rootDirUsed;
float rootDirFree;
float rootDirTotal;
float homeDirUsed;
float homeDirFree;
float homeDirTotal;
struct statvfs rootDir, homeDir;

#ifdef DESKTOP
float massDirUsed;
float massDirFree;
float massDirTotal;
float mediaDirUsed;
float mediaDirFree;
float mediaDirTotal;
struct statvfs massDir, mediaDir;
#endif

#ifdef LAPTOP
timer_t batTimerid;
struct sigevent batSev;
struct itimerspec batTrigger;
timer_t batStatTimerid;
struct sigevent batStatSev;
struct itimerspec batStatTrigger;
char* batStatusOut;
int batCap = 100;
int batCapRounded = 100;
#endif

struct statusBar {
    char dateString[25];
    char timeString[25];
    char ramUseString[25];
    char cpuLoadString[35];
    char cpuTempString[15];
    char ipString[25];
#ifdef LAPTOP
    char batString[25];
#endif
    char homeSpaceString[25];
    char rootSpaceString[25];
#ifdef DESKTOP
    char massSpaceString[25];
    char mediaSpaceString[25];
#endif
};

struct statusBar mainStatusBar;
char stringOut[2048];

void getTime();
void getCPULoad();
void findCPUTempFile();
void getCPUTemp();
void getRAMUse();
void getIP();
void getFreeSpace();

#ifdef LAPTOP
void getBatStatus();
void getBatCharge();
#endif
