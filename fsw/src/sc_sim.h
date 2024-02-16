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
** Purpose: Define Spacecraft Simulator Object
**
** Notes:
**   1. For the initial versions of the subsystem simulations I broke from
**      convention of defining one object per file. The simulations are
**      trivial and tightly coupled for hard coded scenarios.
**   2. The simulation models will evolve as will the combination of 
**      using models vs actual apps for function like C&DH recorder and also
**      the incorporation of 42. My goal is NOT to rewrite 42 but leverage
**      its capabilities.
**   3. Consider managing the sim with a state machine that has events
**      that define sim model behavior. Could be very generic with an
**      event-sim-model superclass. I would like the sim models to be driven
**      by 42 reality.
**
*/

#ifndef _sc_sim_
#define _sc_sim_

/*
** Includes
*/

#include "app_cfg.h"
#include "sc_sim_tbl.h"
#include "sc_sim_eds_typedefs.h"

/***********************/
/** Macro Definitions **/
/***********************/


/*
** Event Message IDs
*/

#define SC_SIM_START_SIM_EID        (SC_SIM_BASE_EID +  0)
#define SC_SIM_START_SIM_ERR_EID    (SC_SIM_BASE_EID +  1)
#define SC_SIM_STOP_SIM_EID         (SC_SIM_BASE_EID +  2)
#define SC_SIM_LOAD_TBL_EID         (SC_SIM_BASE_EID +  3)
#define SC_SIM_LOAD_TBL_OBJ_EID     (SC_SIM_BASE_EID +  4)
#define SC_SIM_START_REC_PLBK_EID   (SC_SIM_BASE_EID +  5)
#define SC_SIM_STOP_REC_PLBK_EID    (SC_SIM_BASE_EID +  6)
#define SC_SIM_ADD_EVENT_EID        (SC_SIM_BASE_EID +  7)
#define SC_SIM_EXECUTE_EVENT_EID    (SC_SIM_BASE_EID +  8)
#define SC_SIM_EVENT_ERR_EID        (SC_SIM_BASE_EID +  9)
#define SC_SIM_EXECUTE_EID          (SC_SIM_BASE_EID + 10)
#define SC_SIM_ACCEPT_NEW_TBL_EID   (SC_SIM_BASE_EID + 11)
#define SC_SIM_PROCESS_MQTT_CMD_EID (SC_SIM_BASE_EID + 12)

#define ADCS_ENTER_ECLIPSE_EID    (SC_SIM_BASE_EID + 20)
#define ADCS_EXIT_ECLIPSE_EID     (SC_SIM_BASE_EID + 21)
#define ADCS_CHANGE_MODE_EID      (SC_SIM_BASE_EID + 22)

#define CDH_WATCHDOG_RESET_EID    (SC_SIM_BASE_EID + 30)

#define COMM_START_CONTACT_EID    (SC_SIM_BASE_EID + 40)
#define COMM_PROCESS_EVENT_EID    (SC_SIM_BASE_EID + 41)

// FSW + 50

#define INSTR_DIS_POWER_EID       (SC_SIM_BASE_EID + 60)

// POWER + 70

// THERM + 80

/*
** Simulation management 
**
** Simulations have the following phases:
**   1. Initialization
**      - Each subsystem should have one or more commands to initialize the
**        subsystem to a known state
**   2. Time-lapse
**      - The simulation runs much faster than realtime to propagate each 
**        subsystem state to a point for the simulation to start
**      - Time-lapse starts with the first command scheduled prior to the
**        realtime epoch. 
**   3. Realtime
**      - The simulation progresses at realtime
**
** Sim event time tag conventions drive each simulation phase. 
** - 0: A single init time of zero is used to initialize models so all cmds
**   used to set initial model states should have the same time stamp.
** - 1..(Epoch-1) time is used for the time-lapse phase. This time is 
**   relative to the epoch.
** - Epoch defines when realtime begins. The epoch was defined so one
**   orbit can be performed during the time-lapsed phase.
*/

#define SC_SIM_IDLE_TIME             (0)  /* No sim active */

#define SC_SIM_INIT_TIME             (1)  /* Model initialization */  

#define SC_SIM_TIME_LAPSE_EXE_CNT (1000)  /* Number of simulation seconds to perform in one SC_SIM execution cycle during time lapse phase */

#define SC_SIM_REALTIME_EPOCH    (10000)  /* Time when realtime simulation starts */ 
#define SC_SIM_REALTIME_END      (20000)  /* Sim doesn't execute until this time. Thsi time indicates sim is over */ 

#define SC_SIM_EVT_CMD_MAX          (25)  /* Maximum number of event commands */
#define SC_SIM_EVT_CMD_NULL_IDX     (99)  /* Maximum number of event commands */

/**********************/
/** Type Definitions **/
/**********************/

/* 
** The following enumerate types are defined in the EDS mainly 
** to allow text to be used in commands and displayed in telemetry:
**    SC_SIM_Phase
**    SC_SIM_SubSys
**    SC_SIM_EventCmd
**    SC_SIM_AdcsMode
**
** In an object oriented design, the EDS enumerations and the
** enums/structs defined below would be defined in abstract
** base classes for subsystems and events. This encapsulation
** would be simplify maintainance. Care must be taken to update
** the EDS and definitions below as subsystems and events evolve. 
*/

typedef enum 
{

   SC_SIM_SCANF_UNDEF    = 0,
   SC_SIM_SCANF_1_INT    = 1,
   SC_SIM_SCANF_2_INT    = 2,
   SC_SIM_SCANF_3_INT    = 3,
   SC_SIM_SCANF_1_FLT    = 4,
   SC_SIM_SCANF_3_FLT    = 5,
   SC_SIM_SCANF_4_FLT    = 6,
   SC_SIM_SCANF_NONE     = 7,
   SC_SIM_SCANF_TYPE_CNT = 8
   
} SC_SIM_ScanfType_t;

typedef struct
{
   
   int32  OneInt;
   int32  TwoInt[2];
   int32  ThreeInt[3];
   float  OneFlt;
   float  ThreeFlt[3];
   float  FourFlt[4];
   
} SC_SIM_EventCmdParam_t;

/*
** Events have the general format of 
** - Time, Subsystem ID, Subsystem Event, Scanf Type, Parameters 
**
** See file prologue and macro section above for a simulation architecture
** overview.
*/


/*

TODO: Restructure and use const for payload
TODO: Should time be unsigned?

typedef struct
{

   int32   Time;    
   uint16  Prev;
   uint16  Next;
      
} SC_SIM_EventCmdHeader_t;


typedef struct
{

   SC_SIM_Subsystem_Enum_t  SubSys;
   uint8                    Id;
   SC_SIM_ScanfType_t       ScanfType;
   const char               *Param;
      
} SC_SIM_EventCmdPayload_t;

typedef struct
{

   SC_SIM_EventCmdHeader_t         Header;
   const SC_SIM_EventCmdPayload_t  Payload;
      
} SC_SIM_EventCmd_t;

*/

typedef struct
{
 
   uint16  Prev;
   uint16  Next;
      
} SC_SIM_EventCmdLink_t;

typedef struct
{

   SC_SIM_EventCmdLink_t    Link;
   int32                    Time;
   SC_SIM_Subsystem_Enum_t  SubSys;
   uint8                    Id;
   SC_SIM_ScanfType_t       ScanfType;
   const char               *Param;
      
} SC_SIM_EventCmd_t;


/**********/
/** ADCS **/
/**********/

/* Preliminary events to get started. Need scenarios to determine what's needed */
typedef enum
{

   ADCS_EVT_UNDEF          = 0,
   ADCS_EVT_SET_MODE       = 1,
   ADCS_EVT_ENTER_ECLIPSE  = 2,
   ADCS_EVT_EXIT_ECLIPSE   = 3,
   ADCS_EVT_SET_ATTITUDE   = 4,
   ADCS_EVT_SET_ORBIT      = 5

} ADCS_EventCmd_t;

typedef enum
{

   ADCS_MODE_UNDEF      = 0,
   ADCS_MODE_SAFEHOLD   = 1,
   ADCS_MODE_SUN_POINT  = 2,
   ADCS_MODE_INERTIAL   = 3,
   ADCS_MODE_SLEW       = 4

} ADCS_Mode_t;

typedef struct
{

   /* Model Management */

   SC_SIM_EventCmd_t  LastEventCmd;

   /* Model State */
   
   bool         Eclipse;
   ADCS_Mode_t  Mode;
   double       AttErr;
   
} ADCS_Model_t;


/*********/
/** CDH **/
/*********/


typedef enum
{

   CDH_EVT_UNDEF        = 0,
   CDH_EVT_WATCHDOG_RST = 1,
   CDH_EVT_SEND_HW_CMD  = 2
   
} CDH_EventCmd_t;


typedef enum
{

   CDH_HW_CMD_UNDEF       = 0,
   CDH_HW_CMD_RST_SBC     = 1,
   CDH_HW_CMD_PWR_CYC_SBC = 2,
   CDH_HW_CMD_SEL_BOOT_A  = 3,
   CDH_HW_CMD_SEL_BOOT_B  = 4
   
} CDH_HardwareCmd_t;


typedef struct
{

   /* Model Management */

   SC_SIM_EventCmd_t  LastEventCmd;

   /* Model State */

   uint16  SbcRstCnt;   /* From either watchdog or hardware command */
   uint16  HwCmdCnt;
   CDH_HardwareCmd_t  LastHwCmd;
   
} CDH_Model_t;

/**********/
/** COMM **/
/**********/

typedef enum
{

   COMM_EVT_UNDEF         = 0,
   COMM_EVT_SCH_AOS       = 1,  /* Schedule relative start time, length, Link Type */
   COMM_EVT_LOS           = 2,  /* Schedule loss of signal */
   COMM_EVT_SET_DATA_RATE = 3,  /* Set rate for next contact */
   COMM_EVT_SET_TDRS_ID   = 4,
   COMM_EVT_ABORT_CONTACT = 5
   
} COMM_EventCmd_t;


typedef enum
{

   COMM_TDRS_UNDEF    = 0,
   COMM_TDRS_ATLANTIC = 1,
   COMM_TDRS_INDIAN   = 2,
   COMM_TDRS_PACIFIC  = 3
   
} COMM_TdrsId_t;


typedef enum {

   COMM_LINK_UNDEF    = 0,
   COMM_LINK_DUPLEX   = 1,
   COMM_LINK_SIMPLEX  = 2

} COMM_Link_t;

typedef struct
{

   COMM_Link_t Link;
   uint16      TdrsId;
   uint16      DataRate;
   int16       TimePending;  /* Countdown timer until next contact. Negative value indicates no contect pending */ 
   uint16      Length;
   uint16      TimeConsumed;
   uint16      TimeRemaining;
        
} COMM_Contact_t;


typedef struct
{

   /* Model Management */

   SC_SIM_EventCmd_t  LastEventCmd;

   /* Model State */

   bool           InContact;
   COMM_Contact_t Contact;
   
} COMM_Model_t;


/*********/
/** FSW **/
/*********/

/* 
** FSW uses a combination of simulated behavior and actual FSW status. Things like 
** playback are much easier to manipulate in a simulation. 
*/
typedef enum
{

   FSW_EVT_UNDEF             = 0,
   FSW_EVT_SET_REC_FILE_CNT  = 1, /* Percentage of recorder memory used */
   FSW_EVT_SET_REC_PCT_USED  = 2, /* Number of files in recorder */
   FSW_EVT_START_REC_PLBK    = 3,
   FSW_EVT_STOP_REC_PLBK     = 4,
   FSW_EVT_CLR_EVT_LOG       = 5

} FSW_EventCmd_t;


typedef struct
{

   float   PctUsed;
   uint16  FileCnt;
   bool    PlaybackEna;

} FSW_Recorder_t;


typedef struct
{

   /* Model Management */

   SC_SIM_EventCmd_t  LastEventCmd;

   /* Model State */

   FSW_Recorder_t Recorder;
        
} FSW_Model_t;


/****************/
/** Instrument **/
/****************/

#define INSTR_CYCLES_PER_FILE  30 /* Number of simulation cycles to 'generate' a new file */
 
typedef enum
{

   INSTR_EVT_UNDEF       = 0,
   INSTR_EVT_ENA_POWER   = 1,
   INSTR_EVT_DIS_POWER   = 2,
   INSTR_EVT_ENA_SCIENCE = 3,
   INSTR_EVT_DIS_SCIENCE = 4

} INSTR_EventCmd_t;

typedef struct
{

   /* Model Management */

   SC_SIM_EventCmd_t  LastEventCmd;

   /* Model State */

   bool   PwrEna;
   bool   SciEna;
   
   int16  FileCnt;
   int16  FileCycCnt;
        
} INSTR_Model_t;


/***********/
/** Power **/
/***********/

typedef enum
{

   POWER_EVT_UNDEF          = 0,
   POWER_EVT_SET_BATT_SOC   = 1,
   POWER_EVT_SET_SA_CURRENT = 2

} POWER_EventCmd_t;

typedef struct
{

   /* Model Management */

   SC_SIM_EventCmd_t  LastEventCmd;

   /* Model State */

   float  BattSoc;
   float  SaCurrent;
   
} POWER_Model_t;


/*************/
/** Thermal **/
/*************/

typedef enum
{

   THERM_EVT_UNDEF        = 0,
   THERM_EVT_ENA_HEATER_1 = 1,
   THERM_EVT_ENA_HEATER_2 = 2

} THERM_EventCmd_t;

typedef struct
{

   /* Model Management */

   SC_SIM_EventCmd_t  LastEventCmd;

   /* Model State */

   bool   Heater1Ena;
   bool   Heater2Ena;
        
} THERM_Model_t;


/******************************************************************************
** Command & Telmetery Packets
**
** See EDS definitions in sc_sim.xml
*/


/******************************************************************************
** SC_SIM_Class
*/

typedef struct
{

   /*
   ** Commands
   */
  
   /*
   ** Telemetry Packets
   */
  
   SC_SIM_MgmtTlm_t  MgmtTlm;
   SC_SIM_ModelTlm_t ModelTlm;


   /*
   ** Tables
   */
  
   SC_SIM_TBL_Class_t Tbl;
   
   
   /* Sim Management */
   
   bool                 Active;
   SC_SIM_Phase_Enum_t  Phase;
   CFE_TIME_SysTime_t   Time;   /* Subseconds unused */
   uint32               Count;
   
   SC_SIM_EventCmd_t       *LastEventCmd;
   SC_SIM_EventCmd_t       *NextEventCmd;   
   SC_SIM_EventCmdParam_t  EventCmdParam;

   SC_SIM_EventCmd_t  *Scenario;
   uint16             ScenarioId;
   uint16             RunTimeCmdIdx;
 
   /* Sim Models */
   
   ADCS_Model_t   Adcs;
   CDH_Model_t    Cdh;
   COMM_Model_t   Comm;
   FSW_Model_t    Fsw;
   INSTR_Model_t  Instr;
   POWER_Model_t  Power;
   THERM_Model_t  Therm;
   

} SC_SIM_Class_t;


/************************/
/** Exported Functions **/
/************************/

/******************************************************************************
** Function: SC_SIM_Constructor
**
** Initialize the instrument simulator to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**   2. The table values are populated using the default table  This is done when the table is 
**      registered with the table manager.
**
*/
void SC_SIM_Constructor(SC_SIM_Class_t *ScSimPtr, INITBL_Class_t *IniTbl,
                        TBLMGR_Class_t *TblMgr);


/******************************************************************************
** Function: SC_SIM_Execute
**
** Execute a single simulation step.
**
*/
bool SC_SIM_Execute(void);


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
bool SC_SIM_ProcessMqttJsonCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: SC_SIM_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void SC_SIM_ResetStatus(void);


/******************************************************************************
** Functions: SC_SIM_StartSimCmd
**
** Start a precanned simulation. Currently this is a placeholder.
**
** Notes:
**  1. This function must comply with the CMDMGR_CmdFuncPtr definition
**
*/
bool SC_SIM_StartSimCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Functions: SC_SIM_StopSimCmd
**
** Stop the current simulation. Currently this is a placeholder.
**
** Notes:
**  1. This function must comply with the CMDMGR_CmdFuncPtr definition
**
*/
bool SC_SIM_StopSimCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Functions: SC_SIM_StartPlbkCmd
**
** Stop a recorder playback.
**
** Notes:
**  1. This function must comply with the CMDMGR_CmdFuncPtr definition
**
*/
bool SC_SIM_StartPlbkCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Functions: SC_SIM_StopPlbkCmd
**
** Stop a recorder playback.
**
** Notes:
**  1. This function must comply with the CMDMGR_CmdFuncPtr definition
**
*/
bool SC_SIM_StopPlbkCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


#endif /* _sc_sim_ */
