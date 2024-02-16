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
**   Manage Spacecraft Sim's command topic
**
** Notes:
**   1. The JSON payload format is defined in the .c file.
**
*/

#ifndef _sc_sim_mqtt_topic_cmd_
#define _sc_sim_mqtt_topic_cmd_

/*
** Includes
*/

#include "mqtt_gw_topic_plugin.h"
#include "sc_sim_eds_typedefs.h"

/***********************/
/** Macro Definitions **/
/***********************/

/*
** Event Message IDs
*/

#define SC_SIM_MQTT_TOPIC_CMD_INIT_SB_MSG_TEST_EID  (MQTT_GW_TOPIC_PLUGIN_4_BASE_EID + 0)
#define SC_SIM_MQTT_TOPIC_CMD_SB_MSG_TEST_EID       (MQTT_GW_TOPIC_PLUGIN_4_BASE_EID + 1)
#define SC_SIM_MQTT_TOPIC_CMD_LOAD_JSON_DATA_EID    (MQTT_GW_TOPIC_PLUGIN_4_BASE_EID + 2)
#define SC_SIM_MQTT_TOPIC_CMD_JSON_TO_CCSDS_ERR_EID (MQTT_GW_TOPIC_PLUGIN_4_BASE_EID + 3)
       
/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Telemetry
** 
** SC_SIM_MqttCmd_t & SC_SIM_MqttCmd_CmdPayload_t defined in EDS
*/

typedef struct
{

   /*
   ** SC_SIM Command
   */
   
   SC_SIM_MqttJsonCmd_t  MqttJsonCmd;
   char JsonMsgPayload[1024];
      
   /*
   ** Subset of the standard CJSON table data because this isn't using the
   ** app_c_fw table manager service, but is using core-json in the same way
   ** as an app_c_fw managed table.
   */
   size_t  JsonObjCnt;

   uint32  CfeToJsonCnt;
   uint32  JsonToCfeCnt;
   
   
} SC_SIM_MQTT_TOPIC_CMD_Class_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: SC_SIM_MQTT_TOPIC_CMD_Constructor
**
** Initialize the SC_SIM MQTT Command topic
**
** Notes:
**   None
**
*/
void SC_SIM_MQTT_TOPIC_CMD_Constructor(SC_SIM_MQTT_TOPIC_CMD_Class_t *sMqttTopicCmdPtr,
                                       MQTT_TOPIC_TBL_PluginFuncTbl_t *PluginFuncTbl,
                                       CFE_SB_MsgId_t CmdsMsgMid);


#endif /* _sc_sim_mqtt_topic_cmd_ */
