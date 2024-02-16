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
**  GNU Affero General Public License for more details.** Purpose: Implement the spacecraft simulator table.
**
** Notes:
**   None
**
*/

/*
** Include Files:
*/

#include <string.h>
#include "sc_sim_tbl.h"


/***********************/
/** Macro Definitions **/
/***********************/


/**********************/
/** Type Definitions **/
/**********************/


/************************************/
/** Local File Function Prototypes **/
/************************************/

static bool LoadJsonData(size_t JsonFileLen);

/**********************/
/** Global File Data **/
/**********************/

static SC_SIM_TBL_Class_t* ScSimTbl = NULL;

static SC_SIM_TBL_Data_t TblData; /* Working buffer for loads */

static CJSON_Obj_t JsonTblObjs[] = {

   /* Table Data Address   Data Length      Updated  Data Type   Float,  Query string    Query string len (exclude '\0') */
   
   { &TblData.Adcs.Tbd1,   sizeof(uint32),  false,   JSONNumber, false,  { "adcs.tbd-1",  (sizeof("adcs.tbd-1")-1)}  },
   { &TblData.Adcs.Tbd2,   sizeof(uint32),  false,   JSONNumber, false,  { "adcs.tbd-2",  (sizeof("adcs.tbd-2")-1)}  },

   { &TblData.Cdh.Tbd1,    sizeof(uint32),  false,   JSONNumber, false,  { "cdh.tbd-1",   (sizeof("cdh.tbd-1")-1)}   },
   { &TblData.Cdh.Tbd2,    sizeof(uint32),  false,   JSONNumber, false,  { "cdh.tbd-2",   (sizeof("cdh.tbd-2")-1)}   },

   { &TblData.Comm.Tbd1,   sizeof(uint32),  false,   JSONNumber, false,  { "comm.tbd-1",  (sizeof("comm.tbd-1")-1)}  },
   { &TblData.Comm.Tbd2,   sizeof(uint32),  false,   JSONNumber, false,  { "comm.tbd-2",  (sizeof("comm.tbd-2")-1)}  },

   { &TblData.Fsw.Tbd1,    sizeof(uint32),  false,   JSONNumber, false,  { "fsw.tbd-1",   (sizeof("fsw.tbd-1")-1)}   },
   { &TblData.Fsw.Tbd2,    sizeof(uint32),  false,   JSONNumber, false,  { "fsw.tbd-2",   (sizeof("fsw.tbd-2")-1)}   },

   { &TblData.Power.Tbd1,  sizeof(uint32),  false,   JSONNumber, false,  { "power.tbd-1", (sizeof("power.tbd-1")-1)} },
   { &TblData.Power.Tbd2,  sizeof(uint32),  false,   JSONNumber, false,  { "power.tbd-2", (sizeof("power.tbd-2")-1)} },

   { &TblData.Therm.Tbd1,  sizeof(uint32),  false,   JSONNumber, false,  { "therm.tbd-1", (sizeof("therm.tbd-1")-1)} },
   { &TblData.Therm.Tbd2,  sizeof(uint32),  false,   JSONNumber, false,  { "therm.tbd-2", (sizeof("therm.tbd-2")-1)} },

};


/******************************************************************************
** Function: SCSIMTBL_Constructor
**
** Notes:
**    1. This must be called prior to any other functions
**
*/
void SC_SIM_TBL_Constructor(SC_SIM_TBL_Class_t *ScSimTblPtr, 
                            SC_SIM_TBL_LoadFunc_t LoadFunc)
{

   ScSimTbl = ScSimTblPtr;

   CFE_PSP_MemSet(ScSimTbl, 0, sizeof(SC_SIM_TBL_Class_t));
 
   ScSimTbl->LoadFunc = LoadFunc;
   ScSimTbl->JsonObjCnt = (sizeof(JsonTblObjs)/sizeof(CJSON_Obj_t));
         
} /* End SC_SIM_TBL_Constructor() */


/******************************************************************************
** Function: SC_SIM_TBL_DumpCmd
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr_t.
**  2. File is formatted so it can be used as a load file.
*/
bool SC_SIM_TBL_DumpCmd(osal_id_t FileHandle)
{

   char DumpRecord[256];

   sprintf(DumpRecord,"   \"adcs\": {\n   \"tbd-1\": %d,\n   \"tbd-2\": %d\n   },\n", 
           ScSimTbl->Data.Adcs.Tbd1, ScSimTbl->Data.Adcs.Tbd2);
   OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
   
   sprintf(DumpRecord,"   \"cdh\": {\n   \"tbd-1\": %d,\n   \"tbd-2\": %d\n   },\n", 
           ScSimTbl->Data.Cdh.Tbd1, ScSimTbl->Data.Cdh.Tbd2);
   OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
   
   sprintf(DumpRecord,"   \"comm\": {\n   \"tbd-1\": %d,\n   \"tbd-2\": %d\n   },\n", 
           ScSimTbl->Data.Comm.Tbd1, ScSimTbl->Data.Comm.Tbd2);
   OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
   
   sprintf(DumpRecord,"   \"fsw\": {\n   \"tbd-1\": %d,\n   \"tbd-2\": %d\n   },\n", 
           ScSimTbl->Data.Fsw.Tbd1, ScSimTbl->Data.Fsw.Tbd2);
   OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
   
   sprintf(DumpRecord,"   \"power\": {\n   \"tbd-1\": %d,\n   \"tbd-2\": %d\n   },\n", 
           ScSimTbl->Data.Power.Tbd1, ScSimTbl->Data.Power.Tbd2);
   OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
   
   sprintf(DumpRecord,"   \"therm\": {\n   \"tbd-1\": %d,\n   \"tbd-2\": %d\n   }\n", 
           ScSimTbl->Data.Therm.Tbd1, ScSimTbl->Data.Therm.Tbd2);
   OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
      

   return true;
   
} /* End of SC_SIM_TBL_DumpCmd() */


/******************************************************************************
** Function: SC_SIM_TBL_LoadCmd
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr_t.
*/
bool SC_SIM_TBL_LoadCmd(APP_C_FW_TblLoadOptions_Enum_t LoadType, const char *Filename)
{

   bool  RetStatus = false;

   if (CJSON_ProcessFile(Filename, ScSimTbl->JsonBuf, SC_SIM_TBL_JSON_FILE_MAX_CHAR, LoadJsonData))
   {
      ScSimTbl->Loaded = true;
      if (ScSimTbl->LoadFunc != NULL) (ScSimTbl->LoadFunc)();
      RetStatus = true;   
   }

   return RetStatus;
   
} /* End SC_SIM_TBL_LoadCmd() */


/******************************************************************************
** Function: SC_SIM_TBL_ResetStatus
**
*/
void SC_SIM_TBL_ResetStatus(void)
{

   ScSimTbl->LastLoadCnt = 0;
 
} /* End SC_SIM_TBL_ResetStatus() */


/******************************************************************************
** Function: LoadJsonData
**
** Notes:
**  1. See file prologue for full/partial table load scenarios
*/
static bool LoadJsonData(size_t JsonFileLen)
{

   bool    RetStatus = false;
   size_t  ObjLoadCnt;


   ScSimTbl->JsonFileLen = JsonFileLen;

   /* 
   ** 1. Copy table owner data into local table buffer
   ** 2. Process JSON file which updates local table buffer with JSON supplied values
   ** 3. If valid, copy local buffer over owner's data 
   */
   
   memcpy(&TblData, &ScSimTbl->Data, sizeof(SC_SIM_TBL_Data_t));
   
   ObjLoadCnt = CJSON_LoadObjArray(JsonTblObjs, ScSimTbl->JsonObjCnt, ScSimTbl->JsonBuf, ScSimTbl->JsonFileLen);

   /* Only accept fixed sized bin arrays */
   if (!ScSimTbl->Loaded && (ObjLoadCnt != ScSimTbl->JsonObjCnt))
   {

      CFE_EVS_SendEvent(SC_SIM_TBL_LOAD_EID, CFE_EVS_EventType_ERROR, 
                        "Table has never been loaded and new table only contains %d of %d data objects",
                        (unsigned int)ObjLoadCnt, (unsigned int)ScSimTbl->JsonObjCnt);
   
   }
   else
   {
   
      memcpy(&ScSimTbl->Data,&TblData, sizeof(SC_SIM_TBL_Data_t));
      ScSimTbl->LastLoadCnt = ObjLoadCnt;
      CFE_EVS_SendEvent(SC_SIM_TBL_LOAD_EID, CFE_EVS_EventType_DEBUG, 
                        "Successfully loaded %d JSON objects",
                        (unsigned int)ObjLoadCnt);
      RetStatus = true;
      
   }
   
   return RetStatus;
   
} /* End LoadJsonData() */
