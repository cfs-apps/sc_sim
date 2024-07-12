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
**   Construct Spacecraft Sim's user topic plugins
**
** Notes:
**   1. Always construct plugins regardless of whether they are enabled. This
**      allows plugins to be enabled/disabled during runtime. If a topic's
**      JSON is invalid the plugin will be automatically disabled.
**   2. See jmsg_lib/docs/jmsg_topic_plugin_guide.txt for 
**      plugin creation and installation instructions
*/

/*
** Include Files:
*/

#include "sc_sim_tplug.h"
#include "sc_sim_tplug_cmd.h"
#include "sc_sim_tplug_event_msg.h"
#include "sc_sim_tplug_event_plbk.h"
#include "sc_sim_tplug_mgmt.h"
#include "sc_sim_tplug_model.h"


/******************************************************************************
** Function: SC_SIM_TPLUG_Constructor
**
*/
void SC_SIM_TPLUG_Constructor(void)
{

   SC_SIM_TPLUG_CMD_Constructor(JMSG_PLATFORM_TopicPlugin_USR_1);
   SC_SIM_TPLUG_MGMT_Constructor(JMSG_PLATFORM_TopicPlugin_USR_2);
   SC_SIM_TPLUG_MODEL_Constructor(JMSG_PLATFORM_TopicPlugin_USR_3);
   SC_SIM_TPLUG_EVENT_MSG_Constructor(JMSG_PLATFORM_TopicPlugin_USR_4);
   SC_SIM_TPLUG_EVENT_PLBK_Constructor(JMSG_PLATFORM_TopicPlugin_USR_5);
      
    
} /* SC_SIM_TPLUG_Constructor() */




