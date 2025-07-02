/*****************************************************************************/
/* $Header: $ */
/*  Copyright (C) 1997 Hewlett-Packard Company                               */
/*---------------------------------------------------------------------------*/
/*  VXIplug&play C Function Driver for HP 661xC and HP 663xB DC Sources      */
/*  Driver Version: A.00.00                                                  */
/*---------------------------------------------------------------------------*/
/*  This driver is compatible with the following VXIplug&play standards:     */
/*    WIN32 System Framework revision 4.0                                    */
/*    VISA revision 1.0 (does not work with VTL)                             */
/*****************************************************************************/

#define HP661X_REV_CODE "A.00.00"  /* Driver Revision */

/*****************************************************************************/
/* Exactly one of the following must be defined for the program to           */
/*   compile correctly.                                                      */
/*                                                                           */
/* In addition, if WIN32 is defined, _CVI_ may also be defined to indicate   */
/*   that the program is being compiled under LabWindows CVI 4.0 or          */
/*   greater.                                                                */
/*****************************************************************************/
/* #define __hpux           for compilation for HP-UX */
/* #define WIN32			compilation for 32-bit Windows */

/*****************************************************************************/
/* The following defines are intended to provide OS compatibility among      */
/*   Windows 32-bit and HP-UX C compilers.                                   */
/*****************************************************************************/

#ifdef __hpux
#define _fcvt    fcvt
#endif

#define _huge

/*****************************************************************************/

#include <stdlib.h>		/* prototype for malloc() */
#include <string.h>		/* prototype for strcpy() */
#include <stdio.h>		/* prototype for sprintf() */
#include "visa.h"
#include "hp661x.h"

#ifdef __hpux
#define _INCLUDE_HPUX_SOURCE
#define _INCLUDE_XOPEN_SOURCE_EXTENDED
#include <sys/time.h>
#else
#include <windows.h>
#endif

#define HP661X_ERR_MSG_LENGTH 256  /* size of error message buffer */

/*****************************************************************************/
/*  The strings below are used to do the instrument identification in the    */
/*    init routine.  The string matches the first part of the instrument's   */
/*    response to it's *IDN? command.                                        */
/*****************************************************************************/
#define	HP_6611C				"6611C"
#define	HP_6612C				"6612C"
#define	HP_6613C				"6613C"
#define	HP_6614C				"6614C"
#define	HP_6631B				"6631B"
#define	HP_6632B				"6632B"
#define	HP_6633B				"6633B"
#define	HP_6634B				"6634B"

const	static	ViChar		szHP[]="HEWLETT-PACKARD";
const	static	ViPString	hp661x_model_table[]=
{
	HP_6611C,
	HP_6612C,
	HP_6613C,
	HP_6614C,
	HP_6631B,
	HP_6632B,
	HP_6633B,
	HP_6634B,
	NULL
};

/*****************************************************************************/
/*  VISA Globals data structure.                                             */
/*  It is desireable to store global variables in VISA rather than the       */
/*    driver.  This avoids having any state variables in the driver.  This   */
/*    structure defines the driver's global data.  Memory is allocated to    */
/*    this structure in the init function, which passes the pointer to VISA, */
/*    and every function which follows retrieves a pointer to it.            */
/*****************************************************************************/
struct hp661x_globals
{
	ViSession			defRMSession;
	char 				address[256];

	ViStatus	errNumber;
	char 		errFuncName[40];
	char		errMessage[160];

	ViBoolean	errQueryDetect;
    ViUInt16    interfaceType;
	ViInt32		measTimeout;
        
	/**************************/
	/*  Instrument Specific   */
	/**************************/
	ViReal64	currMax;	/* maximum output current */
	ViReal64	voltMax;	/* maximum output voltage */
	ViReal64	ovpMax;		/* maximum over-voltage level */
	ViReal64	tintMax;	/* maximum sweep interval */

	/* Setting this to VI_TRUE,
     * will prevent I/O from occuring in an SRQ
     * This is needed, because VTL 3.0 doesn't provide
     * an atomic write/read operations.  Don't want
     * to do I/O if the driver is in the middle of
     * a read from the instrument.
     */
	ViBoolean                       blockSrqIO;
	ViInt32                         countSrqIO;

	/* the maximum amt. of time a command takes */
	ViUInt32			cmdTime;	/* command processing */
	ViUInt32			settleTime; /* hw settle */
	ViUInt32			clrTime;	/* reset */
	ViUInt32			testTime;	/* self-test time */

	/* if an error occurred, indicate if err or fault need to be queried */
	ViBoolean 			error_occurred;
	ViBoolean			fault_occurred;
	ViBoolean			pon_occurred;
};  /* end globals */



/*****************************************************************************/

#if (defined _WIN32) || (defined __WIN32__)
	#define HP661X_DECLARE_TIME_LOCALS 	SYSTEMTIME st;
	#define HP661X_GET_TIME		GetLocalTime(&st);
	#define HP661X_TIME_FIELDS	st.wMonth, st.wDay, st.wHour, \
					st.wMinute, st.wSecond,	st.wMilliseconds
#else				/* not win32 */
	#ifdef _CVI_  		/* LabWindows for Win 3.1 */
		#include <utility.h>

	#else  /* not win32, not labWindows */
		#include <time.h>		/* standard time functions */
		#ifdef __unix
			#define HP661X_DECLARE_TIME_LOCALS 	struct timeval tv; \
							struct timezone tz; \
							struct tm *tmp;
			#define HP661X_GET_TIME		gettimeofday(&tv, &tz); \
							tmp = localtime((time_t*)&tv.tv_sec);
			#define HP661X_TIME_FIELDS	tmp->tm_mon+1, tmp->tm_mday, tmp->tm_hour, \
							tmp->tm_min, tmp->tm_sec, tv.tv_usec/1000	
		#else				/* not unix, win32, or labwindows,so use ANSI time function */
			#define HP661X_DECLARE_TIME_LOCALS 	struct tm *tmp; time_t seconds;
			#define HP661X_GET_TIME		time(&seconds); \
							tmp = localtime(&seconds);
			#define HP661X_TIME_FIELDS	tmp->tm_mon+1, tmp->tm_mday, tmp->tm_hour, \
							tmp->tm_min, tmp->tm_sec, 0
		#endif  /* ifdef __unix  */
	#endif /* ifdef _CVI_ */
#endif  /* ifdef _WIN32 */

/*=============================================================== 
 *	
 *  All messages are stored in this area to aid in localization 
 *
 *=============================================================== 
 */

#define HP661X_MSG_VI_OPEN_ERR \
	"instrumentHandle was zero.  Was the hp661x_init() successful?"

#define HP661X_MSG_CONDITION \
	"condition"
	/* hp661x_statCond_Q() */

#define HP661X_MSG_EVENT \
	"event"	
	/* hp661x_statEvent_Q() */

#define HP661X_MSG_EVENT_HDLR_INSTALLED \
	"event handler is already installed for event happening"
	/* hp661x_statEvent_Q() */

#define HP661X_MSG_EVENT_HDLR_INST2 \
	"Only 1 handler can be installed at a time."	
	/* hp661x_statEvent_Q() */

#define HP661X_MSG_INVALID_HAPPENING \
	"is not a valid happening."
	/* hp661x_statCond_Q() */
	/* hp661x_statEven_Q() */
	/* hp661x_statEvenHdlr() */
	/* hp661x_statEvenHdlr_Q() */

#define HP661X_MSG_NOT_QUERIABLE \
	"is not queriable."	
	/* hp661x_statCond_Q() */
	/* hp661x_statEven_Q() */


#define HP661X_MSG_IN_FUNCTION	\
	"in function" 		
	/* hp661x_error_message() */

#define HP661X_MSG_INVALID_STATUS \
  	"Parameter 2 is invalid" \
	" in function hp661x_error_message()."
	/* hp661x_error_message() */

#define HP661X_MSG_INVALID_STATUS_VALUE \
	"is not a valid viStatus value."
	/* hp661x_error_message() */

#define  HP661X_MSG_INVALID_VI \
  	"Parameter 1 is invalid" \
	" in function hp661x_error_message()." \
	"  Using an inactive ViSession may cause this error."	\
	"  Was the instrument driver closed prematurely?"
	/* hp661x_message_query() */

#define HP661X_MSG_NO_ERRORS \
	"No Errors"
	/* hp661x_error_message() */

#define HP661X_MSG_SELF_TEST_FAILED \
	"Self test failed." 	
	/* hp661x_self_test() */

#define HP661X_MSG_SELF_TEST_PASSED \
	"Self test passed."
	/* hp661x_self_test() */

/* the following messages are used by the functions to check parameters */

#define HP661X_MSG_BOOLEAN		"Expected 0 or 1; Got %d"
#define HP661X_MSG_REAL		"Expected %lg to %lg; Got %lg"
#define HP661X_MSG_INT			"Expected %hd to %hd; Got %hd"
#define HP661X_MSG_LONG		"Expected %ld to %ld; Got %ld"
#define HP661X_MSG_LOOKUP		"Error converting string response to integer"
#define HP661X_MSG_NO_MATCH	"Could not match string %s"

/* 
 * static error message 
 */

#define VI_ERROR_PARAMETER1_MSG			"Parameter 1 is invalid"
#define VI_ERROR_PARAMETER2_MSG			"Parameter 2 is invalid"
#define VI_ERROR_PARAMETER3_MSG			"Parameter 3 is invalid"
#define VI_ERROR_PARAMETER4_MSG			"Parameter 4 is invalid"
#define VI_ERROR_PARAMETER5_MSG			"Parameter 5 is invalid"
#define VI_ERROR_PARAMETER6_MSG			"Parameter 6 is invalid"
#define VI_ERROR_PARAMETER7_MSG			"Parameter 7 is invalid"
#define VI_ERROR_PARAMETER8_MSG			"Parameter 8 is invalid"
#define VI_ERROR_FAIL_ID_QUERY_MSG		"Instrument IDN does not match."
#define INSTR_ERROR_INV_SESSION_MSG		"ViSession (parmeter 1) was not created by this driver"
#define INSTR_ERROR_NULL_PTR_MSG		"NULL pointer detected"
#define INSTR_ERROR_RESET_FAILED_MSG	"reset failed"
#define INSTR_ERROR_UNEXPECTED_MSG 		"An unexpected error occurred"
#define INSTR_ERROR_DETECTED_MSG		"Instrument Error Detected, call hp661x_error_query()."
#define INSTR_ERROR_LOOKUP_MSG   		"String not found in table"

#define	INSTR_ERROR_MEAS_TMO_MSG		"A measurement timeout occured"
#define	INSTR_ERROR_PORT_CONFIG_MSG		"The digital port is not configured for the request operation"
#define	INSTR_ERROR_GET_MAX_FAILED_MSG	"Instrument failed to return a max value during init"

#define	CRD_RI_MODE_OFF			"OFF"
#define	CRD_RI_MODE_LATCHING	"LATC"
#define	CRD_RI_MODE_LIVE		"LIVE"

#define	CRD_DFI_SRC_PREV		""	
#define	CRD_DFI_SRC_OFF			"OFF"
#define	CRD_DFI_SRC_QUES		"QUES"
#define	CRD_DFI_SRC_OPER		"OPER"
#define	CRD_DFI_SRC_ESB			"ESB"
#define	CRD_DFI_SRC_RQS			"RQS"

#define	CRD_CURR_DET_DC			"DC"
#define	CRD_CURR_DET_ACDC		"ACDC"

const	static	ViChar		hp661x_srd_VOLTAGE[]="\"VOLT\"";

const	static	ViChar		szLANG_COMP[]="COMP";

const	static ViPString	hp661x_ri_mode_table[]=
{
	CRD_RI_MODE_OFF,
	CRD_RI_MODE_LATCHING,
	CRD_RI_MODE_LIVE,
	NULL
};

const	static ViPString	hp661x_dfi_src_table[]=
{
	CRD_DFI_SRC_PREV,
	CRD_DFI_SRC_QUES,
	CRD_DFI_SRC_OPER,
	CRD_DFI_SRC_ESB,
	CRD_DFI_SRC_RQS,
	CRD_DFI_SRC_OFF,
	NULL
};

/*****************************************************************************/
/*  Error table structure.                                                   */
/*    The first element of the table is the error number, the second is the  */
/*    error message.  Error numbers in the "VISA Generic" section of the     */
/*    table are defined in the VISA header files.  Error numbers in the      */
/*    "Instrument Specific" section are defined in the driver header file.   */
/*    All of the error messages are defined above.                           */
/*****************************************************************************/
struct instrErrStruct
{
	ViStatus errStatus;
	ViString errMessage;
};

const static struct instrErrStruct instrErrMsgTable[] =
{
	{ VI_ERROR_PARAMETER1,	VI_ERROR_PARAMETER1_MSG },
	{ VI_ERROR_PARAMETER2,	VI_ERROR_PARAMETER2_MSG },
	{ VI_ERROR_PARAMETER3,	VI_ERROR_PARAMETER3_MSG },
	{ VI_ERROR_PARAMETER4,	VI_ERROR_PARAMETER4_MSG },
	{ VI_ERROR_PARAMETER5,	VI_ERROR_PARAMETER5_MSG },
	{ VI_ERROR_PARAMETER6,	VI_ERROR_PARAMETER6_MSG },
	{ VI_ERROR_PARAMETER7,	VI_ERROR_PARAMETER7_MSG },
	{ VI_ERROR_PARAMETER8,	VI_ERROR_PARAMETER8_MSG },
	{ VI_ERROR_FAIL_ID_QUERY,	VI_ERROR_FAIL_ID_QUERY_MSG },

	{ HP661X_INSTR_ERROR_INV_SESSION,	INSTR_ERROR_INV_SESSION_MSG },
	{ HP661X_INSTR_ERROR_NULL_PTR,		INSTR_ERROR_NULL_PTR_MSG },
	{ HP661X_INSTR_ERROR_RESET_FAILED,	INSTR_ERROR_RESET_FAILED_MSG },
	{ HP661X_INSTR_ERROR_UNEXPECTED,	INSTR_ERROR_UNEXPECTED_MSG },
	{ HP661X_INSTR_ERROR_DETECTED,		INSTR_ERROR_DETECTED_MSG },
	{ HP661X_INSTR_ERROR_LOOKUP,		INSTR_ERROR_LOOKUP_MSG },
	{ HP661X_INSTR_ERROR_MEAS_TMO,		INSTR_ERROR_MEAS_TMO_MSG },
	{ HP661X_INSTR_ERROR_PORT_CONFIG,	INSTR_ERROR_PORT_CONFIG_MSG },
	{ HP661X_INSTR_ERROR_GET_MAX,		INSTR_ERROR_GET_MAX_FAILED_MSG },

};

#define VIREAL64_LSB	1.11022303e-16	/* ~ 1/pow(2,53) */


/*****************************************************************************/
/*****************************************************************************/
/********************                                     ********************/
/**********                                                         **********/
/****                   SUPPORTING MACROS AND FUNCTIONS                   ****/
/**                                                                         **/
/**-------------------------------------------------------------------------**/
/**     These macros and functions are not exposed to the user, but         **/
/**     are used by other functions in the driver.                          **/
/****                                                                     ****/
/**********                                                         **********/
/********************                                     ********************/
/*****************************************************************************/
/*****************************************************************************/

/*****************************************************************************/
/*  MACRO GET_GLOBALS                                                        */
/*****************************************************************************/
/*  Returns a pointer to the VISA globals storage.  This storage was         */
/*    allocated in init, but it is turned over to VISA to avoid holding      */
/*    state variables in the driver.                                         */
/*  Defines context dependent errors                                         */
/*                                                                           */
/*  PARAMETERS                                                               */
/*    ViSession            instrumentHandle                                  */
/*      The instrument's VISA address.                                       */
/*    struct hp661x_globals  *thisPtr                                        */
/*      A pointer to the VISA globals storage                                */
/*    funcname                                                               */
/*      The name of the function to be stored with the context dependent     */
/*      error information.                                                   */
/*****************************************************************************/

#define GET_GLOBALS(instrumentHandle,thisPtr)                                \
{                                                                            \
    errStatus = viGetAttribute(instrumentHandle, VI_ATTR_USER_DATA,          \
                                                      (ViAddr) &thisPtr);    \
    if (errStatus < VI_SUCCESS)                                              \
        return statusUpdate(instrumentHandle, NULL, errStatus);              \
}

/*****************************************************************************/
/*  MACRO DEBUG_CHK_THIS                                                     */
/*****************************************************************************/
/* don't check the debug pointer all the time!                               */
/*****************************************************************************/
#ifdef DEBUG
#define DEBUG_CHK_THIS(instrumentHandle, thisPtr)						\
{																		\
	ViSession defRM;													\
	/* check for NULL user data */										\
	if( thisPtr==NULL )													\
	{																	\
 		return updateStatus(vi, NULL, HP661X_INSTR_ERROR_INV_SESSION );\
	}																	\
	/* This should never fail */										\
	errStatus = viGetAttribute(instrumentHandle,						\
		VI_ATTR_RM_SESSION, &defRM);									\
	if( VI_SUCCESS > errStatus )										\
	{																	\
 		return updateStatus( vi, NULL, HP661X_INSTR_ERROR_UNEXPECTED );\
	}																	\
	if( defRM != thisPtr->defRMSession )								\
	{																	\
 		return updateStatus( vi, NULL, HP661X_INSTR_ERROR_INV_SESSION );	\
	}																	\
}
#else
#define DEBUG_CHK_THIS(instrumentHandle, thisPtr) 	
#endif

/*****************************************************************************/
/*  PARAMETERS                                                               */
/*    ViSession              instrumentHandle (in)                           */
/*    struct hp661x_globals *thisPtr (in)                                   */
/*      A pointer to the VISA globals for this VISA session                  */
/*    ViStatus               s (in)                                          */
/*      A driver error return code.                                          */
/*****************************************************************************/
static ViStatus statusUpdate(ViSession  instrumentHandle,
							 struct     hp661x_globals *thisPtr,
							 ViStatus   s ) 
{   
	ViStatus  errStatus;
	ViInt16   eventQ;

	if (thisPtr)
		thisPtr->errNumber = s;

        /*************************************************************/
        /*  If error query detect is set on and the instrument       */
        /*    has not timed out, then ...                            */
        /*  Read the status byte into eventQ and check the error     */
        /*    bytes.                                                 */
        /*  Potential status byte errors are 0x04 (Query Error),     */
        /*    0x08 (Device Dependent Error), 0x10 (Execution Error), */
        /*    and 0x20 (Command Error).                              */
        /*************************************************************/

	if (thisPtr && thisPtr->errQueryDetect && s != VI_ERROR_TMO)
	{   
		if ((errStatus = viQueryf(instrumentHandle, "*ESR?\n", "%hd%*t",
			&eventQ)) < VI_SUCCESS)
			return VI_ERROR_SYSTEM_ERROR;

		if( (0x04  | 0x08  | 0x10  | 0x20) & eventQ )
			return HP661X_INSTR_ERROR_DETECTED;
	}

	return s;
}        /* ----- end of function ----- */


/*****************************************************************************/
/*  MACRO CHK_BOOLEAN                                                        */
/*****************************************************************************/
/*  Ref chk_boolean and statusUpdate for info.                               */
/*****************************************************************************/

#define CHK_BOOLEAN(chk_val, err) \
if (chk_boolean(thisPtr, chk_val)) \
    return statusUpdate(instrumentHandle, thisPtr, err);

/*****************************************************************************/
/*  PARAMETERS                                                               */
/*    struct hp661x_globals *thisPtr (in)                                   */
/*      A pointer to the VISA globals for this VISA session                  */
/*    ViBoolean              chk_val (in)                                    */
/*      Check this value to make sure it is a legal Boolean.                 */
/*****************************************************************************/
static ViBoolean chk_boolean(struct     hp661x_globals *thisPtr,
							 ViBoolean  chk_val)
{
	ViChar message[HP661X_ERR_MSG_LENGTH];

	if ( (chk_val != VI_TRUE) && (chk_val != VI_FALSE) )
	{
		sprintf(message, HP661X_MSG_BOOLEAN, chk_val);	
		strcpy(thisPtr->errMessage, message);				
		return VI_TRUE;
	}

	return VI_FALSE;
}        /* ----- end of function ----- */


/*****************************************************************************/
/*  MACRO CHK_REAL_RANGE                                                     */
/*****************************************************************************/
/*  Ref chk_real_range and statusUpdate for info.                            */
/*****************************************************************************/

#define CHK_REAL_RANGE(chk_val, min, max, err)         \
if (chk_real_range(thisPtr, chk_val, min, max))   \
    return statusUpdate(instrumentHandle, thisPtr, err);


/*****************************************************************************/
/*  Tests to see if a ViReal64 is in range.                                  */
/*                                                                           */
/*  PARAMETERS                                                               */
/*    struct hp661x_globals *thisPtr (in)                                   */
/*      A pointer to the VISA globals for this VISA session                  */
/*    ViReal64             chk_val (in)                                      */
/*      The value to be checked.                                             */
/*    ViReal64             min (in)                                          */
/*      The bottom of the range.                                             */
/*    ViReal64             max (in)                                          */
/*      The top of the range.                                                */
/*****************************************************************************/
static ViBoolean chk_real_range(struct    hp661x_globals *thisPtr,
								ViReal64  chk_val,
								ViReal64  min,
								ViReal64  max)
{
	ViChar message[HP661X_ERR_MSG_LENGTH];

	if ( (chk_val < min) || (chk_val > max) )  			
	{								
		sprintf(message, HP661X_MSG_REAL, min, max, chk_val);	
		strcpy(thisPtr->errMessage, message);
		return VI_TRUE;
	}

	return VI_FALSE;
}        /* ----- end of function ----- */ 
 
  
/*****************************************************************************/
/*  MACRO CHK_INT_RANGE                                                      */
/*****************************************************************************/
/*  Ref chk_int_range and statusUpdate for info.                             */
/*****************************************************************************/

#define CHK_INT_RANGE(chk_val, min, max, err)       \
if (chk_int_range(thisPtr, chk_val, min, max) )      \
    return statusUpdate(instrumentHandle, thisPtr, err);


/*****************************************************************************/
/*  Tests to see if a ViInt16 is in range.                                   */
/*                                                                           */
/*  PARAMETERS                                                               */
/*    struct hp661x_globals *thisPtr (in)                                   */
/*      A pointer to the VISA globals for this VISA session                  */
/*    ViInt16              chk_val (in)                                      */
/*      The value to be checked.                                             */
/*    ViInt16              min (in)                                          */
/*      The bottom of the range.                                             */
/*    ViInt16              max (in)                                          */
/*      The top of the range.                                                */
/*****************************************************************************/
static ViBoolean chk_int_range(struct   hp661x_globals *thisPtr,
							   ViInt16  chk_val,
							   ViInt16  min,
							   ViInt16  max)
{
	ViChar message[HP661X_ERR_MSG_LENGTH];

	if ( (chk_val < min) || (chk_val > max) )  			
	{								
		sprintf(message, HP661X_MSG_INT, min, max, chk_val);	
		strcpy(thisPtr->errMessage, message);
		return VI_TRUE;
	}

	return VI_FALSE;
}        /* ----- end of function ----- */ 
   
/*****************************************************************************/
/*  MACRO CHK_LONG_RANGE                                                     */
/*****************************************************************************/
/*  Ref chk_long_range and statusUpdate for info.                            */
/*****************************************************************************/

#define CHK_LONG_RANGE(chk_val, min, max, err)      \
if (chk_long_range(thisPtr, chk_val, min, max))     \
    return statusUpdate(instrumentHandle, thisPtr, err);

/*****************************************************************************/
/*  Tests to see if a ViInt32 is in range.                                   */
/*                                                                           */
/*  PARAMETERS                                                               */
/*    struct hp661x_globals *thisPtr (in)                                   */
/*      A pointer to the VISA globals for this VISA session                  */
/*    ViInt32              chk_val (in)                                      */
/*      The value to be checked.                                             */
/*    ViInt32              min (in)                                          */
/*      The bottom of the range.                                             */
/*    ViInt32              max (in)                                          */
/*      The top of the range.                                                */
/*****************************************************************************/
static ViBoolean chk_long_range( struct  hp661x_globals *thisPtr,
								ViInt32 chk_val,
								ViInt32 min,
								ViInt32 max)
{
	ViChar message[HP661X_ERR_MSG_LENGTH];

	if ( (chk_val < min) || (chk_val > max) )  			
	{								
		sprintf(message, HP661X_MSG_LONG, min, max, chk_val);	
		strcpy(thisPtr->errMessage, message);
		return VI_TRUE;
	}

	return VI_FALSE;
}        /* ----- end of function ----- */ 
   
   
/*****************************************************************************/
/*  MACRO CHK_ENUM                                                           */
/*****************************************************************************/
/*  Ref chk_enum and statusUpdate for info.                                  */
/*****************************************************************************/

#define CHK_ENUM( chk_val, limit, err )							\
	if (chk_enum( thisPtr, chk_val, limit) )					\
		return statusUpdate(instrumentHandle, thisPtr, err);

/*****************************************************************************/
/* Chk_enum searches for a string in an array of strings.  It is used by     */
/* the CHK_ENUM macro                                                        */
/*                                                                           */
/*  PARAMETERS                                                               */
/*    struct hp661x_globals *thisPtr (in)                                    */
/*      A pointer to the VISA globals for this VISA session                  */
/*    ViInt16              chk_val (in)                                      */
/*      The value to be checked.                                             */
/*    ViInt16              limit (in)                                        */
/*      The upper limit of the enumeration values.                           */
/*****************************************************************************/
static ViBoolean chk_enum (struct hp661x_globals *thisPtr,
						   ViInt16 chk_val,
						   ViInt16 limit)
{
	ViChar message[HP661X_ERR_MSG_LENGTH];

	if ( ( chk_val < 0 ) || (chk_val > limit) )  			
	{								
		sprintf(message, HP661X_MSG_INT, 0, limit, chk_val);	
		strcpy(thisPtr->errMessage, message);
		return VI_TRUE;
	}

	return VI_FALSE;
}        /* ----- end of function ----- */

/*----------------------------------------------------------------------------
 *  This function is only called from hp661x_init.
 *
 *  PARAMETERS
 *    ViSession      vi (in)
 *      Instrument Handle returned by viOpen.
 *    ViSession      defRM (in)
 *      Resource Manager handle from viOpen.
 *    ViStatus       errStatus (in)
 *      A driver error return code.
 *----------------------------------------------------------------------------
 */
static ViStatus _VI_FUNC initError(ViPSession  vi,
								   ViSession  defRM,
								   ViStatus   errStatus)
{
	viClose(*vi);
	viClose(defRM);
	*vi=VI_NULL;
	return errStatus;
}        /* ----- end of function ----- */


/*----------------------------------------------------------------------------
	This function parses the input idString for the HP manufacuter
	name and a model ID.  The ID is compared to a table of supported
	model for this driver.  If a supported model is found,
	the table index is returned, and the VI_SUCCESS function status is returned.
	Else HP661X_INSTR_ERROR_LOOKUP is returned.
 *----------------------------------------------------------------------------
 */
static ViStatus findModel (struct	hp661x_globals *thisPtr,
						   ViPChar	pIdString, /* ID string read from instrument */
						   ViPInt16 index) /* result index */
{
    ViInt16 i;
	ViChar	szMan[256], szModel[256];

	if (strlen(pIdString)>256)
		return HP661X_INSTR_ERROR_LOOKUP;

	sscanf(pIdString, "%[^,], %[^,],", szMan, szModel);

	if (strcmp(szMan, szHP) != 0)
		return HP661X_INSTR_ERROR_LOOKUP;

    for (i = 0; hp661x_model_table[i]; i++)
    {
		if (!strcmp (hp661x_model_table[i], szModel))
		{
			*index = i;
			return VI_SUCCESS;
		}
	}

	/* if we got here, we did not find it */
    return HP661X_INSTR_ERROR_LOOKUP;
}

/*----------------------------------------------------------------------------
     This function searches an array of strings for a specific string and     
     returns its index.  If successful, a VI_SUCCESS is returned, 
     else HP661X_INSTR_ERROR_LOOKUP is returned.
 *----------------------------------------------------------------------------
 */
static ViStatus findIndex (struct hp661x_globals *thisPtr,
						   const char * const array_of_strings[],
						   /*last entry in array must be 0 */
						   const char *string, /* string read from instrument */
						   ViPInt16 index) /* result index */
{
    ViInt16 i;
    ViInt16 my_len;
    ViChar	search_str[20];

    strcpy(search_str, string);

	/* get rid of newline if present in string */
	/* needed because %s includes newline in some VTL's */
    my_len = strlen(search_str);
	if (search_str[my_len - 1] == '\n')
		search_str[my_len - 1] = '\0';

    for (i = 0; array_of_strings[i]; i++)
    {
		if (!strcmp (array_of_strings[i], search_str))
		{
			*index = i;
			return VI_SUCCESS;
		}
	}

	/* if we got here, we did not find it */
    return HP661X_INSTR_ERROR_LOOKUP;
}

/*-----------------------------------------------------------------------------
 * hp661x_getMilliSeconds()
 *-----------------------------------------------------------------------------
 *	Parameters:
 *		None
 *  Return Value:
 *		ViUInt32: A number of milliseconds, taken from the system clock
 *	
 *  What it does:
 *		This function can be used as a system-independent clock
 *		function.  It calls system-dependent functions to get the
 *		number of milliseconds in the system clock.  By calling
 *		this function twice, you can find the number of milliseconds
 *		that have occurred between the calls.  Note: in the Windows
 *		3.1/LabWindows environment, this number will go up to
 *		86,400,000 and then flip back to 0.  When doing relative
 *		time measurements, be sure to take this into consideration.
 *		In the Win 32 environment, the value will flip back to 0 every
 *		49.7 days approximately.
 *-----------------------------------------------------------------------------
 */
ViUInt32 _VI_FUNC hp661x_getMilliSeconds(void)
{

#ifdef __unix

	/* this finds the number of milliseconds since Jan. 1, 1970 */

	struct timeval tv;
	struct timezone tz;
	ViUInt32 retval, msec;

	gettimeofday(&tv, &tz);


	/* get milliseconds */
	msec = ((ViUInt32)(tv.tv_usec)) / ((ViUInt32)1000);

	/* add on the number of seconds */
	/* since gettimeofday() finds the # of seconds since 1970, */
	/* subtract a fixed # from this amount with the knowledge that
	it's now at least 1996.  Otherwise multiplying by 1000 will cause overflow. */

	retval = msec + ((ViUInt32)((tv.tv_sec) - 788400000) * (ViUInt32)1000);

	return retval;

#else		/* not unix */

#if (defined _WIN32) || (defined __WIN32__)

	/* this finds the number of milliseconds since windows was started */

	ViUInt32 retval;

	retval = (ViUInt32)GetTickCount();
	return retval;

#else /* not unix, not win32 */

#ifdef _CVI_  

	ViReal64 t;
	ViUInt32 retval;

	/* gets # of seconds in system clock */
	t = Timer();
	t = t * 1000;  /* convert to milliseconds */
	retval = (ViUInt32) t;

	return retval;

#endif /* labwindows */
#endif /* win32 */
#endif /* unix */
}

/*-----------------------------------------------------------------------------
 * wait()
 *-----------------------------------------------------------------------------
 *	Parameters:
 *		ViSession instrumentHandle: the instrument session
 *		struct hp661x_globals *thisPtr : specific inst. information
 *			- assume it's already been initialized with 
 *				GET_GLOBALS().
 *	Return Value:
 *		ViStatus : VI_SUCCESS
 *
 *	What it does:
 *		Wait until a specific amount of time has passed
 *		and then return.
 *-----------------------------------------------------------------------------
 */
static ViStatus wait(ViUInt32 delayTime)
{
	ViUInt32 t, timeout;

	timeout = hp661x_getMilliSeconds() + delayTime;

	t = hp661x_getMilliSeconds();

    while (t < timeout)
		t = hp661x_getMilliSeconds();

	return VI_SUCCESS;
}


/*
 *----------------------------------------------------------------------------
 *	hp661x_getErrMsgFromNum()
 *----------------------------------------------------------------------------
 *	Parameters:
 *		ViInt32 errnum: IN - the numeric code of the error
 *		ViString errmsg: OUT - the message associated with that code
 *
 *  Return Value:
 *		ViStatus: status of operation
 *
 *  What it does:
 *		gets the error message associated with that errnum for the
 *		ERR? query.
 *----------------------------------------------------------------------------
 */
static ViStatus _VI_FUNC hp661x_getErrMsgFromNum(ViInt32 errnum, ViChar _VI_FAR error_message[])
{
    /* this is from the table in the manual */
	switch(errnum)
	{
	case 0:
		sprintf(error_message, "No Errors");
		break;

	case -100:
		sprintf(error_message, "Command error");
		break;

	case -101:
		sprintf(error_message, "Invalid character");
		break;

	case -102:
		sprintf(error_message, "Syntax error");
		break;

	case -103:
		sprintf(error_message, "Invalid separator");
		break;

	case -104:
		sprintf(error_message, "Data type error");
		break;

	case -105:
		sprintf(error_message, "GET not allowed");
		break;

	case -108:
		sprintf(error_message, "Parameter not allowed");
		break;

	case -109:
		sprintf(error_message, "Missing parameter");
		break;

	case -112:
		sprintf(error_message, "Program mnemonic too long");
		break;

	case -113:
		sprintf(error_message, "Undefined header");
		break;

	case -121:
		sprintf(error_message, "Invalid character in number");
		break;

	case -123:
		sprintf(error_message, "Numeric Overflow");
		break;

	case -124:
		sprintf(error_message, "Too many digits");
		break;

	case -128:
		sprintf(error_message, "Numeric data not allowed");
		break;

	case -131:
		sprintf(error_message, "Invalid suffix");
		break;

	case -138:
		sprintf(error_message, "Suffix not allowed");
		break;

	case -141:
		sprintf(error_message, "Invalid character data");
		break;

	case -144:
		sprintf(error_message, "Character data too long");
		break;

	case -148:
		sprintf(error_message, "Character data not allowed");
		break;

	case -150:
		sprintf(error_message, "String data error");
		break;

	case -151:
		sprintf(error_message, "Invalid string data");
		break;

	case -158:
		sprintf(error_message, "String data not allowed");
		break;

	case -160:
		sprintf(error_message, "Block data error");
		break;

	case -161:
		sprintf(error_message, "Invalid block data");
		break;

	case -168:
		sprintf(error_message, "Block data not allowed");
		break;

	case -170:
		sprintf(error_message, "Expression error");
		break;

	case -171:
		sprintf(error_message, "Invalid expression");
		break;

	case -178:
		sprintf(error_message, "Expression data not allowed");
		break;

	case -200:
		sprintf(error_message, "Execution error");
		break;

	case -222:
		sprintf(error_message, "Data out of range");
		break;

	case -223:
		sprintf(error_message, "Too much data");
		break;

	case -224:
		sprintf(error_message, "Illegal parameter value");
		break;

	case -225:
		sprintf(error_message, "Out of memory");
		break;

	case -241:
		sprintf(error_message, "Missing hardware");
		break;

	case -270:
		sprintf(error_message, "Macro error");
		break;

	case -272:
		sprintf(error_message, "Macro execution error");
		break;

	case -273:
		sprintf(error_message, "Illegal macro label");
		break;

	case -276:
		sprintf(error_message, "Macro recursion error");
		break;

	case -277:
		sprintf(error_message, "Macro redefinition not allowed");
		break;

	case -310:
		sprintf(error_message, "System error");
		break;

	case -350:
		sprintf(error_message, "Too many errors");
		break;

	case -400:
		sprintf(error_message, "Query error");
		break;

	case -410:
		sprintf(error_message, "Query INTERRUPTED");
		break;

	case -420:
		sprintf(error_message, "Query UNTERMINATED");
		break;

	case -430:
		sprintf(error_message, "Query DEADLOCKED");
		break;

	case -440:
		sprintf(error_message, "Query UNTEMINATED");
		break;

	case HP661X_INSTR_ERROR_NVRAM_RD0_CS:
		sprintf(error_message, "Non-volatile RAM RD0 section checksum failed");
		break;

	case HP661X_INSTR_ERROR_NVRAM_CONFIG_CS:
		sprintf(error_message, "Non-volatile RAM CONFIG section checksum failed");
		break;

	case HP661X_INSTR_ERROR_NVRAM_CAL_CS:
		sprintf(error_message, "Non-volatile RAM CAL section checksum failed");
		break;

	case HP661X_INSTR_ERROR_NVRAM_STATE_CS:
		sprintf(error_message, "Non-volatile RAM STATE section checksum failed");
		break;

	case HP661X_INSTR_ERROR_NVRAM_RST_CS:
		sprintf(error_message, "Non-volatile RST section checksum failed");
		break;

	case HP661X_INSTR_ERROR_RAM_SELFTEST:
		sprintf(error_message, "RAM selftest");
		break;

	case HP661X_INSTR_ERROR_DAC_SELFTEST1:
		sprintf(error_message, "VDAC/IDAC selftest 1");
		break;

	case HP661X_INSTR_ERROR_DAC_SELFTEST2:
		sprintf(error_message, "VDAC/IDAC selftest 2");
		break;

	case HP661X_INSTR_ERROR_DAC_SELFTEST3:
		sprintf(error_message, "VDAC/IDAC selftest 3");
		break;

	case HP661X_INSTR_ERROR_DAC_SELFTEST4:
		sprintf(error_message, "VDAC/IDAC selftest 4");
		break;

	case HP661X_INSTR_ERROR_OVDAC_SELFTEST:
		sprintf(error_message, "OVDAC selftest");
		break;

	case HP661X_INSTR_ERROR_DIGIO_SELFTEST:
		sprintf(error_message, "Digital I/O selftest error");
		break;

	case HP661X_INSTR_ERROR_INGUARD_RXBUF_OVR:
		sprintf(error_message, "Ingrd receiver buffer overrun");
		break;

	case HP661X_INSTR_ERROR_RS232_FRAMING:
		sprintf(error_message, "RS-232 receiver framing error");
		break;

	case HP661X_INSTR_ERROR_RS232_PARITY:
		sprintf(error_message, "RS-232 receiver parity error");
		break;

	case HP661X_INSTR_ERROR_RS232_RX_OVR:
		sprintf(error_message, "RS-232 receiver overrun error");
		break;

	case HP661X_INSTR_ERROR_FP_UART_OVR:
		sprintf(error_message, "Front panel uart overrun");
		break;

	case HP661X_INSTR_ERROR_FP_UART_FRAMING:
		sprintf(error_message, "Front panel uart framing");
		break;

	case HP661X_INSTR_ERROR_FP_UART_PARITY:
		sprintf(error_message, "Front panel uart parity");
		break;

	case HP661X_INSTR_ERROR_FP_RXBUF_OVR:
		sprintf(error_message, "Front panel buffer overrun");
		break;

	case HP661X_INSTR_ERROR_FP_TIMEOUT:
		sprintf(error_message, "Front panel timeout");
		break;

	case HP661X_INSTR_ERROR_CAL_SWITCH:
		sprintf(error_message, "CAL switch prevents calibration");
		break;

	case HP661X_INSTR_ERROR_CAL_PASSWORD:
		sprintf(error_message, "CAL password is incorrect");
		break;

	case HP661X_INSTR_ERROR_CAL_DISABLED:
		sprintf(error_message, "CAL not enabled");
		break;

	case HP661X_INSTR_ERROR_CAL_RB_CONST:
		sprintf(error_message, "Computed readback cal constants are incorrect");
		break;

	case HP661X_INSTR_ERROR_CAL_PROG_CONST:
		sprintf(error_message, "Computed programming cal constants are incorrect");
		break;

	case HP661X_INSTR_ERROR_CAL_CMD_SEQUENCE:
		sprintf(error_message, "Incorrect sequence of calibration commands");
		break;

	case HP661X_INSTR_ERROR_CVCC_STATUS:
		sprintf(error_message, "CV or CC status is incorrect for this command");
		break;

	case HP661X_INSTR_ERROR_ALC_NOT_NORMAL:
		sprintf(error_message, "Output mode switch mus be in NORMAL position");
		break;

	case HP661X_INSTR_ERROR_TOO_MANY_SWE_POINTS:
		sprintf(error_message, "Too many sweep points");
		break;

	case HP661X_INSTR_ERROR_RS232_CMD_ONLY:
		sprintf(error_message, "Command only applies to RS-232 interface");
		break;

	case HP661X_INSTR_ERROR_INCOMPATIBLE_FETCH:
		sprintf(error_message, "CURRent or VOLTage fetch incompatible with last acquisition");
		break;

	case HP661X_INSTR_ERROR_MEAS_OVERRANGE:
		sprintf(error_message, "Measurement overrange");
		break;

	default:
		sprintf(error_message, "Unknown Error");
		break;
	}


	return VI_SUCCESS;
}	

/*****************************************************************************/
/********************                                     ********************/
/**********                                                         **********/
/****                          DRIVER FUNCTIONS                           ****/
/**                                                                         **/
/**-------------------------------------------------------------------------**/
/**     The functions which are exposed to the driver user start here.      **/
/**     They are in the following order:                                    **/
/**        VPP required functions.                                          **/
/**        HP required utility functions                                    **/
/**        HP passthrough functions                                         **/
/**        Instrument specific functions                                    **/
/****                                                                     ****/
/**********                                                         **********/
/********************                                     ********************/
/*****************************************************************************/

/*----------------------------------------------------------------------------
 * hp661x_init
 *----------------------------------------------------------------------------
 *   Parameter Name                                       Type    Direction
 *  ------------------------------------------------------------------------
 * | resourceName                                        ViRsrc      IN
 * |   ---------------------------------------------------------------------
 * |  | The Instrument Description.
 * |  |
 * |  | Examples: GPIB0::5::INSTR
 *  ------------------------------------------------------------------------
 * | idQuery                                             ViBoolean   IN
 * |   ---------------------------------------------------------------------
 * |  | if( VI_TRUE) Perform In-System Verification.
 * |  | if(VI_FALSE) Do not perform In-System Verification
 *  ------------------------------------------------------------------------
 * | resetDevice                                         ViBoolean   IN
 * |   ---------------------------------------------------------------------
 * |  | IF( VI_TRUE) Perform Reset Operation.
 * |  | if(VI_FALSE) Do not perform Reset operation
 *  ------------------------------------------------------------------------
 * | instrumentHandle                                    ViPSession  OUT
 * |   ---------------------------------------------------------------------
 * |  | Instrument Handle. This is VI_NULL if an error occurred
 * |  | during the init.
 *----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_init (ViRsrc resourceName,
								ViBoolean idQuery,
								ViBoolean resetDevice,
								ViPSession instrumentHandle)
{
	struct		hp661x_globals *thisPtr;
	/* added some variables */
	ViStatus	errStatus;
	ViSession	defRM;
	char		idn_buf[HP661X_CMDSTRING_MAX];
	ViReal64	tempLev;
	ViInt16		i;

	*instrumentHandle = VI_NULL;

    /*************************************************************/
    /* Find the Default Resource Manager.  Potential errors are  */
    /*   VI_ERROR_SYSTEM_ERROR, VI_ERROR_ALLOC                   */
    /*************************************************************/

	if ((errStatus = viOpenDefaultRM(&defRM)) < VI_SUCCESS)
		return errStatus;

    /*************************************************************/
    /* Open a session to the instrument.  Potential errors are   */
    /*   VI_ERROR_NSUP_OPER, VI_ERROR_INV_RSRC_NAME,             */
    /*   VI_ERROR_INV_ACC_MODE, VI_ERROR_RSRC_NFOUND,            */
    /*   VI_ERROR_ALLOC                                          */
    /*************************************************************/

	if ((errStatus = viOpen(defRM, resourceName,
		VI_NULL, VI_NULL, instrumentHandle)) < VI_SUCCESS)
	{
		viClose(defRM);
		*instrumentHandle=VI_NULL;
		return errStatus;
	}

    /*************************************************************/
    /* Allocate global memory for the session.  Inititalize the  */
    /*   memory.  Note that viSetAttribute should never fail,    */
    /*   as all of it's input conditions are satisfied.          */
    /*************************************************************/

	if ((thisPtr = (struct hp661x_globals *)malloc(sizeof(struct hp661x_globals))) == 0)
	{
		viClose(*instrumentHandle);
		viClose(defRM);
		*instrumentHandle=VI_NULL;
		return VI_ERROR_ALLOC;
	}

	if ((errStatus = viSetAttribute(*instrumentHandle, VI_ATTR_USER_DATA,
		(ViAttrState)thisPtr)) < VI_SUCCESS)
	{
		viClose(*instrumentHandle);
		viClose(defRM);
		*instrumentHandle=VI_NULL;
		return errStatus;
	}

    /*************************************************************/
    /* Set initial values of variables in global memory.         */
    /*************************************************************/

	/* common variables */
	thisPtr->errNumber = VI_SUCCESS;
	thisPtr->errMessage[0] = 0;
	thisPtr->errQueryDetect = VI_FALSE;
	if ((errStatus = viGetAttribute(*instrumentHandle, VI_ATTR_INTF_TYPE,
		&thisPtr->interfaceType)) < VI_SUCCESS)
		return initError(instrumentHandle, defRM, errStatus);

	/* instrument specific variables */
	thisPtr->defRMSession = defRM;
	thisPtr->errFuncName[0] = 0;
	strcpy(thisPtr->address, resourceName);
	thisPtr->blockSrqIO = VI_FALSE;
	thisPtr->countSrqIO = 0;
	thisPtr->settleTime = HP661X_CLOCK_HW_SETTLE;
	thisPtr->clrTime = HP661X_CLOCK_RESET_INST;
	thisPtr->testTime = HP661X_CLOCK_SELF_TEST;
	thisPtr->measTimeout = HP661X_DEFAULT_MEAS_TIMEOUT ;

	/* no errors have occurred */
	thisPtr->fault_occurred = VI_FALSE;
	thisPtr->error_occurred = VI_FALSE;
	thisPtr->pon_occurred = VI_FALSE;


    /*************************************************************/
    /* Reset command parser										 */
    /*************************************************************/
	if (thisPtr->interfaceType == VI_INTF_GPIB)
		if (viClear(*instrumentHandle) <  VI_SUCCESS)
			return initError(instrumentHandle, defRM, VI_ERROR_FAIL_ID_QUERY);

    /*************************************************************/
    /* Check programming language of the instrument.			 */
	/* This dirver only supports SCPI.                           */
    /*************************************************************/
	if (viQueryf(*instrumentHandle, "SYST:LANG?\n", "%s", idn_buf) < VI_SUCCESS)
		return initError(instrumentHandle, defRM, VI_ERROR_FAIL_ID_QUERY);

	if (strcmp(idn_buf, szLANG_COMP)==0)
	{
		/* change programming language to SCPI */
		if (viPrintf(*instrumentHandle, "SYST:LANG SCPI\n") < VI_SUCCESS)
			return initError(instrumentHandle, defRM, VI_ERROR_FAIL_ID_QUERY);
		wait(100);
	}
    
	/*************************************************************/
    /* Reset the instrument if so instructed.                    */
    /*   GPIB reset includes a 488.2 device clear.               */
    /*************************************************************/

	if( VI_TRUE == resetDevice ) 
	{
		if (hp661x_reset(*instrumentHandle) <  VI_SUCCESS)
			return initError(instrumentHandle, defRM,
				HP661X_INSTR_ERROR_RESET_FAILED);
	}

    /*************************************************************/
    /* Validate the instrument's identity.                       */
    /*************************************************************/

	if( VI_TRUE == idQuery ) 
	{
		if (viQueryf(*instrumentHandle, "*IDN?\n", "%t", idn_buf) < VI_SUCCESS)
			return initError(instrumentHandle, defRM, VI_ERROR_FAIL_ID_QUERY);

		if (findModel(thisPtr, idn_buf, &i))
			return initError(instrumentHandle, defRM, VI_ERROR_FAIL_ID_QUERY);
	}

    /*************************************************************/
	/* initialize maximum ratings of instrument                       */
    /*************************************************************/

	/* get max programmable voltage values from instrument */
	if (errStatus=viQueryf(*instrumentHandle, "VOLT? MAX\n", "%lf", &tempLev) < VI_SUCCESS)
			return initError(instrumentHandle, defRM, VI_ERROR_FAIL_ID_QUERY);
	thisPtr->voltMax=tempLev + tempLev * VIREAL64_LSB;

	/* get max voltage protection values from instrument */
	if (errStatus=viQueryf(*instrumentHandle, "VOLT:PROT? MAX\n", "%lf", &tempLev) < VI_SUCCESS)
			return initError(instrumentHandle, defRM, VI_ERROR_FAIL_ID_QUERY);
	thisPtr->ovpMax=tempLev + tempLev * VIREAL64_LSB;

	/* get max programmable current values from instrument */
	if (errStatus=viQueryf(*instrumentHandle, "CURR? MAX\n", "%lf", &tempLev) < VI_SUCCESS)
			return initError(instrumentHandle, defRM, VI_ERROR_FAIL_ID_QUERY);
	thisPtr->currMax=tempLev + tempLev * VIREAL64_LSB;

	/* get max sample time from instrument */
	if (errStatus=viQueryf(*instrumentHandle, "SENS:SWE:TINT? MAX\n", "%lf", &tempLev) < VI_SUCCESS)
			return initError(instrumentHandle, defRM, VI_ERROR_FAIL_ID_QUERY);
	thisPtr->tintMax=tempLev + tempLev * VIREAL64_LSB;

	return statusUpdate( *instrumentHandle, thisPtr, VI_SUCCESS);
}

/*----------------------------------------------------------------------------
 * hp661x_close
 *----------------------------------------------------------------------------
 *   Parameter Name                                       Type    Direction
 *  ------------------------------------------------------------------------
 * | instrumentHandle                                    ViSession   IN
 * |   ---------------------------------------------------------------------
 * |  | Instrument Handle returned from hp661x_init()
 *
 *----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_close(ViSession instrumentHandle)
{
	struct hp661x_globals *thisPtr;
	ViStatus errStatus=0;
	ViSession defRM;

	GET_GLOBALS(instrumentHandle, thisPtr);
	DEBUG_CHK_THIS(instrumentHandle, thisPtr);

	/* retrieve Resource Management session */
	defRM = thisPtr->defRMSession;

	/* free memory */
	if( thisPtr)		
	{
		/* make sure there is something to free */
		free( thisPtr);
	}	

	/* close the instrumentHandle and RM sessions */
	return viClose( defRM);
} /*  _close */



/*----------------------------------------------------------------------------
 * hp661x_reset
 *----------------------------------------------------------------------------
 *    Parameter Name                                       Type    Direction
 *   ------------------------------------------------------------------------
 *  | instrumentHandle                                   ViSession   IN
 *  |   ---------------------------------------------------------------------
 *  |  | Instrument Handle returned from hp661x_init()
 *
 *----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_reset(ViSession instrumentHandle)
{
	struct hp661x_globals *thisPtr;
	ViStatus errStatus=0;

	GET_GLOBALS(instrumentHandle, thisPtr)

	DEBUG_CHK_THIS(instrumentHandle, thisPtr);

	errStatus = viPrintf(instrumentHandle, "*RST;*CLS\n");

	return statusUpdate(instrumentHandle, thisPtr, errStatus);
}/* _reset */



/*----------------------------------------------------------------------------
 * hp661x_self_test
 *----------------------------------------------------------------------------
 *   Parameter Name                                       Type    Direction
 *  ------------------------------------------------------------------------
 * | instrumentHandle                                   ViSession   IN
 * |   ---------------------------------------------------------------------
 * |  | Instrument Handle returned from hp661x_init()
 *  ------------------------------------------------------------------------
 * | test_result                                        ViPInt16    OUT
 * |   ---------------------------------------------------------------------
 * |  | Numeric result from self-test operation
 * |  |
 * |  | 0 = no error ( test passed)
 * |  | anything else = failure
 *  ------------------------------------------------------------------------
 * | test_message                                       ViChar _VI_FAR []OUT
 * |   ---------------------------------------------------------------------
 * |  | Self-test status message.  This is limited to 256 characters.
 *
 *----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_self_test (ViSession instrumentHandle,
                                     ViPInt16 testResult,
									 ViChar _VI_FAR testMessage[])
{
	struct hp661x_globals *thisPtr;
	ViStatus errStatus=0;

	/* initialize output parameters */
	*testResult = -1; 
	testMessage[0] = 0; 

	GET_GLOBALS(instrumentHandle, thisPtr);

	DEBUG_CHK_THIS(instrumentHandle, thisPtr);

	thisPtr->blockSrqIO = VI_TRUE;

	errStatus = viPrintf(instrumentHandle, "*TST?\n");
	if( VI_SUCCESS > errStatus)
	{	
		return statusUpdate(instrumentHandle, thisPtr, errStatus);
	}

	wait(thisPtr->testTime);

	/* read test result */
	errStatus = viScanf(instrumentHandle, "%hd", testResult);
	if( VI_SUCCESS > errStatus)
	{
		*testResult = -1; 
		return statusUpdate(instrumentHandle, thisPtr, errStatus);
	}

	errStatus = viQueryf(instrumentHandle, "SYST:ERR?\n", "%t", testMessage);
	if( VI_SUCCESS > errStatus)
		*testResult = -1; 

	return statusUpdate(instrumentHandle, thisPtr, errStatus);
}



/*----------------------------------------------------------------------------
 * hp661x_error_query
 *----------------------------------------------------------------------------
    Parameter Name                                       Type    Direction
   ------------------------------------------------------------------------
  | instrumentHandle                                    ViSession   IN
  |   ---------------------------------------------------------------------
  |  | Instrument Handle returned from hp661x_init()
   ------------------------------------------------------------------------
  | error_number                                        ViPInt32    OUT
  |   ---------------------------------------------------------------------
  |  | Instruments error code
   ------------------------------------------------------------------------
  | error_message                                       ViChar _VI_FAR []OUT
  |   ---------------------------------------------------------------------
  |  | Instrument's error message.  This is limited to 256 characters.
 *----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_error_query (ViSession instrumentHandle,
                                       ViPInt32 errorCode,
									   ViChar _VI_FAR errorMessage[])
{
	struct hp661x_globals *thisPtr;
	ViStatus errStatus=0;	

	/* initialize output parameters */
	*errorCode = -1; 
	errorMessage[0] = 0; 

	GET_GLOBALS(instrumentHandle, thisPtr);

	DEBUG_CHK_THIS(instrumentHandle, thisPtr);

	errStatus = viQueryf(instrumentHandle, "SYST:ERR?\n", "%ld,%t", errorCode, errorMessage);

	if( VI_SUCCESS > errStatus)
	{
		*errorCode = -1; 
		errorMessage[0] = 0; 
		return statusUpdate(instrumentHandle, thisPtr, errStatus);
	}

	hp661x_getErrMsgFromNum(*errorCode, errorMessage);

	return statusUpdate(instrumentHandle, thisPtr, VI_SUCCESS);
}


/*----------------------------------------------------------------------------
 * hp661x_error_message
 *----------------------------------------------------------------------------
 *  PARAMETERS
 *    ViSession   instrumentHandle (in)
 *      Instrument Handle returned from hp661x_init()
 *      May be VI_NULL for this function
 *    ViStatus    statusCode (in)
 *      The error return value from an instrument driver function
 *    ViPString   message[] (out)
 *      Error message string.  This is limited to 256 characters.
 *-----------------------------------------------------------------------------
*/
ViStatus _VI_FUNC hp661x_error_message (ViSession instrumentHandle,
                                         ViStatus statusCode,
                                         ViChar _VI_FAR statusMessage[])
{
	struct hp661x_globals *thisPtr;
	ViStatus errStatus=0;  
	ViInt32 idx;

	/* initialize output parameters */
	statusMessage[0] = 0;

	/* if we have a VI_NULL, then we need to return a error message */
	if( VI_NULL == instrumentHandle)
	{
		strcpy(statusMessage, HP661X_MSG_VI_OPEN_ERR);
		return HP661X_INSTR_ERROR_INV_SESSION;
	} 

	thisPtr = NULL;

	/* try to find a thisPtr */
	if( VI_NULL != instrumentHandle)
	{
		errStatus = viGetAttribute(instrumentHandle,
			VI_ATTR_USER_DATA, (ViAddr) &thisPtr);
		if( VI_SUCCESS > errStatus)
		{
			/* Errors: VI_ERROR_INV_SESSION	*/
			strcpy( statusMessage, HP661X_MSG_INVALID_VI);
			return errStatus;
		}
		DEBUG_CHK_THIS(instrumentHandle, thisPtr);
	} 

	if( VI_SUCCESS == statusCode)
	{
		sprintf( statusMessage, HP661X_MSG_NO_ERRORS);
		return statusUpdate(instrumentHandle, thisPtr, VI_SUCCESS);
	}

    /*************************************************************/
    /* Search the error message table to see if the error is in  */
    /*   that table.  If it is, copy the corresponding error     */
    /*   message to the output error message.  If context        */
    /*   dependent error messages are being used, append the     */
    /*   context dependent information to the message.           */
    /* If the error is found in the table, exit the routine      */
    /*   successfully.                                           */
    /*************************************************************/
	for(idx=0; 
	    idx < (sizeof instrErrMsgTable / sizeof( struct instrErrStruct));
	    idx++)
	{
		/* check for a matching error number */
		if( instrErrMsgTable[idx].errStatus == statusCode)
		{
			if( (thisPtr) &&
			    (thisPtr->errNumber == statusCode))
			{
				/* context dependent error
				 * message is available.
				 */
				sprintf( statusMessage,
					"%s - %s",
					instrErrMsgTable[idx].errMessage,
					thisPtr->errMessage);
			}
			else
			{
				/* No context dependent eror 
				 * message available so copy 
				 * the static error message
				 */
				strcpy(statusMessage, instrErrMsgTable[idx].errMessage);
			}
            
			return statusUpdate(instrumentHandle, thisPtr, VI_SUCCESS);
		}
	}

    /*************************************************************/
    /* Check to see if the error is a VTL/VISA error number      */
    /*   using viStatusDesc, which will also return the error    */
    /*   message.  If context dependent error messages are       */
    /*   used, append the context dependent information to the   */
    /*   message.                                                */
    /* If the error is found in the table, exit the routine      */
    /*   successfully.                                           */
    /*************************************************************/
	errStatus = viStatusDesc(instrumentHandle, statusCode, statusMessage);
	if( VI_SUCCESS == errStatus)
	{
		/* check for a context dependent error message */
		if( (thisPtr) &&
		    (thisPtr->errNumber == statusCode))
		{
			/* context dependent error
			 * message is available.
			 */
			strcat( statusMessage, " ");
			strcat( statusMessage, HP661X_MSG_IN_FUNCTION);
			strcat( statusMessage, " ");
			strcat( statusMessage, thisPtr->errFuncName);
			strcat( statusMessage, "() ");
			strcat( statusMessage, thisPtr->errMessage);
		}

		/* VTL found an error message, so return success */
		return statusUpdate(instrumentHandle, thisPtr, VI_SUCCESS);
	}

    /*************************************************************/
    /*   At this point vi is either INVALID or VI_NULL           */
    /*************************************************************/

	/* user passed in a invalid status */
	sprintf( statusMessage,
		HP661X_MSG_INVALID_STATUS
		"  %ld"
		HP661X_MSG_INVALID_STATUS_VALUE,
		(long)statusCode );
	
	return statusUpdate(instrumentHandle, thisPtr, VI_ERROR_PARAMETER2);
}


/****************************************************************************
hp661x_revision_query
*****************************************************************************
    Parameter Name                                       Type    Direction
   ------------------------------------------------------------------------
  | instrumentHandle                                                ViSession      IN
  |   ---------------------------------------------------------------------
  |  | Instrument Handle returned from hp661x_init()
   ------------------------------------------------------------------------
  | instrumentDriverRevision                                ViChar _VI_FAR []      OUT
  |   ---------------------------------------------------------------------
  |  | Instrument driver revision.  This is limited to 256 characters.
   ------------------------------------------------------------------------
  | firmwareRevision                                 ViChar _VI_FAR []      OUT
  |   ---------------------------------------------------------------------
  |  | Instrument firmware revision.  This is limited to 256 characters.

*****************************************************************************/

ViStatus _VI_FUNC hp661x_revision_query (ViSession instrumentHandle,
                                          ViChar _VI_FAR instrumentDriverRevision[],
                                          ViChar _VI_FAR firmwareRevision[])
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus=0;
	ViChar		szString[HP661X_CMDSTRING_MAX];		/* temp hold for instr rev string */

	GET_GLOBALS(instrumentHandle, thisPtr)
	DEBUG_CHK_THIS(instrumentHandle, thisPtr);

	/* initialize output parameters */
	firmwareRevision[0] = 0; 
	sprintf( instrumentDriverRevision, "%s\0", HP661X_REV_CODE);

	if(( errStatus = viPrintf(instrumentHandle, "*IDN?\n")) < VI_SUCCESS)
	{
		return statusUpdate(instrumentHandle, thisPtr, errStatus); 
	}

	if(( errStatus = viScanf(instrumentHandle, "%s", szString)) < VI_SUCCESS)
	{
		return statusUpdate(instrumentHandle, thisPtr, errStatus); 
	}

    strcpy(firmwareRevision, szString);
	
	return statusUpdate(instrumentHandle, thisPtr, VI_SUCCESS); 
}


/****************************************************************************
hp661x_timeOut
*****************************************************************************
    Parameter Name                                       Type    Direction
   ------------------------------------------------------------------------
  | instrumentHandle                                                ViSession   IN
  |   ---------------------------------------------------------------------
  |  | Instrument Handle returned from hp661x_init()
   ------------------------------------------------------------------------
  | timeOut                                           ViInt32     IN
  |   ---------------------------------------------------------------------
  |  | This value sets the I/O timeout for all functions in
  |  | the driver. It is specified in milliseconds.

*****************************************************************************/
/* ----------------------------------------------------------------------- */
/* Purpose:  Changes the timeout value of the instrument.  Input is in     */
/*           milliseconds.                                                 */
/* ----------------------------------------------------------------------- */
ViStatus _VI_FUNC hp661x_timeOut (ViSession instrumentHandle, ViInt32 timeOut)
{
	struct hp661x_globals *thisPtr;
	ViStatus errStatus = 0;
   

	GET_GLOBALS(instrumentHandle, thisPtr)
	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	CHK_LONG_RANGE( timeOut, 1, 2147483647, VI_ERROR_PARAMETER2 );

	errStatus = viSetAttribute(instrumentHandle, VI_ATTR_TMO_VALUE, timeOut);

	return statusUpdate(instrumentHandle, thisPtr, errStatus );
}


/****************************************************************************
hp661x_timeOut_Q
*****************************************************************************
    Parameter Name                                       Type    Direction
   ------------------------------------------------------------------------
  | instrumentHandle                                                ViSession   IN
  |   ---------------------------------------------------------------------
  |  | Instrument Handle returned from hp661x_init()
   ------------------------------------------------------------------------
  | pTimeOut                                          ViPInt32    OUT
  |   ---------------------------------------------------------------------
  |  | This is the minimum timeout period that the driver
  |  | can be set to. It is specified in milliseconds.

*****************************************************************************/
/* ----------------------------------------------------------------------- */
/* Purpose:  Returns the current setting of the timeout value of the       */
/*           instrument in milliseconds.                                   */
/* ----------------------------------------------------------------------- */
ViStatus _VI_FUNC hp661x_timeOut_Q (ViSession instrumentHandle, ViPInt32 timeOut)
{
	struct hp661x_globals *thisPtr;
	ViStatus errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr)
	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	errStatus = viGetAttribute(instrumentHandle, VI_ATTR_TMO_VALUE, timeOut );

	return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/****************************************************************************
hp661x_errorQueryDetect
*****************************************************************************
    Parameter Name                                       Type    Direction
   ------------------------------------------------------------------------
  | instrumentHandle                                                ViSession   IN
  |   ---------------------------------------------------------------------
  |  | Instrument Handle returned from hp661x_init()
   ------------------------------------------------------------------------
  | errorQueryDetect                                  ViBoolean   IN
  |   ---------------------------------------------------------------------
  |  | Boolean which enables (VI_TRUE) or disables (VI_FALSE)
  |  | automatic instrument error querying.

*****************************************************************************/
ViStatus _VI_FUNC hp661x_errorQueryDetect (ViSession instrumentHandle, ViBoolean errDetect)
/*same for both driver types */
{
	struct hp661x_globals *thisPtr;
	ViStatus errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr)
	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	CHK_BOOLEAN( errDetect, VI_ERROR_PARAMETER2 );

	thisPtr->errQueryDetect = errDetect;

	return statusUpdate(instrumentHandle, thisPtr, VI_SUCCESS );
}

/****************************************************************************
hp661x_errorQueryDetect_Q
*****************************************************************************
    Parameter Name                                       Type    Direction
   ------------------------------------------------------------------------
  | instrumentHandle                                                ViSession   IN
  |   ---------------------------------------------------------------------
  |  | Instrument Handle returned from hp661x_init()
   ------------------------------------------------------------------------
  | firmwareRevision                                        ViPBoolean  OUT
  |   ---------------------------------------------------------------------
  |  | Boolean indicating if automatic instrument error
  |  | querying is performed.

*****************************************************************************/
ViStatus _VI_FUNC hp661x_errorQueryDetect_Q (ViSession instrumentHandle,
                                              ViPBoolean errorQueryDetect)
/* same for both types of driver */
{
	struct hp661x_globals *thisPtr;
	ViStatus errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr)
	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	*errorQueryDetect = thisPtr->errQueryDetect;

	return statusUpdate(instrumentHandle, thisPtr, VI_SUCCESS );
}

/****************************************************************************
hp661x_dcl
*****************************************************************************
    Parameter Name                                       Type    Direction
   ------------------------------------------------------------------------
  | instrumentHandle                                                ViSession   IN
  |   ---------------------------------------------------------------------
  |  | Instrument Handle returned from hp661x_init()

*****************************************************************************/
ViStatus _VI_FUNC hp661x_dcl (ViSession instrumentHandle)
{
	struct hp661x_globals *thisPtr;
	ViStatus errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr)
	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	errStatus = viClear(instrumentHandle);

	return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_outputVoltCurr
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function sets the current limit and voltage, and enables
 *           the output.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViReal64 volt
 * IN        
 *            The voltage in volts.
 * 
 * PARAM 3 : ViReal64 curr
 * IN        
 *            The current in amps.
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_outputVoltCurr(ViSession instrumentHandle,
						     ViReal64 voltLev,
						     ViReal64 currLev)
{
	struct hp661x_globals *thisPtr;
	ViStatus errStatus = 0;
	ViChar szString[HP661X_CMDSTRING_MAX];

	GET_GLOBALS(instrumentHandle, thisPtr);

	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	CHK_REAL_RANGE(voltLev,
		HP661X_VOLT_MIN,
		thisPtr->voltMax,
		VI_ERROR_PARAMETER2);

 	CHK_REAL_RANGE(currLev,
		HP661X_CURR_MIN,
		thisPtr->currMax,
		VI_ERROR_PARAMETER3);

	sprintf(szString, "VOLT %.5lG;CURR %.5lG;OUTP 1\n", voltLev, currLev);
	errStatus = viPrintf(instrumentHandle, szString);

    return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_setMeasure
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViReal64 volt
 * IN        
 *            The voltage in volts.
 * 
 * PARAM 3 : ViReal64 curr
 * IN        
 *            The current in amps.
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_setMeasure (ViSession instrumentHandle,
                                      ViInt16	outputParam,
									  ViReal64	outputLev,
									  ViInt16	settlingTime,
                                      ViPReal64 voltQLev,
									  ViPReal64 currQLev,
                                      ViPInt16	outputStatus)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr);

	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	CHK_INT_RANGE(outputParam,
		HP661X_SET_VOLT,
		HP661X_SET_CURR,
		VI_ERROR_PARAMETER2);
	
	if (outputParam==HP661X_SET_VOLT)
	{
		CHK_REAL_RANGE(outputLev,
			HP661X_VOLT_MIN,
			thisPtr->voltMax,
			VI_ERROR_PARAMETER3);
	}
	else
	{
		CHK_REAL_RANGE(outputLev,
			HP661X_CURR_MIN,
			thisPtr->currMax,
			VI_ERROR_PARAMETER3);
	}

	CHK_INT_RANGE(settlingTime,
		HP661X_HW_SETTLING_MIN,
		HP661X_HW_SETTLING_MAX,
		VI_ERROR_PARAMETER4);

	if (outputParam==HP661X_SET_VOLT)
		errStatus = viPrintf(instrumentHandle, "VOLT %.5lG\n", outputLev);
	else
		errStatus = viPrintf(instrumentHandle, "CURR %.5lG\n", outputLev);

	if (settlingTime)
	{
		wait(settlingTime);
	}
    
	/* do measurements and get status */
	errStatus = hp661x_measureVolt(instrumentHandle, voltQLev);
	if (errStatus < VI_SUCCESS)
    {
       return statusUpdate(instrumentHandle, thisPtr, errStatus );
    }

	errStatus = hp661x_measureCurr(instrumentHandle, currQLev);
	if (errStatus < VI_SUCCESS)
    {
       return statusUpdate(instrumentHandle, thisPtr, errStatus );
    }

	errStatus = hp661x_readOutputStatus(instrumentHandle, outputStatus);

    return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_doDelay
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViInt16	milliSeconds
 * IN        
 *            The delay time in milliseconds.
 *
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_doDelay(ViSession	instrumentHandle,
								  ViInt16	milliseconds)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr);

	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	CHK_INT_RANGE(milliseconds,
		HP661X_HW_SETTLING_MIN,
		HP661X_HW_SETTLING_MAX,
		VI_ERROR_PARAMETER4);

	if (milliseconds)
	{
		wait(milliseconds);
	}
    
    return statusUpdate(instrumentHandle, thisPtr, VI_SUCCESS );
}


/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_readOutputStatus
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function quries the power source for the output status.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViReal64 volt
 * OUT       
 *            The present output status as follow:
 *
 *			  HP661X_OUTP_OFF		0	Power source output is in the off state.
 *			  HP661X_OUTP_CV		1   Power source is in Constant Voltage mode.
 *			  HP661X_OUTP_CC		2   Power source is in Constant Current mode.
 *                                      Either the CC+ or CC- bit is set.
 *            HP661X_OUTP_UNREG		3   Power source is unregulated.
 *			  HP661X_OUTP_PROT		4   Protection has tripped (OV, OCP, OT, or RI bit is set).
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_readOutputStatus (ViSession instrumentHandle,
                                            ViPInt16 outputStatus)

{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus = 0;
	ViInt16		i;

	GET_GLOBALS(instrumentHandle, thisPtr);

	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	errStatus = viQueryf(instrumentHandle, "OUTP?\n", "%hd%*t",	&i);
	if (errStatus < VI_SUCCESS)
    {
		return statusUpdate(instrumentHandle, thisPtr, errStatus );
    }

	/* done if output is Off */
    if (i==HP661X_OUTP_STATUS_OFF)
	{
		*outputStatus=i;
	    return statusUpdate(instrumentHandle, thisPtr, VI_SUCCESS );
	}

	errStatus = viQueryf(instrumentHandle, "STAT:QUES:COND?\n", "%hd%*t",	&i);
	if (errStatus < VI_SUCCESS)
    {
		return statusUpdate(instrumentHandle, thisPtr, errStatus );
    }

	/* check protection conditions */
	if ( i & (HP661X_QUES_OV | HP661X_QUES_OCP | HP661X_QUES_OT | HP661X_QUES_RI))
	{
		*outputStatus=HP661X_OUTP_STATUS_PROT;
	    return statusUpdate(instrumentHandle, thisPtr, VI_SUCCESS );
	}

	/* check unregulated conditions */
	if ( i & HP661X_QUES_UNREG)
	{
		*outputStatus=HP661X_OUTP_STATUS_UNREG;
	    return statusUpdate(instrumentHandle, thisPtr, VI_SUCCESS );
	}

	errStatus = viQueryf(instrumentHandle, "STAT:OPER:COND?\n", "%hd%*t",	&i);
	if (errStatus < VI_SUCCESS)
    {
		return statusUpdate(instrumentHandle, thisPtr, errStatus );
    }

	/* check CC+, CC- conditions */
	if ( i & (HP661X_OPER_CC_POS | HP661X_OPER_CC_NEG))
	{
		*outputStatus=HP661X_OUTP_STATUS_CC;
	    return statusUpdate(instrumentHandle, thisPtr, VI_SUCCESS );
	}

	/* check unregulated conditions */
	if ( i & HP661X_OPER_CV)
	{
		*outputStatus=HP661X_OUTP_STATUS_CV;
	    return statusUpdate(instrumentHandle, thisPtr, VI_SUCCESS );
	}

    return statusUpdate(instrumentHandle, thisPtr, errStatus );  /* unknown status ??? */
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_rippleRiDfi
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_rippleRiDfi (ViSession instrumentHandle)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus = 0;
	ViUInt16	i;

	GET_GLOBALS(instrumentHandle, thisPtr);

	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	/* get Questionable Enable register */
	errStatus = viQueryf(instrumentHandle, "STAT:QUES:ENAB?\n", "%hd%*t",	&i);
	if (errStatus < VI_SUCCESS)
    {
		return statusUpdate(instrumentHandle, thisPtr, errStatus );
    }

	/* mask in RI condition */
	i |= HP661X_QUES_RI;
	errStatus = viPrintf(instrumentHandle, "STAT:QUES:ENAB %hd\n", i);
	if (errStatus < VI_SUCCESS)
    {
		return statusUpdate(instrumentHandle, thisPtr, errStatus );
    }

	/* configure RI/DFI port */

	/* configure digital port for RI/DFI function */
	errStatus = viPrintf(instrumentHandle, "DIG:FUNC RIDF\n");
    if (errStatus<VI_SUCCESS)
	{
	    return statusUpdate(instrumentHandle, thisPtr, errStatus );
	}

	/* set RI mode */
	errStatus = viPrintf(instrumentHandle, "OUTP:RI:MODE LATC\n");
    if (errStatus<VI_SUCCESS)
	{
	    return statusUpdate(instrumentHandle, thisPtr, errStatus );
	}

	/* set DFI state and source */
	errStatus = viPrintf(instrumentHandle, "OUTP:DFI:SOUR QUES;STAT 1\n");

    return statusUpdate(instrumentHandle, thisPtr, errStatus );  /* unknown status ??? */
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_setCurr
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function sets the immediate or trigggered current level
 *            of the power source.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViInt16 currParam
 * IN        
 *            The current parameter select.
 * 
 *      HP661X_CURR        0  the immediate current level
 *      HP661X_CURR_TRIG   1  the triggered current level
 * 
 * PARAM 3 : 64 curr
 * IN        
 *            The current in amps.
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_setCurr (ViSession instrumentHandle,
								   ViInt16 currParam,
								   ViReal64 currLev)
{
    struct hp661x_globals *thisPtr;
    ViStatus errStatus = 0;
    ViChar szString[HP661X_CMDSTRING_MAX];

    GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

    CHK_INT_RANGE(currParam,
		HP661X_CURR_IMM,
		HP661X_CURR_TRIG,
		VI_ERROR_PARAMETER2);


    if (currParam==HP661X_CURR_IMM)
	{
		CHK_REAL_RANGE(currLev,
			HP661X_CURR_MIN,
			thisPtr->currMax,
			VI_ERROR_PARAMETER3);
		sprintf(szString, "CURR %.5lG", currLev);
	}
	else
	{
		CHK_REAL_RANGE(currLev,
			HP661X_CURR_TRIG_MIN,
			thisPtr->currMax,
			VI_ERROR_PARAMETER3);
		sprintf(szString, "CURR:TRIG %.5lG", currLev);
	}
    
    errStatus = viPrintf(instrumentHandle, "%s\n", szString);
    
    return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_getCurr
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function queries the instrument for the triggered current
 *           setting.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViPReal64 currQParam
 * IN       
 *            Select the current parameter to query.
 * 
 * PARAM 3 : ViPReal64 curr
 * OUT       
 *            The current parameter setting in amps.
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_getCurr (ViSession instrumentHandle,
								   ViInt16 currQParam,
								   ViPReal64 currLev)
{
    struct hp661x_globals *thisPtr;
	ViStatus errStatus = 0;
    ViChar szString[HP661X_CMDSTRING_MAX];

    GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	switch (currQParam)
	{
	case HP661X_CURR_Q:
		sprintf(szString, "CURR?\n");
		break;
	case HP661X_CURR_Q_TRIG:
		sprintf(szString, "CURR:TRIG?\n");
		break;
	case HP661X_CURR_Q_MAX:
		sprintf(szString, "CURR? MAX\n");
		break;
	case HP661X_CURR_Q_MIN:
		sprintf(szString, "CURR? MIN\n");
		break;
	default:
		return statusUpdate(instrumentHandle, thisPtr, VI_ERROR_PARAMETER2 );
		break;
	}

    errStatus=viQueryf(instrumentHandle, szString, "%lf", currLev);
	
    return statusUpdate(instrumentHandle, thisPtr, VI_SUCCESS );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_setDisplay
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function enables or disables the display.  Disabling
 *            the display makes remote commands complete faster.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViBoolean dispState
 * IN        
 *            VI_TRUE: enable the display.
 *            VI_FALSE: disable the display.
 * 
 * PARAM 3 : ViInt16 dispMode
 * IN        
 *            NORMAL 0 display normal instrument function.
 *            TEXT   1 display user message.
 * 
 * PARAM 4 : ViString messageText
 * IN        
 *            a user message less than 22 characters long.  If a null string is
 *            passed, the message text command is not set.  To clear the current
 *            message text, at least one blank character must be sent.
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_setDisplay (ViSession instrumentHandle,
									  ViBoolean dispState,
									  ViInt16	dispMode,
									  ViChar	_VI_FAR messageText[])
{
    struct		hp661x_globals *thisPtr;
    ViStatus	errStatus = 0;
    ViChar		szString[HP661X_CMDSTRING_MAX];
    ViChar		szFmt[HP661X_CMDSTRING_MAX];
	ViInt16		i;

    GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

    CHK_BOOLEAN(dispState, VI_ERROR_PARAMETER2);
    CHK_INT_RANGE(dispMode,
		HP661X_DISP_NORMAL,
		HP661X_DISP_TEXT,
		VI_ERROR_PARAMETER3);

	if (dispState==VI_FALSE)
		sprintf(szString, "DISP 0");
	else
	{
		if (dispMode==HP661X_DISP_NORMAL)
			sprintf(szString, "DISP:STAT 1;MODE NORM");
		else
		{
			i=strlen(messageText);
			if (i)
			{
				/* send the message text only if the string is not null */
				if (i>14) i=14;
				sprintf(szFmt, "%s%ds\"", "%s\"%.", i);
				sprintf(szString, szFmt, "DISP:STAT 1;MODE TEXT;TEXT ", messageText);
			}
			else
				sprintf(szString, "DISP:STAT 1;MODE TEXT");
		}
	}

	errStatus = viPrintf(instrumentHandle, "%s\n", szString);
    
    return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_getDisplay
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function enables or disables the display.  Disabling
 *            the display makes remote commands complete faster.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViBoolean dispState
 * IN        
 *            VI_TRUE: enable the display.
 *            VI_FALSE: disable the display.
 * 
 * PARAM 3 : ViInt16 dispMode
 * IN        
 *            NORMAL 0 display normal instrument function.
 *            TEXT   1 display user message.
 * 
 * PARAM 4 : ViString messageText
 * IN        
 *            a user message less than 22 characters long.  If a null string is
 *            passed, the message text command is not set.  To clear the current
 *            message text, at least one blank character must be sent.
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_getDisplay (ViSession		instrumentHandle,
                                      ViPBoolean	dispState,
									  ViPInt16		dispMode,
                                      ViChar		_VI_FAR messageText[])
{
    struct		hp661x_globals *thisPtr;
    ViStatus	errStatus = 0;
    ViChar		szString[HP661X_CMDSTRING_MAX];
	ViInt16		i;

    GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	messageText[0]=0;
	errStatus = viQueryf(instrumentHandle, "DISP:STAT?;MODE?\n","%hd;%s", dispState, szString);

	if (strcmp(szString, "NORM")==0)
		*dispMode=HP661X_DISP_NORMAL;
	else
		*dispMode=HP661X_DISP_TEXT;

	errStatus = viQueryf(instrumentHandle, "DISP:TEXT?\n","%t", szString);
	if (errStatus != VI_SUCCESS)
	{
		return statusUpdate(instrumentHandle, thisPtr, errStatus );
	}

	i=strlen(szString);
	if (i>3)
	{
		i -= 3;
		strncpy(messageText, &szString[1], i);
		messageText[i]=0;
	}
	return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_arm
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function initializes a trigger sequence of a subsystem.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViInt16 sequence
 * IN        
 * 
 *      Macro Name                      Value  Description
 *      -----------------------------------------------------------
 *      HP661X_ARM_OUTPUT_ONCE            0  
 *      HP661X_ARM_OUTPUT_CONTINUOUS      1  
 *      HP661X_ARM_OUTPUT_CONTINUOUS_OFF  2  
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_arm (ViSession instrumentHandle, ViInt16 trigSystem)
{
    struct hp661x_globals *thisPtr;
    ViStatus errStatus = 0;
    ViChar szString[HP661X_CMDSTRING_MAX];

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

    switch (trigSystem)
	{
    case HP661X_ARM_OUTPUT_ONCE:
    	sprintf(szString, "INIT:NAME TRAN");
		break;
    case HP661X_ARM_OUTPUT_CONTINUOUS:
    	sprintf(szString, "INIT:CONT:SEQ1 1");
		break;
    case HP661X_ARM_OUTPUT_CONTINUOUS_OFF:
    	sprintf(szString, "INIT:CONT:SEQ1 0");
		break;
    default:
           return statusUpdate(instrumentHandle, thisPtr, VI_ERROR_PARAMETER2 );
    break;
    }
    
	errStatus = viPrintf(instrumentHandle, "%s\n", szString);
    
    return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus measure
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function preforms scalar measurements or fetches.
 *
 *-----------------------------------------------------------------------------
 */
static ViStatus _VI_FUNC measure(ViSession	instrumentHandle,
								 struct		hp661x_globals *thisPtr,
								 ViPString	szMeasCmd,
								 ViPReal64	resp)
{
	ViStatus	errStatus = 0;
	ViInt32		timeOut;

	/* swap bus timeout with measurement timeout */
	errStatus = viGetAttribute(instrumentHandle, VI_ATTR_TMO_VALUE, &timeOut);
	if (errStatus < VI_SUCCESS)
		return errStatus;
	errStatus = viSetAttribute(instrumentHandle, VI_ATTR_TMO_VALUE, thisPtr->measTimeout);
	if (errStatus < VI_SUCCESS)
		return errStatus;

    errStatus=viQueryf(instrumentHandle, szMeasCmd, "%lf", resp);

	/* restore bus timeout value */
	viSetAttribute(instrumentHandle, VI_ATTR_TMO_VALUE, timeOut);

	/* if bus timeout, abort measurement and set measurement timeout */
	if (errStatus == VI_ERROR_TMO)
	{
		viClear(instrumentHandle);
		viFlush(instrumentHandle, VI_READ_BUF_DISCARD);
		errStatus = HP661X_INSTR_ERROR_MEAS_TMO;
	}

    return errStatus;
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_measureVolt
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function queries the instrument for a voltage measurement.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViPReal64 measResult
 * OUT       
 *            Result of the measurement.
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_measureVolt(ViSession instrumentHandle,
									  ViPReal64 measVolt)
{
	ViStatus	errStatus = 0;
	struct		hp661x_globals *thisPtr;

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

    errStatus = measure(instrumentHandle, thisPtr, "MEAS:VOLT?\n", measVolt);

    return statusUpdate(instrumentHandle, thisPtr, errStatus);
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_measureCurr
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function queries the instrument for a current measurement.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 *
 * PARAM 2 : ViPReal64 measResult
 * OUT       
 *            Result of the measurement.
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_measureCurr(ViSession instrumentHandle,
									  ViPReal64 measCurr)
{
	ViStatus errStatus = 0;
	struct hp661x_globals *thisPtr;

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	errStatus = measure(instrumentHandle, thisPtr, "MEAS:CURR?\n", measCurr);

    return statusUpdate(instrumentHandle, thisPtr, errStatus);
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_getMeasTimeout
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  Set timeout value for all measurement functions.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViInt32 measTimeout
 * INT        
 *            The measurement timeout value.
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_setMeasTimeout (ViSession instrumentHandle,
                                          ViInt32 measTimeout)
{
    struct hp661x_globals *thisPtr;
    ViStatus errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	CHK_LONG_RANGE (measTimeout,
		HP661X_TIMEOUT_MIN,
		HP661X_TIMEOUT_MAX,
		VI_ERROR_PARAMETER2);
   
	thisPtr->measTimeout = measTimeout;

	return VI_SUCCESS;
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_getMeasTimeout
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  Query the measurement timeout setting.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViInt32 measTimeout
 * OUT        
 *            The measurement timeout value.
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_getMeasTimeout (ViSession instrumentHandle,
                                          ViPInt32 measTimeout)
{
    struct hp661x_globals *thisPtr;
    ViStatus errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );
   
	*measTimeout = thisPtr->measTimeout;

	return VI_SUCCESS;
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_setSweepParams
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function sets the measurement sweep parameters -
 *			  interval, size, and delay offset.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViReal64 sweepInterval
 * IN        
 *            The time interval for measurement sweeps.
 * 
 *      MAX = HP661X_INTERVAL_MAX   0.0003900
 *      MIN = HP661X_INTERVAL_MIN   0.0000156
 *
 *		Note: the maximum sampling interval is FW version dependent.
 * 
 * PARAM 3 : ViInt32 sweepSize
 * IN        
 *            The number of points in an acquisition.
 * 
 *      MAX = 4096
 *      MIN = 1
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_setSweepParams (ViSession instrumentHandle,
										 ViReal64 sampleInterval,
										 ViInt32 sweepSize)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus = 0;
    ViChar		szString[HP661X_CMDSTRING_MAX];
    
	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

    CHK_REAL_RANGE(sampleInterval,
		HP661X_SWEEP_INT_MIN,
		thisPtr->tintMax,
		VI_ERROR_PARAMETER2);

	CHK_LONG_RANGE(sweepSize,
		HP661X_SWEEP_SIZE_MIN,
		HP661X_SWEEP_SIZE_MAX,
		VI_ERROR_PARAMETER3);

    sprintf(szString, "SENS:SWE:TINT %.5lG;POIN %d", sampleInterval, sweepSize);

	errStatus = viPrintf(instrumentHandle, "%s\n", szString);

	return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_getSweepParams
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function queries the measurement sweep parameters -
 *			  interval, and size.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViReal64 sweepInterval
 * OUT        
 *            The time interval between measurement.
 * 
 * PARAM 3 : ViInt32 sweepSize
 * OUT        
 *            The number of points in an acquisition.
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_getSweepParams (ViSession	instrumentHandle,
										 ViPReal64	sampleInterval,
										 ViPInt32	sweepSize)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus = 0;
    
	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	errStatus = viQueryf(instrumentHandle, "SENS:SWE:TINT?;POIN?\n",
		"%lf;%ld", sampleInterval, sweepSize);

	return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_clearOutpProt
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function resets overvoltage and overcurrent protection
 *           after one has been tripped.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_clearOutpProt(ViSession instrumentHandle)
{
	struct hp661x_globals *thisPtr;
	ViStatus errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	errStatus = viPrintf(instrumentHandle, "OUTP:PROT:CLE\n");

    return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_setOutpRelay
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function opens or closes the output relay contacts.
 *			  It is only valid for units with Option 760, otherwise an
 *			  error will occur. The relay is controlled independently
 *			  of the output state.  If the  instrument is supplying power
 *			  to a load, that power will appear at the relay contacts
 *			  during switching.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViBoolean relayState
 * IN        
 *			  VI_TRUE: closes relay contacts
 *			  VI_FALSE: opens relay contacts
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_setOutpRelay (ViSession instrumentHandle,
										ViBoolean relayState)
{
	struct hp661x_globals *thisPtr;
	ViStatus errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	if (relayState)
		errStatus = viPrintf(instrumentHandle, "OUTP:REL 1\n");
	else
		errStatus = viPrintf(instrumentHandle, "OUTP:REL 0\n");

    return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_getOutpRelay
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function queries the state of the output relay contacts.
 *			  It is only valid for units with Option 760, otherwise an error
 *			  will occur. The relay state is independently of the output state.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViPBoolean relayState
 * OUT        
 *			  VI_TRUE: closes relay contacts
 *			  VI_FALSE: opens relay contacts
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_getOutpRelay (ViSession instrumentHandle,
										ViPBoolean relayState)
{
	struct hp661x_globals *thisPtr;
	ViStatus errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	errStatus = viQueryf(instrumentHandle, "OUTP:REL?\n", "%ld%*t", relayState);

    return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_setOutpRelayPolarity
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViBoolean relayState
 * IN        
 *			  HP661X_RELAY_POLARITY_REVERSE: relay contacts reversed
 *			  HP661X_RELAY_POLARITY_NORMAL: relay contacts normal
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_setOutpRelayPolarity (ViSession instrumentHandle,
												ViInt16 relayPolarity)
{
	struct hp661x_globals *thisPtr;
	ViStatus errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	CHK_INT_RANGE(relayPolarity,
		HP661X_RELAY_POLARITY_NORMAL,
		HP661X_RELAY_POLARITY_REVERSE,
		VI_ERROR_PARAMETER2);

	if (relayPolarity==HP661X_RELAY_POLARITY_NORMAL)
		errStatus = viPrintf(instrumentHandle, "OUTP:REL:POL NORM\n");
	else
		errStatus = viPrintf(instrumentHandle, "OUTP:REL:POL REV\n");

    return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_getOutpRelayPolarity
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function queries the state of the output relay contacts.
 *			  It is only valid for units with Option 760, otherwise an error
 *			  will occur. The relay state is independently of the output state.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViPBoolean relayPolarity
 * OUT        
 *			  HP661X_RELAY_POLARITY_REVERSE: relay contacts reversed
 *			  HP661X_RELAY_POLARITY_NORMAL: relay contacts normal
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_getOutpRelayPolarity (ViSession instrumentHandle,
												ViPInt16 relayPolarity)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus = 0;
	ViChar		szString[HP661X_CMDSTRING_MAX];
	ViInt16		iTemp=sizeof(szString);

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	errStatus = viQueryf(instrumentHandle, "OUTP:REL:POL?\n", "%#t", &iTemp, &szString);

	if (errStatus==VI_SUCCESS)
	{
		if (strcmp(szString, "NORM")==0)
			*relayPolarity=HP661X_RELAY_POLARITY_NORMAL;
		else 
			*relayPolarity=HP661X_RELAY_POLARITY_REVERSE;
	}
    return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_setRiDfi
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE : This function configures the digital control port for Remote
 *           Inhibit/Discrete Fault Interrupt (RI/DFI) operation and sets
 *           the RI/DFI parameters to the specified conditions.
 *           The parameters are saved in non-volatile memory.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViInt16 riMode
 * IN        
 *			 This parameter selects the operation mode of the
 *           Remote Inhibit protection feature.
 * 
 *			 Macro Name           Description
 *			 --------------------------------
 *			 HP661X_RI_OFF		    Off
 *			 HP661X_RI_LATCHING	    Latching
 *			 HP661X_RI_LIVE			Live
 * 
 * PARAM 3 : ViBoolean dfiState
 * IN        
 *			  Specify if the discrete fault indicator signal is enabled
 *            at the digital control port.
 *
 *			  VI_TRUE: DFI is enabled.
 *			  VI_FALSE: DFI is disabled.
 * 
 * PARAM 4 : ViInt16 dfiSourceBit
 * IN
 *
 *			  Specify the source of the DFI signal.
 *
 *			  The internal sources that drive the discrete fault
 *			  indicator signal are:
 *
 *			  MACRO						 Description
 *			  ---------------------------------------------------------
 *			  HP661X_DFI_SRC_OFF		 None, DFI off 
 *			  HP661X_DFI_SRC_QUES		 Questionable event summary bit
 *			  HP661X_DFI_SRC_OPER		 Operation Event summary bit
 *			  HP661X_DFI_SRC_ESB		 Standard Event summary bit
 *			  HP661X_DFI_SRC_RQS		 Request for Service bit
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_setRiDfi (ViSession instrumentHandle,
									ViInt16 riMode,
                                    ViBoolean dfiState,
									ViInt16 dfiSourceBit)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus = 0;
	ViInt16		i;
	ViPString	pszRiMode, pszDfiSourceBit;
	ViChar		szString[HP661X_CMDSTRING_MAX];

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	switch (riMode)
	{
	case HP661X_RI_OFF:
		pszRiMode=hp661x_ri_mode_table[HP661X_RI_OFF];
		break;
	case HP661X_RI_LATCHING:
		pszRiMode=hp661x_ri_mode_table[HP661X_RI_LATCHING];
		break;
	case HP661X_RI_LIVE:
		pszRiMode=hp661x_ri_mode_table[HP661X_RI_LIVE];
		break;
	default:
		return statusUpdate(instrumentHandle, thisPtr, VI_ERROR_PARAMETER2);
		break;
	}

	CHK_BOOLEAN(dfiState, VI_ERROR_PARAMETER3);
	i = dfiState==VI_TRUE? 1:0;

	switch (dfiSourceBit)
	{
	case HP661X_DFI_SRC_PREV:
		break;
	case HP661X_DFI_SRC_QUES:
		pszDfiSourceBit=hp661x_dfi_src_table[HP661X_DFI_SRC_QUES];
		break;
	case HP661X_DFI_SRC_OPER:
		pszDfiSourceBit=hp661x_dfi_src_table[HP661X_DFI_SRC_OPER];
		break;
	case HP661X_DFI_SRC_ESB:
		pszDfiSourceBit=hp661x_dfi_src_table[HP661X_DFI_SRC_ESB];
		break;
	case HP661X_DFI_SRC_RQS:
		pszDfiSourceBit=hp661x_dfi_src_table[HP661X_DFI_SRC_RQS];
		break;
	case HP661X_DFI_SRC_OFF:
		pszDfiSourceBit=hp661x_dfi_src_table[HP661X_DFI_SRC_OFF];
		break;
	default:
		return statusUpdate(instrumentHandle, thisPtr, VI_ERROR_PARAMETER4);
		break;
	}

	/* configure digital port for RI/DFI function */
	errStatus = viPrintf(instrumentHandle, "DIG:FUNC RIDF\n");
    if (errStatus<VI_SUCCESS)
	{
	    return statusUpdate(instrumentHandle, thisPtr, errStatus );
	}

	/* set RI mode */
	sprintf(szString, "OUTP:RI:MODE %s\n", pszRiMode);
    errStatus = viPrintf(instrumentHandle, szString);
    if (errStatus<VI_SUCCESS)
	{
	    return statusUpdate(instrumentHandle, thisPtr, errStatus );
	}

	/* set DFI state and source */
	sprintf(szString, "OUTP:DFI:STAT %d", i);
	if (dfiState==VI_TRUE)
	{
		if (!(dfiSourceBit==HP661X_DFI_SRC_PREV))
			sprintf(szString, "OUTP:DFI:SOUR %s;STAT %d", pszDfiSourceBit, i);
	}

    errStatus = viPrintf(instrumentHandle,"%s\n", szString);

    return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_getRiDfi
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE : This function queries the RI/DFI settings of the power source.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViPInt16 ponState
 * OUT        
 *			 Remote inhibit operating mode.
 * 
 *			 Macro Name           Description
 *			 --------------------------------
 *			 HP661X_RI_OFF		    Off
 *			 HP661X_RI_LATCHING	    Latching
 *			 HP661X_RI_LIVE			Live
 * 
 * PARAM 3 : ViPBoolean dfiState
 * OUT       
 *			  Discrete fault indicator operation state.
 *
 *			  VI_TRUE: DFI is enabled.
 *			  VI_FALSE: DFI is disabled.
 * 
 * PARAM 4 : ViPInt16 dfiSourceBit
 * OUT
 *
 *			  Source of discrete faults.
 *
 *			  MACRO						Description
 *			  --------------------------------------------------------
 *			  HP661X_DFI_SRC_OFF		None, DFI off 
 *			  HP661X_DFI_SRC_QUES		Questionable event summary bit
 *			  HP661X_DFI_SRC_OPER		Operation Event summary bit
 *			  HP661X_DFI_SRC_ESB		Standard Event summary bit
 *			  HP661X_DFI_SRC_RQS		Request for Service bit
 *
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_getRiDfi (ViSession instrumentHandle,
									ViPInt16 riMode,
                                    ViPBoolean dfiState,
									ViPInt16 dfiSourceBit)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus = 0;
	ViInt16		i;
	ViChar		szString[HP661X_CMDSTRING_MAX];

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	i=sizeof(szString);
	errStatus = viQueryf(instrumentHandle, "DIG:FUNC?\n", "%#s", &i, szString);
	if (errStatus<VI_SUCCESS)
	{
		return statusUpdate(instrumentHandle, thisPtr, errStatus );
	}

	if (strcmp(szString, "RIDF"))
	{
		return statusUpdate(instrumentHandle, thisPtr, HP661X_INSTR_ERROR_PORT_CONFIG ); /* wrong mode */
	}

	i=sizeof(szString);
	errStatus=viQueryf(instrumentHandle, "OUTP:RI:MODE?\n", "%#s", &i, szString);
    if (errStatus<VI_SUCCESS)
	{
	    return statusUpdate(instrumentHandle, thisPtr, errStatus );
	}

	errStatus = findIndex(thisPtr, hp661x_ri_mode_table, szString, &i);
    if (errStatus<VI_SUCCESS)
	{
	    return statusUpdate(instrumentHandle, thisPtr, errStatus );
	}
	*riMode=i;

	if ((errStatus = viQueryf(instrumentHandle, "OUTP:DFI?\n","%hd",	dfiState)) < VI_SUCCESS)
		return statusUpdate(instrumentHandle, thisPtr, errStatus);

	i=sizeof(szString);
	errStatus=viQueryf(instrumentHandle, "OUTP:DFI:SOUR?\n", "%#s", &i, szString);
    if (errStatus<VI_SUCCESS)
	{
	    return statusUpdate(instrumentHandle, thisPtr, errStatus );
	}

	errStatus = findIndex(thisPtr, hp661x_dfi_src_table, szString, &i);
    if (errStatus==VI_SUCCESS)
	{
	    *dfiSourceBit=i;
	}
    
	return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

 /*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_setPonState
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE : This function selects the power-on state of the instrument.
 *			 This information is saved in non-volatile memory.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViInt16 ponState
 * IN        
 *			 This parameter selects the power-on state of the power source.
 * 
 *			 Macro Name               Description
 *			 ------------------------------------
 *			 HP661X_PON_STATE_RST     *RST  state
 *			 HP661X_PON_STATE_RCL0    *RCL0 State
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_setPonState (ViSession instrumentHandle,
									   ViInt16 ponState)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	CHK_INT_RANGE(ponState,
		HP661X_PON_STATE_RST,
		HP661X_PON_STATE_RCL0,
		VI_ERROR_PARAMETER2);

	if (ponState==HP661X_PON_STATE_RST)
		errStatus=viPrintf(instrumentHandle, "OUTP:PON:STAT RST\n");
	else
		errStatus=viPrintf(instrumentHandle, "OUTP:PON:STAT RCL0\n");

    return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_getPonState
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE : This function queries the power-on state setting of the instrument.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViPInt16 ponState
 * OUT       
 *			 The power-on state of the power source.
 * 
 *			 Macro Name               Description
 *			 ------------------------------------
 *			 HP661X_PON_STATE_RST     *RST  state
 *			 HP661X_PON_STATE_RCL0    *RCL0 State
 * 
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_getPonState (ViSession instrumentHandle,
									   ViPInt16 ponState)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus = 0;
	ViChar		szString[HP661X_CMDSTRING_MAX];

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	errStatus=viQueryf(instrumentHandle, "OUTP:PON:STAT?\n", "%s", &szString);
	if (errStatus<VI_SUCCESS)
	{
		return statusUpdate(instrumentHandle, thisPtr, errStatus );
	}

	if (strcmp(szString, "RST")==0)
		*ponState = HP661X_PON_STATE_RST;
	else
		*ponState = HP661X_PON_STATE_RCL0;
	
    return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_setOcpParams
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This command sets turns overcurrent protection on or off and
 *			  and optionally sets the protection delay between the programming
 *			  of an output change that produces a Constant Current
 *            condition and the recording of that condition by the Operation
 *            Status Condition register. The delay prevents the momentary
 *            changes in dc source status that can occur during reprogramming
 *            from being registered as events by the status subsystem. Since
 *            the delay applies to Constant Current status, it also delays the
 *            OverCurrent Protection feature. The OverVoltage Protection
 *            feature is not affected by this delay.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViBoolean ocpEnable
 * IN
 *			  VI_TRUE: enables the over-current protection feature
 *			  VI_FALSE: disables the over-current protection feature,
 *						the ccDelay parameter is ignored
 *
 * PARAM 3 : ViReal64 ccDelay
 * IN        
 *            The delay in seconds.
 * 
 *      MAX = HP661X_DLY_MAX   2147483.647
 *      MIN = HP661X_DLY_MIN   0.0
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_setOcpParams (ViSession instrumentHandle,
										ViBoolean ocpEnable,
										ViReal64 ccDelay)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus = 0;

    GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	if (ocpEnable)
	{
    CHK_REAL_RANGE(ccDelay,
		HP661X_OCP_DELAY_MIN,
		HP661X_OCP_DELAY_MAX,
		VI_ERROR_PARAMETER2);
	errStatus = viPrintf(instrumentHandle, "%s%.51G\n", "CURR:PROT:STAT 1;:OUTP:PROT:DEL ", ccDelay);
	}
	else
		errStatus = viPrintf(instrumentHandle, "CURR:PROT:STAT 0\n");

	return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_getOcpParam
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This command queries the time between the programming of an
 *           output change that produces a Constant Voltage or Constant
 *           Current condition and the recording of that condition by the
 *           Operation Status Condition register.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViPReal64 delay
 * OUT       
 *            The delay in seconds.
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_getOcpParams (ViSession instrumentHandle,
										ViPBoolean ocpEnable,
										ViPReal64 ccDelay)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr)

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	if ((errStatus = viQueryf(instrumentHandle,
			"CURR:PROT:STAT?\n",
			"%ld%*t",
			ocpEnable)) < VI_SUCCESS)
		return statusUpdate(instrumentHandle, thisPtr, errStatus);

	errStatus = viQueryf(instrumentHandle,
			"OUTP:PROT:DEL?\n",
			"%lf%*t",
			ccDelay);

	return statusUpdate(instrumentHandle, thisPtr, errStatus );

}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_setDigio
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function configures the power supply digital control port to
 *			  function as a digital I/O port and outputs the digOutData parameter
 *            to the port.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViBoolean digOutData
 * IN        
 *			  I/O port value. Only the least 3 significant bits are used.
 *			  Therefore the output value can range from 0 to 7.
 *
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_setDigio (ViSession instrumentHandle,
									ViInt16 digIoData)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr)

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	CHK_INT_RANGE(digIoData,
		HP661X_DIG_OUT_MIN,
		HP661X_DIG_OUT_MAX,
		VI_ERROR_PARAMETER2);

	errStatus = viPrintf(instrumentHandle, "DIG:FUNC DIG;DATA %d\n", digIoData);

	return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_getDigio
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function reads the digital control port when the port is
 *			  configured for Digital I/O operation. Configuring of the port
 *			  is done with the hp661x_setDigio function.  The query returns
 *			  the last programmed value in bits 0 and 1 and the value read
 *			  at pin 3 in bit 2.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViBoolean digInData
 * OUT        
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_getDigio (ViSession instrumentHandle,
									ViPInt16 digIoData)

{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus = 0;
	ViInt16		i;
	ViChar		szString[HP661X_CMDSTRING_MAX];


	GET_GLOBALS(instrumentHandle, thisPtr)

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	i=sizeof(szString);
	errStatus = viQueryf(instrumentHandle, "DIG:FUNC?\n", "%#s", &i, &szString);
	if (errStatus<VI_SUCCESS)
	{
		return statusUpdate(instrumentHandle, thisPtr, errStatus );
	}

	if (strcmp(szString, "DIG"))
	{
		return statusUpdate(instrumentHandle, thisPtr, HP661X_INSTR_ERROR_PORT_CONFIG );
	}

	errStatus = viQueryf(instrumentHandle, "DIG:DATA?\n", "%hd",	digIoData);

	return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_getVoltAlcBandwidth
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function queries the setting of the output mode switch.
 *
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViBoolean alcBandwidth
 * OUT        
 *            Output mode switch setting.  The returned value is 15,000 if the
 *			  switch is set to Normal and 60,000 if the switch is set to Fast
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_getVoltAlcBandwidth (ViSession instrumentHandle,
											   ViPInt32 alcBandwidth)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	errStatus=viQueryf(instrumentHandle, "VOLT:ALC:BAND?\n", "%ld", alcBandwidth);

    return statusUpdate(instrumentHandle, thisPtr, errStatus );
}
/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_setOutpState
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function enables or disables the power source output.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViBoolean outputState
 * IN        
 *            VI_TRUE: enable output.
 *            VI_FALSE: disable output.
 * 
 * PARAM 3 : ViBoolean switchRelay
 * IN        
 *            VI_TRUE: switch Option 760 output relay.
 *            VI_FALSE: do not switch Option 760 output relay.
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_setOutpState (ViSession instrumentHandle,
										ViBoolean outputState,
										ViBoolean switchRelay)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus = 0;
	ViChar		szString[HP661X_CMDSTRING_MAX];

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

    CHK_BOOLEAN(outputState, VI_ERROR_PARAMETER2);
    CHK_BOOLEAN(switchRelay, VI_ERROR_PARAMETER3);

	if (outputState)
		strcpy(szString, "OUTP 1");
	else
		strcpy(szString, "OUTP 0");

	if (!switchRelay)
		strcat(szString, ",NOR");

	errStatus = viPrintf(instrumentHandle, "%s\n", szString);
    
    return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_getOutpState
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  Queries the present output state of the power source output.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViPInt16 outputState
 * OUT        
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_getOutpState (ViSession instrumentHandle,
										ViPBoolean outputState)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	errStatus = viQueryf(instrumentHandle,
			"OUTP?\n",
			"%hd%*t",
			outputState);

	return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_setCurrSenseParams
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function sets the current sensing range and detector parameters.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViInt16 currSenseRange
 * IN        
 * 
 *			  The dc current measurement range.
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_setCurrSenseRange (ViSession instrumentHandle,
											ViReal64 currSenseRange)
{
    struct		hp661x_globals *thisPtr;
    ViStatus	errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

 	CHK_REAL_RANGE(currSenseRange,
		HP661X_CURR_MIN,
		thisPtr->currMax,
		VI_ERROR_PARAMETER2);

	errStatus = viPrintf(instrumentHandle, "SENS:CURR:RANG %.5lG\n",  currSenseRange);

    return statusUpdate(instrumentHandle, thisPtr, VI_SUCCESS );
}


/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_getCurrSenseParams
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function queries the current sensing range and detector
 *			  settings.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViPInt16 currSenseRange
 * OUT        
 * 
 *			  The dc current measurement range.
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_getCurrSenseRange (ViSession instrumentHandle,
											ViPReal64 currSenseRange)
{
    struct		hp661x_globals *thisPtr;
    ViStatus	errStatus = 0;
    ViChar		szString[HP661X_CMDSTRING_MAX];
	ViInt16		i;

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	i=sizeof(szString);
	szString[0]=0;
	errStatus = viQueryf(instrumentHandle, "SENS:CURR:RANG?\n",  "%lf", currSenseRange);

	return statusUpdate(instrumentHandle, thisPtr, errStatus);
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_abort
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  Causes the instrument to cancel any trigger actions in
 *           progress.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_abort(ViSession instrumentHandle)
{
    struct		hp661x_globals *thisPtr;
    ViStatus	errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

    errStatus = viPrintf(instrumentHandle,"ABOR\n");
	
	return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_trigger
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  Causes the instrument to trigger the output transient system.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_trigger (ViSession instrumentHandle)
{
    struct hp661x_globals *thisPtr;
    ViStatus errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );
   
	errStatus = viPrintf(instrumentHandle, "TRIG\n");
    
    return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_trg
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE : This function sends an IEEE Group Execute Trigger
 *			  (GET or *TRG) to the instrument. 
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_trg (ViSession instrumentHandle)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus = 0;

	GET_GLOBALS(instrumentHandle, thisPtr);

	DEBUG_CHK_THIS( instrumentHandle, thisPtr );
 
	errStatus = viPrintf(instrumentHandle,"*TRG\n");

	return statusUpdate(instrumentHandle, thisPtr, errStatus);
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_setVolt
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function sets the immediate, trigggered, or protection
 *            DC voltage level of the power source.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViInt16 voltParam
 * IN        
 *            The current parameter select.
 * 
 *      HP661X_VOLT        0  the immediate voltage level
 *      HP661X_VOLT_TRIG   1  the triggered voltage level
 *      HP661X_VOLT_PROT   2  the over-voltage protection level
 * 
 * PARAM 3 : ViReal64 voltLev
 * IN        
 *            The current in amps.
 * 
 *      MAX = HP661X_CURR_MAX   2.0475
 *      MIN = HP661X_CURR_MIN   0.0
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_setVolt (ViSession instrumentHandle,
								   ViInt16 voltParam,
								   ViReal64 voltLev)
{
    struct hp661x_globals *thisPtr;
    ViStatus errStatus = 0;
    ViChar szString[HP661X_CMDSTRING_MAX];

    GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	switch (voltParam)
	{
	case HP661X_VOLT_IMM:
		CHK_REAL_RANGE(voltLev,
			HP661X_VOLT_MIN,
			thisPtr->voltMax,
			VI_ERROR_PARAMETER3);
		sprintf(szString, "VOLT %.5lG", voltLev);
		break;

	case HP661X_VOLT_TRIG:
		CHK_REAL_RANGE(voltLev,
			HP661X_VOLT_MIN,
			thisPtr->voltMax,
			VI_ERROR_PARAMETER3);
		sprintf(szString, "VOLT:TRIG %.5lG", voltLev);
		break;

	case HP661X_VOLT_OVP:
		CHK_REAL_RANGE(voltLev,
			HP661X_VOLT_MIN,
			thisPtr->ovpMax,
			VI_ERROR_PARAMETER3);
		sprintf(szString, "VOLT:PROT %.5lG", voltLev);
		break;

	case HP661X_VOLT_OVP_MAX:
		sprintf(szString, "VOLT:PROT MAX");
		break;

	case HP661X_VOLT_OVP_MIN:
		sprintf(szString, "VOLT:PROT MIN");
		break;

	default:
		return statusUpdate(instrumentHandle, thisPtr, VI_ERROR_PARAMETER2 );
		break;
	}

    errStatus = viPrintf(instrumentHandle, "%s\n", szString);
    
    return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*-----------------------------------------------------------------------------
 * FUNC    : ViStatus _VI_FUNC hp661x_getVolt
 *-----------------------------------------------------------------------------
 * 
 * PURPOSE :  This function queries the instrument for the selected voltage
 *           paramter setting.
 * 
 * PARAM 1 : ViSession instrumentHandle
 * IN        
 *            The handle to the instrument.
 * 
 * PARAM 2 : ViInt16 voltParam
 * OUT       
 *            The voltage parameter selection.
 * 
 * PARAM 3 : ViPReal64 voltLev
 * OUT       
 *            The voltage level in volts.
 * 
 * RETURN  :  VI_SUCCESS: No error. Non VI_SUCCESS: Indicates error
 *           condition. To determine error message, pass the return value to
 *           routine "hp661x_error_message".
 * 
 *-----------------------------------------------------------------------------
 */
ViStatus _VI_FUNC hp661x_getVolt (ViSession instrumentHandle,
								   ViInt16 voltQParam,
								   ViPReal64 voltLev)
{
    struct hp661x_globals *thisPtr;
	ViStatus errStatus = 0;
    ViChar szString[HP661X_CMDSTRING_MAX];

    GET_GLOBALS(instrumentHandle, thisPtr);

    DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	switch (voltQParam)
	{
	case HP661X_VOLT_Q:
		sprintf(szString, "VOLT?\n");
		break;
	case HP661X_VOLT_Q_MAX:
		sprintf(szString, "VOLT? MAX\n");
		break;
	case HP661X_VOLT_Q_MIN:
		sprintf(szString, "VOLT? MIN\n");
		break;
	case HP661X_VOLT_Q_TRIG:
		sprintf(szString, "VOLT:TRIG?\n");
		break;
	case HP661X_VOLT_Q_TRIG_MAX:
		sprintf(szString, "VOLT:TRIG? MAX\n");
		break;
	case HP661X_VOLT_Q_TRIG_MIN:
		sprintf(szString, "VOLT:TRIG? MIN\n");
		break;
	case HP661X_VOLT_Q_OVP:
		sprintf(szString, "VOLT:PROT?\n");
		break;
	case HP661X_VOLT_Q_OVP_MAX:
		sprintf(szString, "VOLT:PROT? MAX\n");
		break;
	case HP661X_VOLT_Q_OVP_MIN:
		sprintf(szString, "VOLT:PROT? MIN\n");
		break;
	default:
		return statusUpdate(instrumentHandle, thisPtr, VI_ERROR_PARAMETER2 );
		break;
	}

    errStatus=viQueryf(instrumentHandle, szString, "%lf", voltLev);
	
    return statusUpdate(instrumentHandle, thisPtr, errStatus );
}

/*---------------------------------------------------------------------------*/
/* FUNC: ViStatus _VI_FUNC hp661x_opc_Q                                     */
/*---------------------------------------------------------------------------*/
/*  Sends an *OPC? command to the instrument and returns VI_TRUE when all    */
/*  pending operations are complete.                                         */
/*                                                                           */
/*  PARAMETERS                                                               */
/*    ViSession   instrumentHandle (in)                                      */
/*      Instrument handle returned from hp661x_init()                       */
/*    ViPBoolean  instrumentReady (out)                                      */
/*      Returns VI_TRUE when pending operations are complete.                */
/*---------------------------------------------------------------------------*/
ViStatus _VI_FUNC hp661x_opc_Q(ViSession	instrumentHandle,
								ViPBoolean  instrumentReady)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus;
   
	GET_GLOBALS(instrumentHandle, thisPtr);

	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	errStatus = viQueryf(instrumentHandle, "*OPC?\n", "%hd%*t", instrumentReady);

	return statusUpdate(instrumentHandle, thisPtr, errStatus);
}        /* ----- end of function ----- */ 

/*---------------------------------------------------------------------------*/
/* FUNC: ViStatus _VI_FUNC hp661x_stdEventStatus_Q                                */
/*---------------------------------------------------------------------------*/
/*  This function queries the Standart Event Status register.				 */
/*                                                                           */
/*  PARAMETERS                                                               */
/*    ViSession  instrumentHandle (in)                                       */
/*      Instrument handle returned from hp661x_init()                       */
/*    ViPInt16   stdEventStatus (out)                                        */
/*      The integer returned from the instrument.                            */
/*---------------------------------------------------------------------------*/
ViStatus _VI_FUNC hp661x_stdEvent_Q(ViSession  instrumentHandle,
									 ViPInt16   stdEventStatus)

{
	struct   hp661x_globals *thisPtr;
	ViStatus errStatus;
   
	GET_GLOBALS(instrumentHandle, thisPtr);

	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	errStatus = viQueryf(instrumentHandle, "*ESR?\n", "%hd%*t", stdEventStatus);

	return statusUpdate(instrumentHandle, thisPtr, errStatus );
}        /* ----- end of function ----- */ 


/*---------------------------------------------------------------------------*/
/* FUNC: ViStatus _VI_FUNC hp661x_setEnableReg                              */
/*---------------------------------------------------------------------------*/
/*  This function queries the Standart Event Status register.				 */
/*                                                                           */
/*  PARAMETERS                                                               */
/*    ViSession  instrumentHandle (in)                                       */
/*      Instrument handle returned from hp661x_init()                       */
/*    ViPInt16   enableMask (in)											 */
/*      The integer returned from the instrument.                            */
/*---------------------------------------------------------------------------*/
ViStatus _VI_FUNC hp661x_setEnableReg (ViSession instrumentHandle,
                                        ViInt16 enableRegister,
										ViInt16 enableMask)
{
	struct   hp661x_globals *thisPtr;
	ViStatus errStatus;
	ViChar	*pszString;
   
	GET_GLOBALS(instrumentHandle, thisPtr);

	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	switch (enableRegister)
	{
	case HP661X_REG_ESE:
		CHK_INT_RANGE(enableMask,
			0,
			255,
			VI_ERROR_PARAMETER3);
		pszString = "*ESE";
		break;
	case HP661X_REG_SRE:
		CHK_INT_RANGE(enableMask,
			0,
			255,
			VI_ERROR_PARAMETER3);
		pszString = "*SRE";
		break;
	case HP661X_REG_OPER:
		CHK_INT_RANGE(enableMask,
			0,
			32767,
			VI_ERROR_PARAMETER3);
		pszString = "STAT:OPER:ENAB";
		break;
	case HP661X_REG_QUES:
		CHK_INT_RANGE(enableMask,
			0,
			32767,
			VI_ERROR_PARAMETER3);
		pszString = "STAT:QUES:ENAB";
		break;
	default:
		return statusUpdate(instrumentHandle, thisPtr, VI_ERROR_PARAMETER2 );
		break;
	}

	errStatus = viPrintf(instrumentHandle, "%s %d\n", pszString, enableMask);

	return statusUpdate(instrumentHandle, thisPtr, errStatus );
}        /* ----- end of function ----- */ 


/*---------------------------------------------------------------------------*/
/* FUNC: ViStatus _VI_FUNC hp661x_getEnableReg                              */
/*---------------------------------------------------------------------------*/
/*  This function queries the Standart Event Status register.				 */
/*                                                                           */
/*  PARAMETERS                                                               */
/*    ViSession  instrumentHandle (in)                                       */
/*      Instrument handle returned from hp661x_init()                       */
/*    ViInt16   enableRegister (in)                                          */
/*      The integer returned from the instrument.                            */
/*    ViPInt16   enableMask (out)                                        */
/*      The integer returned from the instrument.                            */
/*---------------------------------------------------------------------------*/
ViStatus _VI_FUNC hp661x_getEnableReg (ViSession instrumentHandle,
                                        ViInt16 enableRegister,
										ViPInt16 enableMask)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus;
	ViChar		*pszString;
   
	GET_GLOBALS(instrumentHandle, thisPtr);

	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	switch (enableRegister)
	{
	case HP661X_REG_ESE:
		pszString = "*ESE";
		break;
	case HP661X_REG_SRE:
		pszString = "*SRE";
		break;
	case HP661X_REG_OPER:
		pszString = "STAT:OPER:ENAB";
		break;
	case HP661X_REG_QUES:
		pszString = "STAT:QUES:ENAB";
		break;
	default:
		return statusUpdate(instrumentHandle, thisPtr, VI_ERROR_PARAMETER2 );
		break;
	}

	errStatus = viQueryf(instrumentHandle, "%s?\n", "%hd%*t", pszString, enableMask);

	return statusUpdate(instrumentHandle, thisPtr, errStatus );
}        /* ----- end of function ----- */ 

        /***************************************************/
        /*  HP standard status functions                   */
        /***************************************************/

/*---------------------------------------------------------------------------*/
/* FUNC: ViStatus _VI_FUNC hp661x_readStatusByte_Q                          */
/*---------------------------------------------------------------------------*/
/*  PARAMETERS                                                               */
/*    ViSession instrumentHandle (in)                                        */
/*      Instrument Handle returned from hp661x_init()                       */
/*    ViPInt16  statusByte (out)                                             */
/*      Returns the contents of the status byte                              */
/*---------------------------------------------------------------------------*/
ViStatus _VI_FUNC hp661x_readStatusByte_Q(ViSession  instrumentHandle,
										   ViPInt16   statusByte)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus = 0;
	ViUInt16	stb;

	GET_GLOBALS(instrumentHandle, thisPtr)

	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	if ((errStatus = viReadSTB(instrumentHandle, &stb)) < VI_SUCCESS)
		return statusUpdate(instrumentHandle, thisPtr, errStatus );

	*statusByte = (ViInt16)stb;

	return statusUpdate(instrumentHandle, thisPtr, VI_SUCCESS);
}        /* ----- end of function ----- */


/*---------------------------------------------------------------------------*/
/* FUNC: ViStatus _VI_FUNC hp661x_operEvent_Q                               */
/*---------------------------------------------------------------------------*/
/*  PARAMETERS                                                               */
/*    ViSession vi (in)                                                      */
/*      Instrument Handle returned from hp661x_init()                       */
/*    ViPInt32  operationEventRegister (out)                                 */
/*      Returns the contents of the operation event register                 */
/*---------------------------------------------------------------------------*/
ViStatus _VI_FUNC hp661x_operEvent_Q(ViSession  instrumentHandle,
									  ViPInt32   operationEventRegister)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus;

	GET_GLOBALS(instrumentHandle, thisPtr)

	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	if ((errStatus = viQueryf(instrumentHandle,"STAT:OPER:EVEN?\n", "%ld%*t",
		operationEventRegister)) < VI_SUCCESS)
		return statusUpdate(instrumentHandle, thisPtr, errStatus);

	return statusUpdate(instrumentHandle, thisPtr, VI_SUCCESS);
}        /* ----- end of function ----- */


/*---------------------------------------------------------------------------*/
/* FUNC: ViStatus _VI_FUNC hp661x_operCond_Q                                */
/*---------------------------------------------------------------------------*/
/*  PARAMETERS                                                               */
/*    ViSession instrumentHandle (in)                                        */
/*      Instrument Handle returned from hp661x_init()                       */
/*    ViPInt32  operationConditionRegister (out)                             */
/*      Returns the contents of the operation condition register             */
/*---------------------------------------------------------------------------*/
ViStatus _VI_FUNC hp661x_operCond_Q(ViSession instrumentHandle,
									 ViPInt32 operationConditionRegister)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus;

	GET_GLOBALS(instrumentHandle, thisPtr)

	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	if ((errStatus = viQueryf(instrumentHandle, "STAT:OPER:COND?\n", "%ld%*t",
		operationConditionRegister)) < VI_SUCCESS)
		return statusUpdate(instrumentHandle, thisPtr, errStatus);

	return statusUpdate(instrumentHandle, thisPtr, VI_SUCCESS);
}        /* ----- end of function ----- */


/*---------------------------------------------------------------------------*/
/* FUNC: ViStatus _VI_FUNC hp661x_quesEvent_Q                               */
/*---------------------------------------------------------------------------*/
/*  PARAMETERS                                                               */
/*    ViSession instrumentHandle (in)                                        */
/*      Instrument Handle returned from hp661x_init()                       */
/*    ViPInt32  questionableEventRegister (out)                              */
/*      Returns the contents of the questionable event register              */
/*---------------------------------------------------------------------------*/
ViStatus _VI_FUNC hp661x_quesEvent_Q(ViSession instrumentHandle,
									  ViPInt32  questionableEventRegister)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus;

	GET_GLOBALS(instrumentHandle, thisPtr)

	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	if ((errStatus = viQueryf(instrumentHandle, "STAT:QUES:EVEN?\n", "%ld%*t",
		questionableEventRegister)) < VI_SUCCESS)
		return statusUpdate(instrumentHandle, thisPtr, errStatus);

	return statusUpdate(instrumentHandle, thisPtr, VI_SUCCESS );
}        /* ----- end of function ----- */


/*---------------------------------------------------------------------------*/
/* FUNC: ViStatus _VI_FUNC hp661x_quesCond_Q                                */
/*---------------------------------------------------------------------------*/
/*  PARAMETERS                                                               */
/*    ViSession instrumentHandle (in)                                        */
/*      Instrument Handle returned from hp661x_init()                        */
/*    ViPInt32  questionableConditionRegister (out)                          */
/*      Returns the contents of the questionable condition register          */
/*---------------------------------------------------------------------------*/
ViStatus _VI_FUNC hp661x_quesCond_Q(ViSession instrumentHandle,
									 ViPInt32  questionableConditionRegister)
{
	struct		hp661x_globals *thisPtr;
	ViStatus	errStatus;

	GET_GLOBALS(instrumentHandle, thisPtr)
 
	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	if ((errStatus = viQueryf(instrumentHandle, "STAT:QUES:COND?\n", "%ld%*t",
		questionableConditionRegister)) < VI_SUCCESS)
		return statusUpdate(instrumentHandle, thisPtr, errStatus);

	return statusUpdate(instrumentHandle, thisPtr, VI_SUCCESS);
}        /* ----- end of function ----- */

        /***************************************************/
        /*  HP standard command passthrough functions      */
        /***************************************************/

/*---------------------------------------------------------------------------*/
/* FUNC: ViStatus _VI_FUNC hp661x_cmd                                       */
/*---------------------------------------------------------------------------*/
/*  Send a scpi command, it does not look for a response                     */
/*                                                                           */
/*  PARAMETERS                                                               */
/*    ViSession     instrumentHandle (in)                                    */
/*      Instrument handle returned from hp661x_init()                       */
/*    ViString      sendStringCommand (in)                                   */
/*      The SCPI command string to be sent to the instrument                 */
/*---------------------------------------------------------------------------*/
ViStatus _VI_FUNC hp661x_cmd(ViSession instrumentHandle,
							  ViString  sendStringCommand)
{
	ViStatus errStatus;
	struct   hp661x_globals *thisPtr;

	GET_GLOBALS(instrumentHandle, thisPtr)

	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	errStatus = viPrintf(instrumentHandle,"%s\n", sendStringCommand);
   
	return statusUpdate( instrumentHandle, thisPtr, errStatus );
}        /* ----- end of function ----- */


/*---------------------------------------------------------------------------*/
/* FUNC: ViStatus _VI_FUNC hp661x_cmdString_Q                               */
/*---------------------------------------------------------------------------*/
/*  PARAMETERS                                                               */
/*    ViSession       instrumentHandle (in)                                  */
/*      Instrument handle returned from hp661x_init()                       */
/*    ViString        queryStringCommand (in)                                */
/*      The SCPI command string to be sent to the instrument                 */
/*    ViInt32         stringSize (in)                                        */
/*      The size of the char array (result) passed to the function to hold   */
/*      the string returned by the instrument                                */
/*    ViChar _VI_FAR  stringResult[] (out)                                   */
/*      The string returned by the instrument                                */
/*---------------------------------------------------------------------------*/
ViStatus _VI_FUNC hp661x_cmdString_Q (ViSession	instrumentHandle,
									   ViString		queryStringCommand,
									   ViInt32		stringSize,
									   ViChar		_VI_FAR stringResult[])
{
	struct    hp661x_globals *thisPtr;
	ViStatus  errStatus;
	int       sz;

        /* Command strings must have at least one non-null character */
	if(stringSize < 2)
		return statusUpdate(instrumentHandle,VI_NULL,VI_ERROR_PARAMETER2);

	GET_GLOBALS(instrumentHandle, thisPtr)

	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	sz = (int)stringSize;
	if ((errStatus = viQueryf(instrumentHandle, "%s\n", "%#t",
                         queryStringCommand, &sz, stringResult)) < VI_SUCCESS)
		return statusUpdate(instrumentHandle,thisPtr,errStatus);
	stringResult[sz]='\0';
      
	return statusUpdate(instrumentHandle, thisPtr, errStatus );
}        /* ----- end of function ----- */ 

/*---------------------------------------------------------------------------*/
/* FUNC: ViStatus _VI_FUNC hp661x_cmdInt                                    */
/*---------------------------------------------------------------------------*/
/*  Sends an instrument command which takes a single integer parameter.      */
/*                                                                           */
/*  PARAMETERS                                                               */
/*    ViSession  instrumentHandle (in)                                       */
/*      Instrument handle returned from hp661x_init()                        */
/*    ViString   sendIntegerCommand (in)                                     */
/*      The instrument command string to be sent to the instrument.          */
/*    ViPInt32   sendInteger (in)                                            */
/*      The integer sent to the instrument at the end of the instrument      */
/*      command.  Can be ViInt16 or ViInt32.                                 */
/*---------------------------------------------------------------------------*/
ViStatus _VI_FUNC hp661x_cmdInt(ViSession  instrumentHandle,
								 ViString   sendIntegerCommand,
								 ViInt32    sendInteger )
{  
ViStatus errStatus;
struct hp661x_globals *thisPtr;
   
	GET_GLOBALS(instrumentHandle, thisPtr)

	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	if ((errStatus = viPrintf(instrumentHandle, "%s %ld\n", sendIntegerCommand,
									sendInteger)) < VI_SUCCESS)
		return statusUpdate(instrumentHandle,thisPtr,errStatus);

	return statusUpdate(instrumentHandle, thisPtr, errStatus );
}        /* ----- end of function ----- */ 


/*---------------------------------------------------------------------------*/
/* FUNC: ViStatus _VI_FUNC hp661x_cmdInt16_Q                                */
/*---------------------------------------------------------------------------*/
/*  Sends scpi command and waits for a response that must be representable   */
/*    as an int16.  A non numeric instrument response returns zero in        */
/*    result.                                                                */
/*                                                                           */
/*  PARAMETERS                                                               */
/*    ViSession  instrumentHandle (in)                                       */
/*      Instrument handle returned from hp661x_init()                        */
/*    ViString   queryI16Command (in)                                        */
/*      The SCPI command string to be sent to the instrument.                */
/*    ViPInt16   i16Result (out)                                             */
/*      The integer returned from the instrument.                            */
/*---------------------------------------------------------------------------*/
ViStatus _VI_FUNC hp661x_cmdInt16_Q(ViSession  instrumentHandle,
									 ViString   queryI16Command,
									 ViPInt16   i16Result)

{
ViStatus errStatus;
struct   hp661x_globals *thisPtr;
   
	GET_GLOBALS(instrumentHandle, thisPtr)

	DEBUG_CHK_THIS(instrumentHandle, thisPtr );

	if ((errStatus = viQueryf(instrumentHandle, "%s\n", "%hd%*t",
									queryI16Command, i16Result)) < VI_SUCCESS)
		return statusUpdate( instrumentHandle, thisPtr, errStatus );

	return statusUpdate(instrumentHandle, thisPtr, errStatus );
}        /* ----- end of function ----- */ 

