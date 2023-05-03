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
**      MQTT playback message. The MQTT message contents are tailored
**      for its needs.
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

#include "sc_sim_mqtt_topic_event_plbk.h"
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

static SC_SIM_MQTT_TOPIC_EVENT_PLBK_Class_t *MqttTopicEventPlbk = NULL;

static KIT_TO_PlbkEventTlm_Payload_t PlbkEvent; /* Working buffer for loads */


/*
** basecamp/sc_sim/event_msg payload: 
** {
**     "log_file":    string,
**     "event_count": uint16,
**     "plbk_index":  uint16,
**     "event1_time:  uint32,
**     "event1_app:   string,
**     "event1_type:  uint16,
**     "event1_msg:   string,
**     "event2_time:  uint32,
**     "event2_app:   string,
**     "event2_type:  uint16,
**     "event2_msg:   string,
**     "event3_time:  uint32,
**     "event3_app:   string,
**     "event3_type:  uint16,
**     "event3_msg:   string,
**     "event4_time:  uint32,
**     "event4_app:   string,
**     "event4_type:  uint16,
**     "event4_msg:   string
** }
*/

static CJSON_Obj_t JsonTblObjs[] = 
{

   /* Table                                  Data                                             core-json       length of query       */
   /* Data Address,                          Len,             Updated,  Data Type,  Float,    query string,   string(exclude '\0')  */
   
   { &PlbkEvent.EventLogFile,                OS_MAX_PATH_LEN, false,    JSONString, false,  { "log_file",     (sizeof("log_file")-1)}},
   { &PlbkEvent.EventCnt,                    2,               false,    JSONNumber, false,  { "event_count",  (sizeof("event_count")-1)}},
   { &PlbkEvent.PlbkIdx,                     2,               false,    JSONNumber, false,  { "plbk_index",   (sizeof("plbk_index")-1)}},
   { &PlbkEvent.Event[0].Time,               4,               false,    JSONNumber, false,  { "event1_time",  (sizeof("event1_time")-1)}},
   { &PlbkEvent.Event[0].PacketID.AppName,   OS_MAX_API_NAME, false,    JSONString, false,  { "event1_app",   (sizeof("event1_app")-1)}},
   { &PlbkEvent.Event[0].PacketID.EventType, 2,               false,    JSONNumber, false,  { "event1_type",  (sizeof("event1_type")-1)}},
   { &PlbkEvent.Event[0].Message,            EVENT_MSG_LEN,   false,    JSONString, false,  { "event1_msg",   (sizeof("event1_msg")-1)}},
   { &PlbkEvent.Event[1].Time,               4,               false,    JSONNumber, false,  { "event2_time",  (sizeof("event2_time")-1)}},
   { &PlbkEvent.Event[1].PacketID.AppName,   OS_MAX_API_NAME, false,    JSONString, false,  { "event2_app",   (sizeof("event2_app")-1)}},
   { &PlbkEvent.Event[1].PacketID.EventType, 2,               false,    JSONNumber, false,  { "event2_type",  (sizeof("event2_type")-1)}},
   { &PlbkEvent.Event[1].Message,            EVENT_MSG_LEN,   false,    JSONString, false,  { "event2_msg",   (sizeof("event2_msg")-1)}},
   { &PlbkEvent.Event[2].Time,               4,               false,    JSONNumber, false,  { "event3_time",  (sizeof("event3_time")-1)}},
   { &PlbkEvent.Event[2].PacketID.AppName,   OS_MAX_API_NAME, false,    JSONString, false,  { "event3_app",   (sizeof("event3_app")-1)}},
   { &PlbkEvent.Event[2].PacketID.EventType, 2,               false,    JSONNumber, false,  { "event3_type",  (sizeof("event3_type")-1)}},
   { &PlbkEvent.Event[2].Message,            EVENT_MSG_LEN,   false,    JSONString, false,  { "event3_msg",   (sizeof("event3_msg")-1)}},
   { &PlbkEvent.Event[3].Time,               4,               false,    JSONNumber, false,  { "event4_time",  (sizeof("event4_time")-1)}},
   { &PlbkEvent.Event[3].PacketID.AppName,   OS_MAX_API_NAME, false,    JSONString, false,  { "event4_app",   (sizeof("event4_app")-1)}},
   { &PlbkEvent.Event[3].PacketID.EventType, 2,               false,    JSONNumber, false,  { "event4_type",  (sizeof("event4_type")-1)}},
   { &PlbkEvent.Event[3].Message,            EVENT_MSG_LEN,   false,    JSONString, false,  { "event4_msg",   (sizeof("event4_msg")-1)}}
   
};

static const char *NullEventPlbkMsg = "{\"log_file\": \"null\",\"event_count\": 0,\"plbk_index\": 0, \
                                      \"event1_time\": 0,\"event1_app\": \"null\",\"event1_type\": 0,\"event1_msg\": \"null\", \
                                      \"event2_time\": 0,\"event2_app\": \"null\",\"event2_type\": 0,\"event2_msg\": \"null\", \
                                      \"event3_time\": 0,\"event3_app\": \"null\",\"event3_type\": 0,\"event3_msg\": \"null\", \
                                      \"event4_time\": 0,\"event4_app\": \"null\",\"event4_type\": 0,\"event4_msg\": \"null\"}";


/******************************************************************************
** Function: SC_SIM_MQTT_TOPIC_EVENT_PLBK_Constructor
**
** Initialize the MQTT KIT_TO Event Playback topic
**
** Notes:
**   None
**
*/
void SC_SIM_MQTT_TOPIC_EVENT_PLBK_Constructor(SC_SIM_MQTT_TOPIC_EVENT_PLBK_Class_t *MqttTopicEventPlbkPtr,
                                              MQTT_TOPIC_TBL_PluginFuncTbl_t *PluginFuncTbl,
                                              CFE_SB_MsgId_t TlmMsgMid)
{
OS_printf("SC_SIM_MQTT_TOPIC_EVENT_PLBK_Constructor()\n");
   MqttTopicEventPlbk = MqttTopicEventPlbkPtr;
   memset(MqttTopicEventPlbk, 0, sizeof(SC_SIM_MQTT_TOPIC_EVENT_PLBK_Class_t));
   
   PluginFuncTbl->CfeToJson = CfeToJson;
   PluginFuncTbl->JsonToCfe = JsonToCfe;  
   PluginFuncTbl->SbMsgTest = SbMsgTest;
   
   MqttTopicEventPlbk->JsonObjCnt = (sizeof(JsonTblObjs)/sizeof(CJSON_Obj_t));
   
   CFE_MSG_Init(CFE_MSG_PTR(MqttTopicEventPlbk->TlmMsg), TlmMsgMid, sizeof(KIT_TO_PlbkEventTlm_t));
      
} /* End SC_SIM_MQTT_TOPIC_EVENT_PLBK_Constructor() */


/******************************************************************************
** Function: CfeToJson
**
** Convert a cFE KIT_TO Event Playback message to a JSON topic message 
**
** Notes:
**   1.  Signature must match MQTT_TOPIC_TBL_CfeToJson_t
*/
static bool CfeToJson(const char **JsonMsgPayload, const CFE_MSG_Message_t *CfeMsg)
{

   bool  RetStatus = false;
   int   PayloadLen; 
   const KIT_TO_PlbkEventTlm_Payload_t *PlbkEventMsg = CMDMGR_PAYLOAD_PTR(CfeMsg, KIT_TO_PlbkEventTlm_t);
   KIT_TO_PlbkEventTlm_Payload_t LocalMsg;

   *JsonMsgPayload = NullEventPlbkMsg;
   
   // TODO: This is a quick fix to meet a deadline. A longterm solution shoudl include KIT_TO to change messages when
   // TODO: the event log is processed and/or make a cFS Basecamp system strategy to avoid double quotes in events  
   for (int i=0; i<4; i++)
   {
      strncpy(LocalMsg.Event[i].Message,PlbkEventMsg->Event[i].Message,CFE_MISSION_EVS_MAX_MESSAGE_LENGTH);
      SC_SIM_MQTT_TOPIC_EVENT_MSG_ReplaceQuotes(LocalMsg.Event[i].Message,'\"','\'');
   }
   PayloadLen = sprintf(MqttTopicEventPlbk->JsonMsgPayload,
                        "{\"log_file\": \"%s\",\"event_count\": %d,\"plbk_index\": %d, \
                        \"event1_time\": %d,\"event1_app\": \"%s\",\"event1_type\": %d,\"event1_msg\": \"%s\", \
                        \"event2_time\": %d,\"event2_app\": \"%s\",\"event2_type\": %d,\"event2_msg\": \"%s\", \
                        \"event3_time\": %d,\"event3_app\": \"%s\",\"event3_type\": %d,\"event3_msg\": \"%s\", \
                        \"event4_time\": %d,\"event4_app\": \"%s\",\"event4_type\": %d,\"event4_msg\": \"%s\"}",
                        PlbkEventMsg->EventLogFile, PlbkEventMsg->EventCnt, PlbkEventMsg->PlbkIdx,
                        PlbkEventMsg->Event[0].Time.Seconds, PlbkEventMsg->Event[0].PacketID.AppName, PlbkEventMsg->Event[0].PacketID.EventType, LocalMsg.Event[0].Message,
                        PlbkEventMsg->Event[1].Time.Seconds, PlbkEventMsg->Event[1].PacketID.AppName, PlbkEventMsg->Event[1].PacketID.EventType, LocalMsg.Event[1].Message,
                        PlbkEventMsg->Event[2].Time.Seconds, PlbkEventMsg->Event[2].PacketID.AppName, PlbkEventMsg->Event[2].PacketID.EventType, LocalMsg.Event[2].Message,
                        PlbkEventMsg->Event[3].Time.Seconds, PlbkEventMsg->Event[3].PacketID.AppName, PlbkEventMsg->Event[3].PacketID.EventType, LocalMsg.Event[3].Message);

   if (PayloadLen > 0)
   {
      *JsonMsgPayload = MqttTopicEventPlbk->JsonMsgPayload;
      MqttTopicEventPlbk->CfeToJsonCnt++;
      RetStatus = true;
   }
   
   return RetStatus;
   
} /* End CfeToJson() */


/******************************************************************************
** Function: JsonToCfe
**
** Convert a JSON SC_SIM Event Playback topic message to a cFE message 
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
      *CfeMsg = (CFE_MSG_Message_t *)&MqttTopicEventPlbk->TlmMsg;

      ++MqttTopicEventPlbk->JsonToCfeCnt;
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

   KIT_TO_PlbkEventTlm_Payload_t *TlmPayload = &MqttTopicEventPlbk->TlmMsg.Payload;
   CFE_TIME_SysTime_t  Time;

   if (Init)
   {
      
      memset(TlmPayload, 0, sizeof(KIT_TO_PlbkEventTlm_Payload_t));

      strncpy(TlmPayload->EventLogFile, "Test Log File", OS_MAX_PATH_LEN);
      TlmPayload->EventCnt = 1;
      TlmPayload->PlbkIdx  = 0;
      
      Time.Subseconds = 0;
      for (int i=0; i < 4; i++)
      {
         Time.Seconds = i;
         TlmPayload->Event[i].Time = Time;
         sprintf(TlmPayload->Event[i].PacketID.AppName,"App_%d",i);
         TlmPayload->Event[i].PacketID.EventType = i;
         sprintf(TlmPayload->Event[i].Message,"Message_%d",i);
      }
      
      CFE_EVS_SendEvent(SC_SIM_MQTT_TOPIC_EVENT_PLBK_INIT_SB_MSG_TEST_EID, CFE_EVS_EventType_INFORMATION,
                        "Event Playback telemetry topic test started");
   
   }
   else
   {
      TlmPayload->EventCnt++;
   }
   
   CFE_SB_TimeStampMsg(CFE_MSG_PTR(MqttTopicEventPlbk->TlmMsg.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(MqttTopicEventPlbk->TlmMsg.TelemetryHeader), true);
   
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

   memset(&MqttTopicEventPlbk->TlmMsg.Payload, 0, sizeof(KIT_TO_PlbkEventTlm_Payload_t));
   ObjLoadCnt = CJSON_LoadObjArray(JsonTblObjs, MqttTopicEventPlbk->JsonObjCnt, 
                                   JsonMsgPayload, PayloadLen);
   CFE_EVS_SendEvent(SC_SIM_MQTT_TOPIC_EVENT_PLBK_LOAD_JSON_DATA_EID, CFE_EVS_EventType_DEBUG,
                     "SC_SIM MQTT Event Playback LoadJsonData() processed %d JSON objects", (uint16)ObjLoadCnt);

   if (ObjLoadCnt == MqttTopicEventPlbk->JsonObjCnt)
   {
      memcpy(&MqttTopicEventPlbk->TlmMsg.Payload, &PlbkEvent, sizeof(KIT_TO_PlbkEventTlm_Payload_t));      
      RetStatus = true;
   }
   else
   {
      CFE_EVS_SendEvent(SC_SIM_MQTT_TOPIC_EVENT_PLBK_JSON_TO_CCSDS_ERR_EID, CFE_EVS_EventType_ERROR, 
                        "Error processing SC_SIM Event Playback message, payload contained %d of %d data objects",
                        (unsigned int)ObjLoadCnt, (unsigned int)MqttTopicEventPlbk->JsonObjCnt);
   }
   
   return RetStatus;
   
} /* End LoadJsonData() */

