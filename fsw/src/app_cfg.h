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
** Purpose: Define configurations for the Spacecraft Simulator App
**
** Notes:
**   1. These macros can only be built with the application and can't
**      have a platform scope because the same file name is used for
**      all applications following the object-based application design.
**
** References:
**   1. cFS Basecamp Object-based Application Developers Guide.
**   2. cFS Application Developer's Guide.
**
*/

#ifndef _app_cfg_
#define _app_cfg_

/*
** Includes
*/

#include "cfe.h"
#include "sc_sim_platform_cfg.h"
#include "app_c_fw.h"


/******************************************************************************
** SC_SIM Application Macros
*/

/*
** Versions:
**
** 1.0 - Initial release
*/

#define  SC_SIM_MAJOR_VER      1
#define  SC_SIM_MINOR_VER      1


/******************************************************************************
** Init File declarations create:
**
**  typedef enum {
**     CMD_PIPE_DEPTH,
**     CMD_PIPE_NAME
**  } INITBL_ConfigEnum;
**    
**  typedef struct {
**     CMD_PIPE_DEPTH,
**     CMD_PIPE_NAME
**  } INITBL_ConfigStruct;
**
**   const char *GetConfigStr(value);
**   ConfigEnum GetConfigVal(const char *str);
**
** XX(name,type)
*/

#define CFG_APP_CFE_NAME        APP_CFE_NAME
#define CFG_APP_MAIN_PERF_ID    APP_MAIN_PERF_ID

#define CFG_APP_CMD_PIPE_DEPTH  APP_CMD_PIPE_DEPTH
#define CFG_APP_CMD_PIPE_NAME   APP_CMD_PIPE_NAME

#define CFG_SC_SIM_CMD_TOPICID       SC_SIM_CMD_TOPICID
#define CFG_BC_SCH_1_HZ_TOPICID      BC_SCH_1_HZ_TOPICID
#define CFG_SC_SIM_HK_TLM_TOPICID    SC_SIM_HK_TLM_TOPICID
#define CFG_SC_SIM_MGMT_TLM_TOPICID  SC_SIM_MGMT_TLM_TOPICID
#define CFG_SC_SIM_MODEL_TLM_TOPICID SC_SIM_MODEL_TLM_TOPICID
#define CFG_KIT_TO_CMD_TOPICID       KIT_TO_CMD_TOPICID
#define CFG_EVS_CMD_TOPICID          EVS_CMD_TOPICID
#define CFG_TIME_CMD_TOPICID         TIME_CMD_TOPICID

#define CFG_SC_SIM_TBL_LOAD_FILE  SC_SIM_TBL_LOAD_FILE
#define CFG_SC_SIM_TBL_DUMP_FILE  SC_SIM_TBL_DUMP_FILE


#define APP_CONFIG(XX) \
   XX(APP_CFE_NAME,char*) \
   XX(APP_MAIN_PERF_ID,uint32) \
   XX(APP_CMD_PIPE_DEPTH,uint32) \
   XX(APP_CMD_PIPE_NAME,char*) \
   XX(SC_SIM_CMD_TOPICID,uint32) \
   XX(BC_SCH_1_HZ_TOPICID,uint32) \
   XX(SC_SIM_HK_TLM_TOPICID,uint32) \
   XX(SC_SIM_MGMT_TLM_TOPICID,uint32) \
   XX(SC_SIM_MODEL_TLM_TOPICID,uint32) \
   XX(KIT_TO_CMD_TOPICID,uint32) \
   XX(EVS_CMD_TOPICID,uint32) \
   XX(TIME_CMD_TOPICID,uint32) \
   XX(SC_SIM_TBL_LOAD_FILE,char*) \
   XX(SC_SIM_TBL_DUMP_FILE,char*) \

DECLARE_ENUM(Config,APP_CONFIG)


/******************************************************************************
** Event Macros
** 
** Define the base event message IDs used by each object/component used by the
** application. There are no automated checks to ensure an ID range is not
** exceeded so it is the developer's responsibility to verify the ranges. 
*/

#define SC_SIM_APP_BASE_EID  (APP_C_FW_APP_BASE_EID +  0)
#define SC_SIM_BASE_EID      (APP_C_FW_APP_BASE_EID + 10)
#define SC_SIM_TBL_BASE_EID  (APP_C_FW_APP_BASE_EID + 50)
        
/*
** One event ID is used for all initialization debug messages. Uncomment one of
** the SC_SIM_INIT_EVS_TYPE definitions. Set it to INFORMATION if you want to
** see the events during initialization. This is opposite to what you'd expect 
** because INFORMATION messages are enabled by default when an app is loaded.
*/

#define SC_SIM_INIT_DEBUG_EID 999
#define SC_SIM_INIT_EVS_TYPE CFE_EVS_DEBUG
//#define SC_SIM_INIT_EVS_TYPE CFE_EVS_INFORMATION

/******************************************************************************
** SC_SIM
**
*/

#define SC_SIM_DEBUG (0)

/******************************************************************************
** SC_SIM Table Macros
*/

#define SC_SIM_TBL_JSON_FILE_MAX_CHAR  5000 

#endif /* _app_cfg_ */
