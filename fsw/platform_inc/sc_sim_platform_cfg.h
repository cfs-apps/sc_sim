/*
**  Copyright 2023 bitValence, Inc.
**  All Rights Reserved.
**
**  This program is free software; you can modify and/or redistribute it
**  under the terms of the GNU Affero General Public License
**  as published by the Free Software Foundation; version 3 with
**  attribution addendums as found in the LICENSE.txt
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU Affero General Public License for more details.
**
** Purpose: Define platform configurations for the Spacecraft Simulator App
**
** Notes:
**   1. See JSON init file for runtime configurations.
**
** References:
**   1. cFS Basecamp Object-based Application Developer's Guide.
**   2. cFS Application Developer's Guide.
**
*/

#ifndef _sc_sim_platform_cfg_
#define _sc_sim_platform_cfg_

/*
** Includes
*/

#include "sc_sim_mission_cfg.h"


/******************************************************************************
** Platform Deployment Configurations
*/

#define SC_SIM_PLATFORM_REV   0
#define SC_SIM_INI_FILENAME   "/cf/sc_sim_ini.json"


/******************************************************************************
** SC_SIM Table Object Macros
*/

#define  SC_SIM_TBL_DEF_LOAD_FILE  "/cf/sc_sim_tbl.json"
#define  SC_SIM_TBL_DEF_DUMP_FILE  "/cf/sc_sim_tbl~.json"

#endif /* _sc_sim_platform_cfg_ */
