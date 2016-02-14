/*!
* \file ps_query_folders.hxx
* \date 2014/05/04 19:14
*
* \author Mattias Jonsson (jonssonm)
* Contact: jonsson.mattias@siemens.com
*
* \brief Dynamic search folders
*
*/

#pragma once

#include "ps_global.hxx"

//! Preference definitions
#define PS2_QUERY_FOLDER_ENABLED_PREF "PS2_Query_Folder_Enabled"
#define PS2_QUERY_FOLDER_MAXHITS_PREF "PS2_Query_Folder_MaxHits"
#define PS2_QUERY_FOLDER_TYPE "PS2QueryFolder"
#define PS2_QUERY_FOLDER_RUNTIME_PROP "ps2contents"
#define PS2_QUERY_FOLDER_QUERY_PROP "ps2queryName"
#define PS2_QUERY_FOLDER_ENTRIES_PROP "ps2searchEntries"
#define PS2_QUERY_FOLDER_VALUES_PROP "ps2searchValues"
#define PS2_QUERY_FOLDER_QUERY_DELAY_PROP "ps2queryDelay"
#define PS2_QUERY_FOLDER_ACTIVE_PROP "ps2enabled"
#define PS2_QUERY_FOLDER_SORT_KEYS_PROP "ps2sortKeys"
#define PS2_QUERY_FOLDER_SORT_ORDER_PROP "ps2sortOrder"
#define PS2_QUERY_FOLDER_MAX_HITS_PROP "ps2maxHits"

namespace ps {
	//! Configurable naming rule function
	int ps_query_folders(METHOD_message_t *m, va_list  args);
}