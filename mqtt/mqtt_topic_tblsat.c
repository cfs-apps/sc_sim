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
**   Manage TableSat's sensor topic
**
** Notes:
**   None
**
** References:
**   1. OpenSatKit Object-based Application Developer's Guide
**
*/

/*
** Includes
*/

#include "mqtt_topic_tblsat.h"

/************************************/
/** Local File Function Prototypes **/
/************************************/

static bool LoadJsonData(const char *JsonMsgPayload, uint16 PayloadLen);


/**********************/
/** Global File Data **/
/**********************/

static MQTT_TOPIC_TBLSAT_Class_t *MqttTopicTblSat = NULL;

static MQTT_GW_TblSatSensorTlm_Payload_t TblSatSensor; /* Working buffer for loads */

/*
** tablesat/sensor payload: 
** {
**     "delta-time": uint32,
**     "rate": {
**         "x": float,
**         "y": float,
**         "z": float,
**         },
**     "lux": {
**         "a": unit32,
**         "b": unit32,
**         }
** }
*/

static CJSON_Obj_t JsonTblObjs[] = 
{

   /* Table                   Data                                core-json       length of query       */
   /* Data Address,           Len,  Updated,  Data Type,  Float,  query string,   string(exclude '\0')  */
   
   { &TblSatSensor.DeltaTime,  4,     false,  JSONNumber, false,  { "delta-time", (sizeof("delta-time")-1)} },
   { &TblSatSensor.RateX,      4,     false,  JSONNumber, true,   { "rate.x",     (sizeof("rate.x")-1)} },
   { &TblSatSensor.RateY,      4,     false,  JSONNumber, true,   { "rate.y",     (sizeof("rate.y")-1)} },
   { &TblSatSensor.RateZ,      4,     false,  JSONNumber, true,   { "rate.z",     (sizeof("rate.z")-1)} },
   { &TblSatSensor.LuxA,       4,     false,  JSONNumber, false,  { "lux.a",      (sizeof("lux.a")-1)}  },
   { &TblSatSensor.LuxB,       4,     false,  JSONNumber, false,  { "lux.b",      (sizeof("lux.b")-1)}  }
   
};

static const char *NullTblSatMsg = "{\"delta-time\": 0,\"rate\":{\"x\": 0.0,\"y\": 0.0,\"z\": 0.0},\"lux\":{\"a\": 0,\"b\": 0}}";

/******************************************************************************
** Function: MQTT_TOPIC_TBLSAT_Constructor
**
** Initialize the MQTT tblsat topic
**
** Notes:
**   None
**
*/
void MQTT_TOPIC_TBLSAT_Constructor(MQTT_TOPIC_TBLSAT_Class_t *MqttTopicTblSatPtr, 
                                   CFE_SB_MsgId_t TlmMsgMid)
{

   MqttTopicTblSat = MqttTopicTblSatPtr;
   memset(MqttTopicTblSat, 0, sizeof(MQTT_TOPIC_TBLSAT_Class_t));

   MqttTopicTblSat->JsonObjCnt = (sizeof(JsonTblObjs)/sizeof(CJSON_Obj_t));
   
   CFE_MSG_Init(CFE_MSG_PTR(MqttTopicTblSat->SensorTlmMsg), TlmMsgMid, sizeof(MQTT_GW_TblSatSensorTlm_t));
      
} /* End MQTT_TOPIC_TBLSAT_Constructor() */


/******************************************************************************
** Function: MQTT_TOPIC_TBLSAT_CfeToJson
**
** Convert a cFE tblsat message to a JSON topic message 
**
*/
bool MQTT_TOPIC_TBLSAT_CfeToJson(const char **JsonMsgPayload,
                                 const CFE_MSG_Message_t *CfeMsg)
{

   bool  RetStatus = false;
   int   PayloadLen; 
   const MQTT_GW_TblSatSensorTlm_Payload_t *TblSatMsg = CMDMGR_PAYLOAD_PTR(CfeMsg, MQTT_GW_TblSatSensorTlm_t);

   *JsonMsgPayload = NullTblSatMsg;
   
   PayloadLen = sprintf(MqttTopicTblSat->JsonMsgPayload,
                "{\"delta-time\": %d,\"rate\":{\"x\": %0.6f,\"y\": %0.6f,\"z\": %0.6f},\"lux\":{\"a\": %d,\"b\": %d}}",
                TblSatMsg->DeltaTime, TblSatMsg->RateX, TblSatMsg->RateY, TblSatMsg->RateZ,
                TblSatMsg->LuxA, TblSatMsg->LuxB);

   if (PayloadLen > 0)
   {
      *JsonMsgPayload = MqttTopicTblSat->JsonMsgPayload;
      MqttTopicTblSat->CfeToJsonCnt++;
      RetStatus = true;
   }
   
   return RetStatus;
   
} /* End MQTT_TOPIC_TBLSAT_CfeToJson() */


/******************************************************************************
** Function: MQTT_TOPIC_TBLSAT_JsonToCfe
**
** Convert a cFE tblsat message on the SB causing MQTT_TOPIC_TBLSAT_CfeToJson()
** to be called which the converts the SB message to a MQTT JSON tblsat topic. 
** The topic can be observed
**
*/
bool MQTT_TOPIC_TBLSAT_JsonToCfe(CFE_MSG_Message_t **CfeMsg, 
                                 const char *JsonMsgPayload, uint16 PayloadLen)
{
   
   bool RetStatus = false;
   
   *CfeMsg = NULL;
   
   if (LoadJsonData(JsonMsgPayload, PayloadLen))
   {
      *CfeMsg = (CFE_MSG_Message_t *)&MqttTopicTblSat->SensorTlmMsg;

      ++MqttTopicTblSat->JsonToCfeCnt;
      RetStatus = true;
   }

   return RetStatus;
   
} /* End MQTT_TOPIC_TBLSAT_JsonToCfe() */


/******************************************************************************
** Function: MQTT_TOPIC_TBLSAT_SbMsgTest
**
** Convert a JSON tblsat topic message to a cFE tblsat message
**
** Notes:
**   1. Param is unused.
**   2. No need for fancy sim test, just need to verify the data is parsed
**      correctly.
**
*/
void MQTT_TOPIC_TBLSAT_SbMsgTest(bool Init, int16 Param)
{

   if (Init)
   {
      
      MqttTopicTblSat->SensorTlmMsg.Payload.DeltaTime = 1;
      MqttTopicTblSat->SensorTlmMsg.Payload.RateX     = 1.0;
      MqttTopicTblSat->SensorTlmMsg.Payload.RateX     = 2.0;
      MqttTopicTblSat->SensorTlmMsg.Payload.RateX     = 3.0;
      MqttTopicTblSat->SensorTlmMsg.Payload.LuxA      = 100;
      MqttTopicTblSat->SensorTlmMsg.Payload.LuxA      = 200;

      CFE_EVS_SendEvent(MQTT_TOPIC_TBLSAT_INIT_SB_MSG_TEST_EID, CFE_EVS_EventType_INFORMATION,
                        "TblSat topic test started");
   
   }
   else
   {
      MqttTopicTblSat->SensorTlmMsg.Payload.DeltaTime++;   
   }
   
   CFE_SB_TimeStampMsg(CFE_MSG_PTR(MqttTopicTblSat->SensorTlmMsg.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(MqttTopicTblSat->SensorTlmMsg.TelemetryHeader), true);
   
} /* End MQTT_TOPIC_TBLSAT_SbMsgTest() */


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

   memset(&MqttTopicTblSat->SensorTlmMsg.Payload, 0, sizeof(MQTT_GW_TblSatSensorTlm_Payload_t));
   ObjLoadCnt = CJSON_LoadObjArray(JsonTblObjs, MqttTopicTblSat->JsonObjCnt, 
                                   JsonMsgPayload, PayloadLen);
   CFE_EVS_SendEvent(MQTT_TOPIC_TBLSAT_LOAD_JSON_DATA_EID, CFE_EVS_EventType_DEBUG,
                     "TblSat LoadJsonData() processed %d JSON objects", (uint16)ObjLoadCnt);

   if (ObjLoadCnt == MqttTopicTblSat->JsonObjCnt)
   {
      memcpy(&MqttTopicTblSat->SensorTlmMsg.Payload, &TblSatSensor, sizeof(MQTT_GW_TblSatSensorTlm_Payload_t));      
      RetStatus = true;
   }
   else
   {
      CFE_EVS_SendEvent(MQTT_TOPIC_TBLSAT_JSON_TO_CCSDS_ERR_EID, CFE_EVS_EventType_ERROR, 
                        "Error processing tblsat message, payload contained %d of %d data objects",
                        (unsigned int)ObjLoadCnt, (unsigned int)MqttTopicTblSat->JsonObjCnt);
   }
   
   return RetStatus;
   
} /* End LoadJsonData() */

