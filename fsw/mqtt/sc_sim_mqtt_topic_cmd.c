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
**   Manage SC_SIM Command topic
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

#include "sc_sim_mqtt_topic_cmd.h"
#include "sc_sim_eds_cc.h"

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

static SC_SIM_MQTT_TOPIC_CMD_Class_t *MqttTopicCmd = NULL;

static SC_SIM_MqttJsonCmd_Payload_t MqttJsonCmd; /* Working buffer for loads */

/*
** basecamp/sc_sim/cmd payload: 
** {
**     "id":   uint8
** }
*/

static CJSON_Obj_t JsonTblObjs[] = 
{

   /* Table            Data                                core-json       length of query       */
   /* Data Address,    Len,  Updated,  Data Type,  Float,  query string,   string(exclude '\0')  */
   
   { &MqttJsonCmd.Id,  1,     false,  JSONNumber, false,  { "id",         (sizeof("id")-1)}}
   
};

static const char *NullMqttJsonCmd = "{\"id\": 0}";

               
/******************************************************************************
** Function: SC_SIM_MQTT_TOPIC_CMD_Constructor
**
** Initialize the MQTT SC_SIM management topic
**
** Notes:
**   None
**
*/
void SC_SIM_MQTT_TOPIC_CMD_Constructor(SC_SIM_MQTT_TOPIC_CMD_Class_t *MqttTopicCmdPtr,
                                       MQTT_TOPIC_TBL_PluginFuncTbl_t *PluginFuncTbl,
                                       CFE_SB_MsgId_t CmdMsgMid)
{
   
   MqttTopicCmd = MqttTopicCmdPtr;
   memset(MqttTopicCmd, 0, sizeof(SC_SIM_MQTT_TOPIC_CMD_Class_t));
   
   PluginFuncTbl->CfeToJson = CfeToJson;
   PluginFuncTbl->JsonToCfe = JsonToCfe;  
   PluginFuncTbl->SbMsgTest = SbMsgTest;
   
   MqttTopicCmd->JsonObjCnt = (sizeof(JsonTblObjs)/sizeof(CJSON_Obj_t));

   /* 
   ** Initialize the static fields in the MqttJson command. The ID
   ** parameter and checksum are set prior to sending the command
   */
   CFE_MSG_Init(CFE_MSG_PTR(MqttTopicCmd->MqttJsonCmd), CmdMsgMid, sizeof(SC_SIM_MqttJsonCmd_t));
   CFE_MSG_SetFcnCode(CFE_MSG_PTR(MqttTopicCmd->MqttJsonCmd), SC_SIM_MQTT_JSON_CC);
      
} /* End SC_SIM_MQTT_TOPIC_CMD_Constructor() */


/******************************************************************************
** Function: CfeToJson
**
** Convert a cFE SC_SIM Command message to a JSON topic message 
**
** Notes:
**   1.  Signature must match MQTT_TOPIC_TBL_CfeToJson_t
**
*/
static bool CfeToJson(const char **JsonMsgPayload, const CFE_MSG_Message_t *CfeMsg)
{

   bool  RetStatus = false;
   int   PayloadLen; 
   const SC_SIM_MqttJsonCmd_Payload_t *MqttJsonCmdMsg = CMDMGR_PAYLOAD_PTR(CfeMsg, SC_SIM_MqttJsonCmd_t);

   *JsonMsgPayload = NullMqttJsonCmd;
   
   PayloadLen = sprintf(MqttTopicCmd->JsonMsgPayload,
                "{\"id\": %d}", MqttJsonCmdMsg->Id);
                
   if (PayloadLen > 0)
   {
      *JsonMsgPayload = MqttTopicCmd->JsonMsgPayload;
      MqttTopicCmd->CfeToJsonCnt++;
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
**   1.  Signature must match MQTT_TOPIC_TBL_JsonToCfe_t
**
*/
static bool JsonToCfe(CFE_MSG_Message_t **CfeMsg, const char *JsonMsgPayload, uint16 PayloadLen)
{
   
   bool RetStatus = false;
   
   *CfeMsg = NULL;
   
   if (LoadJsonData(JsonMsgPayload, PayloadLen))
   {
      *CfeMsg = (CFE_MSG_Message_t *)&MqttTopicCmd->MqttJsonCmd;

      ++MqttTopicCmd->JsonToCfeCnt;
      RetStatus = true;
   }

   return RetStatus;
   
} /* End JsonToCfe() */


/******************************************************************************
** Function: SbMsgTest
**
** TODO: Generate SC_SIM MQTT command message that can be read by by MQTT client
** and published on the SB. 
**
** Notes:
**   1. Param is unused.
**   2. No need for fancy sim test, just need to verify the data is parsed
**      correctly.
**
*/
static void SbMsgTest(bool Init, int16 Param)
{

   SC_SIM_MqttJsonCmd_Payload_t *CmdPayload = &MqttTopicCmd->MqttJsonCmd.Payload;

   if (Init)
   {
      
      memset(CmdPayload, 0, sizeof(SC_SIM_MqttJsonCmd_Payload_t));

      CmdPayload->Id = 0;

      CFE_EVS_SendEvent(SC_SIM_MQTT_TOPIC_CMD_INIT_SB_MSG_TEST_EID, CFE_EVS_EventType_INFORMATION,
                        "SC_SIM command topic test started");
   
   }
   else
   {
      CmdPayload->Id++;   
   }
   
   CFE_MSG_GenerateChecksum(CFE_MSG_PTR(MqttTopicCmd->MqttJsonCmd));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(MqttTopicCmd->MqttJsonCmd), true);
   
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

   memset(&MqttJsonCmd, 0, sizeof(SC_SIM_MqttJsonCmd_Payload_t));
   ObjLoadCnt = CJSON_LoadObjArray(JsonTblObjs, MqttTopicCmd->JsonObjCnt, 
                                   JsonMsgPayload, PayloadLen);
   CFE_EVS_SendEvent(SC_SIM_MQTT_TOPIC_CMD_LOAD_JSON_DATA_EID, CFE_EVS_EventType_DEBUG,
                     "SC_SIM MQTT LoadJsonData() processed %d JSON objects", (uint16)ObjLoadCnt);

   if (ObjLoadCnt == MqttTopicCmd->JsonObjCnt)
   {
      memcpy(&MqttTopicCmd->MqttJsonCmd.Payload, &MqttJsonCmd, sizeof(SC_SIM_MqttJsonCmd_Payload_t));      
      RetStatus = true;
   }
   else
   {
      CFE_EVS_SendEvent(SC_SIM_MQTT_TOPIC_CMD_JSON_TO_CCSDS_ERR_EID, CFE_EVS_EventType_ERROR, 
                        "Error processing SC_SIM management message, payload contained %d of %d data objects",
                        (unsigned int)ObjLoadCnt, (unsigned int)MqttTopicCmd->JsonObjCnt);
   }
   
   return RetStatus;
   
} /* End LoadJsonData() */

