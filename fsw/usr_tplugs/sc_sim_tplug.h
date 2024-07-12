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
**  Purpose:
**    Construct Spacecraft Sim's user topic plugins
**
**  Notes:
**    1. This module should only export a single constructor function and not
**       define any data because sc_sim_topic_plugin.c encapsulates all 
**       topic plugins for the sc_sim app. See sc_sim_topic_plugin.c file
**       prologue for details.
**
*/

#ifndef _sc_sim_tplug_
#define _sc_sim_tplug_

/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: SC_SIM_TPLUG_Constructor
**
** Call constructors for each user topic plugin
**
*/
void SC_SIM_TPLUG_Constructor(void);


#endif /* _sc_sim_tplug_ */
