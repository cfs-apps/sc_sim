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
**   1. The JSON payload format is defined in the .c file.
**
** References:
**   1. Basecamp Object-based Application Developer's Guide
**
*/

#ifndef _mqtt_topic_tblsat_
#define _mqtt_topic_tblsat_

/*
** Includes
*/

#include "app_cfg.h"


/***********************/
/** Macro Definitions **/
/***********************/


/*
** Event Message IDs
*/

#define MQTT_TOPIC_TBLSAT_INIT_SB_MSG_TEST_EID  (MQTT_TOPIC_TBLSAT_BASE_EID + 0)
#define MQTT_TOPIC_TBLSAT_SB_MSG_TEST_EID       (MQTT_TOPIC_TBLSAT_BASE_EID + 1)
#define MQTT_TOPIC_TBLSAT_LOAD_JSON_DATA_EID    (MQTT_TOPIC_TBLSAT_BASE_EID + 2)
#define MQTT_TOPIC_TBLSAT_JSON_TO_CCSDS_ERR_EID (MQTT_TOPIC_TBLSAT_BASE_EID + 3)

/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Telemetry
** 
** MQTT_GW_TblSatTlm_t & MQTT_GW_TblSatTlm_Payload_t defined in EDS
*/

typedef struct
{

   /*
   ** Table Sat Telemetry
   */
   
   MQTT_GW_TblSatSensorTlm_t  SensorTlmMsg;
   char JsonMsgPayload[1024];

   /*
   ** Subset of the standard CJSON table data because this isn't using the
   ** app_c_fw table manager service, but is using core-json in the same way
   ** as an app_c_fw managed table.
   */
   size_t  JsonObjCnt;

   uint32  CfeToJsonCnt;
   uint32  JsonToCfeCnt;
   
   
} MQTT_TOPIC_TBLSAT_Class_t;


/************************/
/** Exported Functions **/
/************************/


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
                                   CFE_SB_MsgId_t TlmMsgMid);


/******************************************************************************
** Function: MQTT_TOPIC_TBLSAT_CfeToJson
**
** Convert a cFE tblsat message to a JSON topic message 
**
** Notes:
**   1.  Signature must match MQTT_TOPIC_TBL_CfeToJson_t
*/
bool MQTT_TOPIC_TBLSAT_CfeToJson(const char **JsonMsgPayload,
                                 const CFE_MSG_Message_t *CfeMsg);


/******************************************************************************
** Function: MQTT_TOPIC_TBLSAT_JsonToCfe
**
** Convert a JSON tblsat topic message to a cFE tblsat message 
**
** Notes:
**   1.  Signature must match MQTT_TOPIC_TBL_JsonToCfe_t
*/
bool MQTT_TOPIC_TBLSAT_JsonToCfe(CFE_MSG_Message_t **CfeMsg, 
                                 const char *JsonMsgPayload, uint16 PayloadLen);


/******************************************************************************
** Function: MQTT_TOPIC_TBLSAT_SbMsgTest
**
** Generate and send SB tblsat topic messages on SB that are read back by MQTT_GW
** and cause MQTT messages to be generated from the SB messages.  
**
*/
void MQTT_TOPIC_TBLSAT_SbMsgTest(bool Init, int16 Param);


#endif /* _mqtt_topic_tblsat_ */
