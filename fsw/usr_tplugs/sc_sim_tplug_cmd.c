/*
** Copyright 2022 bitValence, Inc.
** All Rights Reserved.
**
** This program is free software; you can modify and/or redistribute it
** under the terms of the GNU Affero General Public License
** as published by the Free Software Foundation; version 3 with
** attribution addendums as found in the LICENSE.txt
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Affero General Public License for more details.
**
** Purpose:
**   Manage SC_SIM TPLUG Command
**
** Notes:
**   1. The JSON payload format is defined below.
**   2. "SC_SIM" is included in event messages to identify the app
**      supplying the plugin. The event messages are reported by
**      the JMSG network app.
**
*/

/*
** Includes
*/

#include "lib_cfg.h"
#include "sc_sim_tplug_cmd.h"
#include "sc_sim_eds_typedefs.h"
#include "sc_sim_eds_cc.h"

/***********************/
/** Macro Definitions **/
/***********************/

/*
** Event Message IDs
*/

#define BASE_EID  (JMSG_PLATFORM_TopicPluginBaseEid_USR_1)

#define SC_SIM_TPLUG_CMD_INIT_SB_MSG_TEST_EID  (BASE_EID + 0)
#define SC_SIM_TPLUG_CMD_SB_MSG_TEST_EID       (BASE_EID + 1)
#define SC_SIM_TPLUG_CMD_LOAD_JSON_DATA_EID    (BASE_EID + 2)
#define SC_SIM_TPLUG_CMD_JSON_TO_CCSDS_ERR_EID (BASE_EID + 3)


/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Telemetry
** 
** SC_SIM_JMsgCmd_t & SC_SIM_JMsgCmd_CmdPayload_t defined in EDS
*/

typedef struct
{

   /*
   ** SC_SIM Command
   */

   CFE_SB_MsgId_t    TlmMsgId;   
   SC_SIM_JMsgCmd_t  JMsgCmd;
   char              JMsgPayload[1024];
      
   /*
   ** Subset of the standard CJSON table data because this isn't using the
   ** app_c_fw table manager service, but is using core-json in the same way
   ** as an app_c_fw managed table.
   */
   size_t  JsonObjCnt;

   uint32  CfeToJsonCnt;
   uint32  JsonToCfeCnt;
   
   
} SC_SIM_TPLUG_CMD_Class_t;


/************************************/
/** Local File Function Prototypes **/
/************************************/

static bool CfeToJson(const char **JMsgPayload, const CFE_MSG_Message_t *CfeMsg);
static bool JsonToCfe(CFE_MSG_Message_t **CfeMsg, const char *JMsgPayload, uint16 PayloadLen);
static bool LoadJsonData(const char *JMsgPayload, uint16 PayloadLen);
static void PluginTest(bool Init, int16 Param);


/**********************/
/** Global File Data **/
/**********************/

static SC_SIM_TPLUG_CMD_Class_t TPlugCmd;

static SC_SIM_JMsgCmd_CmdPayload_t JMsgCmd; /* Working buffer for loads */

/*
** basecamp/sc_sim/cmd payload: 
** {
**     "id":   uint8
** }
*/

static CJSON_Obj_t JsonTblObjs[] = 
{

   /* Table           Data                                core-json       length of query       */
   /* Data Address,   Len,  Updated,  Data Type,  Float,  query string,   string(exclude '\0')  */
   
   { &JMsgCmd.Id,     1,    false,   JSONNumber,  false,  { "id",         (sizeof("id")-1)}}
   
};

static const char *NullJMsgCmd = "{\"id\": 0}";

               
/******************************************************************************
** Function: SC_SIM_TPLUG_CMD_Constructor
**
** Initialize TPLUG Command 
**
** Notes:
**   None
**
*/
void SC_SIM_TPLUG_CMD_Constructor(JMSG_PLATFORM_TopicPlugin_Enum_t TopicPlugin)
{

   memset(&TPlugCmd, 0, sizeof(SC_SIM_TPLUG_CMD_Class_t));
   
   TPlugCmd.JsonObjCnt = (sizeof(JsonTblObjs)/sizeof(CJSON_Obj_t));

   TPlugCmd.TlmMsgId = JMSG_TOPIC_TBL_RegisterPlugin(TopicPlugin, CfeToJson, JsonToCfe, PluginTest);
   
   /* 
   ** Initialize the static fields in the JMsg command. The ID
   ** parameter and checksum are set prior to sending the command
   */
   CFE_MSG_Init(CFE_MSG_PTR(TPlugCmd.JMsgCmd), TPlugCmd.TlmMsgId, sizeof(SC_SIM_JMsgCmd_t));
   CFE_MSG_SetFcnCode(CFE_MSG_PTR(TPlugCmd.JMsgCmd), SC_SIM_J_MSG_CC);
      
} /* End SC_SIM_TPLUG_CMD_Constructor() */


/******************************************************************************
** Function: CfeToJson
**
** Convert a cFE SC_SIM Command message to a JSON topic message 
**
** Notes:
**   1.  Signature must match TPLUG_TBL_CfeToJson_t
**
*/
static bool CfeToJson(const char **JMsgPayload, const CFE_MSG_Message_t *CfeMsg)
{

   bool  RetStatus = false;
   int   PayloadLen; 
   const SC_SIM_JMsgCmd_CmdPayload_t *JMsgCmdMsg = CMDMGR_PAYLOAD_PTR(CfeMsg, SC_SIM_JMsgCmd_t);

   *JMsgPayload = NullJMsgCmd;
   
   PayloadLen = sprintf(TPlugCmd.JMsgPayload, "{\"id\": %d}", JMsgCmdMsg->Id);
                
   if (PayloadLen > 0)
   {
      *JMsgPayload = TPlugCmd.JMsgPayload;
      TPlugCmd.CfeToJsonCnt++;
      RetStatus = true;
   }
   
   return RetStatus;
   
} /* End CfeToJson() */


/******************************************************************************
** Function: JsonToCfe
**
** Convert a JSON SC_SIM Command topic message to a cFE message 
**
** Notes:
**   1.  Signature must match TPLUG_TBL_JsonToCfe_t
**
*/
static bool JsonToCfe(CFE_MSG_Message_t **CfeMsg, const char *JMsgPayload, uint16 PayloadLen)
{
   
   bool RetStatus = false;
   
   *CfeMsg = NULL;
   
   if (LoadJsonData(JMsgPayload, PayloadLen))
   {
      *CfeMsg = (CFE_MSG_Message_t *)&TPlugCmd.JMsgCmd;

      ++TPlugCmd.JsonToCfeCnt;
      RetStatus = true;
   }

   return RetStatus;
   
} /* End JsonToCfe() */


/******************************************************************************
** Function: PluginTest
**
** TODO: Generate SC_SIM JMSG command message that can be read by by MQTT client
** and published on the SB. 
**
** Notes:
**   1. Param is unused.
**   2. No need for fancy sim test, just need to verify the data is parsed
**      correctly.
**
*/
static void PluginTest(bool Init, int16 Param)
{

   SC_SIM_JMsgCmd_CmdPayload_t *CmdPayload = &TPlugCmd.JMsgCmd.Payload;

   if (Init)
   {
      
      memset(CmdPayload, 0, sizeof(SC_SIM_JMsgCmd_CmdPayload_t));

      CmdPayload->Id = 0;

      CFE_EVS_SendEvent(SC_SIM_TPLUG_CMD_INIT_SB_MSG_TEST_EID, CFE_EVS_EventType_INFORMATION,
                        "SC_SIM command topic test started");
   
   }
   else
   {
      CmdPayload->Id++;   
   }
   
   CFE_MSG_GenerateChecksum(CFE_MSG_PTR(TPlugCmd.JMsgCmd));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(TPlugCmd.JMsgCmd), true);
   
} /* PluginTest() */


/******************************************************************************
** Function: LoadJsonData
**
** Notes:
**  1. See file prologue for full/partial table load scenarios
*/
static bool LoadJsonData(const char *JMsgPayload, uint16 PayloadLen)
{

   bool      RetStatus = false;
   size_t    ObjLoadCnt;

   memset(&JMsgCmd, 0, sizeof(SC_SIM_JMsgCmd_CmdPayload_t));
   ObjLoadCnt = CJSON_LoadObjArray(JsonTblObjs, TPlugCmd.JsonObjCnt, 
                                   JMsgPayload, PayloadLen);
   CFE_EVS_SendEvent(SC_SIM_TPLUG_CMD_LOAD_JSON_DATA_EID, CFE_EVS_EventType_DEBUG,
                     "SC_SIM MQTT LoadJsonData() processed %d JSON objects", (uint16)ObjLoadCnt);

   if (ObjLoadCnt == TPlugCmd.JsonObjCnt)
   {
      memcpy(&TPlugCmd.JMsgCmd.Payload, &JMsgCmd, sizeof(SC_SIM_JMsgCmd_CmdPayload_t));      
      RetStatus = true;
   }
   else
   {
      CFE_EVS_SendEvent(SC_SIM_TPLUG_CMD_JSON_TO_CCSDS_ERR_EID, CFE_EVS_EventType_ERROR, 
                        "Error processing SC_SIM management message, payload contained %d of %d data objects",
                        (unsigned int)ObjLoadCnt, (unsigned int)TPlugCmd.JsonObjCnt);
   }
   
   return RetStatus;
   
} /* End LoadJsonData() */

