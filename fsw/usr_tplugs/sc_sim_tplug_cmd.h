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
**   Export SC_SIM's TPLUG Command constructor
**
** Notes:
**   1. Other than the constructor, no other plugin definitions should be
**      exported. Plugin interfaces are through the callback functions
**      that are registered by the plugin's constructor.
**   2. The JSON payload format is defined in the .c file.
**
*/

#ifndef _sc_sim_tplug_cmd_
#define _sc_sim_tplug_cmd_

/*
** Includes
*/

#include "jmsg_platform_eds_typedefs.h"


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: SC_SIM_TPLUG_CMD_Constructor
**
** Initialize TPLUG Command 
**
** Notes:
**   None
**
*/
void SC_SIM_TPLUG_CMD_Constructor(JMSG_PLATFORM_TopicPlugin_Enum_t TopicPlugin);


#endif /* _sc_sim_tplug_cmd_ */
