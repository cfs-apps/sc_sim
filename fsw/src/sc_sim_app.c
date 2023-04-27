/*
**  Copyright 2023 bitValence, Inc.
**  All Rights Reserved.
**
**  This program is free software; you can modify and/or redistribute it
**  under the terms of the GNU Affero General Public License
**  as published by the Free Software Foundation; version 3 with
**  attribution addendums as found in the LICENSE.txt
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU Affero General Public License for more details.
**
** Purpose: Implement the Spacecraft Simulator App
**
** Notes:
**   None
**
** References:
**   1. cFS Basecamp Object-based Application Developer's Guide.
**   2. cFS Application Developer's Guide.
**
*/

/*
** Includes
*/

#include <string.h>
#include "sc_sim_app.h"
#include "sc_sim_eds_cc.h"

/***********************/
/** Macro Definitions **/
/***********************/

/* Convenience macros */
#define  INITBL_OBJ  (&(ScSimApp.IniTbl))
#define  CMDMGR_OBJ  (&(ScSimApp.CmdMgr))
#define  TBLMGR_OBJ  (&(ScSimApp.TblMgr))
#define  SC_SIM      (&(ScSimApp.ScSim))
#define  SC_SIM_TBL  (&(ScSimApp.ScSimTbl))


/*******************************/
/** Local Function Prototypes **/
/*******************************/

static int32 InitApp(void);
static int32 ProcessCommands(void);
static void SendHkTlm(void);

/**********************/
/** File Global Data **/
/**********************/

/* 
** Must match DECLARE ENUM() declaration in app_cfg.h
** Defines "static INILIB_CfgEnum IniCfgEnum"
*/

DEFINE_ENUM(Config,APP_CONFIG)  


/*****************/
/** Global Data **/
/*****************/

SC_SIM_APP_Class_t  ScSimApp;


/******************************************************************************
** Function: SC_SIM_AppMain
**
*/
void SC_SIM_AppMain(void)
{

   uint32  RunStatus = CFE_ES_RunStatus_APP_ERROR;
   
   CFE_EVS_Register(NULL, 0, CFE_EVS_NO_FILTER);

   if (InitApp() == CFE_SUCCESS)      /* Performs initial CFE_ES_PerfLogEntry() call */
   {
      RunStatus = CFE_ES_RunStatus_APP_RUN; 
   }
   
   
   /*
   ** Main process loop
   */
   while (CFE_ES_RunLoop(&RunStatus))
   {

      ProcessCommands();

   } /* End CFE_ES_RunLoop */


   /* Write to system log in case events not working */

   CFE_ES_WriteToSysLog("SC_SIM Terminating, RunLoop status = 0x%08X\n", RunStatus);

   CFE_EVS_SendEvent(SC_SIM_APP_EXIT_EID, CFE_EVS_EventType_CRITICAL, "SC_SIM Terminating,  RunLoop status = 0x%08X", RunStatus);

   CFE_ES_ExitApp(RunStatus);  /* Let cFE kill the task (and any child tasks) */

} /* End of SC_SIM_Main() */


/******************************************************************************
** Function: SC_SIM_NoOpCmd
**
** Function signature must match CMDMGR_CmdFuncPtr typedef 
*/

bool SC_SIM_NoOpCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{

   CFE_EVS_SendEvent (SC_SIM_APP_CMD_NOOP_EID, CFE_EVS_EventType_INFORMATION,
                      "No operation command received for SC_SIM version %d.%d.%d",
                      SC_SIM_MAJOR_VER, SC_SIM_MINOR_VER, SC_SIM_PLATFORM_REV);

   return true;


} /* End SC_SIM_NoOpCmd() */


/******************************************************************************
** Function: SC_SIM_ResetAppCmd
**
** Function signature must match CMDMGR_CmdFuncPtr typedef 
*/

bool SC_SIM_ResetAppCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{

   CMDMGR_ResetStatus(CMDMGR_OBJ);
   TBLMGR_ResetStatus(TBLMGR_OBJ);
   SC_SIM_ResetStatus();

   return true;

} /* End SC_SIM_ResetAppCmd() */


/******************************************************************************
** Function: InitApp
**
*/
static int32 InitApp(void)
{
   
   int32 RetStatus = APP_C_FW_CFS_ERROR;
 
   if (INITBL_Constructor(INITBL_OBJ, SC_SIM_INI_FILENAME, &IniCfgEnum))
   {

      ScSimApp.PerfId  = INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_MAIN_PERF_ID);
      CFE_ES_PerfLogEntry(ScSimApp.PerfId);
      
      ScSimApp.CmdMid     = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_SC_SIM_CMD_TOPICID));
      ScSimApp.ExecuteMid = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_BC_SCH_1_HZ_TOPICID));

      /* Must constructor table manager prior to any app objects that contain tables */
      TBLMGR_Constructor(TBLMGR_OBJ);
      
      SC_SIM_Constructor(SC_SIM, INITBL_OBJ, TBLMGR_OBJ);
      
      /*
      ** Initialize cFE interfaces 
      */

      CFE_SB_CreatePipe(&ScSimApp.CmdPipe, INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_CMD_PIPE_DEPTH), INITBL_GetStrConfig(INITBL_OBJ, CFG_APP_CMD_PIPE_NAME));
      CFE_SB_Subscribe(ScSimApp.CmdMid, ScSimApp.CmdPipe);
      CFE_SB_Subscribe(ScSimApp.ExecuteMid, ScSimApp.CmdPipe);

      /*
      ** Initialize App Framework Components 
      */

      CMDMGR_Constructor(CMDMGR_OBJ);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, SC_SIM_NOOP_CC,  NULL, SC_SIM_NoOpCmd,     0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, SC_SIM_RESET_CC, NULL, SC_SIM_ResetAppCmd, 0);
       
      CMDMGR_RegisterFunc(CMDMGR_OBJ, SC_SIM_LOAD_TBL_CC,  TBLMGR_OBJ, TBLMGR_LoadTblCmd, TBLMGR_LOAD_TBL_CMD_DATA_LEN);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, SC_SIM_DUMP_TBL_CC,  TBLMGR_OBJ, TBLMGR_DumpTblCmd, TBLMGR_DUMP_TBL_CMD_DATA_LEN);
       
      CMDMGR_RegisterFunc(CMDMGR_OBJ, SC_SIM_START_SIM_CC,      SC_SIM, SC_SIM_StartSimCmd,        sizeof(SC_SIM_StartSim_Payload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, SC_SIM_STOP_SIM_CC,       SC_SIM, SC_SIM_StopSimCmd,         0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, SC_SIM_START_PLAYBACK_CC, SC_SIM, SC_SIM_StartPlbkCmd,       0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, SC_SIM_STOP_PLAYBACK_CC,  SC_SIM, SC_SIM_StopPlbkCmd,        0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, SC_SIM_MQTT_JSON_CC,      SC_SIM, SC_SIM_ProcessMqttJsonCmd, sizeof(SC_SIM_MqttJsonCmd_Payload_t));

      CFE_MSG_Init(CFE_MSG_PTR(ScSimApp.HkTlm.TelemetryHeader), 
                   CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_SC_SIM_HK_TLM_TOPICID)),
                   sizeof(SC_SIM_HkTlm_t));
    
      /*
      ** Application startup event message
      */
      CFE_EVS_SendEvent(SC_SIM_APP_INIT_EID, CFE_EVS_EventType_INFORMATION, "SC_SIM App Initialized. Version %d.%d.%d",
                        SC_SIM_MAJOR_VER, SC_SIM_MINOR_VER, SC_SIM_PLATFORM_REV);

      RetStatus = CFE_SUCCESS;

   } /* End if INITBL constructed */

   return(RetStatus);

} /* End of InitApp() */


/******************************************************************************
** Function: ProcessCommands
**
** 
*/
static int32 ProcessCommands(void)
{
   
   int32  RetStatus = CFE_ES_RunStatus_APP_RUN;
   int32  SysStatus;
   CFE_SB_Buffer_t  *SbBufPtr;
   CFE_SB_MsgId_t   MsgId = CFE_SB_INVALID_MSG_ID;

   CFE_ES_PerfLogExit(ScSimApp.PerfId);
   SysStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, ScSimApp.CmdPipe, CFE_SB_PEND_FOREVER);
   CFE_ES_PerfLogEntry(ScSimApp.PerfId);

   if (SysStatus == CFE_SUCCESS)
   {
      SysStatus = CFE_MSG_GetMsgId(&SbBufPtr->Msg, &MsgId);

      if (SysStatus == CFE_SUCCESS)
      {

         if (CFE_SB_MsgId_Equal(MsgId, ScSimApp.CmdMid))
         {
            CMDMGR_DispatchFunc(CMDMGR_OBJ, &SbBufPtr->Msg);
         } 
         else if (CFE_SB_MsgId_Equal(MsgId, ScSimApp.ExecuteMid))
         {
            SC_SIM_Execute();
            SendHkTlm();
         }
         else
         {   
            CFE_EVS_SendEvent(SC_SIM_APP_CMD_INVALID_MID_EID, CFE_EVS_EventType_ERROR,
                              "Received invalid command packet, MID = 0x%04X", 
                              CFE_SB_MsgIdToValue(MsgId));
         }

      } /* End if got message ID */
   } /* End if received buffer */
   else
   {
      RetStatus = CFE_ES_RunStatus_APP_ERROR;
   } 

   return RetStatus;
   
} /* End ProcessCommands() */


/******************************************************************************
** Function: SendHkTlm
**
*/
static void SendHkTlm(void)
{

   /* Good design practice in case app expands to more than one table */
   const TBLMGR_Tbl_t *LastTbl = TBLMGR_GetLastTblStatus(TBLMGR_OBJ);

   SC_SIM_HkTlm_Payload_t *Payload = &ScSimApp.HkTlm.Payload;
   
   /*
   ** CMDMGR Data
   */

   Payload->ValidCmdCnt   = ScSimApp.CmdMgr.ValidCmdCnt;
   Payload->InvalidCmdCnt = ScSimApp.CmdMgr.InvalidCmdCnt;

   
   /*
   ** SC_SIM/SC_SIMTBL Data
   ** - At a minimum all contained object variables effected by a reset
   **   Should be included
   */

   Payload->LastTblAction  = LastTbl->LastAction;
   Payload->SimTblLoaded   = ScSimApp.ScSim.Tbl.Loaded;
   
   /*
   ** Spacecraft Simulator 
   */
   
   Payload->SimActive = ScSimApp.ScSim.Active;
   Payload->SimPhase  = ScSimApp.ScSim.Phase;
   Payload->SimTime   = ScSimApp.ScSim.Time.Seconds;
   
   CFE_SB_TimeStampMsg(CFE_MSG_PTR(ScSimApp.HkTlm.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(ScSimApp.HkTlm.TelemetryHeader), true);

} /* End SendHkTlm() */
