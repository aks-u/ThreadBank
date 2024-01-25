#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

// Structs for time-related information
struct timeval tv;
struct timezone tz;
struct tm *today;

// Function for logging messages to a log file
void logger(char str[]) {
    FILE *fp = fopen("logfile.log", "a");   // Open the log file for appending


    if (fp == NULL){
       printf("Opening file failed");
       exit(1);
    }

    // Get the current time information for a timestamp
    gettimeofday(&tv,&tz);
    today = localtime(&tv.tv_sec);

    // If the input string is a specific separator, write it directly to the log without a timestamp
    if (strcmp(str, "-------------------------------------------") == 0) {
        fprintf(fp, "%s\n", str);
    }
    else {
        // Format and write the time and the input string to the log file
        fprintf(fp, "[TIME] %d:%0d:%0d.%ld  %s\n", today->tm_hour, today->tm_min, today->tm_sec, tv.tv_usec, str);
    }
   
   
    fclose(fp);     // Close the log file
}