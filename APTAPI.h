
//----------------------------------------------------------------
//	© Thorlabs Limited.
//
//	FILE NAME       : APTAPI.h
//
//	AUTHOR          : Dr. Keith Dhese
//
//	CREATED         : 21-05-08
//
//	PLATFORM        : Win32
//
//	MODULE FUNCTION:-
//
//	Defines APT.DLL application programmers interface for accessing the APT system.
//	This interface can be included by C or C++ code.
//
//	RELATED DOCUMENTATION:-
//
//	NOTES:-
//
//----------------------------------------------------------------
//	MODIFICATION HISTORY:-
//
//	DATE			VERSION			AUTHOR
//
//	21-05-08		1.0.1			KAD
//
//	Initial implementation.
//
//
//----------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


// >>>>>>>>>>>>>>>>> MACRO DEFINITIONS <<<<<<<<<<<<<<<<<<

// lHWType definitions - used in exported functions GetNumHWUnitsEx & GetHWSerialNumEx.
#define HWTYPE_BSC001		11	// 1 Ch benchtop stepper driver
#define HWTYPE_BSC101		12	// 1 Ch benchtop stepper driver
#define HWTYPE_BSC002		13	// 2 Ch benchtop stepper driver
#define HWTYPE_BDC101		14	// 1 Ch benchtop DC servo driver
#define HWTYPE_SCC001		21	// 1 Ch stepper driver card (used within BSC102,103 units)
#define HWTYPE_DCC001		22	// 1 Ch DC servo driver card (used within BDC102,103 units)
#define HWTYPE_ODC001		24	// 1 Ch DC servo driver cube
#define HWTYPE_OST001		25	// 1 Ch stepper driver cube
#define HWTYPE_MST601		26	// 2 Ch modular stepper driver module
#define HWTYPE_TST001		29	// 1 Ch Stepper driver T-Cube
#define HWTYPE_TDC001		31	// 1 Ch DC servo driver T-Cube

// Channel idents - used in MOT_SetChannel.
#define CHAN1_INDEX			0		// Channel 1.
#define CHAN2_INDEX			1		// Channel 2.

// Home direction (lDirection) - used in MOT_Set(Get)HomeParams
#define HOME_FWD			1		// Home in the forward direction.
#define HOME_REV			2		// Home in the reverse direction.

// Home limit switch (lLimSwitch) - used in MOT_Set(Get)HomeParams
#define HOMELIMSW_FWD		4		// Use forward limit switch for home datum.
#define HOMELIMSW_REV		1		// Use reverse limit switch for home datum.

// Stage units (lUnits) - used in MOT_Set(Get)StageAxisInfo
#define STAGE_UNITS_MM		1		// Stage units are in mm.
#define STAGE_UNITS_DEG		2		// Stage units are in degrees.

// Hardware limit switch settings (lRevLimSwitch, lFwdLimSwitch) - used in MOT_Set(Get)HWLimSwitches
#define HWLIMSWITCH_IGNORE				1		// Ignore limit switch (e.g. for stages with only one or no limit switches).
#define HWLIMSWITCH_MAKES				2		// Limit switch is activated when electrical continuity is detected.
#define HWLIMSWITCH_BREAKS				3		// Limit switch is activated when electrical continuity is broken.
#define HWLIMSWITCH_MAKES_HOMEONLY		4		// As per HWLIMSWITCH_MAKES except switch is ignored other than when homing (e.g. to support rotation stages).
#define HWLIMSWITCH_BREAKS_HOMEONLY		5		// As per HWLIMSWITCH_BREAKS except switch is ignored other than when homing (e.g. to support rotation stages).

// Move direction (lDirection) - used in MOT_MoveVelocity
#define MOVE_FWD			1		// Move forward.
#define MOVE_REV			2		// Move reverse.


// >>>>>>>>>>>>>>>>> EXPORTED FUNCTIONS <<<<<<<<<<<<<<<<<<

// System Level Exports.
long WINAPI APTInit(void);
long WINAPI APTCleanUp(void);
long WINAPI GetNumHWUnitsEx(long lHWType, long *plNumUnits);
long WINAPI GetHWSerialNumEx(long lHWType, long lIndex, long *plSerialNum);
long WINAPI GetHWInfo(long lSerialNum, TCHAR *szModel, long lModelLen, TCHAR *szSWVer, long lSWVerLen, TCHAR *szHWNotes, long lHWNotesLen);
long WINAPI InitHWDevice(long lSerialNum);
long WINAPI EnableEventDlg(BOOL bEnable);

// Motor Specific Exports.
long WINAPI MOT_SetChannel(long lSerialNum, long lChanID);
long WINAPI MOT_Identify(long lSerialNum);
long WINAPI MOT_EnableHWChannel(long lSerialNum);
long WINAPI MOT_DisableHWChannel(long lSerialNum);
long WINAPI MOT_SetVelParams(long lSerialNum, float fMinVel, float fAccn, float fMaxVel);
long WINAPI MOT_GetVelParams(long lSerialNum, float *pfMinVel, float *pfAccn, float *pfMaxVel);
long WINAPI MOT_GetVelParamLimits(long lSerialNum, float *pfMaxAccn, float *pfMaxVel);
long WINAPI MOT_SetHomeParams(long lSerialNum, long lDirection, long lLimSwitch, float fHomeVel, float fZeroOffset);
long WINAPI MOT_GetHomeParams(long lSerialNum, long *plDirection, long *plLimSwitch, float *pfHomeVel, float *pfZeroOffset);
long WINAPI MOT_GetStatusBits(long lSerialNum, long *plStatusBits);

long WINAPI MOT_SetBLashDist(long lSerialNum, float fBLashDist);
long WINAPI MOT_GetBLashDist(long lSerialNum, float *pfBLashDist);
long WINAPI MOT_SetMotorParams(long lSerialNum, long lStepsPerRev, long lGearBoxRatio);
long WINAPI MOT_GetMotorParams(long lSerialNum, long *plStepsPerRev, long *plGearBoxRatio);
long WINAPI MOT_SetStageAxisInfo(long lSerialNum, float fMinPos, float fMaxPos, long lUnits, float fPitch);
long WINAPI MOT_GetStageAxisInfo(long lSerialNum, float *pfMinPos, float *pfMaxPos, long *plUnits, float *pfPitch);
long WINAPI MOT_SetHWLimSwitches(long lSerialNum, long lRevLimSwitch, long lFwdLimSwitch);
long WINAPI MOT_GetHWLimSwitches(long lSerialNum, long *plRevLimSwitch, long *plFwdLimSwitch);
long WINAPI MOT_SetPIDParams(long lSerialNum, long lProp, long lInt, long lDeriv, long lIntLimit);
long WINAPI MOT_GetPIDParams(long lSerialNum, long *plProp, long *plInt, long *plDeriv, long *plIntLimit);

long WINAPI MOT_GetPosition(long lSerialNum, float *pfPosition);
long WINAPI MOT_MoveHome(long lSerialNum, BOOL bWait);
long WINAPI MOT_MoveRelativeEx(long lSerialNum, float fRelDist, BOOL bWait);
long WINAPI MOT_MoveAbsoluteEx(long lSerialNum, float fAbsPos, BOOL bWait);
long WINAPI MOT_MoveVelocity(long lSerialNum, long lDirection);
long WINAPI MOT_StopProfiled(long lSerialNum);


// Piezo Specific Exports.


// NanoTrak Specific Exports.


#ifdef __cplusplus
}
#endif
