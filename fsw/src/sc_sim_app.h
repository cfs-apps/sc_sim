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
** Purpose: Define the Spacecraft Simulator App
**
** Notes:
**   None
**
** References:
**   1. cFS Basecamp Object-based Application Developer's Guide.
**   2. cFS Application Developer's Guide.
**
*/
#ifndef _sc_sim_app_
#define _sc_sim_app_

/*
** Includes
*/

#include "sc_sim.h"
#include "sc_sim_tbl.h"

/***********************/
/** Macro Definitions **/
/***********************/

#define SC_SIM_APP_INIT_EID            (SC_SIM_APP_BASE_EID + 0)
#define SC_SIM_APP_EXIT_EID            (SC_SIM_APP_BASE_EID + 1)
#define SC_SIM_APP_CMD_NOOP_EID        (SC_SIM_APP_BASE_EID + 2)
#define SC_SIM_APP_CMD_INVALID_MID_EID (SC_SIM_APP_BASE_EID + 3)

/**********************/
/** Type Definitions **/
/**********************/

/******************************************************************************
** Command & Telmetery Packets
**
** See EDS definitions in sc_sim.xml
*/


/******************************************************************************
** SC_SIM Class
*/
typedef struct
{

   /* 
   ** App Framework
   */ 
   
   INITBL_Class_t  IniTbl;
   CFE_SB_PipeId_t CmdPipe;
   CMDMGR_Class_t  CmdMgr;
   TBLMGR_Class_t  TblMgr;

   
   /*
   ** Telemetry Packets
   */
  
   SC_SIM_HkTlm_t HkTlm;


   /*
   ** SC_SIM State & Contained Objects
   */   

   uint32            PerfId;
   CFE_SB_MsgId_t    CmdMid;
   CFE_SB_MsgId_t    ExecuteMid;
   
   SC_SIM_Class_t     ScSim;
   SC_SIM_TBL_Class_t ScSimTbl;
   

} SC_SIM_APP_Class_t;



/*******************/
/** Exported Data **/
/*******************/

extern SC_SIM_APP_Class_t  ScSimApp;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: SC_SIM_AppMain
**
*/
void SC_SIM_AppMain(void);


#endif /* _sc_sim_app_ */
