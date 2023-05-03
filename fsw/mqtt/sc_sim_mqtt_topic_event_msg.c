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
** Notes:
**   1. This plugin is defined with SC_SIM because it needs the 
**      MQTT realtime event message. The MQTT event message contents
**      are tailored for its needs.
**   2. The JSON payload format is defined below.
**   3. "SC_SIM" is included in event messages to identify the app
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

#include "sc_sim_mqtt_topic_event_msg.h"


/***********************/
/** Macro Definitions **/
/***********************/

#define EVENT_MSG_LEN sizeof(CFE_EVS_EventMessage_String_t)


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

static SC_SIM_MQTT_TOPIC_EVENT_MSG_Class_t *MqttTopicEventMsg = NULL;

static CFE_TIME_SysTime_t EventMsgTime;
static CFE_EVS_LongEventTlm_Payload_t EventMsgPayload; /* Working buffer for loads */


/*
** basecamp/sc_sim/event payload: 
** {
**     "time:  uint32,
**     "app:   string,
**     "type:  uint16,
**     "msg:   string
** }
*/

static CJSON_Obj_t JsonTblObjs[] = 
{

   /* Table                               Data                                             core-json      length of query       */
   /* Data Address,                       Len,             Updated,  Data Type,  Float,    query string,  string(exclude '\0')  */
   
   { &EventMsgTime.Seconds,               4,               false,    JSONNumber, false,  { "time",        (sizeof("time")-1)}},
   { &EventMsgPayload.PacketID.AppName,   OS_MAX_API_NAME, false,    JSONString, false,  { "app",         (sizeof("app")-1)}},
   { &EventMsgPayload.PacketID.EventType, 2,               false,    JSONNumber, false,  { "type",        (sizeof("type")-1)}},
   { &EventMsgPayload.Message,            EVENT_MSG_LEN,   false,    JSONString, false,  { "text",        (sizeof("text")-1)}}
   
};

static const char *NullEventMsg = "{\"time\": 0,\"app\": \"null\",\"type\": 0,\"text\": \"null\"}";


/******************************************************************************
** Function: SC_SIM_MQTT_TOPIC_EVENT_MSG_Constructor
**
** Initialize the MQTT KIT_TO Event Message topic
**
** Notes:
**   None
**
*/
void SC_SIM_MQTT_TOPIC_EVENT_MSG_Constructor(SC_SIM_MQTT_TOPIC_EVENT_MSG_Class_t *MqttTopicEventMsgPtr,
                                             MQTT_TOPIC_TBL_PluginFuncTbl_t *PluginFuncTbl,
                                             CFE_SB_MsgId_t TlmMsgMid)
{
OS_printf("SC_SIM_MQTT_TOPIC_EVENT_MSG_Constructor()\n");
   MqttTopicEventMsg = MqttTopicEventMsgPtr;
   memset(MqttTopicEventMsg, 0, sizeof(SC_SIM_MQTT_TOPIC_EVENT_MSG_Class_t));
   
   PluginFuncTbl->CfeToJson = CfeToJson;
   PluginFuncTbl->JsonToCfe = JsonToCfe;  
   PluginFuncTbl->SbMsgTest = SbMsgTest;
   
   MqttTopicEventMsg->JsonObjCnt = (sizeof(JsonTblObjs)/sizeof(CJSON_Obj_t));
   
   CFE_MSG_Init(CFE_MSG_PTR(MqttTopicEventMsg->TlmMsg), TlmMsgMid, sizeof(CFE_EVS_LongEventTlm_t));
      
} /* End SC_SIM_MQTT_TOPIC_EVENT_MSG_Constructor() */


/******************************************************************************
** Function: SC_SIM_MQTT_TOPIC_EVENT_MSG_ReplaceQuote
**;
** Replace all ocurrences of a character in a string.
**
** Notes:
**   1. Main use case is for replacing double quotes with single quaotes in
**      JSON messages.
**
*/
char* SC_SIM_MQTT_TOPIC_EVENT_MSG_ReplaceQuotes(char *Str, char Find, char Replace)
{
   
   char *CurrentPos = strchr(Str,Find);
   
   while (CurrentPos)
   {
      *CurrentPos = Replace;
      CurrentPos = strchr(CurrentPos,Find);
   }
   
   return Str;
    
} /* End SC_SIM_MQTT_TOPIC_EVENT_MSG_ReplaceQuote() */


/******************************************************************************
** Function: CfeToJson
**
** Convert a CFE_EVS Event Message to a JSON topic message 
**
** Notes:
**   1.  Signature must match MQTT_TOPIC_TBL_CfeToJson_t
*/
static bool CfeToJson(const char **JsonMsgPayload, const CFE_MSG_Message_t *CfeMsg)
{

   bool  RetStatus = false;
   int   PayloadLen; 
   const CFE_EVS_LongEventTlm_t *EventMsg = (CFE_EVS_LongEventTlm_t *)CfeMsg;
   const CFE_EVS_LongEventTlm_Payload_t *EventPayload = CMDMGR_PAYLOAD_PTR(CfeMsg, CFE_EVS_LongEventTlm_t);
   char EventText[EVENT_MSG_LEN];  
   
   *JsonMsgPayload = NullEventMsg;
   
   // Events with double quotes cause JSON parser to barf so replace with single quotes
   strncpy(EventText, EventPayload->Message, EVENT_MSG_LEN);
   SC_SIM_MQTT_TOPIC_EVENT_MSG_ReplaceQuotes(EventText,'\"','\'');
   
   PayloadLen = sprintf(MqttTopicEventMsg->JsonMsgPayload,
                        "{\"time\": %d,\"app\": \"%s\",\"type\": %d,\"text\": \"%s\"}",
                        EventMsg->TelemetryHeader.Sec.Seconds, 
                        EventPayload->PacketID.AppName, EventPayload->PacketID.EventType, EventText);

   if (PayloadLen > 0)
   {
      *JsonMsgPayload = MqttTopicEventMsg->JsonMsgPayload;
      MqttTopicEventMsg->CfeToJsonCnt++;
      RetStatus = true;
   }
   
   return RetStatus;
   
} /* End CfeToJson() */


/******************************************************************************
** Function: JsonToCfe
**
** Convert a JSON CFE_EVS Event message to a cFE message 
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
      *CfeMsg = (CFE_MSG_Message_t *)&MqttTopicEventMsg->TlmMsg;

      ++MqttTopicEventMsg->JsonToCfeCnt;
      RetStatus = true;
   }

   return RetStatus;
   
} /* End JsonToCfe() */


/******************************************************************************
** Function: SbMsgTest
**
** Generate and send SB SC_SIM Event Playback topic messages on SB that are
** read back by MQTT_GW and cause MQTT messages to be generated from the SB
** messages.  
**
** Notes:
**   1. Param is unused.
**   2. No need for fancy sim test, just need to verify the data is parsed
**      correctly.
**
*/
static void SbMsgTest(bool Init, int16 Param)
{

   CFE_EVS_LongEventTlm_Payload_t *TlmPayload = &MqttTopicEventMsg->TlmMsg.Payload;

   if (Init)
   {

      memset(TlmPayload, 0, sizeof(CFE_EVS_LongEventTlm_Payload_t));

      strcpy(TlmPayload->PacketID.AppName,"Test App");
      TlmPayload->PacketID.EventType = 1;
      strcpy(TlmPayload->Message,"Test Message");
            
      CFE_EVS_SendEvent(SC_SIM_MQTT_TOPIC_EVENT_MSG_INIT_SB_MSG_TEST_EID, CFE_EVS_EventType_INFORMATION,
                        "Event Message telemetry topic test started");
   
   }
   else
   {
      TlmPayload->PacketID.EventType++;
   }
   
   CFE_SB_TimeStampMsg(CFE_MSG_PTR(MqttTopicEventMsg->TlmMsg.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(MqttTopicEventMsg->TlmMsg.TelemetryHeader), true);
   
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

   memset(&MqttTopicEventMsg->TlmMsg.Payload, 0, sizeof(CFE_EVS_LongEventTlm_Payload_t));
   
   ObjLoadCnt = CJSON_LoadObjArray(JsonTblObjs, MqttTopicEventMsg->JsonObjCnt, JsonMsgPayload, PayloadLen);
   
   CFE_EVS_SendEvent(SC_SIM_MQTT_TOPIC_EVENT_MSG_LOAD_JSON_DATA_EID, CFE_EVS_EventType_DEBUG,
                     "SC_SIM MQTT Event Message LoadJsonData() processed %d JSON objects", (uint16)ObjLoadCnt);

   if (ObjLoadCnt == MqttTopicEventMsg->JsonObjCnt)
   {
      MqttTopicEventMsg->TlmMsg.TelemetryHeader.Sec.Seconds = EventMsgTime.Seconds;  //TODO this gets overwritten
      memcpy(&MqttTopicEventMsg->TlmMsg.Payload, &EventMsgPayload, sizeof(CFE_EVS_LongEventTlm_Payload_t));      
      RetStatus = true;
   }
   else
   {
      CFE_EVS_SendEvent(SC_SIM_MQTT_TOPIC_EVENT_MSG_JSON_TO_CCSDS_ERR_EID, CFE_EVS_EventType_ERROR, 
                        "Error processing SC_SIM Event Message message, payload contained %d of %d data objects",
                        (unsigned int)ObjLoadCnt, (unsigned int)MqttTopicEventMsg->JsonObjCnt);
   }
   
   return RetStatus;
   
} /* End LoadJsonData() */

