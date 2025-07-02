// #ifndef __PS_POWERSUPPLY_H__
// #define __PS_POWERSUPPLY_H__
//  #endif
//==============================================================================
// Filename = PS_powerSupply.h
//==============================================================================
//===========================  I N C L U D E S  ================================
//==============================================================================

// *** Predefined Packages ***
//#include <windows.h>
#include <visa.h>
#include <utility.h>
#include <formatio.h>

#define   MAX_LENGTH		   5000
#define   VI_ERR_TRUE         -1

//==============================================================================
#define PS_FUNCT_PFX int DLLEXPORT __stdcall

//==============================================================================
 

PS_FUNCT_PFX PS_Init ( int iIndex, ViInt16 iIGPIBAddr,char * pszModel,int iIFlag);
PS_FUNCT_PFX PS_Reset (int iIndex);
PS_FUNCT_PFX PS_Close (int iIndex);
PS_FUNCT_PFX PS_SelfTest (int iIndex,ViChar _VI_FAR pszSelfTestMessage[]);
PS_FUNCT_PFX PS_PWOnOff (int iIndex, ViBoolean bPowerSwitch);
PS_FUNCT_PFX PS_SetVoltCurrLevel (int iIndex, ViReal64 dVoltage, ViReal64 dCurrent);  
PS_FUNCT_PFX PS_Measure_Voltage (int iIndex, double *dVoltage);
PS_FUNCT_PFX PS_Measure_Current (int iIndex, double *dCurrent);
PS_FUNCT_PFX PS_SetDigitalIO (int iIndex, ViInt16 iDigitalIOOrValue);
PS_FUNCT_PFX PS_GetErrorMsg (ViChar _VI_FAR pszErrMsg[]);
//---------------------------------------------------------------------------------------------------  
