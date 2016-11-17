/*
 * (C) Copyright 2016 Egor Zindy (https://github.com/zindy)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#define HWTYPE_ANY 0 //Any Thorlabs APT device

#include <stdio.h>
#include <stdlib.h>

#ifndef WIN32
    typedef enum { false, true } BOOL;
    typedef char TCHAR;
    #define WINAPI
#endif

#include "APTAPI.h"

int main() {
    long i, ret, nDevices, SerialNumber;
    long hwType = HWTYPE_ANY;

    ret = APTInit();
    if (ret < 0)
        goto end;


    ret = GetNumHWUnitsEx(hwType, &nDevices);
    printf("Number of Thorlabs devices found: %ld\n", nDevices);

    for (i = 0; i < nDevices; i++) { 
        ret = GetHWSerialNumEx(hwType, i, &SerialNumber);
        printf("\nDev %ld: %ld\n",i,SerialNumber); 

        //open the device
        ret = InitHWDevice(SerialNumber);
        ret = MOT_SetChannel(SerialNumber, 1);

        ret = MOT_Identify(SerialNumber);
        ret = MOT_EnableHWChannel(SerialNumber);
        ret = MOT_MoveHome(SerialNumber,true);

        if (false) {
            float AbsolutePosition = 0;
            ret = MOT_MoveAbsoluteEx(SerialNumber,AbsolutePosition,true);
        }

        if (true) {
            float RelativePosition = 0;
            ret = MOT_MoveRelativeEx(SerialNumber,RelativePosition,true);
        }

        if (true) {
            float Position;
            ret = MOT_GetPosition(SerialNumber, &Position);
            printf("Position=%.2f\n",Position);
        }

        if (false) {
            float MinPos;
            float MaxPos;
            long Units;
            float Pitch;
            MOT_GetStageAxisInfo(SerialNumber, &MinPos, &MaxPos, &Units, &Pitch);
            printf("MinPos=%.2f, MaxPos=%.2f, Units=%ld, Pitch=%.2f\n",MinPos,MaxPos,Units,Pitch);
        }

        if (false) {
            float MinVel;
            float Accn;
            float MaxVel;
            ret = MOT_GetVelParams(SerialNumber, &MinVel, &Accn, &MaxVel);
            printf("MinVel=%.2f, Accn=%.2f, MaxVel=%.2f\n",MinVel,Accn,MaxVel);

            /*
            MinVel = 0;
            Accn = 13.744*10.;
            MaxVel = 134218*99;
            ret = MOT_SetVelParams(SerialNumber, MinVel, Accn, MaxVel);

            ret = MOT_GetVelParams(SerialNumber, &MinVel, &Accn, &MaxVel);
            printf("MinVel=%.2f, Accn=%.2f, MaxVel=%.2f\n",MinVel,Accn,MaxVel);
            */
        }

        if (false) {
            ret = MOT_DisableHWChannel(SerialNumber);
            //sleep_ms(1000);
        }
    } 

end:
    printf("\nCleaning-up...\n");
    ret = APTCleanUp();
    return ret;
}
