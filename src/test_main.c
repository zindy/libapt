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

#include "libapt.c"

int main() {
    long retval = EXIT_SUCCESS;
    long i;
    struct ftdi_version_info version;

    version = ftdi_get_library_version();
    printf("Initialized libftdi %s (major: %d, minor: %d, micro: %d, snapshot ver: %s)\n",
        version.version_str, version.major, version.minor, version.micro,
        version.snapshot_str);

    retval = APTInit();
    printf("Number of Thorlabs devices found: %d\n", numDevs);

    if (retval < 0)
        goto end;

    long ret = 0;

    if (numDevs > 0) { 
        for (i = 0; i < numDevs; i++) { 
            printf("\nDev %ld:\n",i); 

            //open the device
            ret = InitHWDevice(aptInfo[i].SerialNumber);
            ret = MOT_SetChannel(aptInfo[i].SerialNumber, 1);

            ret = MOT_Identify(aptInfo[i].SerialNumber);
            ret = MOT_EnableHWChannel(aptInfo[i].SerialNumber);
            ret = MOT_MoveHome(aptInfo[i].SerialNumber,true);

            if (false) {
                float AbsolutePosition = 0;
                ret = MOT_MoveAbsoluteEx(aptInfo[i].SerialNumber,AbsolutePosition,true);
            }

            if (true) {
                float RelativePosition = 0;
                ret = MOT_MoveRelativeEx(aptInfo[i].SerialNumber,RelativePosition,true);
            }

            if (true) {
                float Position;
                ret = MOT_GetPosition(aptInfo[i].SerialNumber, &Position);
                printf("Position=%.2f\n",Position);
            }

            if (false) {
                float MinPos;
                float MaxPos;
                long Units;
                float Pitch;
                MOT_GetStageAxisInfo(aptInfo[i].SerialNumber, &MinPos, &MaxPos, &Units, &Pitch);
                printf("MinPos=%.2f, MaxPos=%.2f, Units=%ld, Pitch=%.2f\n",MinPos,MaxPos,Units,Pitch);
            }

            if (false) {
                float MinVel;
                float Accn;
                float MaxVel;
                ret = MOT_GetVelParams(aptInfo[i].SerialNumber, &MinVel, &Accn, &MaxVel);
                printf("MinVel=%.2f, Accn=%.2f, MaxVel=%.2f\n",MinVel,Accn,MaxVel);

                /*
                MinVel = 0;
                Accn = 13.744*10.;
                MaxVel = 134218*99;
                ret = MOT_SetVelParams(aptInfo[i].SerialNumber, MinVel, Accn, MaxVel);

                ret = MOT_GetVelParams(aptInfo[i].SerialNumber, &MinVel, &Accn, &MaxVel);
                printf("MinVel=%.2f, Accn=%.2f, MaxVel=%.2f\n",MinVel,Accn,MaxVel);
                */
            }

            /*
            ret = MOT_DisableHWChannel(aptInfo[i].SerialNumber);
            sleep_ms(1000);
            */
            //sleep_ms(1000);

            if (ret < 0) {
                printf(" Error %ld when opening device %ld\n", ret, i);
                fprintf(stderr, "Unable to open device %ld: (%s)",
                        i, ftdi_get_error_string(ftdic));
                continue;
            }

            printf(" APT SerialNumber=%ld\n",aptInfo[i].SerialNumber);
            printf(" APT Model Number='%s'\n",aptInfo[i].ModelNumber);
            printf(" APT Type=%ld\n",aptInfo[i].Type);
            printf(" APT HardwareType=%ld\n",aptInfo[i].HardwareType);
            printf(" APT FirmwareVersion='%s'\n",aptInfo[i].FirmwareVersion);
            printf(" APT Notes='%s'\n",aptInfo[i].Notes);
            printf(" APT HardwareVersion=%ld\n",aptInfo[i].HardwareVersion);
            printf(" APT ModState=%ld\n",aptInfo[i].ModState);
            printf(" APT Number of Channels=%ld\n",aptInfo[i].NumberChannels);
            /*
            printf(" Stage PartNoAxis='%s'\n",aptInfo[i].PartNoAxis);
            printf(" Stage SerialNumAxis=%ld\n",aptInfo[i].SerialNumAxis);
            printf(" Stage CntsPerUnit=%ld\n",aptInfo[i].CntsPerUnit);
            printf(" Stage MinPos=%ld\n",aptInfo[i].MinPos);
            printf(" Stage MaxPos=%ld\n",aptInfo[i].MaxPos);
            printf(" Stage MaxAccn=%ld\n",aptInfo[i].MaxAccn);
            printf(" Stage MaxDec=%ld\n",aptInfo[i].MaxDec);
            printf(" Stage MaxVel=%ld\n",aptInfo[i].MaxVel);
            */
        } 

        //ret = GetNumHWUnitsEx(HWTYPE_TDC001,&i);
        //printf("\nNumber of TDC001 devices: %ld\n",i);
    } 
end:
    printf("\nCleaning-up...\n");
    retval = APTCleanUp();
    return retval;
}
