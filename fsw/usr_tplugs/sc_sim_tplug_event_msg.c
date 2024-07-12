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
**   Manage SC_SIM TPLUG Event Message
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
#include "sc_sim_tplug_event_msg.h"
#include "sc_sim_eds_typedefs.h"


/***********************/
/** Macro Definitions **/
/***********************/

#define EVENT_MSG_LEN sizeof(CFE_EVS_EventMessage_String_t)

/*
** Event Message IDs
*/

#define BASE_EID  (JMSG_PLATFORM_TopicPluginBaseEid_USR_2)

#define SC_SIM_TPLUG_EVENT_MSG_INIT_SB_MSG_TEST_EID  (BASE_EID + 0)
#define SC_SIM_TPLUG_EVENT_MSG_SB_MSG_TEST_EID       (BASE_EID + 1)
#define SC_SIM_TPLUG_EVENT_MSG_LOAD_JSON_DATA_EID    (BASE_EID + 2)
#define SC_SIM_TPLUG_EVENT_MSG_JSON_TO_CCSDS_ERR_EID (BASE_EID + 3)

       
/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Telemetry
** 
** CFE_EVS_LongEventTlm_t is defined in CFE_EVS's EDS
*/

typedef struct
{

   /*
   ** CFE_EVS Event Message Telemetry
   */
   
   CFE_SB_MsgId_t          TlmMsgId;  
   CFE_EVS_LongEventTlm_t  TlmMsg;
   char                    JMsgPayload[2048];

   /*
   ** Subset of the standard CJSON table data because this isn't using the
   ** app_c_fw table manager service, but is using core-json in the same way
   ** as an app_c_fw managed table.
   */
   size_t  JsonObjCnt;

   uint32  CfeToJsonCnt;
   uint32  JsonToCfeCnt;
   
   
} SC_SIM_TPLUG_EVENT_MSG_Class_t;


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

static SC_SIM_TPLUG_EVENT_MSG_Class_t TPlugEventMsg;

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
** Function: SC_SIM_TPLUG_EVENT_MSG_Constructor
**
** Initialize the SC_SIM JMSG Event Playback topic
**
** Notes:
**   None
**
*/
void SC_SIM_TPLUG_EVENT_MSG_Constructor(JMSG_PLATFORM_TopicPlugin_Enum_t TopicPlugin)
{
   
OS_printf("SC_SIM_TPLUG_EVENT_MSG_Constructor()\n");

   memset(&TPlugEventMsg, 0, sizeof(SC_SIM_TPLUG_EVENT_MSG_Class_t));
    
   TPlugEventMsg.JsonObjCnt = (sizeof(JsonTblObjs)/sizeof(CJSON_Obj_t));  
   
   TPlugEventMsg.TlmMsgId = JMSG_TOPIC_TBL_RegisterPlugin(TopicPlugin, CfeToJson, JsonToCfe, PluginTest);
   
   CFE_MSG_Init(CFE_MSG_PTR(TPlugEventMsg.TlmMsg), TPlugEventMsg.TlmMsgId, sizeof(CFE_EVS_LongEventTlm_t));
      
} /* End SC_SIM_TPLUG_EVENT_MSG_Constructor() */


/******************************************************************************
** Function: SC_SIM_TPLUG_EVENT_MSG_ReplaceQuote
**;
** Replace all ocurrences of a character in a string.
**
** Notes:
**   1. Main use case is for replacing double quotes with single quaotes in
**      JSON messages.
**
*/
char* SC_SIM_TPLUG_EVENT_MSG_ReplaceQuotes(char *Str, char Find, char Replace)
{
   
   char *CurrentPos = strchr(Str,Find);
   
   while (CurrentPos)
   {
      *CurrentPos = Replace;
      CurrentPos = strchr(CurrentPos,Find);
   }
   
   return Str;
    
} /* End SC_SIM_TPLUG_EVENT_MSG_ReplaceQuote() */


/******************************************************************************
** Function: CfeToJson
**
** Convert a CFE_EVS Event Message to a JSON topic message 
**
** Notes:
**   1.  Signature must match TPLUG_TBL_CfeToJson_t
*/
static bool CfeToJson(const char **JMsgPayload, const CFE_MSG_Message_t *CfeMsg)
{

   bool  RetStatus = false;
   int   PayloadLen; 
   const CFE_EVS_LongEventTlm_t *EventMsg = (CFE_EVS_LongEventTlm_t *)CfeMsg;
   const CFE_EVS_LongEventTlm_Payload_t *EventPayload = CMDMGR_PAYLOAD_PTR(CfeMsg, CFE_EVS_LongEventTlm_t);
   char EventText[EVENT_MSG_LEN];  
   
   *JMsgPayload = NullEventMsg;
   
   // Events with double quotes cause JSON parser to barf so replace with single quotes
   strncpy(EventText, EventPayload->Message, EVENT_MSG_LEN);
   SC_SIM_TPLUG_EVENT_MSG_ReplaceQuotes(EventText,'\"','\'');
   
   PayloadLen = sprintf(TPlugEventMsg.JMsgPayload,
                        "{\"time\": %d,\"app\": \"%s\",\"type\": %d,\"text\": \"%s\"}",
                        EventMsg->TelemetryHeader.Sec.Seconds, 
                        EventPayload->PacketID.AppName, EventPayload->PacketID.EventType, EventText);

   if (PayloadLen > 0)
   {
      *JMsgPayload = TPlugEventMsg.JMsgPayload;
      TPlugEventMsg.CfeToJsonCnt++;
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
**   1.  Signature must match TPLUG_TBL_JsonToCfe_t
*/
static bool JsonToCfe(CFE_MSG_Message_t **CfeMsg, const char *JMsgPayload, uint16 PayloadLen)
{
   
   bool RetStatus = false;
   
   *CfeMsg = NULL;
   
   if (LoadJsonData(JMsgPayload, PayloadLen))
   {
      *CfeMsg = (CFE_MSG_Message_t *)&TPlugEventMsg.TlmMsg;

      ++TPlugEventMsg.JsonToCfeCnt;
      RetStatus = true;
   }

   return RetStatus;
   
} /* End JsonToCfe() */


/******************************************************************************
** Function: PluginTest
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
static void PluginTest(bool Init, int16 Param)
{

   CFE_EVS_LongEventTlm_Payload_t *TlmPayload = &TPlugEventMsg.TlmMsg.Payload;

   if (Init)
   {

      memset(TlmPayload, 0, sizeof(CFE_EVS_LongEventTlm_Payload_t));

      strcpy(TlmPayload->PacketID.AppName,"Test App");
      TlmPayload->PacketID.EventType = 1;
      strcpy(TlmPayload->Message,"Test Message");
            
      CFE_EVS_SendEvent(SC_SIM_TPLUG_EVENT_MSG_INIT_SB_MSG_TEST_EID, CFE_EVS_EventType_INFORMATION,
                        "Event Message telemetry topic test started");
   
   }
   else
   {
      TlmPayload->PacketID.EventType++;
   }
   
   CFE_SB_TimeStampMsg(CFE_MSG_PTR(TPlugEventMsg.TlmMsg.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(TPlugEventMsg.TlmMsg.TelemetryHeader), true);
   
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

   memset(&TPlugEventMsg.TlmMsg.Payload, 0, sizeof(CFE_EVS_LongEventTlm_Payload_t));
   
   ObjLoadCnt = CJSON_LoadObjArray(JsonTblObjs, TPlugEventMsg.JsonObjCnt, JMsgPayload, PayloadLen);
   
   CFE_EVS_SendEvent(SC_SIM_TPLUG_EVENT_MSG_LOAD_JSON_DATA_EID, CFE_EVS_EventType_DEBUG,
                     "SC_SIM MQTT Event Message LoadJsonData() processed %d JSON objects", (uint16)ObjLoadCnt);

   if (ObjLoadCnt == TPlugEventMsg.JsonObjCnt)
   {
      TPlugEventMsg.TlmMsg.TelemetryHeader.Sec.Seconds = EventMsgTime.Seconds;  //TODO this gets overwritten
      memcpy(&TPlugEventMsg.TlmMsg.Payload, &EventMsgPayload, sizeof(CFE_EVS_LongEventTlm_Payload_t));      
      RetStatus = true;
   }
   else
   {
      CFE_EVS_SendEvent(SC_SIM_TPLUG_EVENT_MSG_JSON_TO_CCSDS_ERR_EID, CFE_EVS_EventType_ERROR, 
                        "Error processing SC_SIM Event Message message, payload contained %d of %d data objects",
                        (unsigned int)ObjLoadCnt, (unsigned int)TPlugEventMsg.JsonObjCnt);
   }
   
   return RetStatus;
   
} /* End LoadJsonData() */

