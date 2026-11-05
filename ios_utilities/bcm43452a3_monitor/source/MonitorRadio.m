// SPDX-License-Identifier: GPL-3.0-only
// Fixed en0 autojoin getter/setter. Exact iOS15.7 selectors; no association API.
#import <Foundation/Foundation.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
@interface NSObject (SurveyRadio)
- (id)initWithServiceType:(long long)type;
- (void)resume;
- (void)invalidate;
- (NSString *)interfaceName;
- (BOOL)userAutoJoinDisabled;
- (BOOL)setUserAutoJoinDisabled:(BOOL)value error:(NSError **)error;
@end
int main(int argc,char **argv) {@autoreleasepool {
    if(argc==2 && !strcmp(argv[1],"--self-test")){puts("{\"firmware_commands\":0,\"preference_writes\":0}");return 0;}
    BOOL get=argc==2 && !strcmp(argv[1],"--get");
    BOOL set=argc==3 && !strcmp(argv[1],"--set") && (!strcmp(argv[2],"0") || !strcmp(argv[2],"1"));
    if(!get && !set)return 64;
    if(!dlopen("/System/Library/PrivateFrameworks/CoreWiFi.framework/CoreWiFi",RTLD_NOW))return 2;
    id wifi=[[NSClassFromString(@"CWFInterface") alloc]initWithServiceType:1];
    if(!wifi)return 2;
    int result=1;
    @try {
        [wifi resume];
        [[NSRunLoop currentRunLoop]runUntilDate:[NSDate dateWithTimeIntervalSinceNow:.5]];
        if(![[wifi interfaceName]isEqualToString:@"en0"]){puts("{\"ok\":false,\"error\":\"CoreWiFi en0 unavailable\"}");return 3;}
        NSError *error=nil;
        if(set && ![wifi setUserAutoJoinDisabled:!strcmp(argv[2],"1") error:&error]) {
            printf("{\"ok\":false,\"error_code\":%ld}\n",(long)error.code);return 4;
        }
        BOOL disabled=[wifi userAutoJoinDisabled];
        if(set && disabled!=(!strcmp(argv[2],"1"))){puts("{\"ok\":false,\"error\":\"Autojoin readback mismatch\"}");return 5;}
        printf("{\"ok\":true,\"disabled\":%s,\"preference_writes\":%d}\n",disabled ? "true":"false",set ? 1:0);result=0;
    } @catch(NSException *error) {puts("{\"ok\":false,\"error\":\"CoreWiFi exception\"}");}
    @finally {[wifi invalidate];}
    return result;
}}
