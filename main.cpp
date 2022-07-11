#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#else
#include <io.h>
#define F_OK 0
#define access _access
#endif


#include "utils.h"


int exist(const char *path)
{
    return (access(path, F_OK) == 0);
}

int main(int argc, char** argv){

    char argDeviceModel[64] = {0};
    char argComponent[256] = {0};
    bool isErase = true;
    char argOutPath[1024] = {0};
    char argFirmwarePath[1024] = {0};

    if( argc < 3 ){
        printf("example: \n");
        printf("ipsw_extract -d n71ap -c iBSS iPhone8,1_13.5_17F35.ipsw\n");
        return -1;
    }


    for(int i=0; i < argc; i++)
    {
        if(strcmp(argv[i], "-d") == 0){
            strcpy(argDeviceModel, argv[i+1]);
        }
        else if(strcmp(argv[i], "-c") == 0){
            strcpy(argComponent, argv[i+1]);
        }
        else if(strcmp(argv[i], "-u") == 0){
            isErase = false;
        }
        else if(strcmp(argv[i], "-o") == 0){
            strcpy(argOutPath, argv[i+1]);
        }
        else{
            strcpy(argFirmwarePath, argv[i+1]);
        }
    }

    if( strlen(argComponent) == 0 )
    {
        if(strlen(argOutPath) == 0 ){
            strcpy(argOutPath, "./BuildManifest.plist");
        }

        if( extract_file(argFirmwarePath, "BuildManifest.plist", argOutPath) < 0){
            printf("ERROR: Cannot extract BuildManifest.plist.");
            return -1;
        }

        printf("Extract BuildManifest.plist ... ok.");
        return 0;
    }
    else
    {
        plist_t buildmanifest =  extract_buildmanifest_plist(argFirmwarePath);
        if(buildmanifest == NULL){
            printf("ERROR: Cannot get buildmanifest.");
            return -1;
        }

        plist_t buildid = find_buildid(buildmanifest, argDeviceModel, isErase);
        if(buildid == NULL){
            printf("ERROR: Cannot get buildid.");
            return -1;
        }

        const char* internal_path = NULL;
        if( find_component_path(buildid, argComponent, &internal_path) < 0 ){
            printf("ERROR: Cannot get internal_path of %s.", argComponent);
            return -1;
        }

        if(strlen(argOutPath) == 0 ){
            const char* p = strrchr(internal_path, '/');
            if( p ){
                strcpy(argOutPath, p + 1);
            }
            else{
                strcpy(argOutPath, internal_path);
            }
        }

        if( extract_file(argFirmwarePath, internal_path, argOutPath) < 0){
            printf("ERROR: Cannot extract %s.", argComponent);
            return -1;
        }

        printf("Extract %s ... ok.", argComponent);
        return 0;
    }

    return -1;
}

