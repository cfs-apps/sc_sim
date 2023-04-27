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
**   Manage KIT_TO's event playback telemetry topic
**
** Notes:
**   1. This plugin is defined with SC_SIM because it needs the 
**      MQTT playback message. The MQTT message contents are tailored
**      for its needs.
**   2. The JSON payload format is defined in the .c file.
**
** References:
**   1. cFS Basecamp Object-based Application Developer's Guide
**
*/

#ifndef _sc_sim_mqtt_topic_event_plbk_
#define _sc_sim_mqtt_topic_event_plbk_

/*
** Includes
*/

#include "mqtt_gw_topic_plugin.h"
#include "kit_to_eds_typedefs.h"


/***********************/
/** Macro Definitions **/
/***********************/

/*
** Event Message IDs
*/

#define SC_SIM_MQTT_TOPIC_EVENT_PLBK_INIT_SB_MSG_TEST_EID  (MQTT_GW_TOPIC_PLUGIN_7_BASE_EID + 0)
#define SC_SIM_MQTT_TOPIC_EVENT_PLBK_SB_MSG_TEST_EID       (MQTT_GW_TOPIC_PLUGIN_7_BASE_EID + 1)
#define SC_SIM_MQTT_TOPIC_EVENT_PLBK_LOAD_JSON_DATA_EID    (MQTT_GW_TOPIC_PLUGIN_7_BASE_EID + 2)
#define SC_SIM_MQTT_TOPIC_EVENT_PLBK_JSON_TO_CCSDS_ERR_EID (MQTT_GW_TOPIC_PLUGIN_7_BASE_EID + 3)
       
/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Telemetry
** 
** KIT_TO_PlbkEventTlm_t is defined in KIT_TO's EDS
*/

typedef struct
{

   /*
   ** KIT_TO Event Playback Telemetry
   */
   
   KIT_TO_PlbkEventTlm_t  TlmMsg;
   char JsonMsgPayload[2048];

   /*
   ** Subset of the standard CJSON table data because this isn't using the
   ** app_c_fw table manager service, but is using core-json in the same way
   ** as an app_c_fw managed table.
   */
   size_t  JsonObjCnt;

   uint32  CfeToJsonCnt;
   uint32  JsonToCfeCnt;
   
   
} SC_SIM_MQTT_TOPIC_EVENT_PLBK_Class_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: SC_SIM_MQTT_TOPIC_EVENT_PLBK_Constructor
**
** Initialize the SC_SIM MQTT Event Playback topic
**
** Notes:
**   None
**
*/
void SC_SIM_MQTT_TOPIC_EVENT_PLBK_Constructor(SC_SIM_MQTT_TOPIC_EVENT_PLBK_Class_t *ScSimMqttTopicEventPlbkPtr,
                                              MQTT_TOPIC_TBL_PluginFuncTbl_t *PluginFuncTbl,
                                              CFE_SB_MsgId_t TlmMsgMid);


#endif /* _sc_sim_mqtt_topic_event_plbk_ */
