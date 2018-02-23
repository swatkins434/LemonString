#include "lemonbar.h"

void getTime()
{
    t = time(NULL);
    tm = localtime(&t);

    strftime(str_time, sizeof(str_time), "%H:%M:%S", tm);
    strftime(str_date, sizeof(str_date), "%a %b %d/%m/%Y", tm);

    sprintf(mainStatusBar.timeString, ": %s", str_time);
    sprintf(mainStatusBar.dateString, ": %s", str_date);
    setUpdateFlag(TRUE);
}

void getCPULoad()
{
    FILE * cpuFP;

    cpuFP = fopen(CPULOAD,"r");
    fscanf(cpuFP,"%Lf %Lf %Lf",&currentLoad[0],&currentLoad[1],&currentLoad[2]);
    fclose(cpuFP);

    cpuFP = fopen(CPUSTAT,"r");
    fscanf(
        cpuFP,
        "cpu %u %u %u %u %u %u %u",
        &cpuStat.user,
        &cpuStat.nice,
        &cpuStat.system,
        &cpuStat.idle,
        &cpuStat.iowait,
        &cpuStat.irq,
        &cpuStat.softirq
    );
    fclose(cpuFP);

    workJiffies2 = cpuStat.user + cpuStat.nice + cpuStat.system;
    totalJiffies2 = cpuStat.user + cpuStat.nice + cpuStat.system + cpuStat.idle + cpuStat.iowait + cpuStat.irq + cpuStat.softirq;

    workJiffiesDiff = workJiffies2 - workJiffies1;
    totalJiffiesDiff = totalJiffies2 - totalJiffies1;

    workJiffies1 = workJiffies2;
    totalJiffies1 = totalJiffies2;

    cpuUsage = ((long double) workJiffiesDiff / (long double) totalJiffiesDiff) * 100.0l;

    sprintf(mainStatusBar.cpuLoadString, ": %.2Lf, %.2Lf, %.2Lf (%6.2Lf%)", currentLoad[0], currentLoad[1], currentLoad[2], cpuUsage);
    setUpdateFlag(TRUE);
}

void findCPUTempFile()
{
    FILE * cpuTempFP;
    int deviceNum = 0;
    // Don't know what the upper limit on device names is
    char deviceName[100];

    // Coretemp is the name of the temperature sensor for the CPU
    while(TRUE)
    {
        sprintf(cpuTempFilename, "/sys/class/hwmon/hwmon%d/name", deviceNum);
        cpuTempFP = fopen(cpuTempFilename,"r");
        fscanf(cpuTempFP, "%s", deviceName);
        fclose(cpuTempFP);

        if(strcmp(deviceName, "coretemp") == 0)
        {
            sprintf(cpuTempFilename, "/sys/class/hwmon/hwmon%d/temp1_input", deviceNum);
            return;
        }
        else
        {
            ++deviceNum;
        }
    }
}

void getCPUTemp()
{
    FILE * cpuTempFP;

    cpuTempFP = fopen(cpuTempFilename,"r");
    fscanf(cpuTempFP, "%d", &cpuTemp);
    fclose(cpuTempFP);

    cpuTemp /= 1000;

    sprintf(mainStatusBar.cpuTempString, ": %d°C", cpuTemp);
    setUpdateFlag(TRUE);
}

void getRAMUse()
{
    FILE* ramFP;
    char * line = NULL;

    ramFP = fopen(RAMUSE, "r");
    getline(&line, &len, ramFP);
    sscanf(line, "%*s %f %*s", &totalRam);

    // Skip second line
    getline(&line, &len, ramFP);

    getline(&line, &len, ramFP);
    sscanf(line, "%*s %f %*s", &freeRam);

    fclose(ramFP);
    if (line)
        free(line);

    totalRam = totalRam / KBINGB;
    freeRam = freeRam / KBINGB;

    usedRam = (totalRam - freeRam);

    sprintf(mainStatusBar.ramUseString, ": %.1fG/%.1fG", usedRam, totalRam);
    setUpdateFlag(TRUE);
}

void getIP()
{
    getifaddrs(&addrs);
    tmp = addrs;

    while (tmp)
    {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
        {
#ifdef LAPTOP
            if(strcmp(tmp->ifa_name, NETDEVWIFI) == 0)
            {
                setWIFIFlag(TRUE);
#else
            if(strcmp(tmp->ifa_name, NETDEV) == 0)
            {
#endif
                struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
                ipaddr = inet_ntoa(pAddr->sin_addr);
            }
#ifdef LAPTOP
            else if(strncmp(tmp->ifa_name, NETDEVLAN, strlen(NETDEVLAN)) == 0)
            {
                setWIFIFlag(FALSE);
                struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
                ipaddr = inet_ntoa(pAddr->sin_addr);
            }
#endif
        }

        tmp = tmp->ifa_next;
    }
    freeifaddrs(addrs);

#ifdef LAPTOP
    if(WIFIFlag == TRUE)
    {
        sprintf(mainStatusBar.ipString, ": %s", ipaddr);
    }
    else
    {
#endif
        sprintf(mainStatusBar.ipString, ": %s", ipaddr);
#ifdef LAPTOP
    }
#endif
setUpdateFlag(TRUE);
}

void getFreeSpace()
{
    statvfs("/", &rootDir);
    statvfs("/home", &homeDir);

    rootDirFree = ((float) (rootDir.f_bavail * rootDir.f_frsize)) / BINGB;
    homeDirFree = ((float) (homeDir.f_bavail * homeDir.f_frsize)) / BINGB;

    rootDirTotal = ((float) (rootDir.f_blocks * rootDir.f_frsize)) / BINGB;
    homeDirTotal = ((float) (homeDir.f_blocks * homeDir.f_frsize)) / BINGB;

    rootDirUsed = rootDirTotal - rootDirFree;
    homeDirUsed = homeDirTotal - homeDirFree;

    sprintf(mainStatusBar.rootSpaceString, "/: %.1lfGB/%.1lfGB", rootDirUsed, rootDirTotal);
    sprintf(mainStatusBar.homeSpaceString, ": %.1lfGB/%.1lfGB", homeDirUsed, homeDirTotal);

#ifdef DESKTOP
    statvfs("/mnt/mass_storage", &massDir);

    massDirFree = ((float) (massDir.f_bavail * massDir.f_frsize)) / BINGB;
    massDirTotal = ((float) (massDir.f_blocks * massDir.f_frsize)) / BINGB;
    massDirUsed = massDirTotal - massDirFree;

    sprintf(mainStatusBar.massSpaceString, ": %.1lfGB/%.1lfGB", massDirUsed, massDirTotal);

    if(statvfs("/mnt/media", &mediaDir) == 0)
    {
        mediaDirFree = ((float) (mediaDir.f_bavail * mediaDir.f_frsize)) / BINGB;
        mediaDirTotal = ((float) (mediaDir.f_blocks * mediaDir.f_frsize)) / BINGB;
        mediaDirUsed = mediaDirTotal - mediaDirFree;

        sprintf(mainStatusBar.mediaSpaceString, ": %.1lfGB/%.1lfGB", mediaDirUsed, mediaDirTotal);
    }
    else
    {
        // There was some error when looking for /mnt/media, in all likelihood
        // it didn't mount correctly.
        sprintf(mainStatusBar.mediaSpaceString, "MEDIA DRIVE NOT MOUNTED!");
    }
#endif
setUpdateFlag(TRUE);
}

#ifdef LAPTOP
void getBatStatus()
{
    char batStatus[12];

    FILE *fp;
    fp = fopen(BATSTATE, "r");
    fscanf(fp, " %s ", &batStatus);
    fclose(fp);

    if(strcmp(batStatus, "Charging") == 0)
    {
        batStatusOut = "";
    }
    else
    {
        batCapRounded = (int) round((double) batCap / 25) * 25;
        if(batCapRounded == 100)
        {
            batStatusOut = "";
        }
        else if(batCapRounded == 75)
        {
            batStatusOut = "";
        }
        else if(batCapRounded == 50)
        {
            batStatusOut = "";
        }
        else if(batCapRounded == 25)
        {
            batStatusOut = "";
        }
        else
        {
            batStatusOut = "";
        }
    }

    sprintf(mainStatusBar.batString, "%s: %d%%", batStatusOut, batCap);
    setUpdateFlag(TRUE);
}
#endif

#ifdef LAPTOP
void getBatCharge()
{
    FILE *fp;
    fp = fopen(BATCAP, "r");
    fscanf(fp, "%d", &batCap);
    fclose(fp);

    sprintf(mainStatusBar.batString, "%s: %d%%", batStatusOut, batCap);
    setUpdateFlag(TRUE);
}
#endif

int main(int argc, char const *argv[])
{
    // Start by finding the CPU temperature sensor file
    findCPUTempFile();

    // sleepTime.tv_sec = 0;
    // sleepTime.tv_nsec = 100000000L;
    sleepTime.tv_sec = 1;
    sleepTime.tv_nsec = 0L;

    // Set up timers for each string function
    memset(&clockSev, 0, sizeof(struct sigevent));
    memset(&cpuSev, 0, sizeof(struct sigevent));
    memset(&cpuTempSev, 0, sizeof(struct sigevent));
    memset(&ramSev, 0, sizeof(struct sigevent));
    memset(&ipSev, 0, sizeof(struct sigevent));
    memset(&diskSev, 0, sizeof(struct sigevent));
#ifdef LAPTOP
    memset(&batSev, 0, sizeof(struct sigevent));
    memset(&batStatSev, 0, sizeof(struct sigevent));
#endif

    memset(&clockTrigger, 0, sizeof(struct itimerspec));
    memset(&cpuTrigger, 0, sizeof(struct itimerspec));
    memset(&cpuTempTrigger, 0, sizeof(struct itimerspec));
    memset(&ramTrigger, 0, sizeof(struct itimerspec));
    memset(&ipTrigger, 0, sizeof(struct itimerspec));
    memset(&diskTrigger, 0, sizeof(struct itimerspec));
#ifdef LAPTOP
    memset(&batTrigger, 0, sizeof(struct itimerspec));
    memset(&batStatTrigger, 0, sizeof(struct itimerspec));
#endif

    clockSev.sigev_notify   = SIGEV_THREAD;
    cpuSev.sigev_notify     = SIGEV_THREAD;
    cpuTempSev.sigev_notify = SIGEV_THREAD;
    ramSev.sigev_notify     = SIGEV_THREAD;
    ipSev.sigev_notify      = SIGEV_THREAD;
    diskSev.sigev_notify    = SIGEV_THREAD;
#ifdef LAPTOP
    batSev.sigev_notify     = SIGEV_THREAD;
    batStatSev.sigev_notify = SIGEV_THREAD;
#endif

    clockSev.sigev_notify_function   = &getTime;
    cpuSev.sigev_notify_function     = &getCPULoad;
    cpuTempSev.sigev_notify_function = &getCPUTemp;
    ramSev.sigev_notify_function     = &getRAMUse;
    ipSev.sigev_notify_function      = &getIP;
    diskSev.sigev_notify_function    = &getFreeSpace;
#ifdef LAPTOP
    batSev.sigev_notify_function     = &getBatCharge;
    batStatSev.sigev_notify_function = &getBatStatus;
#endif

    clockSev.sigev_value.sival_ptr   = clockTimerid;
    cpuSev.sigev_value.sival_ptr     = cpuTimerid;
    cpuTempSev.sigev_value.sival_ptr = cpuTempTimerid;
    ramSev.sigev_value.sival_ptr     = ramTimerid;
    ipSev.sigev_value.sival_ptr      = ipTimerid;
    diskSev.sigev_value.sival_ptr    = diskTimerid;
#ifdef LAPTOP
    batSev.sigev_value.sival_ptr     = batTimerid;
    batStatSev.sigev_value.sival_ptr = batStatTimerid;
#endif

    timer_create(CLOCK_REALTIME, &clockSev,   &clockTimerid);
    timer_create(CLOCK_REALTIME, &cpuSev,     &cpuTimerid);
    timer_create(CLOCK_REALTIME, &cpuTempSev, &cpuTempTimerid);
    timer_create(CLOCK_REALTIME, &ramSev,     &ramTimerid);
    timer_create(CLOCK_REALTIME, &ipSev,      &ipTimerid);
    timer_create(CLOCK_REALTIME, &diskSev,    &diskTimerid);
#ifdef LAPTOP
    timer_create(CLOCK_REALTIME, &batSev,     &batTimerid);
    timer_create(CLOCK_REALTIME, &batStatSev, &batStatTimerid);
#endif

    clockTrigger.it_value.tv_sec   = 1;
    cpuTrigger.it_value.tv_sec     = 1;
    cpuTempTrigger.it_value.tv_sec = 1;
    ramTrigger.it_value.tv_sec     = 1;
    ipTrigger.it_value.tv_sec      = 10;
    diskTrigger.it_value.tv_sec    = 60;
#ifdef LAPTOP
    batTrigger.it_value.tv_sec     = 10;
    batStatTrigger.it_value.tv_sec = 2;
#endif

    clockTrigger.it_interval.tv_sec   = 1;
    cpuTrigger.it_interval.tv_sec     = 1;
    cpuTempTrigger.it_interval.tv_sec = 1;
    ramTrigger.it_interval.tv_sec     = 1;
    ipTrigger.it_interval.tv_sec      = 10;
    diskTrigger.it_interval.tv_sec    = 60;
#ifdef LAPTOP
    batTrigger.it_interval.tv_sec     = 10;
    batStatTrigger.it_interval.tv_sec = 2;
#endif

    timer_settime(clockTimerid,   0, &clockTrigger, NULL);
    timer_settime(cpuTimerid,     0, &cpuTrigger, NULL);
    timer_settime(cpuTempTimerid, 0, &cpuTempTrigger, NULL);
    timer_settime(ramTimerid,     0, &ramTrigger, NULL);
    timer_settime(ipTimerid,      0, &ipTrigger, NULL);
    timer_settime(diskTimerid,    0, &diskTrigger, NULL);
#ifdef LAPTOP
    timer_settime(batTimerid,     0, &batTrigger, NULL);
    timer_settime(batStatTimerid, 0, &batStatTrigger, NULL);
#endif

    // Initially populate statusBar with data.
    getTime();
    getCPULoad();
    getCPUTemp();
    getRAMUse();
    getIP();
    getFreeSpace();
#ifdef LAPTOP
    getBatCharge();
    getBatStatus();
#endif

    while(1)
    {
        if(updateFlag)
        {
            sprintf(
                stringOut,
#ifdef LAPTOP
                "%{Sf}%{r}%1$s %3$s %2$s %4$s %1$s %5$s %2$s %6$s %1$s %7$s %2$s %8$s %1$s %9$s %2$s %10$s %1$s %11$s",
#endif

#ifdef DESKTOP
                "%{Sf}%{r}%1$s %3$s %2$s %4$s %1$s %5$s %2$s %6$s %1$s %7$s %2$s %8$s %1$s %9$s %2$s %10$s %1$s %11$s %2$s %12$s %{Sl}%{r}%1$s %3$s %2$s %4$s %1$s %5$s %2$s %6$s %1$s %7$s %2$s %8$s %1$s %9$s %2$s %10$s %1$s %11$s %2$s %12$s",
#endif
                LIGHTSEP,
                DARKSEP,
                mainStatusBar.rootSpaceString,
                mainStatusBar.homeSpaceString,
#ifdef DESKTOP
                mainStatusBar.massSpaceString,
                mainStatusBar.mediaSpaceString,
#endif
                mainStatusBar.ipString,
                mainStatusBar.cpuLoadString,
                mainStatusBar.cpuTempString,
                mainStatusBar.ramUseString,
#ifdef LAPTOP
                mainStatusBar.batString,
#endif
                mainStatusBar.timeString,
                mainStatusBar.dateString
            );
            setUpdateFlag(FALSE);
        }

        printf("%s \n", stringOut);

        // Don't want to eat all the CPU time
        nanosleep(&sleepTime, NULL);
    }

    timer_delete(clockTimerid);
    timer_delete(cpuTimerid);
    timer_delete(cpuTempTimerid);
    timer_delete(ramTimerid);
    timer_delete(clockTimerid);
    timer_delete(diskTimerid);
#ifdef LAPTOP
    timer_delete(batTimerid);
    timer_delete(batStatTimerid);
#endif

    return 0;
}
