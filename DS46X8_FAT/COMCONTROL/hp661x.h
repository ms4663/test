/*****************************************************************************/
/*  $Header:                                                              $  */
/*  Copyright (C) 1997 Hewlett-Packard Company                               */
/*---------------------------------------------------------------------------*/
/*  Driver for hp661x                                                        */
/*  Driver Version: A.00.00                                                  */
/*****************************************************************************/

#ifndef HP661X_INCLUDE 
#define HP661X_INCLUDE
#include "vpptype.h"

/* Used for "C" externs in C++ */
#if defined(__cplusplus) || defined(__cplusplus__)
extern "C" {
#endif

/*****************************************************************************/
/*  STANDARD SECTION                                                         */
/*  Constants and function prototypes for HP standard functions.             */
/*****************************************************************************/

        /***************************************************/
        /*  Standard constant error conditions returned    */
        /*  by driver functions.                           */
        /*    HP Common Error numbers start at BFFC0D00    */
        /*    The parameter errors extend the number of    */
        /*      errors over the eight defined in VPP 3.4   */
        /***************************************************/

#define HP661X_INSTR_ERROR_NULL_PTR      (_VI_ERROR+0x3FFC0D02L) /* 0xBFFC0D02 */
#define HP661X_INSTR_ERROR_RESET_FAILED  (_VI_ERROR+0x3FFC0D03L) /* 0xBFFC0D03 */
#define HP661X_INSTR_ERROR_UNEXPECTED    (_VI_ERROR+0x3FFC0D04L) /* 0xBFFC0D04 */
#define HP661X_INSTR_ERROR_INV_SESSION   (_VI_ERROR+0x3FFC0D05L) /* 0xBFFC0D05 */
#define HP661X_INSTR_ERROR_LOOKUP        (_VI_ERROR+0x3FFC0D06L) /* 0xBFFC0D06 */
#define HP661X_INSTR_ERROR_DETECTED      (_VI_ERROR+0x3FFC0D07L) /* 0xBFFC0D07 */
#define HP661X_INSTR_NO_LAST_COMMA       (_VI_ERROR+0x3FFC0D08L) /* 0xBFFC0D08 */ 

#define HP661X_INSTR_ERROR_PARAMETER9    (_VI_ERROR+0x3FFC0D20L) /* 0xBFFC0D20 */
#define HP661X_INSTR_ERROR_PARAMETER10   (_VI_ERROR+0x3FFC0D21L) /* 0xBFFC0D21 */
#define HP661X_INSTR_ERROR_PARAMETER11   (_VI_ERROR+0x3FFC0D22L) /* 0xBFFC0D22 */
#define HP661X_INSTR_ERROR_PARAMETER12   (_VI_ERROR+0x3FFC0D23L) /* 0xBFFC0D23 */
#define HP661X_INSTR_ERROR_PARAMETER13   (_VI_ERROR+0x3FFC0D24L) /* 0xBFFC0D24 */
#define HP661X_INSTR_ERROR_PARAMETER14   (_VI_ERROR+0x3FFC0D25L) /* 0xBFFC0D25 */
#define HP661X_INSTR_ERROR_PARAMETER15   (_VI_ERROR+0x3FFC0D26L) /* 0xBFFC0D26 */
#define HP661X_INSTR_ERROR_PARAMETER16   (_VI_ERROR+0x3FFC0D27L) /* 0xBFFC0D27 */
#define HP661X_INSTR_ERROR_PARAMETER17   (_VI_ERROR+0x3FFC0D28L) /* 0xBFFC0D28 */
#define HP661X_INSTR_ERROR_PARAMETER18   (_VI_ERROR+0x3FFC0D29L) /* 0xBFFC0D29 */

#define HP661X_INSTR_ERROR_MEAS_TMO		 (_VI_ERROR+0x3FFC0D2AL) /* 0xBFFC0D2A */
#define HP661X_INSTR_ERROR_PORT_CONFIG	 (_VI_ERROR+0x3FFC0D2BL) /* 0xBFFC0D2B */
#define HP661X_INSTR_ERROR_GET_MAX		 (_VI_ERROR+0x3FFC0D2CL) /* 0xBFFC0D2C */

        /***************************************************/
        /*  Constants used by system status functions      */
        /*    These defines are bit numbers which define   */
        /*    the operation and questionable registers.    */
        /*    They are instrument specific.                */
        /***************************************************/

#define	HP661X_STB_QUES				0x0008
#define HP661X_STB_MAV				0x0010
#define	HP661X_STB_ESB				0x0020
#define	HP661X_STB_MSS				0x0040
#define	HP661X_STB_RQS				0x0040
#define	HP661X_STB_OPER				0x0080

#define	HP661X_SRE_QUES				((ViInt16)8)
#define	HP661X_SRE_MAV				((ViInt16)16)
#define	HP661X_SRE_ESB				((ViInt16)32)
#define	HP661X_SRE_MSS				((ViInt16)64)
#define	HP661X_SRE_OPER				((ViInt16)128)

#define	HP661X_OPER_CAL				0x0001
#define	HP661X_OPER_WTG				0x0020
#define	HP661X_OPER_CV				0x0100
#define	HP661X_OPER_CC_POS			0x0400
#define	HP661X_OPER_CC_NEG			0x0800

#define	HP661X_QUES_OV				0x0001
#define	HP661X_QUES_OCP				0x0002
#define	HP661X_QUES_FUSE			0x0004
#define	HP661X_QUES_OT				0x0010
#define	HP661X_QUES_RI				0x0200
#define	HP661X_QUES_UNREG			0x0400
#define	HP661X_QUES_OVLD			0x4000

#define	HP661X_ESR_OPC				0x0001
#define	HP661X_ESR_QYE				0x0004
#define	HP661X_ESR_DDE				0x0008
#define	HP661X_ESR_EXE				0x0010
#define	HP661X_ESR_CME				0x0020
#define	HP661X_ESR_PON				0x0080

#define	HP661X_REG_ESE				0
#define	HP661X_REG_SRE				1
#define	HP661X_REG_OPER				2
#define	HP661X_REG_QUES				3

#define	HP661X_CMDSTRING_MAX		256

#define	HP661X_CMDSTRING_Q_MIN		2
#define	HP661X_CMDSTRING_Q_MAX		32767L


		/***************************************************/
        /*  Constants used by function hp661x_timeOut      */
        /***************************************************/

#define HP661X_TIMEOUT_MAX         2147483647L
#define HP661X_TIMEOUT_MIN         0L

        /***************************************************/
        /*  Required plug and play functions from VPP-3.1  */
        /***************************************************/

ViStatus _VI_FUNC hp661x_init (ViRsrc resourceName,
							   ViBoolean idQuery,
							   ViBoolean resetDevice,
							   ViPSession instrumentHandle);

ViStatus _VI_FUNC hp661x_reset (ViSession instrumentHandle);

ViStatus _VI_FUNC hp661x_self_test (ViSession instrumentHandle,
									ViPInt16 testResult,
									ViChar _VI_FAR testMessage[]);

ViStatus _VI_FUNC hp661x_error_message (ViSession instrumentHandle,
										ViStatus statusCode,
										ViChar _VI_FAR statusMessage[]);

ViStatus _VI_FUNC hp661x_error_query (ViSession instrumentHandle,
									  ViPInt32	 errorCode,
									  ViChar	 _VI_FAR errorMessage[]);

ViStatus _VI_FUNC hp661x_revision_query (ViSession instrumentHandle,
										 ViChar	_VI_FAR instrumentDriverRevision[],
										 ViChar	_VI_FAR firmwareRevision[] );

ViStatus _VI_FUNC hp661x_close (ViSession instrumentHandle);

ViStatus _VI_FUNC hp661x_errorQueryDetect (ViSession instrumentHandle,
										   ViBoolean errorQueryDetect);

ViStatus _VI_FUNC hp661x_errorQueryDetect_Q (ViSession instrumentHandle,
											 ViPBoolean errorQueryDetect);

        /***************************************************/
        /*  HP standard utility functions                  */
        /***************************************************/

ViStatus _VI_FUNC hp661x_timeOut (ViSession instrumentHandle, ViInt32 timeOut);

ViStatus _VI_FUNC hp661x_timeOut_Q (ViSession instrumentHandle, ViInt32 *timeOut);

ViStatus _VI_FUNC hp661x_dcl (ViSession instrumentHandle);

ViStatus _VI_FUNC hp661x_trg (ViSession instrumentHandle);

ViStatus _VI_FUNC hp661x_opc (ViSession instrumentHandle);

ViStatus _VI_FUNC hp661x_opc_Q (ViSession instrumentHandle,
								ViPBoolean	instrumentReady);

        /***************************************************/
        /*  HP standard status functions                   */
        /***************************************************/

ViStatus _VI_FUNC hp661x_readStatusByte_Q (ViSession instrumentHandle,
										   ViInt16 *statusByte);

ViStatus _VI_FUNC hp661x_operEvent_Q (ViSession instrumentHandle,
									  ViInt32 *operationEventRegister);

ViStatus _VI_FUNC hp661x_operCond_Q (ViSession instrumentHandle,
									 ViInt32 *operationConditionRegister);

ViStatus _VI_FUNC hp661x_quesEvent_Q (ViSession instrumentHandle,
									  ViInt32 *questionableEventRegister);

ViStatus _VI_FUNC hp661x_quesCond_Q (ViSession instrumentHandle,
									 ViInt32 *questionableConditionRegister);

        /***************************************************/
        /*  HP standard command passthrough functions      */
        /***************************************************/

ViStatus _VI_FUNC hp661x_cmd (ViSession	instrumentHandle,
							  ViString		stringCommand);

ViStatus _VI_FUNC hp661x_cmdString_Q (ViSession	instrumentHandle,
									  ViString		queryStringCommand,
									  ViInt32		stringSize,
									  ViChar		_VI_FAR stringResult[]);
	
ViStatus _VI_FUNC hp661x_cmdInt(ViSession  instrumentHandle,
								ViString   sendIntegerCommand,
								ViInt32    sendInteger );

ViStatus _VI_FUNC hp661x_cmdInt16_Q(ViSession  instrumentHandle,
									ViString   queryI16Command,
									ViPInt16   i16Result);

/* End of HP standard declarations */
/*---------------------------------------------------------------------------*/


/*****************************************************************************/
/*  INSTRUMENT SPECIFIC SECTION                                              */
/*  Constants and function prototypes for instrument specific functions.     */
/*****************************************************************************/

        /***************************************************/
        /*  Instrument specific error numbers              */
        /***************************************************/

#define	HP661X_INSTR_ERROR_NVRAM_RD0_CS			1
#define	HP661X_INSTR_ERROR_NVRAM_CONFIG_CS		2
#define	HP661X_INSTR_ERROR_NVRAM_CAL_CS			3
#define	HP661X_INSTR_ERROR_NVRAM_STATE_CS		4
#define	HP661X_INSTR_ERROR_NVRAM_RST_CS			5
#define	HP661X_INSTR_ERROR_RAM_SELFTEST			10
#define	HP661X_INSTR_ERROR_DAC_SELFTEST1		11
#define	HP661X_INSTR_ERROR_DAC_SELFTEST2		12
#define	HP661X_INSTR_ERROR_DAC_SELFTEST3		13
#define	HP661X_INSTR_ERROR_DAC_SELFTEST4		14
#define	HP661X_INSTR_ERROR_OVDAC_SELFTEST		15
#define	HP661X_INSTR_ERROR_DIGIO_SELFTEST		80
#define	HP661X_INSTR_ERROR_INGUARD_RXBUF_OVR	213
#define	HP661X_INSTR_ERROR_RS232_FRAMING		216
#define	HP661X_INSTR_ERROR_RS232_PARITY			217
#define	HP661X_INSTR_ERROR_RS232_RX_OVR			218
#define	HP661X_INSTR_ERROR_FP_UART_OVR			220
#define	HP661X_INSTR_ERROR_FP_UART_FRAMING		221
#define	HP661X_INSTR_ERROR_FP_UART_PARITY		222
#define	HP661X_INSTR_ERROR_FP_RXBUF_OVR			223
#define	HP661X_INSTR_ERROR_FP_TIMEOUT			224
#define	HP661X_INSTR_ERROR_CAL_SWITCH			401
#define	HP661X_INSTR_ERROR_CAL_PASSWORD			402
#define	HP661X_INSTR_ERROR_CAL_DISABLED			403
#define	HP661X_INSTR_ERROR_CAL_RB_CONST			404
#define	HP661X_INSTR_ERROR_CAL_PROG_CONST		405
#define	HP661X_INSTR_ERROR_CAL_CMD_SEQUENCE		406
#define	HP661X_INSTR_ERROR_CVCC_STATUS			407
#define	HP661X_INSTR_ERROR_ALC_NOT_NORMAL		408
#define	HP661X_INSTR_ERROR_TOO_MANY_SWE_POINTS	601
#define	HP661X_INSTR_ERROR_RS232_CMD_ONLY		602
#define	HP661X_INSTR_ERROR_INCOMPATIBLE_FETCH	603
#define	HP661X_INSTR_ERROR_MEAS_OVERRANGE		604

        /***************************************************/
        /*  Instrument specific constants                  */
        /***************************************************/

#define	HP661X_DEFAULT_MEAS_TIMEOUT		10000 /* milliseconds (10 seconds) */

#define	HP661X_DISP_NORMAL			0
#define	HP661X_DISP_TEXT			1

#define	HP661X_RELAY_POLARITY_NORMAL     0
#define	HP661X_RELAY_POLARITY_REVERSE    1

#define	HP661X_RI_OFF				0
#define	HP661X_RI_LATCHING			1
#define	HP661X_RI_LIVE				2

#define	HP661X_DFI_SRC_PREV			0
#define	HP661X_DFI_SRC_QUES			1
#define	HP661X_DFI_SRC_OPER			2
#define	HP661X_DFI_SRC_ESB			3
#define	HP661X_DFI_SRC_RQS			4
#define	HP661X_DFI_SRC_OFF			5

#define	HP661X_DIG_OUT_MAX			7
#define HP661X_DIG_OUT_MIN			0

#define	HP661X_PON_STATE_RST		0
#define	HP661X_PON_STATE_RCL0		1

#define	HP661X_CURR_DET_ACDC		0
#define	HP661X_CURR_DET_DC			1

#define	HP661X_CURR_IMM				0
#define	HP661X_CURR_TRIG			1

#define	HP661X_CURR_Q				0
#define	HP661X_CURR_Q_TRIG			1
#define	HP661X_CURR_Q_MAX			2
#define	HP661X_CURR_Q_MIN			3

#define	HP661X_VOLT_IMM				0
#define	HP661X_VOLT_TRIG			1
#define	HP661X_VOLT_OVP				2
#define	HP661X_VOLT_OVP_MAX			3
#define	HP661X_VOLT_OVP_MIN			4

#define	HP661X_VOLT_Q				0
#define	HP661X_VOLT_Q_MAX			1
#define	HP661X_VOLT_Q_MIN			2
#define	HP661X_VOLT_Q_TRIG			3
#define	HP661X_VOLT_Q_TRIG_MAX		4
#define	HP661X_VOLT_Q_TRIG_MIN		5
#define	HP661X_VOLT_Q_OVP			6
#define	HP661X_VOLT_Q_OVP_MAX		7
#define	HP661X_VOLT_Q_OVP_MIN		8

#define	HP661X_ARM_OUTPUT_ONCE				0
#define	HP661X_ARM_OUTPUT_CONTINUOUS		1
#define	HP661X_ARM_OUTPUT_CONTINUOUS_OFF	2

#define	HP661X_TRIG_OUTPUT			0
#define	HP661X_TRIG_SRC_BUS			0

#define	HP661X_SET_VOLT				0
#define	HP661X_SET_CURR				1

#define	HP661X_OUTP_STATUS_OFF		0
#define	HP661X_OUTP_STATUS_CV		1
#define	HP661X_OUTP_STATUS_CC		2
#define	HP661X_OUTP_STATUS_UNREG	3
#define	HP661X_OUTP_STATUS_PROT		4
#define	HP661X_OUTP_STATUS_UNKNOWN	5

#if 0
/* these are read from the unit at init time */
/*
#define	HP661X_VOLT_TRIG_MAX		20.0
#define	HP661X_VOLT_MAX			20.0
#define	HP661X_CURR_TRIG_MAX		2.0
#define	HP661X_CURR_MAX			2.0
*/
#endif

#define	HP661X_SWEEP_INT_MIN		15.6e-6

#define	HP661X_SWEEP_SIZE_MIN		1
#define	HP661X_SWEEP_SIZE_MAX		4096

#define	HP661X_VOLT_TRIG_MIN		0
#define	HP661X_VOLT_MIN				0
#define	HP661X_CURR_TRIG_MIN		0
#define	HP661X_CURR_MIN				0

#define	HP661X_HW_SETTLING_MIN		0
#define	HP661X_HW_SETTLING_MAX		30000		/* 30 seconds */

#define	HP661X_OCP_DELAY_MIN		0
#define	HP661X_OCP_DELAY_MAX		2147480000

#define HP661X_CLOCK_HW_SETTLE		50
#define HP661X_CLOCK_RESET_INST		100
#define HP661X_CLOCK_SELF_TEST		200

        /***************************************************/
        /*  Instrument specific functions                  */
        /***************************************************/
ViStatus _VI_FUNC hp661x_rippleRiDfi (ViSession instrumentHandle);

ViStatus _VI_FUNC hp661x_readOutputStatus (ViSession instrumentHandle,
										   ViPInt16 outputStatus);

ViStatus _VI_FUNC hp661x_outputVoltCurr(ViSession instrumentHandle,
										ViReal64 voltLev,
										ViReal64 currLev);

ViStatus _VI_FUNC hp661x_setMeasure (ViSession instrumentHandle,
									 ViInt16	outputParam,
									 ViReal64	outputLev,
									 ViInt16	settlingTime,
									 ViPReal64	voltQLev,
									 ViPReal64 currQLev,
									 ViPInt16	outputStatus);

ViStatus _VI_FUNC hp661x_doDelay (ViSession	instrumentHandle,
								  ViInt16		milliSeconds);

ViStatus _VI_FUNC hp661x_setDisplay (ViSession instrumentHandle,
									 ViBoolean dispState,
									 ViInt16 dispMode,
									 ViChar _VI_FAR messageText[]);

ViStatus _VI_FUNC hp661x_getDisplay (ViSession instrumentHandle,
									 ViPBoolean dispState,
									 ViPInt16 dispMode,
									 ViChar _VI_FAR messageText[]);

ViStatus _VI_FUNC hp661x_measureVolt (ViSession instrumentHandle,
									  ViPReal64 measVolt);

ViStatus _VI_FUNC hp661x_measureCurr (ViSession instrumentHandle,
									  ViPReal64 measCurr);

ViStatus _VI_FUNC hp661x_setMeasTimeout (ViSession instrumentHandle,
                                         ViInt32 measTimeout);

ViStatus _VI_FUNC hp661x_getMeasTimeout (ViSession instrumentHandle,
                                         ViPInt32 measTimeout);

ViStatus _VI_FUNC hp661x_setSweepParams (ViSession instrumentHandle,
										 ViReal64 sampleInterval,
										 ViInt32 sweepSize);

ViStatus _VI_FUNC hp661x_getSweepParams (ViSession instrumentHandle,
										 ViPReal64 sampleInterval,
										 ViPInt32 sweepSize);

ViStatus _VI_FUNC hp661x_clearOutpProt (ViSession instrumentHandle);

ViStatus _VI_FUNC hp661x_setOutpRelayPolarity (ViSession instrumentHandle,
											   ViInt16 relayPolarity);

ViStatus _VI_FUNC hp661x_getOutpRelayPolarity (ViSession instrumentHandle,
											   ViPInt16 relayPolarity);

ViStatus _VI_FUNC hp661x_setOutpState (ViSession instrumentHandle,
									   ViBoolean outputState,
									   ViBoolean switchRelay);

ViStatus _VI_FUNC hp661x_getOutpState (ViSession instrumentHandle,
									   ViPBoolean outputState);

ViStatus _VI_FUNC hp661x_setPonState (ViSession instrumentHandle,
									  ViInt16 ponState);

ViStatus _VI_FUNC hp661x_getPonState (ViSession instrumentHandle,
									  ViPInt16 ponState);

ViStatus _VI_FUNC hp661x_setOutpRelay (ViSession instrumentHandle,
									   ViBoolean relayState);

ViStatus _VI_FUNC hp661x_getOutpRelay (ViSession instrumentHandle,
									   ViPBoolean relayState);

ViStatus _VI_FUNC hp661x_setRiDfi (ViSession instrumentHandle,
								   ViInt16 riMode,
								   ViBoolean dfiState,
								   ViInt16 dfiSourceBit);

ViStatus _VI_FUNC hp661x_getRiDfi (ViSession instrumentHandle,
								   ViPInt16 riMode,
								   ViPBoolean dfiState,
								   ViPInt16 dfiSourceBit);

ViStatus _VI_FUNC hp661x_setCurrSenseRange (ViSession instrumentHandle,
											ViReal64 currSenseRange);

ViStatus _VI_FUNC hp661x_getCurrSenseRange (ViSession instrumentHandle,
											ViPReal64 currSenseRange);

ViStatus _VI_FUNC hp661x_setCurr (ViSession instrumentHandle,
								  ViInt16 currParam,
								  ViReal64 currLev);

ViStatus _VI_FUNC hp661x_getCurr (ViSession instrumentHandle,
								  ViInt16 currQParam,
								  ViPReal64 currLev);

ViStatus _VI_FUNC hp661x_setVolt (ViSession instrumentHandle,
								  ViInt16 voltParam,
								  ViReal64 voltLev);

ViStatus _VI_FUNC hp661x_getVolt (ViSession instrumentHandle,
								  ViInt16 voltQParam,
								  ViPReal64 voltLev);

ViStatus _VI_FUNC hp661x_setOcpParams (ViSession instrumentHandle,
									   ViBoolean ocpEnable,
									   ViReal64 ccDelay);

ViStatus _VI_FUNC hp661x_getOcpParams (ViSession instrumentHandle,
									   ViPBoolean ocpEnable,
									   ViPReal64 ccDelay);

ViStatus _VI_FUNC hp661x_setDigio (ViSession instrumentHandle, ViInt16 digOutData);

ViStatus _VI_FUNC hp661x_getDigio (ViSession instrumentHandle,
								   ViPInt16 digInData);

ViStatus _VI_FUNC hp661x_getVoltAlcBandwidth (ViSession instrumentHandle,
											  ViPInt32 alcBandwidth);

ViStatus _VI_FUNC hp661x_abort (ViSession instrumentHandle);


ViStatus _VI_FUNC hp661x_arm (ViSession instrumentHandle, ViInt16 trigSystem);

ViStatus _VI_FUNC hp661x_trigger (ViSession instrumentHandle);

ViStatus _VI_FUNC hp661x_trg (ViSession instrumentHandle);

ViStatus _VI_FUNC hp661x_stdEvent_Q (ViSession instrumentHandle,
									 ViPInt16 stdEventStatus);

ViStatus _VI_FUNC hp661x_setEnableReg (ViSession instrumentHandle,
									   ViInt16 enableRegister,
									   ViInt16 enableMask);

ViStatus _VI_FUNC hp661x_getEnableReg (ViSession instrumentHandle,
									   ViInt16 enableRegister,
									   ViPInt16 enableMask);

/* End of instrument specific declarations */
/*---------------------------------------------------------------------------*/

/* Used for "C" externs in C++ */
#if defined(__cplusplus) || defined(__cplusplus__)
}
#endif 

#endif /* HP661X_INCLUDE */

