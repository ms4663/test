#include <utility.h>
#include <ansi_c.h>
#include <formatio.h>
#include <userint.h>
#include <rs232.h>
#include "PS_PowerSupply.h"
#include "hp663x2.h"
#define COM 5
#define pass 1
#define fail 0
#define PASS 1
#define FAIL 0
#define YES 1
#define NO 0
#define CLOSE				1
#define OPEN				0
#define CR					0x0D
#define LF					0x0A
#define NO_RESPONSE			0
#define GET_RESPONSE		1
#define CR					0x0D
#define LF					0x0A
#define ON 1
#define OFF 0
#define POWERTOOLS_PATH  "C:\\mtp\\Site_Ne_sch\\Projects\\Darter\\powertools"
char rbuffer[250];  
char wbuffer[250];
int status;

__declspec(dllexport)int Initialize_Comport(int port, int baud); 
__declspec(dllexport)int iocontrol (int port, char String[1024], int byte_position, int byte_size, int wait_response, char response[1024], char hex_response[1024]);
__declspec(dllexport)int iocontrol_AD_READ(int port, char String[1024], int byte_position, int byte_size, int wait_response, char response[1024], char hex_response[1024]); 
__declspec(dllexport)int Close_Comport(int port);
__declspec(dllexport)int PowerSupplyInit(int index, int GPIB_Port, char Model[1024],int Flag);
__declspec(dllexport)int PowerSupplyReset(int index);
__declspec(dllexport)int PowerSupplyVolatgeCurrent(int index, int Voltage, int Current);
__declspec(dllexport)int PowerSupplyMeasure_Current(int index, double *Current);
__declspec(dllexport)int PowerSupplyON_OFF(int index, char State[1024]);
int LOAD_APP(char Folder[],char File[],char PCPath[],char tempPath[]);
int RUN_APP(char Folder[],char File[],char Param[],char tempPath[]);
int RUN_APP_NW(char Folder[],char File[],char Param[],char tempPath[]);  
int GET_APP(char Folder[],char File[],char PCPath[],char tempPath[]); 
int GetFileData(char File[],char PCPath[],char param[]); 
int ActiveSyncDetect(char tempPath[]);

//-----------------------------------------------------------------------------
// Delay - processes system events while performing delay
//-----------------------------------------------------------------------------
void DelayTime(double dTime)
{	double tmr = Timer();
	while ( (Timer()-tmr) < dTime ){
		ProcessSystemEvents();
		}
}

//-----------------------------------------------------------------------------
// Clear Comport - processes system events while performing delay
//-----------------------------------------------------------------------------

void ClearComport(int port)
			 {
			 FlushInQ (port);
			 FlushOutQ (port);
			 DelayTime(.1);
			 return;
			 }

int Initialize_Comport(int port, int baud)
		  {
		  int STATUS;
		  STATUS = OpenComConfig (port, "", baud, 0, 8, 1, 512, 512);
		  if (STATUS != 0){
		  	return fail;
		  	}
		  
//		  ComSetEscape (port, 4);						// Clear RTS 
		  
		  ClearComport(port);
		  SetComTime (port, 5);
  		  DelayTime(0.1);
		  return pass;
		  }

 int Close_Comport(int port)
{
CloseCom(port);
return pass;
}
int PowerSupplyInit(int index, int GPIB_Port, char Model[1024],int Flag){
	if (PS_Init ( index,GPIB_Port,Model,Flag)==0)
		return pass;
	else return fail;
//	status = PS_Init ( 0,5,"66309B",0);	
//	status = PS_Reset (0);
//	status = PS_SetVoltCurrLevel (0, 12, 3);    
//	status = PS_PWOnOff (0, ON);
//	status = PS_PWOnOff (0, OFF);   
//	status = PS_SetVoltCurrLevel (0, 5.8, 3);  		// set vcc to 6V cause uut to suspend 
//	status = PS_Measure_Current (0,&CheckCurr); 
}

int PowerSupplyReset(int index){
	if (PS_Reset ( index)==0)
		return pass;
	else return fail;
//	status = PS_Init ( 0,5,"66309B",0);	
//	status = PS_Reset (0);
//	status = PS_SetVoltCurrLevel (0, 12, 3);    
//	status = PS_PWOnOff (0, ON);
//	status = PS_PWOnOff (0, OFF);   
//	status = PS_SetVoltCurrLevel (0, 5.8, 3);  		// set vcc to 6V cause uut to suspend 
//	status = PS_Measure_Current (0,&CheckCurr); 
}


int PowerSupplyVolatgeCurrent(int index, int Voltage, int Current){
	if (PS_SetVoltCurrLevel ( index, Voltage, Current)==0)
		return pass;
	else return fail;
//	status = PS_Init ( 0,5,"66309B",0);	
//	status = PS_Reset (0);
//	status = PS_SetVoltCurrLevel (0, 12, 3);    
//	status = PS_PWOnOff (0, ON);
//	status = PS_PWOnOff (0, OFF);   
//	status = PS_SetVoltCurrLevel (0, 5.8, 3);  		// set vcc to 6V cause uut to suspend 
//	status = PS_Measure_Current (0,&CheckCurr); 
}
int PowerSupplyMeasure_Current(int index, double *Current){
	if (PS_Measure_Current ( index, Current)==0)
		return pass;
	else return fail;
//	status = PS_Init ( 0,5,"66309B",0);	
//	status = PS_Reset (0);
//	status = PS_SetVoltCurrLevel (0, 12, 3);    
//	status = PS_PWOnOff (0, ON);
//	status = PS_PWOnOff (0, OFF);   
//	status = PS_SetVoltCurrLevel (0, 5.8, 3);  		// set vcc to 6V cause uut to suspend 
//	status = PS_Measure_Current (0,&CheckCurr); 
}

int PowerSupplyON_OFF(int index, char State[1024]){
	if ((FindPattern (State, 0, -1, "ON", 0, 0)!=-1)){
		if (PS_PWOnOff  ( index, ON)==0)
			return pass;
	}
	if (PS_PWOnOff  ( index, OFF)==0)
			return pass;
	else return fail;
//	status = PS_Init ( 0,5,"66309B",0);	
//	status = PS_Reset (0);
//	status = PS_SetVoltCurrLevel (0, 12, 3);    
//	status = PS_PWOnOff (0, ON);
//	status = PS_PWOnOff (0, OFF);   
//	status = PS_SetVoltCurrLevel (0, 5.8, 3);  		// set vcc to 6V cause uut to suspend 
//	status = PS_Measure_Current (0,&CheckCurr); 
}

int iocontrol (int port, char String[1024], int byte_position, int byte_size, int wait_response, char response[1024], char hex_response[1024])
{
	int status = 0, response_length = 0, i = 0;	
	int xmit_length = 0;
	unsigned short xmit_checksum =0, act_checksum =0, checksum_lb=0, checksum_hb=0;
	char xmit_buffer_str[768];
	char *pstring, tstring[50],FATCOMMAND[50];
	int strlength, counter=0,counter_old=0;
	int position =0,pos;
	char temp[5];
    char error_string[256];
	
	unsigned char MIMIC_command[256];
	unsigned char MIMIC_response[1024];

	int inQlen = 0; 
	memset(MIMIC_command,0x00,sizeof(MIMIC_command));
	memset(MIMIC_response,0x00,sizeof(MIMIC_response)); 
	memset (response, 0x00, 1024);
	memset (hex_response, 0x00, 1024);  
	ClearComport(port);
//	atoi()
//	FindPattern (COM_BYTE_READ, 10, -1, "PL4500", 0, 0); 
	strlength = StringLength(String);
	memset (FATCOMMAND, 0x00, sizeof(FATCOMMAND)); 
	   for (pos=0;pos<strlength;pos++){
		   if (String[pos]=='0') 
			   String[pos]='m';
		}
		
	while((counter<=strlength) && (counter !=-1)){
		counter = FindPattern (String, counter_old+1, -1, ",", 0, 0); 
		
		if ((counter ==-1) )
			break;
		CopyString (tstring, 0, String, counter_old, counter-counter_old);
		
	   for (pos=0;pos<counter-counter_old;pos++){
		   if (tstring[pos]=='m') 
			   tstring[pos]='0';
		}
		counter_old = counter+1;
		MIMIC_command[position] = strtol (tstring,(char **) NULL , 16);
		position++;
		}
	status=ComWrt (port,MIMIC_command, strlength);
//	status = XmitMIMICCommand(port,MIMIC_command, error_string);
	DelayTime(wait_response);  
////////////////////////////////////////////////////////////////////////////////////	
		for (counter=0;counter<=4;counter++){ //wait up to .4 seconds for responce
			inQlen = GetInQLen(port); 
			if(inQlen>1)
				break;
			DelayTime(0.1); 
			}
	if(inQlen<1)
		 return fail;
	
	inQlen = GetInQLen(port);
	if(inQlen>1024){
		strcpy(error_string,"Com data larger than 1024 bits");
		return fail;
		}
////////////////////////////////////////////////////////////////////////////////////	
	
	
	status = ComRd(port, MIMIC_response, inQlen);
	if(MIMIC_response[0]==0x55 || MIMIC_response[0]==0x01|| MIMIC_response[0]==0x00){
			response_length = inQlen;
		for (i = 0; i < inQlen; i++) {
				response[i] = MIMIC_response[i];
				Fmt (temp, "%s<%x", MIMIC_response[i]);
				if(MIMIC_response[i+byte_position]<=0xf)	 //new r7
					strcat(hex_response,"0");				 //new r7
				strcat(hex_response,temp); strcat(hex_response,",");	 //new r7 
				}
			return pass;
			}
		
	else return fail;
}



int iocontrol_AD_READ(int port, char String[1024], int byte_position, int byte_size, int wait_response, char response[1024], char hex_response[1024])
{
	int status = 0, response_length = 0, i = 0;	
	int xmit_length = 0;
	unsigned short xmit_checksum =0, act_checksum =0, checksum_lb=0, checksum_hb=0;
	char xmit_buffer_str[768];
	char *pstring, tstring[50],FATCOMMAND[50];
	int strlength, counter=0,counter_old=0;
	int position =0,pos;
	char temp[5];
    char error_string[256];
	
	unsigned char MIMIC_command[256];
	unsigned char MIMIC_response[1024];

	int inQlen = 0; 
	memset(MIMIC_command,0x00,sizeof(MIMIC_command));
	memset(MIMIC_response,0x00,sizeof(MIMIC_response)); 
	memset (response, 0x00, 1024);
	memset (hex_response, 0x00, 1024);  
	ClearComport(port);
//	atoi()
//	FindPattern (COM_BYTE_READ, 10, -1, "PL4500", 0, 0); 
	strlength = StringLength(String);
	memset (FATCOMMAND, 0x00, sizeof(FATCOMMAND)); 
	   for (pos=0;pos<strlength;pos++){
		   if (String[pos]=='0') 
			   String[pos]='m';
		}
		
	while((counter<=strlength) && (counter !=-1)){
		counter = FindPattern (String, counter_old+1, -1, ",", 0, 0); 
		
		if ((counter ==-1) )
			break;
		CopyString (tstring, 0, String, counter_old, counter-counter_old);
		
	   for (pos=0;pos<counter-counter_old;pos++){
		   if (tstring[pos]=='m') 
			   tstring[pos]='0';
		}
		counter_old = counter+1;
		MIMIC_command[position] = strtol (tstring,(char **) NULL , 16);
		position++;
		}
	status=ComWrt (port,MIMIC_command, strlength);
//	status = XmitMIMICCommand(port,MIMIC_command, error_string);
	DelayTime(wait_response);  
////////////////////////////////////////////////////////////////////////////////////	
		for (counter=0;counter<=4;counter++){ //wait up to .4 seconds for responce
			inQlen = GetInQLen(port); 
			if(inQlen>1)
				break;
			DelayTime(0.1); 
			}
	if(inQlen<1)
		 return fail;
	
	inQlen = GetInQLen(port);
	if(inQlen>1024){
		strcpy(error_string,"Com data larger than 1024 bits");
		return fail;
		}
////////////////////////////////////////////////////////////////////////////////////	
	
	
	status = ComRd(port, MIMIC_response, inQlen);
//	if(MIMIC_response[0]==0x55 || MIMIC_response[0]==0x01|| MIMIC_response[0]==0x00){
			response_length = inQlen;
		for (i = 0; i < inQlen; i++) {
				response[i] = MIMIC_response[i];
				Fmt (temp, "%s<%x", MIMIC_response[i]);
				if(MIMIC_response[i+byte_position]<=0xf)	 //new r7
					strcat(hex_response,"0");				 //new r7
				strcat(hex_response,temp); strcat(hex_response,",");	 //new r7 
				}
			return pass;
		//	}
		
//	else return fail;
}



//-----------------------------------------------------------------------------
// Copy Application via USB to Mobile device
//-----------------------------------------------------------------------------
int LOAD_APP(char Folder[],char File[],char PCPath[],char tempPath[])
{
	char modMsg[512],Ctemp[255];
	int sleepPolicy;
	static int detected = NO;
	int ExHandle = 0;
	int retry=0, osType= NULL;
	int FileHandle,FileHandle1;
	int i = 0;
	char FileBuffer[5000],tempbuf[512];
	int locate1,locate2,locate3,locate4,locate5,locate6; 
	memset(Ctemp,0x00,sizeof(Ctemp));
//	memset(UUT_TEST_DATA[UUT_N].ComData,0x00,sizeof(UUT_TEST_DATA[UUT_N].ComData));	 
//	memset(UUT_TEST_DATA[UUT_N].Test_Result,0x00,sizeof(UUT_TEST_DATA[UUT_N].Test_Result)); 
//	memset(UUT_TEST_DATA[UUT_N].Exp_Test_Result,0x00,sizeof(UUT_TEST_DATA[UUT_N].Exp_Test_Result)); 
//	memset(UUT_TEST_DATA[UUT_N].Current_Test,0x00,sizeof(UUT_TEST_DATA[UUT_N].Current_Test)); 

while((retry <1) ){//&&	(exiting != YES || StopTesting != YES)){
	sprintf(modMsg,"%s%s%s","Loading[",File,"]"); 
	
	// COPY FILE TO UUT
	sprintf(modMsg,"cmd /c %s\\cecopy /s desk:\"%s\\%s\" dev:\"%s\\%s\" > \"%s\\test0.txt\"",POWERTOOLS_PATH,PCPath,File,Folder,File,tempPath); 				
	LaunchExecutableEx (modMsg, LE_HIDE, &ExHandle);
					
	while(ExecutableHasTerminated(ExHandle)==0){
			DelayTime(0.1); 
			}
	RetireExecutableHandle(ExHandle);
	DelayTime(0.1); 
	memset(modMsg,0x00,sizeof(modMsg));
	sprintf(modMsg,"%s\\test0.txt",tempPath); 				
	FileHandle = OpenFile (modMsg, VAL_READ_ONLY, VAL_OPEN_AS_IS, VAL_ASCII);
	if (FileHandle <0){
		sprintf(modMsg,"Failed to open file[test0.txt]\nPlease Check ActiveSync connection."); 
		MessagePopup("OPERATOR ALERT",modMsg);
	//	return FAIL;
		retry++; continue; 
		}
	memset(FileBuffer,0x00,sizeof(FileBuffer));
	while(ReadLine(FileHandle,FileBuffer,(sizeof(FileBuffer)-1))!=-2){
		//	if (exiting == YES || StopTesting == YES){ 
		//		CloseFile(FileHandle); 
		//		return FAIL;
		//		}
			if((FindPattern(FileBuffer,0,-1,"ok",0,0)>=0) && (FindPattern(FileBuffer,0,-1,File,0,0)>=0)){
					CloseFile(FileHandle); 
					return PASS;
					}
		}
	
	CloseFile(FileHandle); 
	retry++; continue; 
	}//end retry while loop.
	sprintf(modMsg,"Failed to copy File[%s]to UUT\n Please Check ActiveSync connection.",File ); 
	MessagePopup("OPERATOR ALERT",modMsg);
	
	return FAIL;
}




//-----------------------------------------------------------------------------
// Launch_ Application via USB on Mobile device
//-----------------------------------------------------------------------------
int RUN_APP(char Folder[],char File[],char Param[],char tempPath[])
{
	char modMsg[512],Ctemp[512];
	int sleepPolicy;
	static int detected = NO;
	int ExHandle = 0;
	int retry=0, osType= NULL;
	int FileHandle,FileHandle1;
	int i = 0;
	char FileBuffer[512],tempbuf[512];
	int locate1,locate2,locate3,locate4,locate5,locate6; 
	
while((retry <1)){// &&	(exiting != YES || StopTesting != YES))
	 
			memset(Ctemp,0x00,sizeof(Ctemp));
			sprintf(modMsg,"%s%s%s","Executing[",File,"]"); 

	
			// EXECUTE PROGRAM ON UUT 
			memset(modMsg,0x00,sizeof(modMsg));
			sprintf(modMsg,"cmd /c %s\\cerun.exe CE:\"%s\\%s\" \"%s\" > \"%s\\test0.txt\"",POWERTOOLS_PATH,Folder,File,Param,tempPath);
		//	sprintf(modMsg,"cmd /c %s\\rapistart \"%s\\%s\" \"%s\" > \"%s\\test0.txt\"",POWERTOOLS_PATH,Folder,File,Param,tempPath); 
			LaunchExecutableEx (modMsg, LE_HIDE, &ExHandle);
			while(ExecutableHasTerminated(ExHandle)==0){
						DelayTime(0.1);
						}
			RetireExecutableHandle(ExHandle);
			DelayTime(0.1);
			return PASS;
			/*
			memset(modMsg,0x00,sizeof(modMsg));
			sprintf(modMsg,"%s\\test0.txt",tempPath); 				
			FileHandle = OpenFile (modMsg, VAL_READ_ONLY, VAL_OPEN_AS_IS, VAL_ASCII);
			if (FileHandle <0){
				sprintf(modMsg,"Failed to open file[test0.txt]"); 
				MessagePopup("OPERATOR ALERT",modMsg);
				retry++; continue; 
				}
			memset(FileBuffer,0x00,sizeof(FileBuffer));
			while(ReadLine(FileHandle,FileBuffer,(sizeof(FileBuffer)-1))!=-2){
			//		if (exiting == YES || StopTesting == YES){ 
			//			CloseFile(FileHandle); 
			//			return FAIL;
			//			}
					if((FindPattern(FileBuffer,0,-1,"Launched command",0,0)>=0) && (FindPattern(FileBuffer,0,-1,File,0,0)>=0)){
							CloseFile(FileHandle); 
							return PASS;
					}
		}						
		CloseFile(FileHandle);
		*/
		retry++; continue; 
	}//end retry while loop. 
	sprintf(modMsg,"Failed Launch UUT file[%s]\n Please Check ActiveSync connection.",File ); 
	MessagePopup("OPERATOR ALERT",modMsg);
return FAIL;
}


//-----------------------------------------------------------------------------
// Launch_ Application via USB on Mobile device
//-----------------------------------------------------------------------------
int RUN_APP_NW(char Folder[],char File[],char Param[],char tempPath[])
{
	char modMsg[512],Ctemp[512];
	int sleepPolicy;
	static int detected = NO;
	int ExHandle = 0;
	int retry=0, osType= NULL;
	int FileHandle,FileHandle1;
	int i = 0;
	char FileBuffer[512],tempbuf[512];
	int locate1,locate2,locate3,locate4,locate5,locate6; 
	
while((retry <1)){// &&	(exiting != YES || StopTesting != YES))
	 
			memset(Ctemp,0x00,sizeof(Ctemp));
			sprintf(modMsg,"%s%s%s","Executing[",File,"]"); 

	
			// EXECUTE PROGRAM ON UUT 
			memset(modMsg,0x00,sizeof(modMsg));
		//	sprintf(modMsg,"cmd /c %s\\cerun.exe -b CE:\"%s\\%s\" \"%s\" > \"%s\\test0.txt\"",POWERTOOLS_PATH,Folder,File,Param,tempPath);
			sprintf(modMsg,"cmd /c %s\\rapistart.exe \"%s\\%s\" \"%s\" > \"%s\\test0.txt\"",POWERTOOLS_PATH,Folder,File,Param,tempPath); 
			LaunchExecutableEx (modMsg, LE_HIDE, &ExHandle);
		//	while(ExecutableHasTerminated(ExHandle)==0){
						DelayTime(1);
		//				}
			RetireExecutableHandle(ExHandle);
			DelayTime(0.1);
			return PASS;
			/*
			memset(modMsg,0x00,sizeof(modMsg));
			sprintf(modMsg,"%s\\test0.txt",tempPath); 				
			FileHandle = OpenFile (modMsg, VAL_READ_ONLY, VAL_OPEN_AS_IS, VAL_ASCII);
			if (FileHandle <0){
				sprintf(modMsg,"Failed to open file[test0.txt]"); 
				MessagePopup("OPERATOR ALERT",modMsg);
				retry++; continue; 
				}
			memset(FileBuffer,0x00,sizeof(FileBuffer));
			while(ReadLine(FileHandle,FileBuffer,(sizeof(FileBuffer)-1))!=-2){
			//		if (exiting == YES || StopTesting == YES){ 
			//			CloseFile(FileHandle); 
			//			return FAIL;
			//			}
					if((FindPattern(FileBuffer,0,-1,"Launched command",0,0)>=0) && (FindPattern(FileBuffer,0,-1,File,0,0)>=0)){
							CloseFile(FileHandle); 
							return PASS;
					}
		}						
		CloseFile(FileHandle);
		*/
		retry++; continue; 
	}//end retry while loop. 
	sprintf(modMsg,"Failed Launch UUT file[%s]\n Please Check ActiveSync connection.",File ); 
	MessagePopup("OPERATOR ALERT",modMsg);
return FAIL;
}

//-----------------------------------------------------------------------------
// Get File/Data via USB from Mobile device
//-----------------------------------------------------------------------------
int GET_APP(char Folder[],char File[],char PCPath[],char tempPath[])
{
	char modMsg[512],Ctemp[512];
	int sleepPolicy;
	static int detected = NO;
	int ExHandle = 0;
	int retry=0, osType= NULL;
	int FileHandle,FileHandle1;
	int i = 0;
	char FileBuffer[512],tempbuf[512];
	int locate1,locate2,locate3,locate4,locate5,locate6;
	double InUutFUS,InTableFUS;
	memset(modMsg,0x00,sizeof(modMsg));
	sprintf(modMsg,"%s\\%s",PCPath,File);
		DeleteFile(modMsg);
	
while((retry <1)){// &&	(exiting != YES || StopTesting != YES)){  
	memset(Ctemp,0x00,sizeof(Ctemp));
	
	sprintf(modMsg,"%s%s%s","Retriving[",File,"]"); 
	// EXECUTE PROGRAM ON UUT 
		memset(modMsg,0x00,sizeof(modMsg));
		if(StringLength(Folder)<=1)
			sprintf(modMsg,"cmd /c %s\\cecopy.exe /is dev:\"%s\" desk:\"%s\\%s\" > \"%s\\test0.txt\"",POWERTOOLS_PATH,File,PCPath,File,tempPath);	
		else sprintf(modMsg,"cmd /c %s\\cecopy.exe /is dev:\"%s\\%s\" desk:\"%s\\%s\" > \"%s\\test0.txt\"",POWERTOOLS_PATH,Folder,File,PCPath,File,tempPath); 
		//cecopy.exe /is dev:"\Cache Disk\LanProfile.txt" desk:"C:\Software Development\MC95\LanProfile.txt"  
		LaunchExecutableEx (modMsg, LE_HIDE, &ExHandle);
					while(ExecutableHasTerminated(ExHandle)==0){
						DelayTime(0.1);
						}
		RetireExecutableHandle(ExHandle);
		DelayTime(0.5);
		memset(modMsg,0x00,sizeof(modMsg));
	sprintf(modMsg,"%s\\test0.txt",tempPath); 				
	FileHandle = OpenFile (modMsg, VAL_READ_ONLY, VAL_OPEN_AS_IS, VAL_ASCII);
	if (FileHandle <0){
		sprintf(modMsg,"Failed to open file[test0.txt]\nPlease Check ActiveSync connection."); 
		MessagePopup("OPERATOR ALERT",modMsg);
		//return FAIL;
		retry++; 
		continue;
		}
	memset(FileBuffer,0x00,sizeof(FileBuffer));
	while(ReadLine(FileHandle,FileBuffer,(sizeof(FileBuffer)-1))!=-2){
		//	if (exiting == YES || StopTesting == YES){ 
		//		CloseFile(FileHandle); 
		//		return FAIL;
		//		}
			if((FindPattern(FileBuffer,0,-1,"ok",0,0)>=0) && (FindPattern(FileBuffer,0,-1,File,0,0)>=0)){
					CloseFile(FileHandle); 
					return PASS;
					}
		}						
	CloseFile(FileHandle); 
	DelayTime (1);
	retry++; continue;
	}//end retry while loop.
	sprintf(modMsg,"Failed to Retriving data[%s]\nPlease Check ActiveSync connection.",File ); 
	MessagePopup("OPERATOR ALERT",modMsg);
return FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
int GetFileData(char File[],char PCPath[],char param[])
{
	char modMsg[512],Ctemp[512];
	int sleepPolicy;
	static int detected = NO;
	int ExHandle = 0;
	int retry=0, osType= NULL;
	int FileHandle,FileHandle1;
	int i = 0;
	char FileBuffer[512],tempbuf[512];
	int locate1,locate2,locate3,locate4,locate5,locate6;
	double InUutFUS,InTableFUS;
	
while((retry <1)){  
	memset(Ctemp,0x00,sizeof(Ctemp));
	memset(param,0x00,sizeof(param));
	memset(modMsg,0x00,sizeof(modMsg));
	sprintf(modMsg,"%s\\%s",PCPath,File); 				
	FileHandle = OpenFile (modMsg, VAL_READ_ONLY, VAL_OPEN_AS_IS, VAL_ASCII);
	if (FileHandle <0){
		sprintf(modMsg,"Failed to open file[%s]\n Please Check ActiveSync connection.",File); 
		MessagePopup("OPERATOR ALERT",modMsg);
		retry++; continue; 
		
		}
	memset(FileBuffer,0x00,sizeof(FileBuffer));
	while(ReadLine(FileHandle,FileBuffer,(sizeof(FileBuffer)-1))!=-2){
			strcat(param,FileBuffer);
			strcat(param,"\n");
			}						
	CloseFile(FileHandle); 
	
	if(StringLength(param)>1){
		return PASS;
		}
	retry++; continue; 
	}//end retry while loop. 
	sprintf(modMsg,"Failed to Retriving data[%s]\nPlease Check ActiveSync connection.",File ); 
	MessagePopup("OPERATOR ALERT",modMsg);
return FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//Detecting Active SYnc Connection for WM device
int ActiveSyncDetect(char tempPath[])
{
	char modMsg[512],Ctemp[512];
	int sleepPolicy;
	static int detected = NO;
	int ExHandle = 0;
	int retry=0, osType= NULL;
	int FileHandle,FileHandle1;
	int i = 0;
	char FileBuffer[512],tempbuf[512];
	int locate1,locate2,locate3,locate4,locate5,locate6;
	double InUutFUS,InTableFUS;
	
while((retry <1)){// &&	(exiting != YES || StopTesting != YES)){  
	memset(Ctemp,0x00,sizeof(Ctemp));
	
	sprintf(modMsg,"Detecting ActiveSync Connection for WM device"); 
	// EXECUTE PROGRAM ON UUT 
		
		sprintf(modMsg,"cmd /c route print > \"%s\\test0.txt\"",tempPath); 
		 
		LaunchExecutableEx (modMsg, LE_HIDE, &ExHandle);
		DelayTime(1);
		RetireExecutableHandle(ExHandle);
		memset(modMsg,0x00,sizeof(modMsg));
	sprintf(modMsg,"%s\\test0.txt",tempPath); 				
	FileHandle = OpenFile (modMsg, VAL_READ_ONLY, VAL_OPEN_AS_IS, VAL_ASCII);
	if (FileHandle <0){
		sprintf(modMsg,"Failed to open file[test0.txt]\nPlease Check ActiveSync connection."); 
		MessagePopup("OPERATOR ALERT",modMsg);
		//return FAIL;
		retry++; 
		continue;
		}
	memset(FileBuffer,0x00,sizeof(FileBuffer));
	while(ReadLine(FileHandle,FileBuffer,(sizeof(FileBuffer)-1))!=-2){
		//	if (exiting == YES || StopTesting == YES){ 
		//		CloseFile(FileHandle); 
		//		return FAIL;
		//		}
			if((FindPattern(FileBuffer,0,-1,"Windows Mobile-based Device",0,0)>=0)){
					CloseFile(FileHandle); 
					return PASS;
					}
		}						
	CloseFile(FileHandle); 
	DelayTime (1);
	retry++; continue;
	}//end retry while loop.
	sprintf(modMsg,"Failed to Detecting ActiveSync Connection\n Please Check Cradle Contacts then\nre-install UUT into Test Cradle to Continue Test" ); 
	MessagePopup("OPERATOR ALERT",modMsg);
return FAIL;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void main(){
int port =5;
char response[1024];
char error_string[1024];
double CheckCurr=0;
wbuffer[0] = 0xFE;
//wbuffer[1] = 0x20;
wbuffer[1] = 0x82;
//wbuffer[3] = 0x20;
wbuffer[2] = 0x01;
wbuffer[3] = 0x00;

status=Initialize_Comport(port, 115200); 	
//status=OpenComConfig (COM, "COM5", 115200, 0, 8, 1, 512, 512);  
status=SetComTime (port, 5.0);
status=FlushInQ (port);
status=FlushOutQ (port); 
/*//ComWrtByte (2, 0xFE);
//ComWrtByte (2, 0x81);  
//ComWrtByte (2, 0x01);  
	status = PS_Init ( 0,5,"66309B",0);	
	status = PS_Reset (0);
	status = PS_SetVoltCurrLevel (0, 12, 3);    
	status = PS_PWOnOff (0, ON);
	DelayTime(2);
	status = PS_Measure_Current (0,&CheckCurr);
	
*/
	
status=iocontrol(port, "0xFE,0x82,0x01,",0,0, 0.5, response, error_string); 

status=iocontrol(port, "0xFE,0x81,0x01,",0,0, 0.5, response, error_string);   

status=iocontrol_AD_READ(port, "0xFE,0x96,0x01,",0,0, 0.5, response, error_string);    
DelayTime(2);
/*
status = GET_APP("\\temp","ConfigTest.txt","C:\\mtp\\Site_Ne_sch\\Projects\\Darter\\RESULTS","C:\\mtp\\Site_Ne_sch\\Projects\\Darter\\RESULTS");
status = ActiveSyncDetect("C:\\mtp\\Site_Ne_sch\\Projects\\Darter\\RESULTS");  
status = LOAD_APP("\\temp","DeleteFile.exe","C:\\mtp\\Site_Ne_sch\\Projects\\Darter\\powertools","C:\\mtp\\Site_Ne_sch\\Projects\\Darter\\RESULTS");
status = RUN_APP("\\temp","DeleteFile.exe","\\temp\\ConfigTest.txt","C:\\mtp\\Site_Ne_sch\\Projects\\Darter\\RESULTS");

status = LOAD_APP("\\temp\\CONFIG","TechnologySolutions.Rfid.Agouti.Commands.dll","C:\\mtp\\Site_Ne_sch\\Projects\\Darter\\CONFIG","C:\\mtp\\Site_Ne_sch\\Projects\\Darter\\RESULTS"); 
status = LOAD_APP("\\temp\\CONFIG","ConfigTest.exe","C:\\mtp\\Site_Ne_sch\\Projects\\Darter\\CONFIG","C:\\mtp\\Site_Ne_sch\\Projects\\Darter\\RESULTS");  
status = RUN_APP("\\temp\\CONFIG","ConfigTest.exe","","C:\\mtp\\Site_Ne_sch\\Projects\\Darter\\RESULTS");

status = RUN_APP_NW("\\temp\\WARMBOOT","WARMBOOT.exe","","C:\\mtp\\Site_Ne_sch\\Projects\\Darter\\RESULTS");   

status = GET_APP("\\temp","ConfigTest.txt","C:\\mtp\\Site_Ne_sch\\Projects\\Darter\\RESULTS","C:\\mtp\\Site_Ne_sch\\Projects\\Darter\\RESULTS"); 
status = GetFileData("ConfigTest.txt","C:\\mtp\\Site_Ne_sch\\Projects\\Darter\\RESULTS",response) ;
*/
status=CloseCom (port);

}
