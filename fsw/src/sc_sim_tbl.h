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
** Purpose: Define SC_SIM Table
**
** Notes:
**   1. Use the Singleton design pattern. A pointer to the table object
**      is passed to the constructor and saved for all other operations.
**      This is a table-specific file so it doesn't need to be re-entrant.
**
*/
#ifndef _sc_sim_tbl_
#define _sc_sim_tbl_

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

#define SC_SIM_TBL_DUMP_EID  (SC_SIM_TBL_BASE_EID + 0)
#define SC_SIM_TBL_LOAD_EID  (SC_SIM_TBL_BASE_EID + 1)


/**********************/
/** Type Definitions **/
/**********************/

/*
** Table load callback function
*/
typedef void (*SC_SIM_TBL_LoadFunc_t)(void);


/******************************************************************************
** Table 
** 
*/

typedef struct
{

   uint32  Tbd1;
   uint32  Tbd2;
   
} SC_SIM_TBL_Adcs_t;


typedef struct
{

   uint32  Tbd1;
   uint32  Tbd2;
   
} SC_SIM_TBL_Cdh_t;

typedef struct 
{

   uint32  Tbd1;
   uint32  Tbd2;
   
} SC_SIM_TBL_Comm_t;

typedef struct
{

   uint32  Tbd1;
   uint32  Tbd2;
   
} SC_SIM_TBL_Fsw_t;

typedef struct
{

   uint32  Tbd1;
   uint32  Tbd2;
   
} SC_SIM_TBL_Power_t;

typedef struct
{

   uint32  Tbd1;
   uint32  Tbd2;
   
} SC_SIM_TBL_Therm_t;


typedef struct
{

   SC_SIM_TBL_Adcs_t   Adcs;
   SC_SIM_TBL_Cdh_t    Cdh;
   SC_SIM_TBL_Comm_t   Comm;
   SC_SIM_TBL_Fsw_t    Fsw;
   SC_SIM_TBL_Power_t  Power;
   SC_SIM_TBL_Therm_t  Therm;

} SC_SIM_TBL_Data_t;


typedef struct 
{

   /*
   ** Table Data
   */
   
   SC_SIM_TBL_Data_t     Data;
   SC_SIM_TBL_LoadFunc_t LoadFunc; 
   
   /*
   ** Standard CJSON table data
   */
   
   bool         Loaded;   /* Has entire table been loaded? */
   uint16       LastLoadCnt;
   
   size_t       JsonObjCnt;
   char         JsonBuf[SC_SIM_TBL_JSON_FILE_MAX_CHAR];   
   size_t       JsonFileLen;

} SC_SIM_TBL_Class_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: SC_SIM_TBL_Constructor
**
** Initialize the table object.
**
** Notes:
**   1. This must be called prior to any other functions
**   2. The table values are not populated. This is done when the table load
**      command is called.
**
*/
void SC_SIM_TBL_Constructor(SC_SIM_TBL_Class_t *ScSimTblPtr, 
                            SC_SIM_TBL_LoadFunc_t LoadFunc);


/******************************************************************************
** Function: SC_SIM_TBL_DumpCmd
**
** Command to write the table data from memory to a JSON file.
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr_t.
**
*/
bool SC_SIM_TBL_DumpCmd(osal_id_t FileHandle);


/******************************************************************************
** Function: SC_SIM_TBL_LoadCmd
**
** Command to copy the table data from a JSON file to memory.
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr_t.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager.
**
*/
bool SC_SIM_TBL_LoadCmd(APP_C_FW_TblLoadOptions_Enum_t LoadType, const char *Filename);


/******************************************************************************
** Function: SC_SIM_TBL_ResetStatus
**
** Reset counters and status flags to a known reset state.  The behavior of
** the table manager should not be impacted. The intent is to clear counters
** and flags to a known default state for telemetry.
**
*/
void SC_SIM_TBL_ResetStatus(void);


#endif /* _sc_sim_tbl_ */



