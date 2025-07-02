
/*****************************************************************************/     
/*         PS_PowerSupply.c													 */	
/*****************************************************************************/
/*  P&P Driver for HP663x2, Dynamic Measurement dc Sources 					 */
/*                 and HP661xC power supply									 */
/*																			 */
/*  Type define   HP663xB driver 0											 */
/*                HP661xC driver 1											 */
/*                Unknown        >=2										 */
/******************************************************************************/      

#include "PS_powerSupply.h"  
#include <utility.h>
#include <ansi_c.h>
#include <formatio.h>
#include <userint.h>

// *** Specific Instrument Drivers ***
#include "hp663x2.h"
#include "hp661x.h"	

static char pszModelName[10][50];
static int GPIB_address[10]; 
static char pszModels[10][50]; 
static ViSession PS_sessionHandle[10];   
static ViStatus errStatus  =0;
static ViBoolean SIM_flag =0;
static ViSession  instrument_Handle;
static char	PS_errorMessage[MAX_LENGTH]	= "";
//static enum {hp663x2,hp661x} driverType;
static enum {hp661x,hp663x2} driverType; 

//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------
/*
PS_FUNCT_PFX PS_Init (int iIndex, ViInt16 iIGPIBAddr,
                                       char* pszModel,ViBoolean iIFlag)
                  e.g.                     
			       pszModel ="AG66319D"   driverType=hp663x2
				   pszModel ="AG6632B"    driverType=hp661x
			 
*/
//----------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------

PS_FUNCT_PFX PS_Init (int iIndex, ViInt16 iIGPIBAddr, char * pszModel,int iIFlag) 
                                      
	{
	  char buf_in  [ 64];
 	  
 	  errStatus=0;
 	  strcpy(PS_errorMessage,"No Error.");
      StringUpperCase (pszModel);

  		if (! strcmp(pszModel,"AGE6611C")||! strcmp(pszModel,"AGE6612C")) driverType=hp661x;
  		else if (! strcmp(pszModel,"HP6611C")||! strcmp(pszModel,"HP6612C")) driverType=hp661x;
  		else if (! strcmp(pszModel,"6611C")||! strcmp(pszModel,"6612C")) driverType=hp661x;
  		else if (! strcmp(pszModel,"AGE6613C")||! strcmp(pszModel,"AGE6614C")) driverType=hp661x;
  		else if (! strcmp(pszModel,"HP6613C")||! strcmp(pszModel,"HP6614C")) driverType=hp661x;
  		else if (! strcmp(pszModel,"6613C")||! strcmp(pszModel,"6614C")) driverType=hp661x;
  		else if (! strcmp(pszModel,"66309B")||! strcmp(pszModel,"66309D")) driverType=hp663x2;
  		else if (! strcmp(pszModel,"66311A")||! strcmp(pszModel,"66311B")) driverType=hp663x2; 
  		else if (! strcmp(pszModel,"66311D")||! strcmp(pszModel,"66319B")) driverType=hp663x2; 
  		else if (! strcmp(pszModel,"66321B")||! strcmp(pszModel,"66321D")) driverType=hp663x2; 
  		else if (! strcmp(pszModel,"66312A")||! strcmp(pszModel,"66332A")) driverType=hp663x2; 
  		else if (! strcmp(pszModel,"HP663X2")) driverType=hp663x2; 
  		else if (! strcmp(pszModel,"HP6632B")||! strcmp(pszModel,"6632B")) driverType=hp661x; 
		else if (! strcmp(pszModel,"6631B")||! strcmp(pszModel,"6633B")) driverType=hp661x;
  		else if (! strcmp(pszModel,"AG66319D")||! strcmp(pszModel,"66319D")) driverType=hp663x2;
		else if (! strcmp(pszModel,"E3632A")) driverType=hp663x2; 
		else 
			{
	         if (! strcmp(pszModel,"6634B")||! strcmp(pszModel,"HP661X")) driverType=hp661x;
	   		 } 	  
 
	   if (iIFlag==0)       // It is a real test and not a simulation
	   
	    {
		 sprintf ( buf_in, "GPIB0::%d::INSTR", iIGPIBAddr );
		 
	   	 switch ( driverType)
	   	 
			{
				case hp663x2:
				
			        if ((errStatus=hp663x2_init (buf_in, VI_FALSE, VI_TRUE,&instrument_Handle))==0)    
			        			 // 2nd parameter VI_FALSE: no query;3rd parameter VI_TRUE: reset the intrumen.t 
				        {
				             
				              PS_sessionHandle[iIndex]= instrument_Handle;
 						      strcpy (pszModelName[iIndex],"PS_ID_AGILENT_66319D");
 						      strcpy( pszModels[iIndex], pszModel);
 						      SIM_flag = iIFlag;
 						 		//no error
 						 		
 						 }
 					else
 					
 						{	
 							 
 							 { strcpy(PS_errorMessage,"Failed to initialize the power supply"); errStatus=VI_ERR_TRUE ;}
 							 
						}
					
					break;
				
					
				case hp661x:
				
					   if ((errStatus= hp661x_init (buf_in,VI_FALSE, VI_TRUE,&instrument_Handle))==0)
					   			 // 2nd parameter VI_FALSE: no query;3rd parameter VI_TRUE: reset the intrumen.
				        {
				            PS_sessionHandle[iIndex]=  instrument_Handle;
 					        strcpy (pszModelName[iIndex],"PS_ID_AGILENT_66319D");
 					        strcpy( pszModels[iIndex],pszModel);
 					        SIM_flag = iIFlag;
 						 		//no error
 						 }
 					else
 					
 						{
 							 	  { strcpy(PS_errorMessage,"Failed to initialize the power supply");errStatus=VI_ERR_TRUE  ;} 
						}
					
					break;

				default:
					 
					  { 
					      strcpy(PS_errorMessage,"Unknown power supper ID");  errStatus=VI_ERR_TRUE;
					   
					   }
			  }

	   }
	   
	   	else SIM_flag=1;       
	
	    return errStatus ;
	}
	
//--------------------------------------------------------------------------------------------------------	
	
	
PS_FUNCT_PFX PS_Reset (int iIndex)

	{
	    errStatus=0;
	    strcpy(PS_errorMessage,"No Error.");
	     
	    if (SIM_flag==0)       // It is a real test and not a simulation
	    {
	      instrument_Handle = PS_sessionHandle[iIndex];
 		  if  (instrument_Handle !=0)
 		   {
		    switch ( driverType)
		 	{
				case hp663x2:
					
				  if((errStatus=hp663x2_reset (instrument_Handle))!=0)
				  
		  			    		// hp663x2_error_message (instrument_Handle,errStatus,PS_errorMessage);
		  			  strcpy(PS_errorMessage,"Failed to reset the power supply");
		  			break;
				
				case hp661x:
				
				  if((errStatus= hp661x_reset (instrument_Handle)) !=0)
				    
				      			// hp661x_error_message (instrument_Handle,errStatus,PS_errorMessage);
				        strcpy(PS_errorMessage,"Failed to reset the power supply");
				    break;
		  
		  		default:
		  			 
		  			{  strcpy(PS_errorMessage,"Unknown power supper ID"); errStatus = VI_ERR_TRUE  ; }   
		  	  }
 			}
		  }
		  
		return errStatus;
	}
 
//-----------------------------------------------------------------------------------------------------

PS_FUNCT_PFX PS_Close (int iIndex)
	{
	    errStatus=0;
	    strcpy(PS_errorMessage,"No Error.");

	  if (SIM_flag==0)       // It is a real test and not a simulation   FALSE:0 ; TRUE: 1
	    {
	      instrument_Handle = PS_sessionHandle[iIndex];
 
	    	 switch ( driverType)
		 	{
				case hp663x2:
					
				  if((errStatus=hp663x2_close (instrument_Handle))!=0)
				  
		  			  //hp663x2_error_message (instrument_Handle,errStatus,PS_errorMessage);
		  			    strcpy(PS_errorMessage,"Failed to close the power supply");
		  			break;
				
				 case hp661x:
				
				  if((errStatus= hp661x_close (instrument_Handle)) !=0)
				  
				      //hp661x_error_message (instrument_Handle,errStatus,PS_errorMessage);
				        strcpy(PS_errorMessage,"Failed to close the power supply");
				    break;
 		  
		  		  default:
 
		  			{ strcpy(PS_errorMessage,"Unknown power supper ID");errStatus = VI_ERR_TRUE  ; }
		  	  }
		   
		  }
	return errStatus; 
	}
//-------------------------------------------------------------------------------------------	
PS_FUNCT_PFX PS_SelfTest (int iIndex,ViChar _VI_FAR pszSelfTestMessage[])
	{
	    errStatus=0;
	    strcpy(PS_errorMessage,"No Error.");
	    
	  if (SIM_flag==0)       // It is a real test and not a simulation
	    {
	    	 ViInt16   PS_selfTestResult;
	    	  
	      instrument_Handle = PS_sessionHandle[iIndex];
 	 
	    	 switch (driverType)
		 	{
				case hp663x2:
					
				  if((errStatus=hp663x2_self_test (instrument_Handle,&PS_selfTestResult,pszSelfTestMessage))!=0)
				   
				    //hp663x2_error_message (instrument_Handle,errStatus,PS_errorMessage);
					  strcpy(PS_errorMessage,"Failed to self-test the power supply");
		  			break;
				
				case hp661x:

				  if((errStatus=hp661x_self_test (instrument_Handle,&PS_selfTestResult,pszSelfTestMessage))!=0)
				  
					//hp661x_error_message (instrument_Handle,errStatus,PS_errorMessage);
					 strcpy(PS_errorMessage,"Failed to self-test the power supply"); 
				    break;
		  
		 		default:

		  			 strcpy( PS_errorMessage,"UnknownSpectrum Analyzer");
		  	  }
	 
		  }
		  
	return errStatus;  
	
	}
	

//--------------------------------------------------------------------------------------------
PS_FUNCT_PFX PS_PWOnOff (int iIndex, ViBoolean bPowerSwitch)

	{
	  errStatus=0;
	  strcpy(PS_errorMessage,"No Error.");

		
	  if (SIM_flag==0)       // It is a real test and not a simulation
	    {
	    	 
	      instrument_Handle = PS_sessionHandle[iIndex];
 	    	
		   switch ( driverType)
		 	{
				case hp663x2:
				
				if((errStatus=hp663x2_setOutpState (instrument_Handle,bPowerSwitch, VI_TRUE))!=0)
											//second parameter VI_FALSE (0) power output off
											//VI_TRUE (1) power output on
											// Third parameter for relay on or off. VI_FALSE  means relay off.
		  			 						//hp663x2_error_message (instrument_Handle,errStatus,PS_errorMessage);
		  			  strcpy(PS_errorMessage,"Failed to set on-off of the power supply"); 
		  			break;
				
					
				case hp661x: 		   	 
				
				 if((errStatus=hp661x_setOutpState (instrument_Handle,bPowerSwitch, VI_TRUE))==0)
				 	 
				 	 						//hp661x_error_message (instrument_Handle,errStatus,PS_errorMessage);
				 	  strcpy(PS_errorMessage,"Failed to set on-off of the power supply");
				    break;
		  
		  		default:
		  			
		  			strcpy(PS_errorMessage,"Unknown power supper ID");
		  	  }
		    
		  }
	return errStatus; 
	}
//--------------------------------------------------------------------------------------------- 
//---------------------------------------------------------------------------------------------  

		
		
		//Set out voltage and current level; 
//--------------------------------------------------------------------------------------------- 
//---------------------------------------------------------------------------------------------    
PS_FUNCT_PFX PS_SetVoltCurrLevel (int iIndex, ViReal64 dVoltage, ViReal64 dCurrent)

 	{
	    errStatus=0;
	    strcpy(PS_errorMessage,"No Error.");
 		
	  if (SIM_flag==0)       // It is a real test and not a simulation
	    {
	      instrument_Handle = PS_sessionHandle[iIndex];
 	   		
		   switch ( driverType)
		 				    
		 	{
				case hp663x2:
				
				 {  
				     if((errStatus=hp663x2_outputVoltCurr (instrument_Handle, dVoltage,dCurrent))!=0)
		
					 			//  hp663x2_error_message (instrument_Handle,errStatus,PS_errorMessage);
			            strcpy(PS_errorMessage,"Failed to set voltage and current levels of the power supply");
				  }	
		  			break;
				
				case hp661x:
				
				 	{  
				 		if((errStatus=hp661x_outputVoltCurr(instrument_Handle, dVoltage,dCurrent))!=0)
				 				//	hp661x_error_message (instrument_Handle,errStatus,PS_errorMessage);
						  strcpy(PS_errorMessage,"Failed to set voltage and current levels of the power supply");	 
				     }	
				    
				    	break;
		  
		  		default:
		  			 
		  			{strcpy(PS_errorMessage,"Unknown power supper ID"); errStatus = VI_ERR_TRUE  ; }
		  	  }
		     
		  }
		  
		  Delay (1.0);
		  
	return errStatus; 
	}
//---------------------------------------------------------------------------------------------    
//---------------------------------------------------------------------------------------------    
PS_FUNCT_PFX PS_Measure_Voltage (int iIndex, double *dVoltage)
{
		
	    errStatus=0;
	    strcpy(PS_errorMessage,"No Error.");
		
	  if (SIM_flag==0)       // It is a real test and not a simulation
	    {
	      instrument_Handle = PS_sessionHandle[iIndex];
	   		
		   switch ( driverType)
		 				    
		 	{
				case hp663x2:
				
				   {  												   //0 for DC current
				     if((errStatus= hp663x2_measureVolt(instrument_Handle,0,dVoltage))!=0)
		
					 //	  hp663x2_error_message (instrument_Handle,errStatus,PS_errorMessage);
			            strcpy(PS_errorMessage,"Failed to measure the voltage   of the power supply");
				    }	
		  			  break;
					
				case hp661x:
				
				 	{  
				 		if((errStatus=hp661x_measureVolt ( instrument_Handle, dVoltage))!=0)
				 		//	hp661x_error_message (instrument_Handle,errStatus,PS_errorMessage);
					    strcpy(PS_errorMessage,"Failed to measure the voltage of the power supply");		 
				     }	
				    
				    	break;
		  
		  		default:
		  			 
		  			{
		  			   strcpy(PS_errorMessage,"Unknown power supper ID"); errStatus = VI_ERR_TRUE;
		  			 }
		  	  }
		     
		  }
	    return errStatus; 
	}
	
 
//---------------------------------------------------------------------------------------------    
//---------------------------------------------------------------------------------------------  
PS_FUNCT_PFX PS_Measure_Current (int iIndex, double *dCurrent) 

   {
	  errStatus=0;
	  strcpy(PS_errorMessage,"No Error.");
	    
	  if (SIM_flag==0)       // It is a real test and not a simulation
	    {
	      instrument_Handle = PS_sessionHandle[iIndex];
	   		
		   switch ( driverType)
		 				    
		 	{
				case hp663x2:
				
				   {  												   //0 for DC current
				     if((errStatus= hp663x2_measureCurr(instrument_Handle,0,dCurrent))!=0)
		
					 	 // hp663x2_error_message (instrument_Handle,errStatus,PS_errorMessage);
			         	 strcpy(PS_errorMessage,"Failed to measure the current of the power supply");
				    }	
		  			  break;
				
					
				case hp661x:
				
				 	{  
				 		if((errStatus=hp661x_measureCurr ( instrument_Handle, dCurrent))!=0)
				 		//	hp661x_error_message (instrument_Handle,errStatus,PS_errorMessage);
						strcpy(PS_errorMessage,"Failed to measure the current of the power supply");	 
				     }	
				    
				    	break;
		  
		  		default:
		  			 
		  			{
		  			   strcpy(PS_errorMessage,"Unknown power supper ID"); errStatus = VI_ERR_TRUE;
		  			 }
		  	  }
		     
		  }
	    return errStatus; 
	}
	
 
//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------
     /* I/O port value. Only the least 3 significant bits are used.
		Therefore the output value can range from 0 to 7.

		Program  Bit            Pin
		Value    Configuration  Setting
		         2    1    0    4     3          2         1
		------------------------------------------------------
		0        0    0    0    GND   Output     Lo        Lo
		1        0    0    1    GND   Output     Lo        Hi
		2        0    1    0    GND   Output     Hi        Lo
		3        0    1    1    GND   Output     Hi        Hi
		4        1    0    0    GND   Input      Lo        Lo
		5        1    0    1    GND   Input      Lo        Hi
		6        1    1    0    GND   Input      Hi        Lo
		7        1    1    1    GND   Input      Hi        Hi
	 */
//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------
PS_FUNCT_PFX PS_SetDigitalIO (int iIndex, ViInt16 iDigitalIOValue)

   {
	  errStatus=0;
	  strcpy(PS_errorMessage,"No Error.");

	  if (SIM_flag==0)       // It is a real test and not a simulation
	    {
	      instrument_Handle = PS_sessionHandle[iIndex];
	    	
		   switch ( driverType)
		 	{
				case hp663x2:
				if((errStatus= hp663x2_setDigio (instrument_Handle, iDigitalIOValue))!=0)
					  
								//hp663x2_error_message (instrument_Handle,errStatus,PS_errorMessage);
		  			strcpy(PS_errorMessage,"Failed to set digital I/O output of the power supply");
		  			break;
				
					
				case hp661x:
					if((errStatus= hp661x_setDigio (instrument_Handle, iDigitalIOValue))!=0)
					 
				     			//hp661x_error_message (instrument_Handle,errStatus,PS_errorMessage);
				   	strcpy(PS_errorMessage,"Failed to set digital I/O output of the power supply"); 
			     
				    break;
		  
		  		default:
		  		   {	
		  		     strcpy(PS_errorMessage,"Unknown power supper ID"); errStatus = VI_ERR_TRUE;
		  		    }
		  	  }
		  	   
		  }
		return errStatus; 
	}
	
//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------

PS_FUNCT_PFX PS_GetErrorMsg (char* pszErrMsg)
   
   {
   		strcpy(pszErrMsg,PS_errorMessage);
   
   		return errStatus;
    }
   
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
 
