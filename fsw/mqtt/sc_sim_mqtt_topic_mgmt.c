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
**   Manage SC_SIM Management telemetry topic
**
** Notes:
**   1. The JSON payload format is defined below.
**   2. "SC_SIM" is included in event messages to identify the app
**      supplying the plugin. The event messages are reported by
**      MQTT_GW.
**
** References:
**   1. cFS Basecamp Object-based Application Developer's Guide
**
*/

/*
** Includes
*/

#include "sc_sim_mqtt_topic_mgmt.h"

/************************************/
/** Local File Function Prototypes **/
/************************************/

static bool CfeToJson(const char **JsonMsgPayload, const CFE_MSG_Message_t *CfeMsg);
static bool JsonToCfe(CFE_MSG_Message_t **CfeMsg, const char *JsonMsgPayload, uint16 PayloadLen);
static bool LoadJsonData(const char *JsonMsgPayload, uint16 PayloadLen);
static void SbMsgTest(bool Init, int16 Param);


/**********************/
/** Global File Data **/
/**********************/

static SC_SIM_MQTT_TOPIC_MGMT_Class_t *MqttTopicMgmt = NULL;

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
** Function: SC_SIM_MQTT_TOPIC_MGMT_Constructor
**
** Initialize the MQTT SC_SIM management topic
**
** Notes:
**   None
**
*/
void SC_SIM_MQTT_TOPIC_MGMT_Constructor(SC_SIM_MQTT_TOPIC_MGMT_Class_t *ScSimMqttTopicMgmtPtr,
                                        MQTT_TOPIC_TBL_PluginFuncTbl_t *PluginFuncTbl,
                                        CFE_SB_MsgId_t TlmMsgMid)
{
OS_printf("SC_SIM_MQTT_TOPIC_MGMT_Constructor()\n");
   MqttTopicMgmt = ScSimMqttTopicMgmtPtr;
   memset(MqttTopicMgmt, 0, sizeof(SC_SIM_MQTT_TOPIC_MGMT_Class_t));
   
   PluginFuncTbl->CfeToJson = CfeToJson;
   PluginFuncTbl->JsonToCfe = JsonToCfe;  
   PluginFuncTbl->SbMsgTest = SbMsgTest;
   
   MqttTopicMgmt->JsonObjCnt = (sizeof(JsonTblObjs)/sizeof(CJSON_Obj_t));
   
   CFE_MSG_Init(CFE_MSG_PTR(MqttTopicMgmt->TlmMsg), TlmMsgMid, sizeof(SC_SIM_MgmtTlm_t));
      
} /* End SC_SIM_MQTT_TOPIC_MGMT_Constructor() */


/******************************************************************************
** Function: CfeToJson
**
** Convert a cFE SC_SIM management message to a JSON topic message 
**
** Notes:
**   1.  Signature must match MQTT_TOPIC_TBL_CfeToJson_t
*/
static bool CfeToJson(const char **JsonMsgPayload, const CFE_MSG_Message_t *CfeMsg)
{

   bool  RetStatus = false;
   int   PayloadLen; 
   const SC_SIM_MgmtTlm_Payload_t *ScSimMgmtMsg = CMDMGR_PAYLOAD_PTR(CfeMsg, SC_SIM_MgmtTlm_t);

   *JsonMsgPayload = NullScSimMgmtMsg;
   
   PayloadLen = sprintf(MqttTopicMgmt->JsonMsgPayload,
                "{\"sim_time\": %d,\"sim_active\": %d,\"sim_phase\": %d,\"contact_time_pending\": %d,\"contact_length\": %d,\"contact_time_consumed\": %d,\"contact_time_remaining\": %d}",
                ScSimMgmtMsg->SimTime, ScSimMgmtMsg->SimActive, ScSimMgmtMsg->SimPhase,
                ScSimMgmtMsg->ContactTimePending, ScSimMgmtMsg->ContactLength,
                ScSimMgmtMsg->ContactTimeConsumed, ScSimMgmtMsg->ContactTimeRemaining);
                
   if (PayloadLen > 0)
   {
      *JsonMsgPayload = MqttTopicMgmt->JsonMsgPayload;
      MqttTopicMgmt->CfeToJsonCnt++;
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
**   1.  Signature must match MQTT_TOPIC_TBL_JsonToCfe_t
*/
static bool JsonToCfe(CFE_MSG_Message_t **CfeMsg, const char *JsonMsgPayload, uint16 PayloadLen)
{
   
   bool RetStatus = false;
   
   *CfeMsg = NULL;
   
   if (LoadJsonData(JsonMsgPayload, PayloadLen))
   {
      *CfeMsg = (CFE_MSG_Message_t *)&MqttTopicMgmt->TlmMsg;

      ++MqttTopicMgmt->JsonToCfeCnt;
      RetStatus = true;
   }

   return RetStatus;
   
} /* End JsonToCfe() */


/******************************************************************************
** Function: SbMsgTest
**
** Generate and send SB SC_SIM Management topic messages on SB that are read back
** by MQTT_GW and cause MQTT messages to be generated from the SB messages.  
**
** Notes:
**   1. Param is unused.
**   2. No need for fancy sim test, just need to verify the data is parsed
**      correctly.
**
*/
static void SbMsgTest(bool Init, int16 Param)
{

   SC_SIM_MgmtTlm_Payload_t *TlmPayload = &MqttTopicMgmt->TlmMsg.Payload;

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

      CFE_EVS_SendEvent(SC_SIM_MQTT_TOPIC_MGMT_INIT_SB_MSG_TEST_EID, CFE_EVS_EventType_INFORMATION,
                        "SC_SIM Management telemetry topic test started");
   
   }
   else
   {
      TlmPayload->SimTime++;   
   }
   
   CFE_SB_TimeStampMsg(CFE_MSG_PTR(MqttTopicMgmt->TlmMsg.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(MqttTopicMgmt->TlmMsg.TelemetryHeader), true);
   
} /* SbMsgTest() */


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

   memset(&MqttTopicMgmt->TlmMsg.Payload, 0, sizeof(SC_SIM_MgmtTlm_Payload_t));
   ObjLoadCnt = CJSON_LoadObjArray(JsonTblObjs, MqttTopicMgmt->JsonObjCnt, 
                                   JsonMsgPayload, PayloadLen);
   CFE_EVS_SendEvent(SC_SIM_MQTT_TOPIC_MGMT_LOAD_JSON_DATA_EID, CFE_EVS_EventType_DEBUG,
                     "SC_SIM MQTT LoadJsonData() processed %d JSON objects", (uint16)ObjLoadCnt);

   if (ObjLoadCnt == MqttTopicMgmt->JsonObjCnt)
   {
      memcpy(&MqttTopicMgmt->TlmMsg.Payload, &ScSimMgmt, sizeof(SC_SIM_MgmtTlm_Payload_t));      
      RetStatus = true;
   }
   else
   {
      CFE_EVS_SendEvent(SC_SIM_MQTT_TOPIC_MGMT_JSON_TO_CCSDS_ERR_EID, CFE_EVS_EventType_ERROR, 
                        "Error processing SC_SIM management message, payload contained %d of %d data objects",
                        (unsigned int)ObjLoadCnt, (unsigned int)MqttTopicMgmt->JsonObjCnt);
   }
   
   return RetStatus;
   
} /* End LoadJsonData() */

