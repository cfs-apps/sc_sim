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
** Purpose: Implement the spacecraft simulator.
**
** Notes:
**   1. Information events are used in order to trace execution for
**      demonstrations.
**   2. For the initial versions of the subsystem simulations I broke from
**      convention of defining one object per file. The simulations are
**      trivial and tightly coupled for hard coded scenarios.
**   3. TODO - Add sanity and some error checks after initial version. 
**      There are lots of assumptions. This is not flight code and its 
**      purely for demonstration so it doesn't have to be bullet proof
**      but some checks will help especially in SC_SIM_Execute() and
**      SIM_ExecuteEventCmd().
**
** References:
**   1. cFS Basecamp Object-based Application Developers Guide.
**   2. cFS Application Developer's Guide.
*/

/*
** Include Files:
*/

#include <string.h>

#include "cfe_mission_eds_designparameters.h"
#include "cfe_evs_eds_cc.h"
#include "cfe_evs_eds_typedefs.h"
#include "cfe_time_eds_cc.h"
#include "cfe_time_eds_typedefs.h"
#include "kit_to_eds_cc.h"
#include "kit_to_eds_typedefs.h"

#include "sc_sim.h"


/***********************/
/** Macro Definitions **/
/***********************/

/* Convenience Macros */

#define  ADCS  (&(ScSim->Adcs))
#define  CDH   (&(ScSim->Cdh))
#define  COMM  (&(ScSim->Comm))
#define  FSW   (&(ScSim->Fsw))
#define  INSTR (&(ScSim->Instr))
#define  POWER (&(ScSim->Power))
#define  THERM (&(ScSim->Therm))


/**********************/
/** Type Definitions **/
/**********************/


/**********************/
/** Global File Data **/
/**********************/

static SC_SIM_Class_t *ScSim = NULL;

static CFE_TIME_SetTimeCmd_t         CfeSetTimeCmd;
static CFE_EVS_ClearLogCmd_t         CfeClrEventLogCmd;
static CFE_EVS_EnableAppEventsCmd_t  CfeEnaAppEventsCmd;
static CFE_EVS_DisableAppEventsCmd_t CfeDisAppEventsCmd;
static KIT_TO_StartEvtLogPlbk_t      KitToStartEvtLogPlaybkCmd;
static KIT_TO_StopEvtLogPlbk_t       KitToStopEvtLogPlaybkCmd;


/*
** Scanf used to load event command parameters
*/

static const char *ScanfStr[SC_SIM_SCANF_TYPE_CNT] =
{

   "UNDEF",
   "%i",               /* SCANF_1_INT */   
   "%i %i",            /* SCANF_2_INT */   
   "%i %i %i",         /* SCANF_3_INT */   
   "%f",               /* SCANF_1_FLT */
   "%lf %lf %lf",      /* SCANF_3_FLT */
   "%lf %lf %lf %lf",  /* SCANF_4_FLT */
   "NONE"

};

static SC_SIM_EventCmd_t SimIdleCmd = { {0,0}, SC_SIM_IDLE_TIME, SC_SIM_Subsystem_SIM,  SC_SIM_EventCmd_IDLE,  SC_SIM_SCANF_NONE,  NULL};

/* Links get resolved when sim started */
static SC_SIM_EventCmd_t SimScenario1[SC_SIM_EVT_CMD_MAX] = 
{

  /* SC_SIM_Phase_INIT */
  
  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_ADCS,  ADCS_EVT_SET_MODE,        SC_SIM_SCANF_1_INT, "3"},
  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_ADCS,  ADCS_EVT_ENTER_ECLIPSE,   SC_SIM_SCANF_NONE,  NULL},
  
  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_FSW,   FSW_EVT_SET_REC_FILE_CNT, SC_SIM_SCANF_1_INT, "10"},
  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_FSW,   FSW_EVT_SET_REC_PCT_USED, SC_SIM_SCANF_1_FLT, "5"},

  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_COMM,  COMM_EVT_SET_DATA_RATE,   SC_SIM_SCANF_1_INT, "1024"},
  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_COMM,  COMM_EVT_SET_TDRS_ID,     SC_SIM_SCANF_1_FLT,  "1"},

  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_INSTR, INSTR_EVT_ENA_POWER,      SC_SIM_SCANF_NONE,  NULL},
  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_INSTR, INSTR_EVT_ENA_SCIENCE,    SC_SIM_SCANF_NONE,  NULL},

  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_POWER, POWER_EVT_SET_BATT_SOC,   SC_SIM_SCANF_1_FLT,  "50"},
  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_POWER, POWER_EVT_SET_SA_CURRENT, SC_SIM_SCANF_1_FLT,  "0"},

  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_THERM, THERM_EVT_ENA_HEATER_1,    SC_SIM_SCANF_1_INT,  "1"},
  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_THERM, THERM_EVT_ENA_HEATER_2,    SC_SIM_SCANF_1_INT,  "1"},

  /* SC_SIM_Phase_TIME_LAPSE */

  { {0,0}, (SC_SIM_REALTIME_EPOCH-3500), SC_SIM_Subsystem_FSW,  FSW_EVT_CLR_EVT_LOG,   SC_SIM_SCANF_NONE,  NULL},
  { {0,0}, (SC_SIM_REALTIME_EPOCH-2400), SC_SIM_Subsystem_ADCS, ADCS_EVT_EXIT_ECLIPSE, SC_SIM_SCANF_NONE,  NULL},

  /* SC_SIM_Phase_REALTIME */
  
  { {0,0}, SC_SIM_REALTIME_EPOCH,        SC_SIM_Subsystem_COMM, COMM_EVT_SCH_AOS,         SC_SIM_SCANF_3_INT, "30 240 1"},
  { {0,0}, SC_SIM_REALTIME_EPOCH,        SC_SIM_Subsystem_COMM, COMM_EVT_SET_TDRS_ID,     SC_SIM_SCANF_1_INT, "1"},  
  { {0,0}, (SC_SIM_REALTIME_EPOCH+120),  SC_SIM_Subsystem_ADCS, ADCS_EVT_ENTER_ECLIPSE,   SC_SIM_SCANF_NONE,  NULL},
  { {0,0}, (SC_SIM_REALTIME_EPOCH+300),  SC_SIM_Subsystem_SIM,  SC_SIM_EventCmd_STOP_SIM, SC_SIM_SCANF_NONE,  NULL},

  /*  events */

  { {0,0}, SC_SIM_REALTIME_END,    SC_SIM_Subsystem_SIM,  SC_SIM_EventCmd_STOP_SIM,  SC_SIM_SCANF_NONE,  NULL},
  { {0,0}, SC_SIM_REALTIME_END,    SC_SIM_Subsystem_SIM,  SC_SIM_EventCmd_STOP_SIM,  SC_SIM_SCANF_NONE,  NULL},
  { {0,0}, SC_SIM_REALTIME_END,    SC_SIM_Subsystem_SIM,  SC_SIM_EventCmd_STOP_SIM,  SC_SIM_SCANF_NONE,  NULL},
  { {0,0}, SC_SIM_REALTIME_END,    SC_SIM_Subsystem_SIM,  SC_SIM_EventCmd_STOP_SIM,  SC_SIM_SCANF_NONE,  NULL},
  { {0,0}, SC_SIM_REALTIME_END,    SC_SIM_Subsystem_SIM,  SC_SIM_EventCmd_STOP_SIM,  SC_SIM_SCANF_NONE,  NULL},
  { {0,0}, SC_SIM_REALTIME_END,    SC_SIM_Subsystem_SIM,  SC_SIM_EventCmd_STOP_SIM,  SC_SIM_SCANF_NONE,  NULL},
  { {0,0}, SC_SIM_REALTIME_END,    SC_SIM_Subsystem_SIM,  SC_SIM_EventCmd_STOP_SIM,  SC_SIM_SCANF_NONE,  NULL}
  
};

static SC_SIM_EventCmd_t SimScenario2[SC_SIM_EVT_CMD_MAX] =
{

  /* SC_SIM_Phase_INIT */
  
  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_ADCS, ADCS_EVT_SET_MODE,         SC_SIM_SCANF_1_INT, "3"},
  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_ADCS, ADCS_EVT_ENTER_ECLIPSE,    SC_SIM_SCANF_NONE,  NULL},
  
  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_FSW,  FSW_EVT_SET_REC_FILE_CNT,  SC_SIM_SCANF_1_INT, "10"},
  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_FSW,  FSW_EVT_SET_REC_PCT_USED,  SC_SIM_SCANF_1_FLT, "5"},

  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_COMM, COMM_EVT_SET_DATA_RATE,    SC_SIM_SCANF_1_INT, "1024"},
  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_COMM, COMM_EVT_SET_TDRS_ID,      SC_SIM_SCANF_1_FLT,  "2"},

  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_INSTR, INSTR_EVT_ENA_POWER,      SC_SIM_SCANF_NONE,  NULL},
  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_INSTR, INSTR_EVT_ENA_SCIENCE,    SC_SIM_SCANF_NONE,  NULL},

  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_POWER, POWER_EVT_SET_BATT_SOC,   SC_SIM_SCANF_1_FLT,  "50"},
  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_POWER, POWER_EVT_SET_SA_CURRENT, SC_SIM_SCANF_1_FLT,  "0"},

  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_THERM, THERM_EVT_ENA_HEATER_1,   SC_SIM_SCANF_1_INT,  "1"},
  { {0,0}, SC_SIM_INIT_TIME, SC_SIM_Subsystem_THERM, THERM_EVT_ENA_HEATER_2,   SC_SIM_SCANF_1_INT,  "1"},

  /* SC_SIM_Phase_TIME_LAPSE */

  { {0,0}, (SC_SIM_REALTIME_EPOCH-3500), SC_SIM_Subsystem_FSW,   FSW_EVT_CLR_EVT_LOG,   SC_SIM_SCANF_NONE,  NULL},
  { {0,0}, (SC_SIM_REALTIME_EPOCH-2400), SC_SIM_Subsystem_ADCS,  ADCS_EVT_EXIT_ECLIPSE, SC_SIM_SCANF_NONE,  NULL},
  { {0,0}, (SC_SIM_REALTIME_EPOCH-1400), SC_SIM_Subsystem_CDH,   CDH_EVT_WATCHDOG_RST,  SC_SIM_SCANF_NONE,  NULL},
  { {0,0}, (SC_SIM_REALTIME_EPOCH-1398), SC_SIM_Subsystem_ADCS,  ADCS_EVT_SET_MODE,     SC_SIM_SCANF_1_INT, "1"},
  { {0,0}, (SC_SIM_REALTIME_EPOCH-1396), SC_SIM_Subsystem_INSTR, INSTR_EVT_DIS_POWER,   SC_SIM_SCANF_NONE,  NULL},
  { {0,0}, (SC_SIM_REALTIME_EPOCH-1394), SC_SIM_Subsystem_INSTR, INSTR_EVT_DIS_SCIENCE, SC_SIM_SCANF_NONE,  NULL},

  /* SC_SIM_Phase_REALTIME */
  
  { {0,0}, SC_SIM_REALTIME_EPOCH,        SC_SIM_Subsystem_COMM, COMM_EVT_SCH_AOS,         SC_SIM_SCANF_3_INT, "30 240 1"},
  { {0,0}, SC_SIM_REALTIME_EPOCH,        SC_SIM_Subsystem_COMM, COMM_EVT_SET_TDRS_ID,     SC_SIM_SCANF_1_INT, "1"},  
  { {0,0}, (SC_SIM_REALTIME_EPOCH+120),  SC_SIM_Subsystem_ADCS, ADCS_EVT_ENTER_ECLIPSE,   SC_SIM_SCANF_NONE,  NULL},
  { {0,0}, (SC_SIM_REALTIME_EPOCH+300),  SC_SIM_Subsystem_SIM,  SC_SIM_EventCmd_STOP_SIM, SC_SIM_SCANF_NONE,  NULL},

  /*  events */

  { {0,0}, SC_SIM_REALTIME_END,    SC_SIM_Subsystem_SIM,  SC_SIM_EventCmd_STOP_SIM,  SC_SIM_SCANF_NONE,  NULL},
  { {0,0}, SC_SIM_REALTIME_END,    SC_SIM_Subsystem_SIM,  SC_SIM_EventCmd_STOP_SIM,  SC_SIM_SCANF_NONE,  NULL},
  { {0,0}, SC_SIM_REALTIME_END,    SC_SIM_Subsystem_SIM,  SC_SIM_EventCmd_STOP_SIM,  SC_SIM_SCANF_NONE,  NULL}
  
};

/* 
** Subsystem strings
*/
static const char* SubSysStr[] =
{
   
   "UNDEF",  /* SC_SIM_Subsystem_UNDEF  = 0 */   
   "SIM",    /* SC_SIM_Subsystem_SYS    = 1 */
   "ADCS",   /* SC_SIM_Subsystem_ADCS   = 2 */
   "CDH",    /* SC_SIM_Subsystem_CDH    = 3 */
   "COMM",   /* SC_SIM_Subsystem_COMM   = 4 */
   "FSW",    /* SC_SIM_Subsystem_FSW    = 5 */
   "INSTR",  /* SC_SIM_Subsystem_FSW    = 6 */
   "POWER",  /* SC_SIM_Subsystem_POWER    = 7 */
   "THERM"   /* SC_SIM_Subsystem_THERM  = 8 */

};


/*******************************/
/** Local Function Prototypes **/
/*******************************/

static void SIM_AcceptNewTbl(void);
static void SIM_AddEventCmd(SC_SIM_EventCmd_t *NewRunTimeCmd);
static void SIM_ExecuteEventCmd(void);
static void SIM_SetTime(uint32 NewSeconds);
static void SIM_StopSim(void);
static bool SIM_ProcessEventCmd(const SC_SIM_EventCmd_t *EventCmd);
#if (SC_SIM_DEBUG == 1)
static void SIM_DumpScenario(uint8 StartIdx, uint8 EndIdx);
#endif

static void ADCS_Init(ADCS_Model_t *Adcs);
static void ADCS_Execute(ADCS_Model_t *Adcs);
static bool ADCS_ProcessEventCmd(ADCS_Model_t *Adcs, const SC_SIM_EventCmd_t *EventCmd);

static void CDH_Init(CDH_Model_t *Cdh);
static void CDH_Execute(CDH_Model_t *Cdh);
static bool CDH_ProcessEventCmd(CDH_Model_t *Cdh, const SC_SIM_EventCmd_t *EventCmd);

static void COMM_Init(COMM_Model_t *Comm);
static void COMM_Execute(COMM_Model_t *Comm);
static bool COMM_ProcessEventCmd(COMM_Model_t *Comm, const SC_SIM_EventCmd_t *EventCmd);

static void FSW_Init(FSW_Model_t *Fsw);
static void FSW_Execute(FSW_Model_t *Fsw);
static bool FSW_ProcessEventCmd(FSW_Model_t *Fsw, const SC_SIM_EventCmd_t *EventCmd);

static void INSTR_Init(INSTR_Model_t *Instr);
static void INSTR_Execute(INSTR_Model_t *Instr);
static bool INSTR_ProcessEventCmd(INSTR_Model_t *Instr, const SC_SIM_EventCmd_t *EventCmd);

static void POWER_Init(POWER_Model_t *Power);
static void POWER_Execute(POWER_Model_t *Power);
static bool POWER_ProcessEventCmd(POWER_Model_t *Power, const SC_SIM_EventCmd_t *EventCmd);

static void THERM_Init(THERM_Model_t *Therm);
static void THERM_Execute(THERM_Model_t *Therm);
static bool THERM_ProcessEventCmd(THERM_Model_t *Therm, const SC_SIM_EventCmd_t *EventCmd);

static void SC_SIM_SendMgmtPkt(void);
static void SC_SIM_SendModelPkt(void);


/******************************************************************************
** Function: SC_SIM_Constructor
**
*/
void SC_SIM_Constructor(SC_SIM_Class_t *ScSimPtr, INITBL_Class_t *IniTbl,
                        TBLMGR_Class_t *TblMgr)
{
 
   ScSim = ScSimPtr;

   CFE_PSP_MemSet((void*)ScSim, 0, sizeof(SC_SIM_Class_t));

   SC_SIM_TBL_Constructor(&ScSim->Tbl, SIM_AcceptNewTbl, INITBL_GetStrConfig(IniTbl, CFG_APP_CFE_NAME));
   TBLMGR_RegisterTblWithDef(TblMgr, SC_SIM_TBL_LoadCmd, SC_SIM_TBL_DumpCmd, INITBL_GetStrConfig(IniTbl, CFG_SC_SIM_TBL_LOAD_FILE));

   ScSim->Time.Seconds = SC_SIM_IDLE_TIME;
   ScSim->Phase        = SC_SIM_Phase_IDLE;
   ScSim->LastEventCmd   = &SimIdleCmd;
   ScSim->NextEventCmd   = &SimIdleCmd;
   
   ADCS_Init(ADCS);
   CDH_Init(CDH);
   COMM_Init(COMM);
   FSW_Init(FSW);
   INSTR_Init(INSTR);
   POWER_Init(POWER);
   THERM_Init(THERM);

   CFE_MSG_Init(CFE_MSG_PTR(ScSim->MgmtTlm.TelemetryHeader), 
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(IniTbl, SC_SIM_MGMT_TLM_TOPICID)),
                sizeof(SC_SIM_MgmtTlm_t));

   CFE_MSG_Init(CFE_MSG_PTR(ScSim->ModelTlm.TelemetryHeader), 
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(IniTbl, SC_SIM_MODEL_TLM_TOPICID)),
                sizeof(SC_SIM_ModelTlm_t));

   CFE_MSG_Init(CFE_MSG_PTR(CfeSetTimeCmd.CommandBase), 
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(IniTbl, TIME_CMD_TOPICID)), sizeof(CFE_TIME_SetTimeCmd_t));
   CFE_MSG_SetFcnCode(CFE_MSG_PTR(CfeSetTimeCmd.CommandBase), CFE_TIME_SET_TIME_CC);

   CFE_MSG_Init(CFE_MSG_PTR(CfeClrEventLogCmd),
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(IniTbl, EVS_CMD_TOPICID)), sizeof(CFE_EVS_ClearLogCmd_t));
   CFE_MSG_SetFcnCode(CFE_MSG_PTR(CfeClrEventLogCmd), CFE_EVS_CLEAR_LOG_CC);
   CFE_MSG_GenerateChecksum(CFE_MSG_PTR(CfeClrEventLogCmd));
    
   CFE_MSG_Init(CFE_MSG_PTR(CfeEnaAppEventsCmd.CommandBase), 
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(IniTbl, EVS_CMD_TOPICID)), sizeof(CFE_EVS_EnableAppEventsCmd_t));
   CFE_MSG_SetFcnCode(CFE_MSG_PTR(CfeEnaAppEventsCmd.CommandBase), CFE_EVS_ENABLE_APP_EVENTS_CC);

   CFE_MSG_Init(CFE_MSG_PTR(CfeDisAppEventsCmd.CommandBase),
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(IniTbl, EVS_CMD_TOPICID)), sizeof(CFE_EVS_DisableAppEventsCmd_t));
   CFE_MSG_SetFcnCode(CFE_MSG_PTR(CfeDisAppEventsCmd.CommandBase), CFE_EVS_DISABLE_APP_EVENTS_CC);
   
   CFE_MSG_Init(CFE_MSG_PTR(KitToStartEvtLogPlaybkCmd),
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(IniTbl, CFG_KIT_TO_CMD_TOPICID)),
                sizeof(KIT_TO_StartEvtLogPlbk_t));
   CFE_MSG_SetFcnCode(CFE_MSG_PTR(KitToStartEvtLogPlaybkCmd.CommandBase), KIT_TO_START_EVT_LOG_PLBK_CC);
   CFE_MSG_GenerateChecksum(CFE_MSG_PTR(KitToStartEvtLogPlaybkCmd.CommandBase));

   CFE_MSG_Init(CFE_MSG_PTR(KitToStopEvtLogPlaybkCmd.CommandBase),
                CFE_SB_ValueToMsgId(INITBL_GetIntConfig(IniTbl, CFG_KIT_TO_CMD_TOPICID)),
                sizeof(KIT_TO_StopEvtLogPlbk_t));
   CFE_MSG_SetFcnCode(CFE_MSG_PTR(KitToStopEvtLogPlaybkCmd.CommandBase), KIT_TO_STOP_EVT_LOG_PLBK_CC);
   CFE_MSG_GenerateChecksum(CFE_MSG_PTR(KitToStopEvtLogPlaybkCmd.CommandBase));


} /* End SC_SIM_Constructor() */


/******************************************************************************
** Function: SC_SIM_Execute
**
** Execute spacecraft simulation cycle. When to execute the next command is
** in this function rather than ExecuteCmd() because time is managed 
** differently depending on the sim phase.
**
** ScSim time is course with subsecond second accuracy. Since the SC_SIM
** app is driven by the scheduler, it's already synched to the cFS at the
** seconds level of resolution so when seconds are increment nothing is done
** WRT to cFE Time. When time jumps occur cFE Time is synchronized.
**
*/
bool SC_SIM_Execute(void)
{

   bool RetStatus = true;
   uint16 TimeLapseExeCnt = 0;
   
   if (ScSim->Active)
   {

      switch (ScSim->Phase)
      {
         
      /*
      ** SimTime stays fixed during init. 
      ** All init commands executed in one execution cycle
      ** One telemetry packet sent that reflects final init state      
      */
      case SC_SIM_Phase_INIT:
      
         CFE_EVS_SendEvent(SC_SIM_EXECUTE_EID, CFE_EVS_EventType_DEBUG, "SC_SIM_Phase_INIT: Enter");
         while (ScSim->NextEventCmd->Time == SC_SIM_INIT_TIME)
         {
            SIM_ExecuteEventCmd();  
         }
         
         if (ScSim->NextEventCmd->Time < SC_SIM_REALTIME_EPOCH)
         {
            /* Time lapse starts with the first time lapse command */
            ScSim->Phase = SC_SIM_Phase_TIME_LAPSE;
            SIM_SetTime(ScSim->NextEventCmd->Time);  
         }
         else
         {   
            ScSim->Phase = SC_SIM_Phase_REALTIME;
            SIM_SetTime(SC_SIM_REALTIME_EPOCH);
         }

         CFE_EVS_SendEvent(SC_SIM_EXECUTE_EID, CFE_EVS_EventType_DEBUG, "SC_SIM_Phase_INIT: Exit with next phase %d at time %d", ScSim->Phase, ScSim->Time.Seconds);
         break;   
   
      case SC_SIM_Phase_TIME_LAPSE:

         CFE_EVS_SendEvent(SC_SIM_EXECUTE_EID, CFE_EVS_EventType_DEBUG, "SC_SIM_Phase_TIME_LAPSE: Enter at SimTime %d, Next Cmd Time %d", ScSim->Time.Seconds, ScSim->NextEventCmd->Time);
         while (ScSim->Time.Seconds < SC_SIM_REALTIME_EPOCH && TimeLapseExeCnt < SC_SIM_TIME_LAPSE_EXE_CNT)
         {
         
            if (ScSim->Time.Seconds >= (ScSim->NextEventCmd->Time))
            {
               SIM_ExecuteEventCmd();
            }
            
            ADCS_Execute(ADCS);
            CDH_Execute(CDH);
            COMM_Execute(COMM);
            FSW_Execute(FSW);
            INSTR_Execute(INSTR);
            POWER_Execute(POWER);
            THERM_Execute(THERM);    
   
            ScSim->Time.Seconds++;
            TimeLapseExeCnt++;
         
         } /* End while loop */

         SIM_SetTime(ScSim->Time.Seconds);
         if (ScSim->Time.Seconds >= SC_SIM_REALTIME_EPOCH) ScSim->Phase = SC_SIM_Phase_REALTIME;
         
         CFE_EVS_SendEvent(SC_SIM_EXECUTE_EID, CFE_EVS_EventType_DEBUG, "SC_SIM_Phase_TIME_LAPSE: Exit with next phase %d at time %d", ScSim->Phase, ScSim->Time.Seconds);
         break;   
         
      case SC_SIM_Phase_REALTIME:

         if (ScSim->Time.Seconds >= (ScSim->NextEventCmd->Time))
         {
            SIM_ExecuteEventCmd();
         }

         ADCS_Execute(ADCS);
         CDH_Execute(CDH);
         COMM_Execute(COMM);
         FSW_Execute(FSW);
         INSTR_Execute(INSTR);
         POWER_Execute(POWER);
         THERM_Execute(THERM);

         ScSim->Time.Seconds++;
         if (ScSim->Time.Seconds >= SC_SIM_REALTIME_END) SIM_StopSim();
            
         break;   

      default:
         break;   

         
      } /* End phase switch */
   
   
   } /* End if Sim Active */
   
   
   /* 
   ** Manage Telemetry
   ** - Alway send sim management data
   ** - Only send model state data during a ground contact except
   **   during development need to be able to verify time-lapse
   **   model behavior 
   */
   
   SC_SIM_SendMgmtPkt();
   
   if (ScSim->Comm.InContact || SC_SIM_DEBUG)
   {
      SC_SIM_SendModelPkt();
   }

   return RetStatus;
      
} /* SC_SIM_Execute() */


/******************************************************************************
** Functions: SC_SIM_ProcessMqttJsonCmd
**
** Process commands from an MQTT broker
**
** Notes:
**  1. The JSON commands are designed to have a single parameter that can map
**     to SC_SIM commands. The commands are intentionally designed so no other
**     parameters are required. The MQTT interface is used in educational
**     environments so user choices are minimized. 
**
*/
bool SC_SIM_ProcessMqttJsonCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{

   bool RetStatus = true;
   const SC_SIM_MqttJsonCmd_Payload_t *MqttJsonCmd = CMDMGR_PAYLOAD_PTR(MsgPtr,SC_SIM_MqttJsonCmd_t);
   SC_SIM_StartSim_t StartSimCmd;
  
   CFE_EVS_SendEvent(SC_SIM_PROCESS_MQTT_CMD_EID, CFE_EVS_EventType_INFORMATION,
                     "SC_SIM_ProcessMqttJsonCmd() %d", MqttJsonCmd->Id);
   
   switch (MqttJsonCmd->Id)
   {
      case SC_SIM_MqttJsonCmdId_START_SIM_1:
         StartSimCmd.Payload.ScenarioId = SC_SIM_Scenario_GND_CONTACT_1;
         SC_SIM_StartSimCmd(DataObjPtr, CFE_MSG_PTR(StartSimCmd));
         break;
      case SC_SIM_MqttJsonCmdId_START_SIM_2:
         StartSimCmd.Payload.ScenarioId = SC_SIM_Scenario_GND_CONTACT_2;
         SC_SIM_StartSimCmd(DataObjPtr, CFE_MSG_PTR(StartSimCmd));
         break;
      case SC_SIM_MqttJsonCmdId_STOP_SIM:
         SC_SIM_StopSimCmd(DataObjPtr, NULL);
         break;
      case SC_SIM_MqttJsonCmdId_START_EVT_PLBK:
         SC_SIM_StartPlbkCmd(DataObjPtr, NULL);         
         break;
      case SC_SIM_MqttJsonCmdId_STOP_EVT_PLBK:
         SC_SIM_StopPlbkCmd(DataObjPtr, NULL);
         break;
      default:
         CFE_EVS_SendEvent(SC_SIM_PROCESS_MQTT_CMD_EID, CFE_EVS_EventType_ERROR,
                           "SC_SIM received invalid MQTT JSON command ID %d", MqttJsonCmd->Id);
   }
   
   return RetStatus;

} /* End SC_SIM_ProcessMqttJsonCmd() */


/******************************************************************************
** Function:  SC_SIM_ResetStatus
**
*/
void SC_SIM_ResetStatus(void)
{

   /* No counters to be reset that wouldn't effect the simulation state */
   SC_SIM_TBL_ResetStatus();

} /* End SC_SIM_ResetStatus() */


/******************************************************************************
** Functions: SC_SIM_StartSimCmd
**
** Start a simulation
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr definition
*/
bool SC_SIM_StartSimCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   int i;
   bool SimEndCmdFound = false;
   bool RetStatus = true;
   
   const SC_SIM_StartSim_Payload_t *StartSim = CMDMGR_PAYLOAD_PTR(MsgPtr,SC_SIM_StartSim_t);
   
   SIM_SetTime(SC_SIM_INIT_TIME);
   ScSim->Active = true;
   ScSim->Phase  = SC_SIM_Phase_INIT;
   
   /* 
   ** Only allow first sim set time to generated an event message and then disable time
   ** events. 
   */
   strcpy(CfeDisAppEventsCmd.Payload.AppName,"CFE_SB");
   CFE_MSG_GenerateChecksum(CFE_MSG_PTR(CfeDisAppEventsCmd.CommandBase));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(CfeDisAppEventsCmd.CommandBase), true);

   strcpy(CfeDisAppEventsCmd.Payload.AppName,"CFE_TIME");
   CFE_MSG_GenerateChecksum(CFE_MSG_PTR(CfeDisAppEventsCmd.CommandBase));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(CfeDisAppEventsCmd.CommandBase), true);
   
   strcpy(CfeDisAppEventsCmd.Payload.AppName,"KIT_SCH");
   CFE_MSG_GenerateChecksum(CFE_MSG_PTR(CfeDisAppEventsCmd.CommandBase));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(CfeDisAppEventsCmd.CommandBase), true);

   if (StartSim->ScenarioId == SC_SIM_Scenario_GND_CONTACT_1)
   {
      ScSim->Scenario = SimScenario1;
   } 
   else if (StartSim->ScenarioId == SC_SIM_Scenario_GND_CONTACT_2)
   {
      ScSim->Scenario = SimScenario2;
   }
   else
   {   
      CFE_EVS_SendEvent(SC_SIM_START_SIM_ERR_EID, CFE_EVS_EventType_ERROR, 
                        "Start Sim command rejected. Invalid scenario identifier %d. It must be between %d and %d inclusively",
                        StartSim->ScenarioId, SC_SIM_Scenario_Enum_t_MIN, SC_SIM_Scenario_Enum_t_MAX);
      RetStatus = false;
   }

   if (RetStatus == true)
   {   
      ScSim->ScenarioId   = StartSim->ScenarioId;
      ScSim->LastEventCmd = &SimIdleCmd;
      ScSim->NextEventCmd = &ScSim->Scenario[0];
      
      /* 
      ** Configure time sorted linked list and setup so cmds can be 
      ** added at runtime. Assumes scenario is time sorted but no
      ** assumptions about the link indices. 
      ** Last entry in sim has next link set to SC_SIM_EVT_CMD_NULL_IDX.
      */

      ScSim->RunTimeCmdIdx = SC_SIM_EVT_CMD_NULL_IDX;

      ScSim->Scenario[0].Link.Prev = SC_SIM_EVT_CMD_NULL_IDX;
      ScSim->Scenario[0].Link.Next = 1;
      
      for (i=1; i < SC_SIM_EVT_CMD_MAX; i++)
      {
      
         ScSim->Scenario[i].Link.Prev = (i-1);
         ScSim->Scenario[i].Link.Next = (i+1);
         
         /* Ensure unused entries have time set to end time */
         if (SimEndCmdFound)
         {
   
            ScSim->Scenario[i].Time      = SC_SIM_REALTIME_END;
            ScSim->Scenario[i].Link.Prev = SC_SIM_EVT_CMD_NULL_IDX;
            ScSim->Scenario[i].Link.Next = SC_SIM_EVT_CMD_NULL_IDX;

         } 
         else
         {
            
            /* Full table so set last index */
            if (i == (SC_SIM_EVT_CMD_MAX-1))
            {
            
               ScSim->Scenario[i].Link.Next = SC_SIM_EVT_CMD_NULL_IDX;
            
            }
            else
            {
            
               if ( ScSim->Scenario[i].Time == SC_SIM_REALTIME_END)
               {
                  ScSim->RunTimeCmdIdx = i+2;
                  SimEndCmdFound = true;
               }
            
            }
         } /* End if !SimEndCmdFound */
      } /* End scenario loop */

      CFE_EVS_SendEvent(SC_SIM_START_SIM_EID, CFE_EVS_EventType_INFORMATION,
                        "Start Simulation using scenario %d with %d available runtime cmd entries starting at index %d",
                        StartSim->ScenarioId, (SC_SIM_EVT_CMD_MAX-ScSim->RunTimeCmdIdx), ScSim->RunTimeCmdIdx);

      #if (SC_SIM_DEBUG == 1)
         SIM_DumpScenario(0,14);
      #endif

   } /* End if valid command parameters */
   
   return RetStatus;

} /* End SC_SIM_StartSimCmd() */


/******************************************************************************
** Functions: SC_SIM_StopSimCmd
**
** Stop a simulation
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr definition
*/
bool SC_SIM_StopSimCmd (void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{

   bool RetStatus = true;
  
   SIM_StopSim();
   
   return RetStatus;

} /* End SC_SIM_StopSimCmd() */


/******************************************************************************
** Functions: SC_SIM_StartPlbkCmd
**
** Stop a recorder playback.
**
** Notes:
**  1. This function must comply with the CMDMGR_CmdFuncPtr definition
**
*/
bool SC_SIM_StartPlbkCmd (void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{

   bool RetStatus = false;
  
   if (ScSim->Comm.InContact)
   {
      ScSim->Fsw.Recorder.PlaybackEna = true;
      CFE_SB_TransmitMsg(CFE_MSG_PTR(KitToStartEvtLogPlaybkCmd.CommandBase), true);
      CFE_EVS_SendEvent(SC_SIM_START_REC_PLBK_EID, CFE_EVS_EventType_INFORMATION,"FSW recorder playback started"); 
      RetStatus = true;
   }
   else
   {
      CFE_EVS_SendEvent(SC_SIM_START_REC_PLBK_EID, CFE_EVS_EventType_ERROR,"Start playback command rejected because not in ground contact");    
   }
   
   return RetStatus;

} /* End SC_SIM_StartPlbkCmd() */


/******************************************************************************
** Functions: SC_SIM_StopPlbkCmd
**
** Stop a recorder playback.
**
** Notes:
**  1. This function must comply with the CMDMGR_CmdFuncPtr definition
**
*/
bool SC_SIM_StopPlbkCmd (void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{

   bool RetStatus = true;
  
   ScSim->Fsw.Recorder.PlaybackEna = false;
   CFE_SB_TransmitMsg(CFE_MSG_PTR(KitToStopEvtLogPlaybkCmd.CommandBase), true);
   CFE_EVS_SendEvent(SC_SIM_STOP_REC_PLBK_EID, CFE_EVS_EventType_INFORMATION,"FSW recorder playback stopped"); 
         
   return RetStatus;

} /* End SC_SIM_StopPlbkCmd() */


/******************************************************************************
** Function: SC_SIM_SendMgmtPkt
**
*/
static void SC_SIM_SendMgmtPkt(void)
{

   SC_SIM_MgmtTlm_Payload_t *Payload = &ScSim->MgmtTlm.Payload;

   Payload->SimTime   = ScSim->Time.Seconds;
   Payload->SimActive = ScSim->Active;
   Payload->SimPhase  = ScSim->Phase;
   
   Payload->ContactTimePending   = ScSim->Comm.Contact.TimePending;
   Payload->ContactLength        = ScSim->Comm.Contact.Length;   
   Payload->ContactTimeConsumed  = ScSim->Comm.Contact.TimeConsumed;
   Payload->ContactTimeRemaining = ScSim->Comm.Contact.TimeRemaining;
   
   if (ScSim->LastEventCmd != NULL)
   {
      Payload->LastEventSubSysId  = ScSim->LastEventCmd->SubSys;
      Payload->LastEventCmdId     = ScSim->LastEventCmd->Id; 
   }
   else
   {
      Payload->LastEventSubSysId  = SC_SIM_Subsystem_SIM;
      Payload->LastEventCmdId     = SC_SIM_EventCmd_UNDEF;
   }
   
   if (ScSim->NextEventCmd != NULL)
   {
      Payload->NextEventSubSysId  = ScSim->NextEventCmd->SubSys;
      Payload->NextEventCmdId     = ScSim->NextEventCmd->Id; 
   }
   else
   {
      Payload->NextEventSubSysId  = SC_SIM_Subsystem_SIM;
      Payload->NextEventCmdId     = SC_SIM_EventCmd_UNDEF; 
   }
      
   Payload->AdcsLastEventCmd.Time = ScSim->Adcs.LastEventCmd.Time;
   Payload->AdcsLastEventCmd.Id   = ScSim->Adcs.LastEventCmd.Id;

   Payload->CdhLastEventCmd.Time  = ScSim->Cdh.LastEventCmd.Time;
   Payload->CdhLastEventCmd.Id    = ScSim->Cdh.LastEventCmd.Id;

   Payload->CommLastEventCmd.Time = ScSim->Comm.LastEventCmd.Time;
   Payload->CommLastEventCmd.Id   = ScSim->Comm.LastEventCmd.Id;

   Payload->PowerLastEventCmd.Time  = ScSim->Power.LastEventCmd.Time;
   Payload->PowerLastEventCmd.Id    = ScSim->Power.LastEventCmd.Id;

   Payload->ThermLastEventCmd.Time = ScSim->Therm.LastEventCmd.Time;
   Payload->ThermLastEventCmd.Id   = ScSim->Therm.LastEventCmd.Id;
      
   CFE_SB_TimeStampMsg(CFE_MSG_PTR(ScSim->MgmtTlm.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(ScSim->MgmtTlm.TelemetryHeader), true);

} /* End SC_SIM_SendMgmtPkt() */


/******************************************************************************
** Function: SC_SIM_SendModelPkt
**
*/
static void SC_SIM_SendModelPkt(void)
{

   SC_SIM_ModelTlm_Payload_t *Payload = &ScSim->ModelTlm.Payload;

   /*
   ** ADCS
   */

   Payload->Eclipse  = ScSim->Adcs.Eclipse;
   Payload->AdcsMode = ScSim->Adcs.Mode;

   /*
   ** C&DH
   */
   
   Payload->SbcRstCnt = ScSim->Cdh.SbcRstCnt;
   Payload->HwCmdCnt  = ScSim->Cdh.HwCmdCnt;
   Payload->LastHwCmd = ScSim->Cdh.LastHwCmd;

   /*
   ** Comm
   */
   
   Payload->InContact            = ScSim->Comm.InContact;
   Payload->ContactTimePending   = ScSim->Comm.Contact.TimePending;
   Payload->ContactTimeConsumed  = ScSim->Comm.Contact.TimeConsumed;
   Payload->ContactTimeRemaining = ScSim->Comm.Contact.TimeRemaining;
   Payload->ContactLink          = ScSim->Comm.Contact.Link;
   Payload->ContactTdrsId        = ScSim->Comm.Contact.TdrsId;
   Payload->ContactDataRate      = ScSim->Comm.Contact.DataRate;
   
   /*
   ** FSW
   */
   
   Payload->RecPctUsed     = ScSim->Fsw.Recorder.PctUsed;
   Payload->RecFileCnt     = ScSim->Fsw.Recorder.FileCnt;
   Payload->RecPlaybackEna = ScSim->Fsw.Recorder.PlaybackEna;

   /*
   ** Instrument
   */

   Payload->InstrPwrEna = ScSim->Instr.PwrEna;
   Payload->InstrSciEna = ScSim->Instr.SciEna;

   Payload->InstrFileCnt    = ScSim->Instr.FileCnt;
   Payload->InstrFileCycCnt = ScSim->Instr.FileCycCnt;

   /*
   ** Power
   */
   
   Payload->BattSoc   = ScSim->Power.BattSoc;
   Payload->SaCurrent = ScSim->Power.SaCurrent;

   /*
   ** Thermal
   */
   
   Payload->Heater1Ena = ScSim->Therm.Heater1Ena;
   Payload->Heater2Ena = ScSim->Therm.Heater2Ena;
   
   CFE_SB_TimeStampMsg(CFE_MSG_PTR(ScSim->ModelTlm.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(ScSim->ModelTlm.TelemetryHeader), true);

} /* End SC_SIM_SendModelPkt() */


/**************************/
/**************************/
/****                  ****/
/****    SIM OBJECT    ****/
/****                  ****/
/**************************/
/**************************/

/*
** The simulation object is a different from the model pbjects because it is
** not modelling any behavior. Instead it is being used to help control the
**
*/

/******************************************************************************
** Function: SIM_AcceptNewTbl
**
** Notes:
**   1. This is a SC_SIM_TBL table load callback function and must match the
**      SC_SIM_TBL_LoadFunc_t definition.
*/
static void SIM_AcceptNewTbl(void)
{

   CFE_EVS_SendEvent (SC_SIM_ACCEPT_NEW_TBL_EID, CFE_EVS_EventType_INFORMATION, 
                      "New simulation parameter table loaded");
   
   // TODO: Determine what to do on a parameter table load   

} /* End SIM_AcceptNewTbl() */

/******************************************************************************
** Function: SIM_AddEventCmd
**
** TODO - Add array bound check
*/
static void SIM_AddEventCmd(SC_SIM_EventCmd_t *NewRunTimeCmd)
{
   
   bool NewCmdInserted = false;
   bool EndOfBuffer    = false;
   SC_SIM_EventCmd_t *EventCmd   = ScSim->NextEventCmd;
   SC_SIM_EventCmd_t *NewEventCmd;
   
   CFE_EVS_SendEvent(SC_SIM_ADD_EVENT_EID, CFE_EVS_EventType_DEBUG, 
                           "Enter SIM_AddEventCmd() for subsystem %d cmd %d added at scenario index %d",
                           NewRunTimeCmd->SubSys, NewRunTimeCmd->Id, ScSim->RunTimeCmdIdx);
   CFE_EVS_SendEvent(SC_SIM_ADD_EVENT_EID, CFE_EVS_EventType_DEBUG, 
                           "NextCmd: (%d,%d) %d, %d, %d",
                           EventCmd->Link.Prev, EventCmd->Link.Next, EventCmd->Time, EventCmd->SubSys, EventCmd->Id);
                           
   if (ScSim->RunTimeCmdIdx == SC_SIM_EVT_CMD_NULL_IDX)
   {
      
      CFE_EVS_SendEvent(SC_SIM_EVENT_ERR_EID, CFE_EVS_EventType_ERROR, 
                        "Aborting sim due to event cmd buffer overflow while loading new subsystem %d cmd %d",
                        NewRunTimeCmd->SubSys, NewRunTimeCmd->Id);
   
      SIM_StopSim();
      
   }
   else
   {
   
      ScSim->Scenario[ScSim->RunTimeCmdIdx] = *NewRunTimeCmd;

      NewEventCmd = &(ScSim->Scenario[ScSim->RunTimeCmdIdx]);

      while (!NewCmdInserted && !EndOfBuffer)
      {
   
         if (NewEventCmd->Time <= EventCmd->Time)
         {
   
            CFE_EVS_SendEvent(SC_SIM_ADD_EVENT_EID, CFE_EVS_EventType_DEBUG, 
                              "Adding command before: (%d,%d) %d, %d, %d",
                              EventCmd->Link.Prev, EventCmd->Link.Next, EventCmd->Time, EventCmd->SubSys, EventCmd->Id);
  
            NewEventCmd->Link.Prev = EventCmd->Link.Prev;
            NewEventCmd->Link.Next = ScSim->Scenario[EventCmd->Link.Prev].Link.Next;

            ScSim->Scenario[EventCmd->Link.Prev].Link.Next = ScSim->RunTimeCmdIdx;  
            EventCmd->Link.Prev = ScSim->RunTimeCmdIdx;

            NewCmdInserted = true;
         
         }
         
         if (EventCmd->Link.Next == SC_SIM_EVT_CMD_NULL_IDX || EventCmd->Time == SC_SIM_REALTIME_END)
         {
            EndOfBuffer = true;
         }
         else
         {
            EventCmd = &(ScSim->Scenario[EventCmd->Link.Next]);
         }
         
      } /* End while loop */
   
      if (NewCmdInserted)
      {
         
         CFE_EVS_SendEvent(SC_SIM_ADD_EVENT_EID, CFE_EVS_EventType_DEBUG, 
                           "New subsystem %d cmd %d added at scenario index %d",
                           NewEventCmd->SubSys, NewEventCmd->Id, ScSim->RunTimeCmdIdx);

         ScSim->RunTimeCmdIdx++;
         if (ScSim->RunTimeCmdIdx == SC_SIM_EVT_CMD_MAX) ScSim->RunTimeCmdIdx = SC_SIM_EVT_CMD_NULL_IDX;
      
      }
      else
      {

         CFE_EVS_SendEvent(SC_SIM_EVENT_ERR_EID, CFE_EVS_EventType_ERROR, 
                           "Aborting sim due to failure to insert new runtime cmd for subsystem %d cmd %d at index %d",
                           NewRunTimeCmd->SubSys, NewRunTimeCmd->Id, ScSim->RunTimeCmdIdx);
         SIM_StopSim();        
      
      }
   
   } /* End if room for new cmd */

   #if (SC_SIM_DEBUG == 1)
      SIM_DumpScenario(8,14);
   #endif

} /* End SIM_AddEventCmd() */


/******************************************************************************
** Function:  SIM_DumpScenario
**
*/
#if (SC_SIM_DEBUG == 1)
static void SIM_DumpScenario(uint8 StartIdx, uint8 EndIdx)
{
   int i;

   for (i=StartIdx; i <= EndIdx; i++)
   {
      OS_printf ("SimScenario[%d]: (%d,%d) %d: %d, %d, %d\n",
                 i, ScSim->Scenario[i].Link.Prev, ScSim->Scenario[i].Link.Next, ScSim->Scenario[i].Time,  
                 ScSim->Scenario[i].Time, ScSim->Scenario[i].SubSys, ScSim->Scenario[i].Id);
   }
   
} /* End SIM_DumpScenario() */
#endif

/******************************************************************************
** Function:  SIM_ExecuteEventCmd
**
*/
static void SIM_ExecuteEventCmd(void)
{
     
   CFE_EVS_SendEvent(SC_SIM_EXECUTE_EVENT_EID, CFE_EVS_EventType_DEBUG, "Executing %s cmd %d at time %d",
                     SubSysStr[ScSim->NextEventCmd->SubSys],ScSim->NextEventCmd->Id,ScSim->NextEventCmd->Time);
   
   switch (ScSim->NextEventCmd->ScanfType)
   {
   case SC_SIM_SCANF_1_INT:
      sscanf(ScSim->NextEventCmd->Param, ScanfStr[SC_SIM_SCANF_1_INT], &(ScSim->EventCmdParam.OneInt));
      break;

   case SC_SIM_SCANF_2_INT:
      sscanf(ScSim->NextEventCmd->Param, ScanfStr[SC_SIM_SCANF_2_INT], &(ScSim->EventCmdParam.TwoInt[0]), &(ScSim->EventCmdParam.TwoInt[1]));
      break;
      
   case SC_SIM_SCANF_3_INT:
      sscanf(ScSim->NextEventCmd->Param, ScanfStr[SC_SIM_SCANF_3_INT], &(ScSim->EventCmdParam.ThreeInt[0]), &(ScSim->EventCmdParam.ThreeInt[1]), &(ScSim->EventCmdParam.ThreeInt[2]));
      break;
      
   case SC_SIM_SCANF_1_FLT:
      sscanf(ScSim->NextEventCmd->Param, ScanfStr[SC_SIM_SCANF_1_FLT], &(ScSim->EventCmdParam.OneFlt));
      break;

   case SC_SIM_SCANF_3_FLT:
      sscanf(ScSim->NextEventCmd->Param, ScanfStr[SC_SIM_SCANF_3_FLT], &(ScSim->EventCmdParam.ThreeFlt[0]), &(ScSim->EventCmdParam.ThreeFlt[1]), &(ScSim->EventCmdParam.ThreeFlt[2]));
      break;
      
   case SC_SIM_SCANF_4_FLT:
      sscanf(ScSim->NextEventCmd->Param, ScanfStr[SC_SIM_SCANF_3_FLT], &(ScSim->EventCmdParam.FourFlt[0]), &(ScSim->EventCmdParam.FourFlt[1]), &(ScSim->EventCmdParam.FourFlt[2]), &(ScSim->EventCmdParam.FourFlt[3]));
      break;
      
   case SC_SIM_SCANF_NONE:
      break;

   default:
      break;

   } /* End scanf switch */

   switch (ScSim->NextEventCmd->SubSys)
   {

      case SC_SIM_Subsystem_SIM:
         SIM_ProcessEventCmd(ScSim->NextEventCmd);
         break;

      case SC_SIM_Subsystem_ADCS:
         ADCS_ProcessEventCmd(ADCS, ScSim->NextEventCmd);
         break;
      
      case SC_SIM_Subsystem_CDH:
         CDH_ProcessEventCmd(CDH, ScSim->NextEventCmd);
         break;
      
      case SC_SIM_Subsystem_COMM:
         COMM_ProcessEventCmd(COMM, ScSim->NextEventCmd);
         break;
      
      case SC_SIM_Subsystem_FSW:
         FSW_ProcessEventCmd(FSW, ScSim->NextEventCmd);
         break;
      
      case SC_SIM_Subsystem_INSTR:
         INSTR_ProcessEventCmd(INSTR, ScSim->NextEventCmd);
         break;

      case SC_SIM_Subsystem_POWER:
         POWER_ProcessEventCmd(POWER, ScSim->NextEventCmd);
         break;
      
      case SC_SIM_Subsystem_THERM:
         THERM_ProcessEventCmd(THERM, ScSim->NextEventCmd);
         break;

      default:
         break;
   
   } /* End subsystem switch */
 
   
   ScSim->LastEventCmd = ScSim->NextEventCmd;
   
   ScSim->NextEventCmd = &(ScSim->Scenario[ScSim->NextEventCmd->Link.Next]);
       
   CFE_EVS_SendEvent(SC_SIM_EXECUTE_EVENT_EID, CFE_EVS_EventType_DEBUG, 
                     "Exit SIM_ExecuteEventCmd(): Next Cmd link (%d,%d), time %d, susbsy %d, cmd %d",
                     ScSim->NextEventCmd->Link.Prev, ScSim->NextEventCmd->Link.Next, ScSim->NextEventCmd->Time, ScSim->NextEventCmd->SubSys, ScSim->NextEventCmd->Id);
   
} /* End SIM_ExecuteEventCmd() */


/******************************************************************************
** Function:  SIM_SetTime
**
** Use CFE_TIME command messsage interface rather than CFE_TIME_ExternalTime() 
** because CFE_TIME_ExternalTime() requires compiling CFE_TIME in a configuration
** I didn't want to use.  
*/
static void SIM_SetTime(uint32 NewSeconds)
{
   

   ScSim->Time.Seconds = NewSeconds;
   
   CfeSetTimeCmd.Payload.Seconds      = NewSeconds;
   CfeSetTimeCmd.Payload.MicroSeconds = 0;

   CFE_MSG_GenerateChecksum(CFE_MSG_PTR(CfeSetTimeCmd.CommandBase));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(CfeSetTimeCmd.CommandBase), true);

} /* SIM_SetTime() */

/******************************************************************************
** Function: SIM_StopSim
**
*/
static void SIM_StopSim(void)
{
   
   CFE_EVS_SendEvent(SC_SIM_STOP_SIM_EID, CFE_EVS_EventType_INFORMATION, "SC_SIM stopped at %d seconds", ScSim->Time.Seconds);

   ScSim->Time.Seconds = 0;
   ScSim->Active       = false;
   ScSim->Phase        = SC_SIM_Phase_IDLE;
   
   ScSim->LastEventCmd = &SimIdleCmd;
   ScSim->NextEventCmd = &SimIdleCmd;

   SC_SIM_StopPlbkCmd(NULL, NULL);

   strcpy(CfeEnaAppEventsCmd.Payload.AppName,"CFE_SB");
   CFE_MSG_GenerateChecksum(CFE_MSG_PTR(CfeEnaAppEventsCmd.CommandBase));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(CfeEnaAppEventsCmd.CommandBase), true);
   
   strcpy(CfeEnaAppEventsCmd.Payload.AppName,"CFE_TIME");
   CFE_MSG_GenerateChecksum(CFE_MSG_PTR(CfeEnaAppEventsCmd.CommandBase));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(CfeEnaAppEventsCmd.CommandBase), true);

   strcpy(CfeEnaAppEventsCmd.Payload.AppName,"KIT_SCH");
   CFE_MSG_GenerateChecksum(CFE_MSG_PTR(CfeEnaAppEventsCmd.CommandBase));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(CfeEnaAppEventsCmd.CommandBase), true);

} /* End SIM_StopSim() */

/******************************************************************************
** Functions: SIM_ProcessEventCmd
**
** Notes:
**   None
*/
static bool SIM_ProcessEventCmd(const SC_SIM_EventCmd_t *EventCmd)
{
   
   bool RetStatus = true;
   
   switch ((SC_SIM_EventCmd_Enum_t)EventCmd->Id)
   {
      
   case SC_SIM_EventCmd_STOP_SIM:
      SIM_StopSim();
      break;

   default:
	   RetStatus = false;
      break;
      
   } /* End Cmd Id switch */
   
   return RetStatus;
   
} /* End SIM_ProcessEventCmd() */


/********************************/
/********************************/
/****                        ****/
/****    ADCS MODEL OBJECT   ****/
/****                        ****/
/********************************/
/********************************/


static SC_SIM_EventCmd_t AdcsIdleCmd = { {0,0}, SC_SIM_IDLE_TIME, SC_SIM_Subsystem_ADCS,  ADCS_EVT_UNDEF,  SC_SIM_SCANF_NONE,  NULL};

static const char* AdcsModeStr[] =
{
   
   "UNDEF",     /* SC_SIM_AdcsMode_UNDEF      = 0 */
   "SAFEHOLD",  /* SC_SIM_AdcsMode_SAFEHOLD   = 1 */
   "SUN_POINT", /* SC_SIM_AdcsMode_SUN_POINT  = 2 */
   "INTERIAL",  /* SC_SIM_AdcsMode_INERTIAL   = 3 */
   "SLEW"       /* SC_SIM_AdcsMode_SLEW       = 4 */

};

/******************************************************************************
** Functions: ADCS_Init
**
** Initialize the ADCS model to a known state.
**
** Notes:
**   None
*/
static void ADCS_Init(ADCS_Model_t *Adcs)
{

   CFE_PSP_MemSet((void*)Adcs, 0, sizeof(ADCS_Model_t));
   
   Adcs->LastEventCmd = AdcsIdleCmd;
 
   Adcs->Eclipse = true;
   Adcs->Mode    = SC_SIM_AdcsMode_UNDEF;

} /* ADCS_Init() */

/******************************************************************************
** Functions: ADCS_Execute
**
** Update ADCS model state.
**
** Notes:
**   None
*/
static void ADCS_Execute(ADCS_Model_t *Adcs)
{
   
   /* TODO - Implement model */

} /* ADCS_Execute() */


/******************************************************************************
** Functions: ADCS_ProcessEventCmd
**
** Notes:
**   None
*/
static bool ADCS_ProcessEventCmd(ADCS_Model_t *Adcs, const SC_SIM_EventCmd_t *EventCmd)
{
   
   bool RetStatus = true;
   
   switch ((ADCS_EventCmd_t)EventCmd->Id)
   {
      
   case ADCS_EVT_SET_MODE:
      CFE_EVS_SendEvent(ADCS_CHANGE_MODE_EID, CFE_EVS_EventType_INFORMATION,"ADCS: Control mode changed from %s to %s",
      AdcsModeStr[Adcs->Mode], AdcsModeStr[ScSim->EventCmdParam.OneInt]); 
      Adcs->Mode = ScSim->EventCmdParam.OneInt;
      break;

   case ADCS_EVT_ENTER_ECLIPSE:
      Adcs->Eclipse = true;
      CFE_EVS_SendEvent(ADCS_ENTER_ECLIPSE_EID, CFE_EVS_EventType_INFORMATION,"ADCS: Enter eclipse"); 
      break;

   case ADCS_EVT_EXIT_ECLIPSE:
      Adcs->Eclipse = false;
      CFE_EVS_SendEvent(ADCS_EXIT_ECLIPSE_EID, CFE_EVS_EventType_INFORMATION,"ADCS: Exit eclipse"); 
      break;

   default:
	   RetStatus = false;
      break;
      
   } /* End Cmd Id switch */
   
   Adcs->LastEventCmd = *EventCmd;
   
   return RetStatus;
   
} /* ADCS_ProcessEventCmd() */


/**************************/
/**************************/
/****                  ****/
/****    CDH MODEL     ****/
/****                  ****/
/**************************/
/**************************/

static const char *HwCmdStr[] =
{

   "UNDEF",       /* CDH_HW_CMD_UNDEF       = 0 */
   "RST_SBC",     /* CDH_HW_CMD_RST_SBC     = 1 */
   "PWR_CYC_SBC", /* CDH_HW_CMD_PWR_CYC_SBC = 2 */
   "SEL_BOOT_A",  /* CDH_HW_CMD_SEL_BOOT_A  = 3 */
   "SEL_BOOT_B"   /* CDH_HW_CMD_SEL_BOOT_B  = 4 */

};

/******************************************************************************
** Functions: CDH_Init
**
** Initialize the CDH model to a known state.
**
** Notes:
**   None
*/
static void CDH_Init(CDH_Model_t *Cdh)
{

   CFE_PSP_MemSet((void*)Cdh, 0, sizeof(CDH_Model_t));

} /* CDH_Init() */


/******************************************************************************
** Functions: CDH_Execute
**
** Update CDH model state.
**
** Notes:
**   None
*/
static void CDH_Execute(CDH_Model_t *Cdh)
{
   
   /* TODO - Implement model */

} /* CDH_Execute() */


/******************************************************************************
** Functions: CDH_ProcessEventCmd
**
** Notes:
**   None
*/
static bool CDH_ProcessEventCmd(CDH_Model_t *Cdh, const SC_SIM_EventCmd_t *EventCmd)
{
   
   bool RetStatus = true;
   uint16 HwCmdStrIdx = 0;
   
   switch ((CDH_EventCmd_t)EventCmd->Id)
   {
   
   case CDH_EVT_WATCHDOG_RST:
      CFE_EVS_SendEvent(CDH_WATCHDOG_RESET_EID, CFE_EVS_EventType_ERROR, "C&DH: Watchdog Reset. Call Professor Wildermann for assistance!!!!");       
      Cdh->SbcRstCnt++;
      break;

   case CDH_EVT_SEND_HW_CMD:
      Cdh->HwCmdCnt++;
      Cdh->LastHwCmd = (CDH_HardwareCmd_t)ScSim->EventCmdParam.OneInt;
      switch (Cdh->LastHwCmd)
      {
      case CDH_HW_CMD_RST_SBC:
         Cdh->SbcRstCnt++;
         break;
      case CDH_HW_CMD_PWR_CYC_SBC:
         break;
      case CDH_HW_CMD_SEL_BOOT_A:
         break;
      case CDH_HW_CMD_SEL_BOOT_B:
         break;
      default:
	      RetStatus = false;
         break;
      }

      HwCmdStrIdx = (RetStatus == true)? Cdh->LastHwCmd : 0;
      CFE_EVS_SendEvent(998, CFE_EVS_EventType_INFORMATION, "CDH: Received hardware command %s", HwCmdStr[HwCmdStrIdx]);
   
   default:
	   RetStatus = false;
      break;
      
   } /* End Cmd Id switch */
      
   Cdh->LastEventCmd = *EventCmd;
      
   return RetStatus;
   
} /* CDH_ProcessEventCmd() */



/*********************************/
/*********************************/
/****                         ****/
/****    COMM MODEL OBJECT    ****/
/****                         ****/
/*********************************/
/*********************************/

static SC_SIM_EventCmd_t CommIdleCmd = { {0,0}, SC_SIM_IDLE_TIME, SC_SIM_Subsystem_COMM,  COMM_EVT_UNDEF,  SC_SIM_SCANF_NONE,  NULL};

/******************************************************************************
** Functions: COMM_Init
**
** Initialize the Comm model to a known state.
**
** Notes:
**   None
*/
static void COMM_Init(COMM_Model_t *Comm)
{

   CFE_PSP_MemSet((void*)Comm, 0, sizeof(COMM_Model_t));
   
   Comm->LastEventCmd = CommIdleCmd;
   
   Comm->InContact = false;
   Comm->Contact.Link = COMM_LINK_UNDEF;
   Comm->Contact.TimePending = -1;

} /* COMM_Init() */

/******************************************************************************
** Functions: COMM_EndContact
**
*/
static void COMM_EndContact(COMM_Model_t *Comm)
{

   Comm->InContact             = false;
   Comm->Contact.Link          = COMM_LINK_UNDEF;
   Comm->Contact.Length        = 0;
   Comm->Contact.TimePending   = -1;
   Comm->Contact.TimeConsumed  = 0;
   Comm->Contact.TimeRemaining = 0;

} /* COMM_EndContact() */


/******************************************************************************
** Functions: COMM_Execute
**
** Update Comm model state.
**
** Notes:
**   None
*/
static void COMM_Execute(COMM_Model_t *Comm)
{
   if (Comm->InContact)
   {
   
      Comm->Contact.TimeConsumed++;
      Comm->Contact.TimeRemaining--;
      
      if (Comm->Contact.TimeRemaining <= 0)
      {   
         COMM_EndContact(Comm);  
      }
         
   } /* End if in contact */
   else
   {   
      if (Comm->Contact.TimePending > 0)
      {
      
         Comm->Contact.TimePending--;
         if (Comm->Contact.TimePending == 0)
         {
            
            Comm->InContact = true;
            Comm->Contact.TimeConsumed  = 0;
            Comm->Contact.TimeRemaining = Comm->Contact.Length;
            
            CFE_EVS_SendEvent(COMM_START_CONTACT_EID, CFE_EVS_EventType_INFORMATION, "Started contact with length of %d seconds", Comm->Contact.Length);
         }        
      } /* End if pending contact */
   } /* End if not in contact */
   
} /* COMM_Execute() */


/******************************************************************************
** Functions: COMM_ProcessEventCmd
**
** Notes:
**   None
*/
static bool COMM_ProcessEventCmd(COMM_Model_t *Comm, const SC_SIM_EventCmd_t *EventCmd)
{
   
   bool RetStatus = true;
   SC_SIM_EventCmd_t LosEventCmd;
         
   CFE_EVS_SendEvent(COMM_PROCESS_EVENT_EID, CFE_EVS_EventType_DEBUG, "Executing COMM cmd %d", EventCmd->Id);
   
   switch ((COMM_EventCmd_t)EventCmd->Id)
   {
   
   case COMM_EVT_SCH_AOS:
 
      Comm->Contact.TimePending = ScSim->EventCmdParam.ThreeInt[0];
      Comm->Contact.Length      = ScSim->EventCmdParam.ThreeInt[1];
      Comm->Contact.Link        = ScSim->EventCmdParam.ThreeInt[2];

      Comm->InContact = false;
      Comm->Contact.TimeConsumed  = 0;
      Comm->Contact.TimeRemaining = 0;
      CFE_EVS_SendEvent(COMM_PROCESS_EVENT_EID, CFE_EVS_EventType_DEBUG, 
                        "Scheduled AOS in %ds, for %ds with link type %d", 
                        Comm->Contact.TimePending, Comm->Contact.Length, Comm->Contact.Link);
 
      LosEventCmd.Link.Prev = SC_SIM_EVT_CMD_NULL_IDX;
      LosEventCmd.Link.Next = SC_SIM_EVT_CMD_NULL_IDX;
      LosEventCmd.Time      = EventCmd->Time + Comm->Contact.Length;
      LosEventCmd.SubSys    = SC_SIM_Subsystem_COMM;
      LosEventCmd.Id        = COMM_EVT_LOS;
      LosEventCmd.ScanfType = SC_SIM_SCANF_NONE;
      LosEventCmd.Param     = NULL;
      
      SIM_AddEventCmd(&LosEventCmd);
      
      break;
   
   case COMM_EVT_ABORT_CONTACT:
      COMM_EndContact(Comm);  
      break;

   case COMM_EVT_SET_DATA_RATE:
      Comm->Contact.DataRate = ScSim->EventCmdParam.OneInt;
      break;

   case COMM_EVT_SET_TDRS_ID:
      Comm->Contact.TdrsId = ScSim->EventCmdParam.OneInt;
      break;
   
   default:
	   RetStatus = false;
      break;
      
   } /* End Cmd Id switch */

   Comm->LastEventCmd = *EventCmd;
   
   return RetStatus;
   
} /* COMM_ProcessEventCmd() */


/********************************************/
/********************************************/
/****                                    ****/
/****    FLIGHT SOFTWARE MODEL OBJECT    ****/
/****                                    ****/
/********************************************/
/********************************************/

/******************************************************************************
** Functions: FSW_Init
**
** Initialize the Flight Software model to a known state.
**
** Notes:
**   None
*/
static void FSW_Init(FSW_Model_t *Fsw)
{

   CFE_PSP_MemSet((void*)Fsw, 0, sizeof(FSW_Model_t));
   
} /* FSW_Init() */


/******************************************************************************
** Functions: FSW_Execute
**
** Update Flight Software model state.
**
** Notes:
**   None
*/
static void FSW_Execute(FSW_Model_t *Fsw)
{
   
   /* TODO - Sim CDH & FSW files */
   //Fsw->Recorder.FileCnt = INSTR->FileCnt;

   if (Fsw->Recorder.PlaybackEna)
   {
   
      if (Fsw->Recorder.FileCnt == 0) Fsw->Recorder.PlaybackEna = false;
      if (Fsw->Recorder.FileCnt > 0)  Fsw->Recorder.FileCnt--;

   }

} /* FSW_Execute() */


/******************************************************************************
** Functions: FSW_ProcessEventCmd
**
** Notes:
**   None
*/
static bool FSW_ProcessEventCmd(FSW_Model_t* Fsw, const SC_SIM_EventCmd_t* EventCmd)
{
   
   bool RetStatus = true;
   
   switch ((FSW_EventCmd_t)EventCmd->Id)
   {

   case FSW_EVT_SET_REC_FILE_CNT:
      Fsw->Recorder.FileCnt = ScSim->EventCmdParam.OneInt;
      break;

   case FSW_EVT_SET_REC_PCT_USED:
      Fsw->Recorder.PctUsed = ScSim->EventCmdParam.OneFlt;
      break;
      
   case FSW_EVT_START_REC_PLBK:
      Fsw->Recorder.PlaybackEna = true;
      break;

   case FSW_EVT_STOP_REC_PLBK:
      Fsw->Recorder.PlaybackEna = false;
      break;

   case FSW_EVT_CLR_EVT_LOG:
OS_printf("CFE_SB_TransmitMsg(CFE_MSG_PTR(CfeClrEventLogCmd), true);\n");
      CFE_SB_TransmitMsg(CFE_MSG_PTR(CfeClrEventLogCmd), true);
      break;

   default:
	   RetStatus = false;
      break;
      
   } /* End Cmd Id switch */
   
   
   Fsw->LastEventCmd = *EventCmd;
   
   return RetStatus;
   
} /* FSW_ProcessEventCmd() */


/***************************************/
/***************************************/
/****                               ****/
/****    INSTRUMENT MODEL OBJECT    ****/
/****                               ****/
/***************************************/
/***************************************/

/******************************************************************************
** Functions: INSTR_Init
**
** Initialize the Instrument model to a known state.
**
** Notes:
**   None
*/
static void INSTR_Init(INSTR_Model_t *Instr)
{

   CFE_PSP_MemSet((void*)Instr, 0, sizeof(INSTR_Model_t));

} /* INSTR_Init() */


/******************************************************************************
** Functions: INSTR_Execute
**
** Update Instrument model state.
**
** Notes:
**   None
*/
static void INSTR_Execute(INSTR_Model_t *Instr)
{

  if (Instr->PwrEna && Instr->SciEna)
  {
  
     Instr->FileCycCnt++;
     if (Instr->FileCycCnt >= INSTR_CYCLES_PER_FILE)
     {
        
        //Instr->FileCnt++;
        FSW->Recorder.FileCnt++;
        Instr->FileCycCnt = 0;
     
     }
  }
  else
  {
     Instr->FileCycCnt = 0;
  }
   
} /* INSTR_Execute() */


/******************************************************************************
** Functions: INSTR_ProcessEventCmd
**
** Notes:
**   None
*/
static bool INSTR_ProcessEventCmd(INSTR_Model_t *Instr, const SC_SIM_EventCmd_t *EventCmd)
{
   
   bool RetStatus = true;
   
   switch ((INSTR_EventCmd_t)EventCmd->Id)
   {
   case INSTR_EVT_ENA_POWER:
      Instr->PwrEna = true;
      break;

   case INSTR_EVT_DIS_POWER:
      CFE_EVS_SendEvent(INSTR_DIS_POWER_EID, CFE_EVS_EventType_INFORMATION, "INSTR: Science instrument powered off");       
      Instr->PwrEna = false;
      break;

   case INSTR_EVT_ENA_SCIENCE:
      Instr->SciEna = true;
      break;

   case INSTR_EVT_DIS_SCIENCE:
      Instr->SciEna = false;
      break;

   default:
	   RetStatus = false;
      break;
      
   } /* End Cmd Id switch */
      
   Instr->LastEventCmd = *EventCmd;
      
   return RetStatus;
   
} /* INSTR_ProcessEventCmd() */


/**********************************/
/**********************************/
/****                          ****/
/****    POWER MODEL OBJECT    ****/
/****                          ****/
/**********************************/
/**********************************/

/******************************************************************************
** Functions: POWER_Init
**
** Initialize the Power model to a known state.
**
** Notes:
**   None
*/
static void POWER_Init(POWER_Model_t *Power)
{

   CFE_PSP_MemSet((void*)Power, 0, sizeof(POWER_Model_t));

} /* POWER_Init() */


/******************************************************************************
** Functions: POWER_Execute
**
** Update Power model state.
**
** Notes:
**   None
*/
static void POWER_Execute(POWER_Model_t *Power)
{

   /*
   ** Simple linear model not based on any physics. 
   ** +/- 1% change for every minute charging/discharging   
   */
   
   if (ADCS->Eclipse == true)
   {

     Power->SaCurrent = 0.0;

     Power->BattSoc -= 1.0/50.0;
     if (Power->BattSoc > 100.0) Power->BattSoc = 100.0;
     
   }
   else
   {

     Power->SaCurrent = 10.0;

     Power->BattSoc += 1.0/50.0;
     if (Power->BattSoc < 0.0) Power->BattSoc = 0.0;
   
   }
   
} /* POWER_Execute() */


/******************************************************************************
** Functions: POWER_ProcessEventCmd
**
** Notes:
**   None
*/
static bool POWER_ProcessEventCmd(POWER_Model_t *Power, const SC_SIM_EventCmd_t *EventCmd)
{
   
   bool RetStatus = true;
   
   switch ((POWER_EventCmd_t)EventCmd->Id)
   {
      
   case POWER_EVT_SET_BATT_SOC:
      Power->BattSoc = ScSim->EventCmdParam.OneFlt;
      break;

   case POWER_EVT_SET_SA_CURRENT:
      Power->SaCurrent = ScSim->EventCmdParam.OneFlt;
      break;

   default:
	   RetStatus = false;
      break;
      
   } /* End Cmd Id switch */
      
   Power->LastEventCmd = *EventCmd;
      
   return RetStatus;
   
} /* POWER_ProcessEventCmd() */


/************************************/
/************************************/
/****                            ****/
/****    THERMAL MODEL OBJECT    ****/
/****                            ****/
/************************************/
/************************************/

/******************************************************************************
** Functions: THERM_Init
**
** Initialize the Thermal model to a known state.
**
** Notes:
**   None
*/
static void THERM_Init(THERM_Model_t *Therm)
{

   CFE_PSP_MemSet((void*)Therm, 0, sizeof(THERM_Model_t));
   
} /* THERM_Init() */


/******************************************************************************
** Functions: Thermal_Execute
**
** Update Thermal model state.
**
** Notes:
**   None
*/
static void THERM_Execute(THERM_Model_t *Therm)
{
   
   if (ADCS->Eclipse == true)
   {
      Therm->Heater1Ena = true;
      Therm->Heater2Ena = true;  
   }
   else
   {
      Therm->Heater1Ena = false;
      Therm->Heater2Ena = false;
   }
   
} /* THERM_Execute() */


/******************************************************************************
** Functions: THERM_ProcessEventCmd
**
** Notes:
**   None
*/
static bool THERM_ProcessEventCmd(THERM_Model_t *Therm, const SC_SIM_EventCmd_t *EventCmd)
{
   
   bool RetStatus = true;
   
   switch ((THERM_EventCmd_t)EventCmd->Id)
   {

   case THERM_EVT_ENA_HEATER_1:
      Therm->Heater1Ena = ScSim->EventCmdParam.OneInt;
      break;

   case THERM_EVT_ENA_HEATER_2:
      Therm->Heater2Ena = ScSim->EventCmdParam.OneInt;
      break;

   default:
	   RetStatus = false;
      break;
      
   } /* End Cmd Id switch */
   
   
   Therm->LastEventCmd = *EventCmd;
   
   return RetStatus;
   
} /* THERM_ProcessEventCmd() */
