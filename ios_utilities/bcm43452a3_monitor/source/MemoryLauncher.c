// SPDX-License-Identifier: GPL-3.0-only
/* Set a bounded memory budget for this job only, before Python loads. */
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

extern int memorystatus_control(uint32_t, int32_t, uint32_t, void *, size_t);
typedef struct { int32_t active; uint32_t active_attr; int32_t inactive; uint32_t inactive_attr; } Limits;

int main(int argc, char **argv) {
    if(argc<5 || getuid()!=0 || (strcmp(argv[2],"root") && strcmp(argv[2],"mobile")))return 64;
    char *end=NULL; long budget=strtol(argv[1],&end,10);
    if(!end || *end || budget<32 || budget>256 || argv[3][0]!='/')return 64;
    Limits limits={(int32_t)budget,1,(int32_t)budget,1}, actual={0};
    if(memorystatus_control(7,getpid(),0,&limits,sizeof(limits)) ||
       memorystatus_control(8,getpid(),0,&actual,sizeof(actual)) ||
       actual.active!=budget || actual.inactive!=budget) {
        perror("WiFiSurvey memory budget"); return 70;
    }
    fprintf(stderr,"WiFiSurvey job memory budget: %ld MB active/inactive\n",budget);
    // Carry the kernel-verified value across setuid: GET may require root.
    if(setenv("WIFISURVEY_MEMORY_LIMIT_MB",argv[1],1))return 70;
    if(!strcmp(argv[2],"mobile")) {
        struct passwd *account=getpwnam("mobile");
        if(!account || initgroups(account->pw_name,account->pw_gid) || setgid(account->pw_gid) || setuid(account->pw_uid)) {
            perror("WiFiSurvey drop privileges");return 71;
        }
    }
    execv(argv[3],argv+3);
    perror("WiFiSurvey exec");return 72;
}
