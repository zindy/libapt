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
#include <string.h>
#include <ftdi.h>
#include <errno.h>
#include "hexdump.h"

#define VENDOR_ID 0x403
#define PRODUCT_ID 0xfaf0

#ifdef WIN32
    #include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
    #include <time.h>   // for nanosleep
#else
    #include <unistd.h> // for usleep
#endif

#ifndef WIN32
    typedef enum { false, true } BOOL;
    typedef char TCHAR;
    #define WINAPI
#endif

#include "APTAPI.h"

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

int DEBUG = true;
int numDevs = 0;
struct ftdi_context *ftdic = NULL;
struct ftdi_device_list *devlist = NULL;

MY_APT_INFO *aptInfo = NULL;
long uBaudRate = 115200;

//what's the largest buffer needed?
char rxbuf[128];

void sleep_ms(int milliseconds) // cross-platform sleep function
{
#ifdef WIN32
    Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#else
    usleep(milliseconds * 1000);
#endif
}

void SetDebug(int value) {
    DEBUG = value;
}

long GetInfo(long lSerialNum, long *plType, char *pbDestByte) {

    //default values
    long ret = 0;

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
            ret = ENODEV;
    }
    return ret;
}

long ftdi_open_apt_serialnum(long lSerialNum) {
    char buf[9];
    long ret=0;

    sprintf(buf,"%ld",lSerialNum);
    ret = ftdi_usb_open_desc_index(ftdic, VENDOR_ID, PRODUCT_ID, NULL, buf, 0);
    if (ret < 0) goto end;
    ret = ftdi_set_interface(ftdic, INTERFACE_ANY);
    if (ret < 0) goto end;
    ftdic->usb_read_timeout=3000;
    ret = ftdi_set_line_property(ftdic, 8, STOP_BIT_1, NONE);
    if (ret < 0) goto end;
    ret = ftdi_set_baudrate(ftdic,uBaudRate);
    if (ret < 0) goto end;

    //might as well clean the buffers!
    ret = ftdi_usb_purge_rx_buffer(ftdic);
    if (ret < 0) goto end;
    ret = ftdi_usb_purge_tx_buffer(ftdic);
    if (ret < 0) goto end;

    //latency?
    ret = ftdi_set_latency_timer(ftdic,1);

end:
    if (ret < 0) fprintf(stderr,"Error: %s\n",ftdi_get_error_string(ftdic));
    return ret;
}

long ftdi_open_apt_index(long i) {
    return ftdi_open_apt_serialnum(aptInfo[i].SerialNumber);
}


long GetIndex(long lSerialNum, long *index) {
    long i;
    long ret = -1;

    for (i=0; i<numDevs; i++) {
        if (aptInfo[i].SerialNumber == lSerialNum) {
            *index = i;
            ret = 0;
            break;
        }
    }
    return ret;
}


long WINAPI APTInit(void) {
    long int ret, i;
    struct ftdi_device_list *curdev;
    struct ftdi_version_info version;

    if (DEBUG) {
        version = ftdi_get_library_version();
        printf("Initialized libftdi %s (major: %d, minor: %d, micro: %d, snapshot ver: %s)\n",
            version.version_str, version.major, version.minor, version.micro,
            version.snapshot_str);
    }

    char manufacturer[128], description[128], serialno[128];

    // create the device information list 
    if ((ftdic = ftdi_new()) == 0)
    {
        fprintf(stderr, "ftdi_new failed\n");
        return EXIT_FAILURE;
    }

    if ((numDevs = ftdi_usb_find_all(ftdic, &devlist, VENDOR_ID, PRODUCT_ID)) < 0) {
        ret =  ENODEV;
        goto end;
    }

    if (numDevs == 0) {
        ret =  ENODEV;
        goto end;
    }

    // additional info will be stored here
    aptInfo = (MY_APT_INFO *)calloc(sizeof(MY_APT_INFO),numDevs); 
    if (aptInfo == NULL) {
        ret = ENOMEM;
        goto end;
    }

    i = 0;
    for (curdev = devlist; curdev != NULL; i++)
    {
        if (DEBUG) printf("Checking device: %ld\n", i);
        if ((ret = ftdi_usb_get_strings(ftdic, curdev->dev, manufacturer, 128, description, 128, serialno, 128)) < 0)
            break;

        if (DEBUG) printf("Manufacturer: '%s', Description: '%s', Serial No: '%s'\n", manufacturer, description, serialno);
        aptInfo[i].SerialNumber = atoi(serialno);

        //Once you have read from the device, you need to close it!
        if ((ret = ftdi_usb_close(ftdic)) < 0)
             break;

        ret = GetInfo(aptInfo[i].SerialNumber, &aptInfo[i].Type, &aptInfo[i].DestinationByte);
        curdev = curdev->next;
    }

end:
    if (ret < 0)
        fprintf(stderr, "Error: %ld (%s)\n", ret, ftdi_get_error_string(ftdic));
    return ret;
}

long WINAPI APTCleanUp(void) {
    long ret = 0;

    numDevs = 0;
    if (aptInfo != NULL) free(aptInfo);

    /* XXX Do we still need to try and close the devices first?
    for (i=0;i<numDevs;i++) {
        if (devInfo[i].Flags == FT_FLAGS_OPENED)
            ret = FT_Close(devInfo[i].ftHandle);
    }
    */
    ftdi_list_free(&devlist);
    ftdi_deinit(ftdic);
    ftdi_free(ftdic);
    return ret;
}


long WINAPI GetNumHWUnitsEx(long lHWType, long *plNumUnits) {
    long i, ret=0;
    long nDevices = 0;

    //This allows to get the total number of devices
    if (lHWType == 0) {
        nDevices = numDevs;
        goto end;
    }

    for (i=0;i<numDevs;i++) {
        if (aptInfo[i].Type == lHWType) {
            nDevices += 1;
        }
    }

end:
    *plNumUnits = nDevices;
    return ret;
}


long WINAPI GetHWSerialNumEx(long lHWType, long lIndex, long *plSerialNum) {
    long i,j, ret=-1;

    if (lHWType == 0 && numDevs > 0) {
        if (lIndex < 0) lIndex = 0;
        else if (lIndex >= numDevs)
            lIndex = numDevs - 1;
        *plSerialNum = aptInfo[lIndex].SerialNumber;
        ret = 0;
        goto end;
    }

    j = 0;
    for (i=0;i<numDevs;i++) {
        if (aptInfo[i].Type == lHWType) {
            if (j == lIndex) {
                *plSerialNum = aptInfo[i].SerialNumber;
                ret = 0;
                break;
            }
            j = j+1;
        }
    }

end:
    return ret;
}


long WINAPI GetHWInfo(long lSerialNum, TCHAR *szModel, long lModelLen, TCHAR *szSWVer, long lSWVerLen, TCHAR *szHWNotes, long lHWNotesLen) {
    long i, ret = 0;

    //MGMSG_HW_REQ_INFO
    char txbuf[6] ={0x05,0x00,0x00,0x00,0x50,0x01};
    GetIndex(lSerialNum, &i);

    txbuf[2] = aptInfo[i].ChannelId;
    txbuf[4] = aptInfo[i].DestinationByte;
    if (DEBUG) hexDump("GetHWInfo txbuf",txbuf,6);

    if ((ret = ftdi_open_apt_index(i)) < 0) goto end;
    if ((ret = ftdi_write_data(ftdic, txbuf, 6)) < 0) goto end;

    sleep_ms(150);

    if ((ret = ftdi_read_data(ftdic, rxbuf, 90)) <= 0)
        goto end;

    if (DEBUG) hexDump("GetHWInfo rxbuf",rxbuf,ret);

    memset(szModel,0,lModelLen);
    memcpy(szModel,rxbuf+10,8);
    sprintf(szSWVer,"%d.%d.%d", rxbuf[22],rxbuf[21],rxbuf[20]);
    memset(szHWNotes,0,lHWNotesLen);
    memcpy(szHWNotes,rxbuf+24,48);

end:
    if (ret < 0) fprintf(stderr, "Error: %ld (%s)\n", ret, ftdi_get_error_string(ftdic));
    ftdi_usb_close(ftdic);
    return ret;
}


long WINAPI InitHWDevice(long lSerialNum) {
    long i, ret = 0;
    GetIndex(lSerialNum, &i);

    //MGMSG_HW_REQ_INFO
    char txbuf[6] ={0x05,0x00,0x00,0x00,0x50,0x01};

    if ((ret = ftdi_open_apt_index(i)) < 0)
        goto end;

    txbuf[4] = aptInfo[i].DestinationByte;
    if (DEBUG) hexDump("InitHWDevice txbuf",txbuf,6);

    if ((ret = ftdi_write_data(ftdic, txbuf, 6)) < 0)
        goto end;

    sleep_ms(150);

    if ((ret = ftdi_read_data(ftdic, rxbuf, 90)) <= 0)
        goto end;

    if (DEBUG) hexDump("InitHWDevice rxbuf",rxbuf,ret);

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

    if (DEBUG) {
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

    ret = 0;
end:
    if (ret < 0) fprintf(stderr," (%s)\n",ftdi_get_error_string(ftdic));
    ftdi_usb_close(ftdic);
    return ret;
}


long WINAPI MOT_Identify(long lSerialNum) {
    long i, ret = 0;
    char txbuf[6] ={0x23,0x02,0x00,0x00,0x50,0x01};
    GetIndex(lSerialNum, &i);

    txbuf[4] = aptInfo[i].DestinationByte;
    if (DEBUG) hexDump("MOT_Identify txbuf",txbuf,6);

    if ((ret = ftdi_open_apt_index(i)) < 0) goto end;
    if ((ret = ftdi_write_data(ftdic, txbuf, 6)) < 0) goto end;

end:
    if (ret < 0) fprintf(stderr, "Error: %ld (%s)\n", ret, ftdi_get_error_string(ftdic));
    ftdi_usb_close(ftdic);
    return ret;
}


long WINAPI MOT_EnableHWChannel(long lSerialNum) {
    long i, ret = 0;

    //MGMSG_MOD_SET_CHANENABLESTATE
    char txbuf[6] ={0x10,0x02,0x00,0x01,0x50,0x01};
    GetIndex(lSerialNum, &i);

    txbuf[2] = aptInfo[i].ChannelId;
    txbuf[4] = aptInfo[i].DestinationByte;
    if (DEBUG) hexDump("MOT_EnableHWChannel txbuf",txbuf,6);

    if ((ret = ftdi_open_apt_index(i)) < 0) goto end;
    if ((ret = ftdi_write_data(ftdic, txbuf, 6)) < 0) goto end;

end:
    if (ret < 0) fprintf(stderr, "Error: %ld (%s)\n", ret, ftdi_get_error_string(ftdic));
    ftdi_usb_close(ftdic);
    return ret;
}


long WINAPI MOT_DisableHWChannel(long lSerialNum) {
    long i, ret = 0;

    //MGMSG_MOD_SET_CHANENABLESTATE
    char txbuf[6] ={0x10,0x02,0x00,0x00,0x50,0x01};
    GetIndex(lSerialNum, &i);

    txbuf[2] = aptInfo[i].ChannelId;
    txbuf[4] = aptInfo[i].DestinationByte;
    if (DEBUG) hexDump("MOT_DisableHWChannel txbuf",txbuf,6);

    if ((ret = ftdi_open_apt_index(i)) < 0) goto end;
    if ((ret = ftdi_write_data(ftdic, txbuf, 6)) < 0) goto end;

end:
    if (ret < 0) fprintf(stderr, "Error: %ld (%s)\n", ret, ftdi_get_error_string(ftdic));
    ftdi_usb_close(ftdic);
    return ret;
}

long WINAPI MOT_SetVelParams(long lSerialNum, float fMinVel, float fAccn, float fMaxVel) {
    long i, ret = 0;
    int16_t val16;
    int32_t val32;

    //MGMSG_MOD_SET_CHANENABLESTATE
    char txbuf[20];
    GetIndex(lSerialNum, &i);

    txbuf[0] = 0x13;
    txbuf[1] = 0x04;
    txbuf[2] = 0x0E;
    txbuf[3] = 0x00;
    txbuf[4] = aptInfo[i].DestinationByte;
    txbuf[5] = 0x01;

    //copy Chan Ident
    val16 = (int16_t)aptInfo[i].ChannelId;
    memcpy(txbuf+6,(char *)(&val16),2);
    val32 = (int32_t)fMinVel;
    memcpy(txbuf+8,(char *)(&val32),4);
    val32 = (int32_t)fAccn;
    memcpy(txbuf+12,(char *)(&val32),4);
    val32 = (int32_t)fMaxVel;
    memcpy(txbuf+16,(char *)(&val32),4);
    if (DEBUG) hexDump("MOT_SetVelParams txbuf",txbuf,20);

    if ((ret = ftdi_open_apt_index(i)) < 0) goto end;
    if ((ret = ftdi_write_data(ftdic, txbuf, 6)) < 0) goto end;

end:
    if (ret < 0) fprintf(stderr, "Error: %ld (%s)\n", ret, ftdi_get_error_string(ftdic));
    ftdi_usb_close(ftdic);
    return ret;
}


long WINAPI MOT_GetVelParams(long lSerialNum, float *pfMinVel, float *pfAccn, float *pfMaxVel) {
    long i, ret = 0;
    int32_t val32;

    //MGMSG_MOT_REQ_VELPARAMS
    char txbuf[6] ={0x14,0x04,0x00,0x00,0x50,0x01};
    GetIndex(lSerialNum, &i);

    txbuf[2] = aptInfo[i].ChannelId;
    txbuf[4] = aptInfo[i].DestinationByte;
    if (DEBUG) hexDump("MOT_GetVelParams txbuf",txbuf,6);

    if ((ret = ftdi_open_apt_index(i)) < 0) goto end;
    if ((ret = ftdi_write_data(ftdic, txbuf, 6)) < 0) goto end;

    sleep_ms(150);

    if ((ret = ftdi_read_data(ftdic, rxbuf, 20)) <= 0)
        goto end;

    if (DEBUG) hexDump("MOT_GetVelParams rxbuf",rxbuf,ret);
    val32 = *(int32_t *)(rxbuf+8);
    *pfMinVel = (float)val32;

    val32 = *(int32_t *)(rxbuf+12);
    *pfAccn = (float)val32;

    val32 = *(int32_t *)(rxbuf+16);
    *pfMaxVel = (float)val32;

    ret = 0;
end:
    if (ret < 0) fprintf(stderr, "Error: %ld (%s)\n", ret, ftdi_get_error_string(ftdic));
    ftdi_usb_close(ftdic);
    return ret;
}

/* XXX Where do I get the limits from? Check commands accepted by the TDC001, none there.
 *
long WINAPI MOT_GetVelParamLimits(long lSerialNum, float *pfMaxAccn, float *pfMaxVel) {
    long i, ret = 0;
    int32_t val32;

    //MGMSG_MOT_REQ_VELPARAMS
    char txbuf[6] ={0x14,0x04,0x00,0x00,0x50,0x01};
    GetIndex(lSerialNum, &i);

    txbuf[2] = aptInfo[i].ChannelId;
    txbuf[4] = aptInfo[i].DestinationByte;
    if (DEBUG) hexDump("MOT_GetVelParams txbuf",txbuf,6);

    if ((ret = ftdi_open_apt_index(i)) < 0) goto end;
    if ((ret = ftdi_write_data(ftdic, txbuf, 6)) < 0) goto end;

    sleep_ms(150);

    if ((ret = ftdi_read_data(ftdic, rxbuf, 20)) <= 0)
        goto end;

    if (DEBUG) hexDump("MOT_GetVelParams rxbuf",rxbuf,ret);
    val32 = *(int32_t *)(rxbuf+8);
    *pfMinVel = (float)val32;

    val32 = *(int32_t *)(rxbuf+12);
    *pfAccn = (float)val32;

    val32 = *(int32_t *)(rxbuf+16);
    *pfMaxVel = (float)val32;

    ret = 0;
end:
    if (ret < 0) fprintf(stderr, "Error: %ld (%s)\n", ret, ftdi_get_error_string(ftdic));
    ftdi_usb_close(ftdic);
    return ret;
}
*/
//long WINAPI MOT_SetStageAxisInfo(long lSerialNum, float fMinPos, float fMaxPos, long lUnits, float fPitch);

long WINAPI MOT_GetStageAxisInfo(long lSerialNum, float *pfMinPos, float *pfMaxPos, long *plUnits, float *pfPitch) {
    long i, ret = 0;
    int32_t val32;
    uint32_t uval32;

    //MGMSG_MOT_REQ_PMDSTAGEAXISPARAMS
    char txbuf[6] ={0xF1,0x04,0x00,0x00,0x50,0x01};
    GetIndex(lSerialNum, &i);

    txbuf[2] = aptInfo[i].ChannelId;
    txbuf[4] = aptInfo[i].DestinationByte;
    if (DEBUG) hexDump("MOT_GetStageAxisInfo txbuf",txbuf,6);

    if ((ret = ftdi_open_apt_index(i)) < 0) goto end;
    if ((ret = ftdi_write_data(ftdic, txbuf, 6)) < 0) goto end;

    sleep_ms(150);

    if ((ret = ftdi_read_data(ftdic, rxbuf, 80)) <= 0)
        goto end;

    if (DEBUG) hexDump("MOT_GetStageAxisInfo rxbuf",rxbuf,ret);
    val32 = *(int32_t *)(rxbuf+36);
    *pfMinPos = (float)val32;

    val32 = *(int32_t *)(rxbuf+40);
    *pfMaxPos = (float)val32;

    uval32 = *(uint32_t *)(rxbuf+32);
    *plUnits = (long)uval32;

    *pfPitch = 0;

    ret = 0;
end:
    if (ret < 0) fprintf(stderr, "Error: %ld (%s)\n", ret, ftdi_get_error_string(ftdic));
    ftdi_usb_close(ftdic);
    return ret;
}

long WINAPI MOT_GetPosition(long lSerialNum, float *pfPosition) {
    long i, ret = 0;
    int32_t val32;

    //MGMSG_MOT_REQ_POSCOUNTER
    char txbuf[6] ={0x11,0x04,0x00,0x00,0x50,0x01};
    GetIndex(lSerialNum, &i);

    txbuf[2] = aptInfo[i].ChannelId;
    txbuf[4] = aptInfo[i].DestinationByte;
    if (DEBUG) hexDump("MOT_GetPosition txbuf",txbuf,6);

    if ((ret = ftdi_open_apt_index(i)) < 0) goto end;
    if ((ret = ftdi_write_data(ftdic, txbuf, 6)) < 0) goto end;

    sleep_ms(150);

    if ((ret = ftdi_read_data(ftdic, rxbuf, 12)) <= 0)
        goto end;

    if (DEBUG) hexDump("MOT_GetPosition rxbuf",rxbuf,ret);
    val32 = *(int32_t *)(rxbuf+8);
    *pfPosition = (float)val32;

    ret = 0;
end:
    if (ret < 0) fprintf(stderr, "Error: %ld (%s)\n", ret, ftdi_get_error_string(ftdic));
    ftdi_usb_close(ftdic);
    return ret;
}

long WINAPI MOT_SetChannel(long lSerialNum, long lChanID) {
    long i, ret = -1;

    GetIndex(lSerialNum, &i);

    if (lChanID <= aptInfo[i].NumberChannels) {
        ret = 0;
        aptInfo[i].ChannelId = lChanID;
    }

end:
    if (ret < 0) fprintf(stderr, "Error: lChanID %ld > NumberChannels (%ld)\n",lChanID,aptInfo[i].NumberChannels);
    return ret;
}

long WINAPI MOT_MoveHome(long lSerialNum, BOOL bWait) {
    long i, ret = 0;

    char txbuf[6] ={0x43,0x04,0x01,0x00,0x50,0x01};
    GetIndex(lSerialNum, &i);

    txbuf[2] = aptInfo[i].ChannelId;
    txbuf[4] = aptInfo[i].DestinationByte;
    if (DEBUG) hexDump("MOT_MoveHome txbuf",txbuf,6);

    if ((ret = ftdi_open_apt_index(i)) < 0) goto end;
    if ((ret = ftdi_write_data(ftdic, txbuf, 6)) < 0) goto end;

    ret = 0;
    i = 0;
    while (bWait && ret == 0 && i++ < 10) {
        sleep_ms(150);
        ret = ftdi_read_data(ftdic, rxbuf, 6);
    }
    if (ret > 0 && DEBUG) hexDump("MOT_MoveHome rxbuf",rxbuf,ret);

end:
    if (ret < 0) fprintf(stderr, "Error: %ld (%s)\n", ret, ftdi_get_error_string(ftdic));
    ftdi_usb_close(ftdic);
    return ret;
}

long WINAPI MOT_MoveRelativeEx(long lSerialNum, float fRelDist, BOOL bWait) {
    long i, ret = 0;

    int16_t val16;
    int32_t val32;
    GetIndex(lSerialNum, &i);
    char txbuf[12];

    //MGMSG_MOT_MOVE_RELATIVE
    txbuf[0] = 0x48;
    txbuf[1] = 0x04;
    txbuf[2] = 0x06;
    txbuf[3] = 0x00;
    txbuf[4] = aptInfo[i].DestinationByte | 0x80;
    txbuf[5] = 0x01;

    //copy Chan Ident
    val16 = (int16_t)aptInfo[i].ChannelId;
    memcpy(txbuf+6,(char *)(&val16),2);
    val32 = (int32_t)fRelDist;
    memcpy(txbuf+8,(char *)(&val32),4);
    if (DEBUG) hexDump("MOT_MoveRelativeEx txbuf",txbuf,12);

    if ((ret = ftdi_open_apt_index(i)) < 0) goto end;
    if ((ret = ftdi_write_data(ftdic, txbuf, 12)) < 0) goto end;

    ret = 0;
    i = 0;
    while (bWait && ret == 0 && i++ < 10) {
        sleep_ms(150);
        ret = ftdi_read_data(ftdic, rxbuf, 6);
    }
    if (ret > 0 && DEBUG) hexDump("MOT_MoveRelativeEx rxbuf",rxbuf,ret);

end:
    if (ret < 0) fprintf(stderr, "Error: %ld (%s)\n", ret, ftdi_get_error_string(ftdic));
    ftdi_usb_close(ftdic);
    return ret;
}

long WINAPI MOT_MoveAbsoluteEx(long lSerialNum, float fAbsPos, BOOL bWait) {
    long i, ret = 0;

    int16_t val16;
    int32_t val32;
    GetIndex(lSerialNum, &i);
    char txbuf[20];

    //MGMSG_MOD_SET_CHANENABLESTATE
    txbuf[0] = 0x48;
    txbuf[1] = 0x04;
    txbuf[2] = 0x06;
    txbuf[3] = 0x00;
    txbuf[4] = aptInfo[i].DestinationByte | 0x80;
    txbuf[5] = 0x01;

    //copy Chan Ident
    val16 = (int16_t)aptInfo[i].ChannelId;
    memcpy(txbuf+6,(char *)(&val16),2);
    val32 = (int32_t)fAbsPos;
    memcpy(txbuf+8,(char *)(&val32),4);
    if (DEBUG) hexDump("MOT_MoveAbsoluteEx txbuf",txbuf,12);

    if ((ret = ftdi_open_apt_index(i)) < 0) goto end;
    if ((ret = ftdi_write_data(ftdic, txbuf, 12)) < 0) goto end;

    ret = 0;
    i = 0;
    while (bWait && ret == 0 && i++ < 10) {
        sleep_ms(150);
        ret = ftdi_read_data(ftdic, rxbuf, 6);
    }
    if (ret > 0 && DEBUG) hexDump("MOT_MoveAbsoluteEx rxbuf",rxbuf,ret);

end:
    if (ret < 0) fprintf(stderr, "Error: %ld (%s)\n", ret, ftdi_get_error_string(ftdic));
    ftdi_usb_close(ftdic);
    return ret;
}

