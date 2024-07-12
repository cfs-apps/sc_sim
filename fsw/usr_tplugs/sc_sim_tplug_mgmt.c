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
**   Manage SC_SIM TPLUG Management
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
#include "sc_sim_tplug_mgmt.h"
#include "sc_sim_eds_typedefs.h"


/***********************/
/** Macro Definitions **/
/***********************/

/*
** Event Message IDs
*/

#define BASE_EID  (JMSG_PLATFORM_TopicPluginBaseEid_USR_4)

#define SC_SIM_TPLUG_MGMT_INIT_SB_MSG_TEST_EID  (BASE_EID + 0)
#define SC_SIM_TPLUG_MGMT_SB_MSG_TEST_EID       (BASE_EID + 1)
#define SC_SIM_TPLUG_MGMT_LOAD_JSON_DATA_EID    (BASE_EID + 2)
#define SC_SIM_TPLUG_MGMT_JSON_TO_CCSDS_ERR_EID (BASE_EID + 3)


/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Telemetry
** 
** SC_SIM_MgmtTlm_t & SC_SIM_MgmtTlm_Payload_t defined in EDS
*/

typedef struct
{

   /*
   ** SC_SIM Management Telemetry
   */
   
   CFE_SB_MsgId_t    TlmMsgId;
   SC_SIM_MgmtTlm_t  TlmMsg;
   char              JsonMsgPayload[1024];

   /*
   ** Subset of the standard CJSON table data because this isn't using the
   ** app_c_fw table manager service, but is using core-json in the same way
   ** as an app_c_fw managed table.
   */
   size_t  JsonObjCnt;

   uint32  CfeToJsonCnt;
   uint32  JsonToCfeCnt;
   
   
} SC_SIM_TPLUG_MGMT_Class_t;


/************************************/
/** Local File Function Prototypes **/
/************************************/

static bool CfeToJson(const char **JsonMsgPayload, const CFE_MSG_Message_t *CfeMsg);
static bool JsonToCfe(CFE_MSG_Message_t **CfeMsg, const char *JsonMsgPayload, uint16 PayloadLen);
static bool LoadJsonData(const char *JsonMsgPayload, uint16 PayloadLen);
static void PluginTest(bool Init, int16 Param);


/**********************/
/** Global File Data **/
/**********************/

static SC_SIM_TPLUG_MGMT_Class_t TPlugMgmt;

static SC_SIM_MgmtTlm_Payload_t ScSimMgmt; /* Working buffer for loads */

/*
** basecamp/sc_sim/mgmt payload: 
** {
**     "sim_time":   uint32,
**     "sim_active": bool,
**     "sim_phase":  uint8,
**     "contact_time_pending":   uint16,
**     "contact_length":         uint16,
**     "contact_time_consumed":  uint16,
**     "contact_time_remaining": uint16
** }
*/

static CJSON_Obj_t JsonTblObjs[] = 
{

   /* Table                          Data                                  core-json                 length of query       */
   /* Data Address,                  Len,  Updated,  Data Type,  Float,    query string,             string(exclude '\0')  */
   
   { &ScSimMgmt.SimTime,              4,     false,  JSONNumber, false,  { "sim_time",               (sizeof("sim_time")-1)}  },
   { &ScSimMgmt.SimActive,            1,     false,  JSONNumber, false,  { "sim_active",             (sizeof("sim_active")-1)}},
   { &ScSimMgmt.SimPhase,             1,     false,  JSONNumber, false,  { "sim_phase",              (sizeof("sim-phase")-1)} },
   { &ScSimMgmt.ContactTimePending,   2,     false,  JSONNumber, false,  { "contact_time_pending",   (sizeof("contact_time_pending")-1)}  },
   { &ScSimMgmt.ContactLength,        2,     false,  JSONNumber, false,  { "contact_length",         (sizeof("contact_length")-1)}        },
   { &ScSimMgmt.ContactTimeConsumed,  2,     false,  JSONNumber, false,  { "contact_time_consumed",  (sizeof("contact_time_consumed")-1)} },
   { &ScSimMgmt.ContactTimeRemaining, 2,     false,  JSONNumber, false,  { "contact_time_remaining", (sizeof("contact_time_remaining")-1)}}
   
};
static const char *NullScSimMgmtMsg = "{\"sim_time\": 0,\"sim_active\": 0,\"sim_phase\": 0,\"contact_time_pending\": 0,\"contact_length\": 0,\"contact_time_consumed\": 0,\"contact_time_remaining\": 0}";
               
/******************************************************************************
** Function: SC_SIM_TPLUG_MGMT_Constructor
**
** Initialize the SC_SIM TPLUG Management topic
**
** Notes:
**   None
**
*/
void SC_SIM_TPLUG_MGMT_Constructor(JMSG_PLATFORM_TopicPlugin_Enum_t TopicPlugin)
{
OS_printf("SC_SIM_TPLUG_MGMT_Constructor()\n");

   memset(&TPlugMgmt, 0, sizeof(SC_SIM_TPLUG_MGMT_Class_t));
 
   TPlugMgmt.JsonObjCnt = (sizeof(JsonTblObjs)/sizeof(CJSON_Obj_t));
   
   TPlugMgmt.TlmMsgId = JMSG_TOPIC_TBL_RegisterPlugin(TopicPlugin, CfeToJson, JsonToCfe, PluginTest);
  
   CFE_MSG_Init(CFE_MSG_PTR(TPlugMgmt.TlmMsg), TPlugMgmt.TlmMsgId, sizeof(SC_SIM_MgmtTlm_t));
      
} /* End SC_SIM_TPLUG_MGMT_Constructor() */


/******************************************************************************
** Function: CfeToJson
**
** Convert a cFE SC_SIM TPLUG management message to a JSON topic message 
**
** Notes:
**   1.  Signature must match TPLUG_TBL_CfeToJson_t
*/
static bool CfeToJson(const char **JsonMsgPayload, const CFE_MSG_Message_t *CfeMsg)
{

   bool  RetStatus = false;
   int   PayloadLen; 
   const SC_SIM_MgmtTlm_Payload_t *ScSimMgmtMsg = CMDMGR_PAYLOAD_PTR(CfeMsg, SC_SIM_MgmtTlm_t);

   *JsonMsgPayload = NullScSimMgmtMsg;
   
   PayloadLen = sprintf(TPlugMgmt.JsonMsgPayload,
                "{\"sim_time\": %d,\"sim_active\": %d,\"sim_phase\": %d,\"contact_time_pending\": %d,\"contact_length\": %d,\"contact_time_consumed\": %d,\"contact_time_remaining\": %d}",
                ScSimMgmtMsg->SimTime, ScSimMgmtMsg->SimActive, ScSimMgmtMsg->SimPhase,
                ScSimMgmtMsg->ContactTimePending, ScSimMgmtMsg->ContactLength,
                ScSimMgmtMsg->ContactTimeConsumed, ScSimMgmtMsg->ContactTimeRemaining);
                
   if (PayloadLen > 0)
   {
      *JsonMsgPayload = TPlugMgmt.JsonMsgPayload;
      TPlugMgmt.CfeToJsonCnt++;
      RetStatus = true;
   }
   
   return RetStatus;
   
} /* End CfeToJson() */


/******************************************************************************
** Function: JsonToCfe
**
** Convert a JSON SC_SIM Management topic message to a cFE message 
**
** Notes:
**   1.  Signature must match TPLUG_TBL_JsonToCfe_t
*/
static bool JsonToCfe(CFE_MSG_Message_t **CfeMsg, const char *JsonMsgPayload, uint16 PayloadLen)
{
   
   bool RetStatus = false;
   
   *CfeMsg = NULL;
   
   if (LoadJsonData(JsonMsgPayload, PayloadLen))
   {
      *CfeMsg = (CFE_MSG_Message_t *)&TPlugMgmt.TlmMsg;

      ++TPlugMgmt.JsonToCfeCnt;
      RetStatus = true;
   }

   return RetStatus;
   
} /* End JsonToCfe() */


/******************************************************************************
** Function: PluginTest
**
** Generate and send SB SC_SIM Management topic messages on SB that are read back
** by JMSG network app and cause JMSGs to be generated from the SB messages.  
**
** Notes:
**   1. Param is unused.
**   2. No need for fancy sim test, just need to verify the data is parsed
**      correctly.
**
*/
static void PluginTest(bool Init, int16 Param)
{

   SC_SIM_MgmtTlm_Payload_t *TlmPayload = &TPlugMgmt.TlmMsg.Payload;

   if (Init)
   {
      
      memset(TlmPayload, 0, sizeof(SC_SIM_MgmtTlm_Payload_t));

      TlmPayload->SimTime   = 1000;
      TlmPayload->SimActive = 10;
      TlmPayload->SimPhase  = 5;
      TlmPayload->ContactTimePending   = 100;
      TlmPayload->ContactLength        = 200;
      TlmPayload->ContactTimeConsumed  = 300;
      TlmPayload->ContactTimeRemaining = 400;

      CFE_EVS_SendEvent(SC_SIM_TPLUG_MGMT_INIT_SB_MSG_TEST_EID, CFE_EVS_EventType_INFORMATION,
                        "SC_SIM TPLUG Management telemetry topic test started");
   
   }
   else
   {
      TlmPayload->SimTime++;   
   }
   
   CFE_SB_TimeStampMsg(CFE_MSG_PTR(TPlugMgmt.TlmMsg.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(TPlugMgmt.TlmMsg.TelemetryHeader), true);
   
} /* PluginTest() */


/******************************************************************************
** Function: LoadJsonData
**
** Notes:
**  1. See file prologue for full/partial table load scenarios
*/
static bool LoadJsonData(const char *JsonMsgPayload, uint16 PayloadLen)
{

   bool      RetStatus = false;
   size_t    ObjLoadCnt;

   memset(&TPlugMgmt.TlmMsg.Payload, 0, sizeof(SC_SIM_MgmtTlm_Payload_t));
   ObjLoadCnt = CJSON_LoadObjArray(JsonTblObjs, TPlugMgmt.JsonObjCnt, 
                                   JsonMsgPayload, PayloadLen);
   CFE_EVS_SendEvent(SC_SIM_TPLUG_MGMT_LOAD_JSON_DATA_EID, CFE_EVS_EventType_DEBUG,
                     "SC_SIM TPLUG LoadJsonData() processed %d JSON objects", (uint16)ObjLoadCnt);

   if (ObjLoadCnt == TPlugMgmt.JsonObjCnt)
   {
      memcpy(&TPlugMgmt.TlmMsg.Payload, &ScSimMgmt, sizeof(SC_SIM_MgmtTlm_Payload_t));      
      RetStatus = true;
   }
   else
   {
      CFE_EVS_SendEvent(SC_SIM_TPLUG_MGMT_JSON_TO_CCSDS_ERR_EID, CFE_EVS_EventType_ERROR, 
                        "Error processing SC_SIM management message, payload contained %d of %d data objects",
                        (unsigned int)ObjLoadCnt, (unsigned int)TPlugMgmt.JsonObjCnt);
   }
   
   return RetStatus;
   
} /* End LoadJsonData() */

