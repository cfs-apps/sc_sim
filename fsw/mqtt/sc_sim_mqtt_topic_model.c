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
**   Manage SC_SIM Model telemetry topic
**
** Notes:
**   1. The JSON payload format is defined below.
**   2. "SC_SIM" is included in event messages to identify the app
**      supplying the plugin. The event messages are reported by
**      MQTT_GW.
**
*/

/*
** Includes
*/

#include "sc_sim_mqtt_topic_model.h"


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

static SC_SIM_MQTT_TOPIC_MODEL_Class_t *MqttTopicModel = NULL;

static SC_SIM_ModelTlm_Payload_t ScSimModel; /* Working buffer for loads */

/*
** basecamp/sc_sim/model payload: 
** {
**     "adcs_eclipse": bool,
**     "adcs_mode": uint8,
**     "cdh_sbc_rst_cnt": uint16,
**     "cdh_hw_cmd_cnt": uint16,
**     "cdh_last_hw_cmd": uint16,
**     "comm_in_contact": bool,
**     "comm_contact_time_pending": uint16,
**     "comm_contact_time_consumed": uint16,
**     "comm_contact_time_remaining": uint16,
**     "comm_contact_link": uint8,
**     "comm_contact_tdrs_id": uint8,
**     "comm_contact_data_rate": uint16,
**     "fsw_rec_pct_used": float,
**     "fsw_rec_file_cnt": uint16,
**     "fsw_rec_playback_ena": bool,
**     "instr_instr_pwr_ena": bool,
**     "instr_instr_sci_ena": bool,
**     "instr_instr_file_cnt": uint16,
**     "instr_instr_file_cyc_cnt": uint16,
**     "power_batt_soc": float,
**     "power_sa_current": float,
**     "therm_heater1_ena": bool,
**     "therm_heater2_ena": bool
** }
*/

static CJSON_Obj_t JsonTblObjs[] = 
{

   /* Table                           Data                                  core-json                      length of query       */
   /* Data Address,                   Len,  Updated,  Data Type,  Float,    query string,                  string(exclude '\0')  */
   
   { &ScSimModel.Eclipse,              1,     false,  JSONNumber, false,  { "adcs_eclipse",                (sizeof("adcs_eclipse")-1)}},
   { &ScSimModel.AdcsMode,             1,     false,  JSONNumber, false,  { "adcs_mode",                   (sizeof("adcs_mode")-1)}},

   { &ScSimModel.SbcRstCnt,            2,     false,  JSONNumber, false,  { "cdh_sbc_rst_cnt",             (sizeof("cdh_sbc_rst_cnt")-1)}},
   { &ScSimModel.HwCmdCnt,             2,     false,  JSONNumber, false,  { "cdh_hw_cmd_cnt",              (sizeof("cdh_hw_cmd_cnt")-1)}},
   { &ScSimModel.LastHwCmd,            2,     false,  JSONNumber, false,  { "cdh_last_hw_cmd",             (sizeof("cdh_last_hw_cmd")-1)}},
 
   { &ScSimModel.InContact,            1,     false,  JSONNumber, false,  { "comm_in_contact",             (sizeof("comm_in_contact")-1)}},
   { &ScSimModel.ContactTimePending,   2,     false,  JSONNumber, false,  { "comm_contact_time_pending",   (sizeof("comm_contact_time_pending")-1)}},
   { &ScSimModel.ContactTimeConsumed,  2,     false,  JSONNumber, false,  { "comm_contact_time_consumed",  (sizeof("comm_contact_time_consumed")-1)}},
   { &ScSimModel.ContactTimeRemaining, 2,     false,  JSONNumber, false,  { "comm_contact_time_remaining", (sizeof("comm_contact_time_remaining")-1)}},
  
   { &ScSimModel.ContactLink,          1,     false,  JSONNumber, false,  { "comm_contact_link",           (sizeof("comm_contact_link")-1)} },
   { &ScSimModel.ContactTdrsId,        1,     false,  JSONNumber, false,  { "comm_contact_tdrs_id",        (sizeof("comm_contact_tdrs_id")-1)}},
   { &ScSimModel.ContactDataRate,      2,     false,  JSONNumber, false,  { "comm_contact_data_rate",      (sizeof("comm_contact_data_rate")-1)}},

   { &ScSimModel.RecPctUsed,           4,     false,  JSONNumber, true,   { "fsw_rec_pct_used",            (sizeof("fsw_rec_pct_used")-1)} },
   { &ScSimModel.RecFileCnt,           2,     false,  JSONNumber, false,  { "fsw_rec_file_cnt",            (sizeof("fsw_rec_file_cnt")-1)}},
   { &ScSimModel.RecPlaybackEna,       1,     false,  JSONNumber, false,  { "fsw_rec_playback_ena",        (sizeof("fsw_rec_playback_ena")-1)}},

   { &ScSimModel.InstrPwrEna,          1,     false,  JSONNumber, false,  { "instr_instr_pwr_ena",         (sizeof("instr_instr_pwr_ena")-1)}},
   { &ScSimModel.InstrSciEna,          1,     false,  JSONNumber, false,  { "instr_instr_sci_ena",         (sizeof("instr_instr_sci_ena")-1)}},
   { &ScSimModel.InstrFileCnt,         2,     false,  JSONNumber, false,  { "instr_instr_file_cnt",        (sizeof("instr_instr_file_cnt")-1)}},
   { &ScSimModel.InstrFileCycCnt,      2,     false,  JSONNumber, false,  { "instr_instr_file_cyc_cnt",    (sizeof("instr_instr_file_cyc_cnt")-1)}},
  
   { &ScSimModel.BattSoc,              4,     false,  JSONNumber, true,   { "power_batt_soc",              (sizeof("power_batt_soc")-1)}},
   { &ScSimModel.SaCurrent,            4,     false,  JSONNumber, true,   { "power_sa_current",            (sizeof("power_sa_current")-1)}},

   { &ScSimModel.Heater1Ena,           1,     false,  JSONNumber, false,  { "therm_heater1_ena",           (sizeof("therm_heater1_ena")-1)}},
   { &ScSimModel.Heater2Ena,           1,     false,  JSONNumber, false,  { "therm_heater2_ena",           (sizeof("therm_heater2_ena")-1)}}
   
};
static const char *NullScSimModelMsg = "{\"sim_time\": 0,\"sim_active\": 0,\"sim_phase\": 0,\"contact_time_pending\": 0,\"contact_length\": 0,\"contact_time_consumed\": 0,\"contact_time_remaining\": 0}";
      

/******************************************************************************
** Function: SC_SIM_MQTT_TOPIC_MODEL_Constructor
**
** Initialize the MQTT SC_SIM management topic
**
** Notes:
**   None
**
*/
void SC_SIM_MQTT_TOPIC_MODEL_Constructor(SC_SIM_MQTT_TOPIC_MODEL_Class_t *ScSimMqttTopicModelPtr,
                                         MQTT_TOPIC_TBL_PluginFuncTbl_t *PluginFuncTbl,
                                         CFE_SB_MsgId_t TlmMsgMid)
{

   MqttTopicModel = ScSimMqttTopicModelPtr;
   memset(MqttTopicModel, 0, sizeof(SC_SIM_MQTT_TOPIC_MODEL_Class_t));
   
   PluginFuncTbl->CfeToJson = CfeToJson;
   PluginFuncTbl->JsonToCfe = JsonToCfe;  
   PluginFuncTbl->SbMsgTest = SbMsgTest;
   
   MqttTopicModel->JsonObjCnt = (sizeof(JsonTblObjs)/sizeof(CJSON_Obj_t));
   
   CFE_MSG_Init(CFE_MSG_PTR(MqttTopicModel->TlmMsg), TlmMsgMid, sizeof(SC_SIM_ModelTlm_t));
      
} /* End SC_SIM_MQTT_TOPIC_MODEL_Constructor() */


/******************************************************************************
** Function: CfeToJson
**
** Convert a cFE SC_SIM Model message to a JSON topic message 
**
** Notes:
**   1.  Signature must match MQTT_TOPIC_TBL_CfeToJson_t
*/
static bool CfeToJson(const char **JsonMsgPayload, const CFE_MSG_Message_t *CfeMsg)
{

   bool  RetStatus = false;
   int   PayloadLen; 
   const SC_SIM_ModelTlm_Payload_t *ScSimModelMsg = CMDMGR_PAYLOAD_PTR(CfeMsg, SC_SIM_ModelTlm_t);

   *JsonMsgPayload = NullScSimModelMsg;
   
   PayloadLen = sprintf(MqttTopicModel->JsonMsgPayload,
      "{\"adcs_eclipse\": %d, \"adcs_mode\": %d, \
      \"cdh_sbc_rst_cnt\": %d, \"cdh_hw_cmd_cnt\": %d, \"cdh_last_hw_cmd\": %d, \
      \"comm_in_contact\": %d, \"comm_contact_time_pending\": %d, \"comm_contact_time_consumed\": %d, \"comm_contact_time_remaining\": %d, \
      \"comm_contact_link\": %d, \"comm_contact_tdrs_id\": %d, \"comm_contact_data_rate\": %d, \
      \"fsw_rec_pct_used\": %0.3f, \"fsw_rec_file_cnt\": %d, \"fsw_rec_playback_ena\": %d, \
      \"instr_instr_pwr_ena\": %d, \"instr_instr_sci_ena\": %d, \"instr_instr_file_cnt\": %d, \"instr_instr_file_cyc_cnt\": %d, \
      \"power_batt_soc\": %.3f, \"power_sa_current\": %.3f, \
      \"therm_heater1_ena\": %d, \"therm_heater2_ena\": %d}",
      ScSimModelMsg->Eclipse, ScSimModelMsg->AdcsMode, 
      ScSimModelMsg->SbcRstCnt, ScSimModelMsg->HwCmdCnt, ScSimModelMsg->LastHwCmd, 
      ScSimModelMsg->InContact, ScSimModelMsg->ContactTimePending, ScSimModelMsg->ContactTimeConsumed, ScSimModelMsg->ContactTimeRemaining, 
      ScSimModelMsg->ContactLink, ScSimModelMsg->ContactTdrsId, ScSimModelMsg->ContactDataRate, 
      ScSimModelMsg->RecPctUsed, ScSimModelMsg->RecFileCnt, ScSimModelMsg->RecPlaybackEna, 
      ScSimModelMsg->InstrPwrEna, ScSimModelMsg->InstrSciEna, ScSimModelMsg->InstrFileCnt, ScSimModelMsg->InstrFileCycCnt, 
      ScSimModelMsg->BattSoc, ScSimModelMsg->SaCurrent, 
      ScSimModelMsg->Heater1Ena, ScSimModelMsg->Heater2Ena);

   if (PayloadLen > 0)
   {
      *JsonMsgPayload = MqttTopicModel->JsonMsgPayload;
      MqttTopicModel->CfeToJsonCnt++;
      RetStatus = true;
   }
   
   return RetStatus;
   
} /* End CfeToJson() */


/******************************************************************************
** Function: JsonToCfe
**
** Convert a JSON SC_SIM Model topic message to a cFE message 
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
      *CfeMsg = (CFE_MSG_Message_t *)&MqttTopicModel->TlmMsg;

      ++MqttTopicModel->JsonToCfeCnt;
      RetStatus = true;
   }

   return RetStatus;
   
} /* End JsonToCfe() */


/******************************************************************************
** Function: SbMsgTest
**
** Generate and send SB SC_SIM Model topic messages on SB that are read back
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

   SC_SIM_ModelTlm_Payload_t *TlmPayload = &MqttTopicModel->TlmMsg.Payload;

   if (Init)
   {
      
      memset(TlmPayload, 0, sizeof(SC_SIM_ModelTlm_Payload_t));

      TlmPayload->Eclipse   = true;
      TlmPayload->AdcsMode  = SC_SIM_AdcsMode_SUN_POINT; 
      TlmPayload->SbcRstCnt = 3;
      TlmPayload->HwCmdCnt  = 4;
      TlmPayload->LastHwCmd = 5; 
      TlmPayload->InContact = true;
      TlmPayload->ContactTimePending   = 1000;
      TlmPayload->ContactTimeConsumed  = 0;
      TlmPayload->ContactTimeRemaining = 3000; 
      TlmPayload->ContactLink     = 6;
      TlmPayload->ContactTdrsId   = 7;
      TlmPayload->ContactDataRate = 8; 
      TlmPayload->RecPctUsed = 10.5;
      TlmPayload->RecFileCnt = 234;
      TlmPayload->RecPlaybackEna = true;
      TlmPayload->InstrPwrEna  = false;
      TlmPayload->InstrSciEna  = true;
      TlmPayload->InstrFileCnt = 111;
      TlmPayload->InstrFileCycCnt =  222;
      TlmPayload->BattSoc    = 25.3; 
      TlmPayload->SaCurrent  = 76.8;
      TlmPayload->Heater1Ena = false;
      TlmPayload->Heater2Ena = true;

      CFE_EVS_SendEvent(SC_SIM_MQTT_TOPIC_MODEL_INIT_SB_MSG_TEST_EID, CFE_EVS_EventType_INFORMATION,
                        "SC_SIM Model telemetry topic test started");
   
   }
   else
   {
      // Meaningless behavior, just want to make values change
      TlmPayload->ContactTimePending--;
      TlmPayload->ContactTimeConsumed++;
      TlmPayload->ContactTimeRemaining--; 
   }
   
   CFE_SB_TimeStampMsg(CFE_MSG_PTR(MqttTopicModel->TlmMsg.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(MqttTopicModel->TlmMsg.TelemetryHeader), true);
   
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

   memset(&MqttTopicModel->TlmMsg.Payload, 0, sizeof(SC_SIM_ModelTlm_Payload_t));
   ObjLoadCnt = CJSON_LoadObjArray(JsonTblObjs, MqttTopicModel->JsonObjCnt, 
                                   JsonMsgPayload, PayloadLen);
   CFE_EVS_SendEvent(SC_SIM_MQTT_TOPIC_MODEL_LOAD_JSON_DATA_EID, CFE_EVS_EventType_DEBUG,
                     "SC_SIM MQTT LoadJsonData() processed %d JSON objects", (uint16)ObjLoadCnt);

   if (ObjLoadCnt == MqttTopicModel->JsonObjCnt)
   {
      memcpy(&MqttTopicModel->TlmMsg.Payload, &ScSimModel, sizeof(SC_SIM_ModelTlm_Payload_t));      
      RetStatus = true;
   }
   else
   {
      CFE_EVS_SendEvent(SC_SIM_MQTT_TOPIC_MODEL_JSON_TO_CCSDS_ERR_EID, CFE_EVS_EventType_ERROR, 
                        "Error processing SC_SIM model message, payload contained %d of %d data objects",
                        (unsigned int)ObjLoadCnt, (unsigned int)MqttTopicModel->JsonObjCnt);
   }
   
   return RetStatus;
   
} /* End LoadJsonData() */

