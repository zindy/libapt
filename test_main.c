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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <windef.h>
#include <winbase.h>
#include <ftd2xx.h>
#include "APTAPI.h"

#ifdef _WIN32
    #include <windows.h>

    void sleep(unsigned milliseconds)
    {
        Sleep(milliseconds);
    }
#else
    #include <unistd.h>

    void sleep(unsigned milliseconds)
    {
        usleep(milliseconds * 1000); // takes microseconds
    }
#endif

typedef struct {
     long SerialNumber;
     char ModelNumber[9];
     long Type;
     long HardwareType;
     char DestinationByte; //depends on type of controller
     char FirmwareVersion[13];
     char Notes[49];
     long HardwareVersion;
     long ModState;
     long NumberChannels;
     long ChannelId;

     //these are the axis value (changed when changing channel);
     long StageId;
     long AxisId;
     char PartNoAxis[17];
     long SerialNumAxis;
     long CntsPerUnit;
     long MinPos;
     long MaxPos;
     long MaxAccn;
     long MaxDec;
     long MaxVel;
} MY_APT_INFO;

DWORD numDevs;
FT_DEVICE_LIST_INFO_NODE *devInfo = NULL;
MY_APT_INFO *aptInfo = NULL;
ULONG uBaudRate = 115200;

long GetInfo(long lSerialNum, long *plType, char *pbDestByte) {
    long i;

    //default values
    FT_STATUS ftStatus = FT_OK;
    *pbDestByte = 0x50; // 0x50 Generic USB hardware unit

    switch (lSerialNum / 1000000) {
        case 20:
            // 20xxxxxx  Legacy single channel stepper driver  BSC001
            *plType = HWTYPE_BSC001; break;
        case 30:
            // 30xxxxxx  Legacy dual channel stepper driver  BSC002
            *plType = HWTYPE_BSC002; break;
        case 40:
            // 40xxxxxx  Single channel stepper driver  BSC101
            *plType = HWTYPE_BSC101; break;
        case 60:
            // 60xxxxxx  OptoSTDriver (mini stepper driver)  OST001
            *plType = HWTYPE_OST001; break;
        case 63:
            // 63xxxxxx  OptoDCDriver (mini DC servo driver)  ODC001
            *plType = HWTYPE_ODC001; break;
        case 70:
            // 70xxxxxx  Three channel card slot stepper driver BSC103
            *plType = HWTYPE_SCC001; break;
        case 80:
            // 80xxxxxx  Stepper Driver T-Cube TST001
            *plType = HWTYPE_TST001; break;
        case 83:
            // 83xxxxxx  DC servo driver T-Cube TDC001
            *plType = HWTYPE_TDC001; break;
        default:
            // 25xxxxxx  Legacy single channel mini stepper driver  BMS001
            // 35xxxxxx  Legacy dual channel mini stepper driver BMS002
            // 73xxxxxx  Brushless DC motherboard  BBD102/BBD103
            // 94xxxxxx  Brushless DC motor card  BBD102/BBD103
            *plType = 0;
            ftStatus = FT_DEVICE_NOT_FOUND;
    }
    return ftStatus;
}
                    
FT_STATUS GetHandle(long lSerialNum, FT_HANDLE *ftHandle) {
    long i;
    FT_STATUS ftStatus = FT_DEVICE_NOT_FOUND;

    for (i=0;i<numDevs;i++) {
        if (aptInfo[i].SerialNumber == lSerialNum) {
            *ftHandle = devInfo[i].ftHandle;
            if (devInfo[i].Flags == 0)
                ftStatus = FT_DEVICE_NOT_OPENED; 
            else
                ftStatus = FT_OK;
            break;
        }
    }
    return ftStatus;
}

FT_STATUS GetIndex(long lSerialNum, long *index) {
    long i;
    FT_STATUS ftStatus = FT_DEVICE_NOT_FOUND;

    for (i=0;i<numDevs;i++) {
        if (aptInfo[i].SerialNumber == lSerialNum) {
            *index = i;
            ftStatus = FT_OK;
            break;
        }
    }
    return ftStatus;
}

FT_STATUS GetChannelInfo(long lSerialNum, FT_HANDLE *ftHandle, long *channel, long *destination) {
    long i;
    FT_STATUS ftStatus = FT_DEVICE_NOT_FOUND;

    for (i=0;i<numDevs;i++) {
        if (aptInfo[i].SerialNumber == lSerialNum) {
            *channel = aptInfo[i].ChannelId;
            *ftHandle = devInfo[i].ftHandle;
            *destination = aptInfo[i].DestinationByte;
            ftStatus = FT_OK;
            break;
        }
    }
    return ftStatus;
}

long WINAPI APTInit(void) {
    FT_STATUS ftStatus;
    long i;

    // create the device information list 
    ftStatus = FT_CreateDeviceInfoList(&numDevs); 

    if (numDevs > 0) { 
        // allocate storage for list based on numDevs 
        devInfo = (FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*numDevs); 

        // additional info will be stored here
        aptInfo = (MY_APT_INFO *)calloc(sizeof(MY_APT_INFO),numDevs); 

        // get the device information list 
        ftStatus = FT_GetDeviceInfoList(devInfo,&numDevs); 

        if (ftStatus == FT_OK) { 
            for (i = 0; i < numDevs; i++) { 
                aptInfo[i].SerialNumber = atoi(devInfo[i].SerialNumber);
                ftStatus = GetInfo(aptInfo[i].SerialNumber, &aptInfo[i].Type, &aptInfo[i].DestinationByte);
            } 
        } 
    }

    return ftStatus;
}

long WINAPI APTCleanUp(void) {
    long i;
    FT_STATUS ftStatus = FT_OK;

    if (devInfo != NULL) free(devInfo);
    numDevs = 0;

    for (i=0;i<numDevs;i++) {
        if (devInfo[i].Flags == FT_FLAGS_OPENED)
            ftStatus = FT_Close(devInfo[i].ftHandle);
    }

    return ftStatus;
}


long WINAPI GetNumHWUnitsEx(long lHWType, long *plNumUnits) {
    long i;
    long nDevices = 0;
    FT_STATUS ftStatus = FT_OK;

    for (i=0;i<numDevs;i++) {
        if (aptInfo[i].Type == lHWType) {
            nDevices += 1;
        }
    }

    *plNumUnits = nDevices;
    return ftStatus;
}

long WINAPI GetHWSerialNumEx(long lHWType, long lIndex, long *plSerialNum) {
    long i,j;

    long nDevices = 0;

    FT_STATUS ftStatus = FT_DEVICE_NOT_FOUND;

    j = 0;
    for (i=0;i<numDevs;i++) {
        if (devInfo[i].Type == lHWType) {
            if (j == lIndex) {
                *plSerialNum = aptInfo[i].SerialNumber;
                ftStatus = FT_OK;
                break;
            }
            j = j+1;
        }
    }
    return ftStatus;
}

FT_STATUS QueryDevice(long i) {
    FT_STATUS ftStatus = FT_DEVICE_NOT_FOUND;
    FT_HANDLE ftHandle;
    long nBytes;

    BYTE txbuf[6] ={0x05,0x00,0x00,0x00,0x50,0x01};
    BYTE rxbuf[90];

    ftHandle = devInfo[i].ftHandle;
    ftStatus = (ftHandle == 0);
    txbuf[4] = aptInfo[i].DestinationByte;

    if (ftStatus == FT_OK) {
        ftStatus = FT_Write(ftHandle, txbuf, 6, &nBytes);
    }
    if (ftStatus == FT_OK) {
        ftStatus = FT_Read(ftHandle, rxbuf, 90, &nBytes);
        printf("nBytes=%d\n",nBytes);
    }
    if (ftStatus == FT_OK) {
        //copy to the appropriate structure.
        memset(aptInfo[i].ModelNumber,0,9);
        memcpy(aptInfo[i].ModelNumber,rxbuf+10,8);
        aptInfo[i].HardwareType = *(unsigned short int *)(rxbuf+18);
        sprintf(aptInfo[i].FirmwareVersion,"%d.%d.%d",
                rxbuf[22],rxbuf[21],rxbuf[20]);
        memset(aptInfo[i].Notes,0,49);
        memcpy(aptInfo[i].Notes,rxbuf+24,48);
        aptInfo[i].HardwareVersion = *(unsigned short int *)(rxbuf+84);
        aptInfo[i].ModState = *(unsigned short int *)(rxbuf+86);
        aptInfo[i].NumberChannels = *(unsigned short int *)(rxbuf+88);
        aptInfo[i].ChannelId = 0;
    }

    return ftStatus;
}

long WINAPI InitHWDevice(long lSerialNum) {
    long i;
    FT_STATUS ftStatus = FT_DEVICE_NOT_FOUND;
    FT_HANDLE ftHandle;

    for (i=0;i<numDevs;i++) {
        if (aptInfo[i].SerialNumber == lSerialNum) {
            ftStatus = FT_OK;
            if (devInfo[i].Flags != FT_FLAGS_OPENED) {
                ftStatus = FT_Open(i, &ftHandle);
            }
            if (ftStatus == FT_OK) {
                devInfo[i].ftHandle = ftHandle;
                devInfo[i].Flags = FT_FLAGS_OPENED;

                ftStatus = FT_SetDataCharacteristics(ftHandle,FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
                ftStatus += FT_SetBaudRate(ftHandle, (ULONG)uBaudRate); 
                ftStatus += FT_Purge(ftHandle, FT_PURGE_RX || FT_PURGE_TX);
                ftStatus += FT_SetTimeouts(ftHandle, 3000, 3000);//extend timeout while board finishes reset
                sleep(150);
                ftStatus += QueryDevice(i);
                sleep(150);
                ftStatus += MOT_SetChannel(lSerialNum, 1);
            }
            break;
        }
    }
    return ftStatus;
}

long WINAPI GetHWInfo(long lSerialNum, TCHAR *szModel, long lModelLen, TCHAR *szSWVer, long lSWVerLen, TCHAR *szHWNotes, long lHWNotesLen) {
    FT_STATUS ftStatus = FT_DEVICE_NOT_FOUND;
    FT_HANDLE ftHandle;
    long nBytes;
    long channel;
    long destination;
    long i;

    BYTE txbuf[6] ={0x05,0x00,0x00,0x00,0x50,0x01};
    BYTE rxbuf[90];

    ftStatus = GetIndex(lSerialNum, &i);
    if (ftStatus != FT_OK)
        goto end;

    ftStatus = GetChannelInfo(lSerialNum, &ftHandle, &channel, &destination);

    if (ftStatus == FT_OK) {
        txbuf[2] = channel+1;
        txbuf[4] = destination;
        ftStatus = FT_Write(ftHandle, txbuf, 6, &nBytes);
    }
    
    if (ftStatus == FT_OK) {
        ftStatus = FT_Read(ftHandle, rxbuf, 90, &nBytes);
    }
    
    if (ftStatus == FT_OK) {
        memset(szModel,0,lModelLen);
        memcpy(szModel,rxbuf+10,8);
        sprintf(szSWVer,"%d.%d.%d",
                rxbuf[22],rxbuf[21],rxbuf[20]);
        memset(szHWNotes,0,lHWNotesLen);
        memcpy(szHWNotes,rxbuf+24,48);
    }

end:
    return ftStatus;
}

long WINAPI MOT_SetChannel(long lSerialNum, long lChanID) {
    FT_STATUS ftStatus = FT_DEVICE_NOT_FOUND;
    FT_HANDLE ftHandle;
    long i;
    long nBytes;
    long destination;


    //MGMSG_MOT_REQ_PMDSTAGEAXISPARAMS
    BYTE txbuf[6] ={0xF1,0x04,0x01,0x00,0x21,0x01};
    BYTE rxbuf[80];

    ftStatus = GetIndex(lSerialNum, &i);
    if (ftStatus != FT_OK)
        goto end;

    if (lChanID <= aptInfo[i].NumberChannels) {
        aptInfo[i].ChannelId = lChanID;
        ftStatus = FT_OK;
    } else ftStatus = FT_IO_ERROR;

    //now write
    ftHandle = devInfo[i].ftHandle;
    printf("dest=0x%x - channelId=%d\n",aptInfo[i].DestinationByte,lChanID);
    txbuf[2] = lChanID;
    txbuf[4] = aptInfo[i].DestinationByte;

    if (ftStatus == FT_OK) {
        //ftStatus = FT_Purge(ftHandle, FT_PURGE_RX || FT_PURGE_TX);
        ftStatus += FT_Write(ftHandle, txbuf, 6, &nBytes);
    }

    //now read
    if (ftStatus == FT_OK) {
        ftStatus = FT_Read(ftHandle, rxbuf, 80, &nBytes);
        printf("nBytes=%d\n",nBytes);
    }

    //now store in aptInfo
    if (ftStatus == FT_OK) {
        memset(aptInfo[i].PartNoAxis,0,17);
        memcpy(aptInfo[i].PartNoAxis,rxbuf+12,16);
        aptInfo[i].StageId = *(unsigned short int *)(rxbuf+8);
        aptInfo[i].AxisId = *(unsigned short int *)(rxbuf+10);
        aptInfo[i].SerialNumAxis = *(unsigned short int *)(rxbuf+28);
        aptInfo[i].CntsPerUnit = *(unsigned long *)(rxbuf+32);
        aptInfo[i].MinPos = *(unsigned long *)(rxbuf+36);
        aptInfo[i].MaxPos = *(unsigned long *)(rxbuf+40);
        aptInfo[i].MaxAccn = *(unsigned long *)(rxbuf+44);
        aptInfo[i].MaxDec = *(unsigned long *)(rxbuf+48);
        aptInfo[i].MaxVel = *(unsigned long *)(rxbuf+52);
    }

end:
    return ftStatus;
}

long WINAPI MOT_Identify(long lSerialNum) {
    FT_STATUS ftStatus;
    FT_HANDLE ftHandle;
    long nBytes;
    long channel;
    long destination;

    BYTE txbuf[6] ={0x23,0x02,0x00,0x00,0x50,0x01};

    ftStatus = GetChannelInfo(lSerialNum, &ftHandle, &channel, &destination);

    if (ftStatus == FT_OK) {
        txbuf[2] = channel+1;
        txbuf[4] = destination;
        ftStatus = FT_Write(ftHandle, txbuf, 6, &nBytes);
    }

    return ftStatus;
}

long WINAPI MOT_EnableHWChannel(long lSerialNum) {
    FT_STATUS ftStatus;
    FT_HANDLE ftHandle;
    long nBytes;
    long channel;
    long destination;

    BYTE txbuf[6] ={0x10,0x02,0x00,0x01,0x50,0x01};

    ftStatus = GetChannelInfo(lSerialNum, &ftHandle, &channel, &destination);

    if (ftStatus == FT_OK) {
        txbuf[2] = channel+1;
        txbuf[4] = destination;
        ftStatus = FT_Write(ftHandle, txbuf, 6, &nBytes);
    }
    return ftStatus;
}

long WINAPI MOT_DisableHWChannel(long lSerialNum) {
    FT_STATUS ftStatus;
    FT_HANDLE ftHandle;
    long nBytes;
    long channel;
    long destination;

    BYTE txbuf[6] ={0x10,0x02,0x00,0x02,0x50,0x01};

    ftStatus = GetChannelInfo(lSerialNum, &ftHandle, &channel, &destination);

    if (ftStatus == FT_OK) {
        txbuf[2] = channel+1;
        txbuf[4] = destination;
        ftStatus = FT_Write(ftHandle, txbuf, 6, &nBytes);
    }
    return ftStatus;
}

long WINAPI MOT_MoveHome(long lSerialNum, BOOL bWait) {
    FT_STATUS ftStatus;
    FT_HANDLE ftHandle;
    long nBytes;
    long channel;
    long destination;

    BYTE txbuf[6] ={0x43,0x04,0x01,0x00,0x50,0x01};
    BYTE rxbuf[6];

    ftStatus = GetChannelInfo(lSerialNum, &ftHandle, &channel, &destination);

    if (bWait)
        ftStatus += FT_SetTimeouts(ftHandle, 1000, 3000);//extend timeout while board finishes reset

    if (ftStatus == FT_OK) {
        txbuf[2] = channel+1;
        txbuf[4] = destination;
        ftStatus = FT_Write(ftHandle, txbuf, 6, &nBytes);
    }

    if (bWait && ftStatus == FT_OK) {
        //FIXME! This is just to check that the method is working. Need a wait to break out of the loop or use events as per D2XX manual.
        do {
            ftStatus = FT_Read(ftHandle, rxbuf, 90, &nBytes);
        } while (nBytes == 0);
        ftStatus = FT_SetTimeouts(ftHandle, 3000, 3000);//extend timeout while board finishes reset
    }

    return ftStatus;
}

int main()
{
    FT_STATUS ftStatus;
    long i;
    char szModel[256], szSWVer[256], szHWNotes[256];

    ftStatus = APTInit();

    if (ftStatus == FT_OK) { 
        printf("Number of devices is %d\n",numDevs); 
    }

    if (numDevs > 0) { 
        for (i = 0; i < numDevs; i++) { 
            printf("Dev %d:\n",i); 
            printf(" Flags=0x%x\n",devInfo[i].Flags); 
            printf(" Type=0x%x\n",devInfo[i].Type); 
            printf(" ID=0x%x\n",devInfo[i].ID); 
            printf(" LocId=0x%x\n",devInfo[i].LocId); 
            printf(" SerialNumber='%s'\n",devInfo[i].SerialNumber); 
            printf(" Description='%s'\n",devInfo[i].Description); 
            printf(" ftHandle=0x%x\n",devInfo[i].ftHandle); 

            //printf(" APT serial=%d\n",aptInfo[i].SerialNumber);
            //printf(" APT type=%d\n",aptInfo[i].Type);

            //open the device
            ftStatus = InitHWDevice(aptInfo[i].SerialNumber);

            if (ftStatus != FT_OK) {
                printf(" Error FT_Open(%d), device %d\n", ftStatus, i);
                continue;
            }

            printf(" Flags=0x%x\n",devInfo[i].Flags); 
            printf(" ftHandle=0x%x\n",devInfo[i].ftHandle); 

            //ftStatus = QueryDevice(i);
            printf(" APT SerialNumber=%d\n",aptInfo[i].SerialNumber);
            printf(" APT Model Number='%s'\n",aptInfo[i].ModelNumber);
            printf(" APT Type=%d\n",aptInfo[i].Type);
            printf(" APT HardwareType=%d\n",aptInfo[i].HardwareType);
            printf(" APT FirmwareVersion='%s'\n",aptInfo[i].FirmwareVersion);
            printf(" APT Notes='%s'\n",aptInfo[i].Notes);
            printf(" APT HardwareVersion=%d\n",aptInfo[i].HardwareVersion);
            printf(" APT ModState=%d\n",aptInfo[i].ModState);
            printf(" APT Number of Channels=%d\n",aptInfo[i].NumberChannels);
            printf(" Stage PartNoAxis='%s'\n",aptInfo[i].PartNoAxis);
            printf(" Stage SerialNumAxis=%d\n",aptInfo[i].SerialNumAxis);
            printf(" Stage CntsPerUnit=%d\n",aptInfo[i].CntsPerUnit);
            printf(" Stage MinPos=%d\n",aptInfo[i].MinPos);
            printf(" Stage MaxPos=%d\n",aptInfo[i].MaxPos);
            printf(" Stage MaxAccn=%d\n",aptInfo[i].MaxAccn);
            printf(" Stage MaxDec=%d\n",aptInfo[i].MaxDec);
            printf(" Stage MaxVel=%d\n",aptInfo[i].MaxVel);

            if (0) {
                MOT_Identify(aptInfo[i].SerialNumber);
                MOT_MoveHome(aptInfo[i].SerialNumber,1);
                MOT_Identify(aptInfo[i].SerialNumber);
            }

            if (1) {
                MOT_Identify(aptInfo[i].SerialNumber);
                //MOT_MoveHome(aptInfo[i].SerialNumber,0);
                //MOT_Identify(aptInfo[i].SerialNumber);
            }

            if (1) {
                //Get the hardware info
                GetHWInfo(aptInfo[i].SerialNumber, szModel, 256, szSWVer, 256, szHWNotes, 256);
                printf(" Model='%s'\n",szModel); 
                printf(" Software version='%s'\n",szSWVer); 
                printf(" Notes='%s'\n",szHWNotes); 
            }
        } 

        ftStatus = GetNumHWUnitsEx(HWTYPE_TDC001,&i);
        printf("\nNumber of TDC001 devices: %d\n",i);
    } 

    printf("Cleaning-up...\n");
    ftStatus = APTCleanUp();
}
