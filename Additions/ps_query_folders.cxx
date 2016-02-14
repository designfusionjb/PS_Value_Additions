#include "ps_global.hxx"
#include "ps_defines.hxx"
#include "ps_query_folders.hxx"
#include <time.h>

using namespace ps;

typedef struct ResultCache_s
{
	c_ptr<tag_t>	*results;
	time_t			*lastRun;
} ResultCache_t, *ResultCache_pt;

int ps::ps_query_folders(METHOD_message_t *m, va_list args)
{
	const char			*debug_name = "PS2 Dynamic Folders";
	int					result = ITK_ok;
	tag_t				tQuery = 0;
	c_ptr<char>			queryName;	
	c_pptr<char>		entries;
	c_pptr<char>		values;
	c_pptr<char>		sortKeys;
	c_ptr<int>			sortOrder;
	logical				isActive;
	int					maxHits = 0;
	static int			globalMaxHits = -1;
	int					delay = 5;
	time_t				epoch = time(NULL);
	tag_t				tDynamicFolder = m->object_tag;
	tag_t				tProp = va_arg(args, tag_t);
	int					*pNumResults = va_arg(args, int*);
	tag_t				**ptResults = va_arg(args, tag_t**);

	static map<tag_t, ResultCache_pt>
						resultCacheMap;

	log_debug("[START] %s", debug_name);
	hr_start_debug(debug_name);

	try
	{
		if (globalMaxHits == -1)
			if (!get_preference(PS2_QUERY_FOLDER_MAXHITS_PREF, &globalMaxHits))
				globalMaxHits = 0;

		itk(AOM_ask_value_logical(tDynamicFolder, PS2_QUERY_FOLDER_ACTIVE_PROP, &isActive));
		if (isActive)
		{
			ResultCache_pt resultCache;
			map<tag_t, ResultCache_pt>::iterator resultCacheMapIter;

			// Get the delay for how often to update folder
			itk(AOM_ask_value_int(tDynamicFolder, PS2_QUERY_FOLDER_QUERY_DELAY_PROP, &delay));

			if ((resultCacheMapIter = resultCacheMap.find(tDynamicFolder)) == resultCacheMap.end())
			{
				log_trace("Adding new Query Folder");
				//printf("New!\n");
				sm_alloc(resultCache, ResultCache_t, 1);
				sm_alloc(resultCache->lastRun, time_t, 1);
				resultCache->results = new c_ptr<tag_t>();

				*resultCache->lastRun = epoch - delay - 1;

				resultCacheMap.insert(pair<tag_t, ResultCache_pt>(tDynamicFolder, resultCache));
			}
			else
			{
				log_trace("Reusing Query Folder");
				resultCache = resultCacheMapIter->second;
			}

			if (epoch - *resultCache->lastRun > delay)
			{
				log_trace("Refreshing Query Folder");
				itk(AOM_ask_value_string(tDynamicFolder, PS2_QUERY_FOLDER_QUERY_PROP, queryName.pptr()));
				itk(QRY_find2(queryName.ptr(), &tQuery));
				if (tQuery != 0)
				{
					itk(AOM_ask_value_strings(tDynamicFolder, PS2_QUERY_FOLDER_ENTRIES_PROP, entries.plen(), entries.pptr()));

					if (entries.len() > 0)
					{
						// Get all preference settings
						itk(AOM_ask_value_strings(tDynamicFolder, PS2_QUERY_FOLDER_VALUES_PROP, values.plen(), values.pptr()));
						itk(AOM_ask_value_strings(tDynamicFolder, PS2_QUERY_FOLDER_SORT_KEYS_PROP, sortKeys.plen(), sortKeys.pptr()));
						itk(AOM_ask_value_ints(tDynamicFolder, PS2_QUERY_FOLDER_SORT_ORDER_PROP, sortOrder.plen(), sortOrder.pptr()));
						itk(AOM_ask_value_int(tDynamicFolder, PS2_QUERY_FOLDER_MAX_HITS_PROP, &maxHits));

						// Execute query
						resultCache->results->dealloc();
						itk(QRY_execute_with_sort(tQuery, entries.len(), entries.ptr(), values.ptr(),
							sortKeys.len(), sortKeys.ptr(), sortOrder.ptr(), resultCache->results->plen(), resultCache->results->pptr()));
						
						// Global maxhits value takes precedence over user maxhits
						if (globalMaxHits > 0 && globalMaxHits < maxHits)
							maxHits = globalMaxHits;

						// If query result larger than maxHits, shrink array
						if (resultCache->results->len() > maxHits)
						{
							resultCache->results->set_length(maxHits);
						}
					}
				}
				*resultCache->lastRun = epoch;
			}
			else
				log_trace("Query folder up to date");

			if (resultCache->results->len() > 0)
			{
				*pNumResults = resultCache->results->len();
				sm_alloc(*ptResults, tag_t, *pNumResults);
				memcpy(*ptResults, resultCache->results->ptr(), *pNumResults * sizeof(tag_t));
			}
		}
	}
	catch (tcexception& e)
	{
		result = e.getstat();
		EMH_store_error_s1(EMH_severity_error, QUERY_FOLDERS_DEFAULT_IFAIL, e.what());
		log_error(e.what());
	}
	catch (psexception& e)
	{
		result = QUERY_FOLDERS_DEFAULT_IFAIL;
		EMH_store_error_s1(EMH_severity_error, QUERY_FOLDERS_DEFAULT_IFAIL, e.what());
		log_error(e.what());
	}

	hr_stop_debug(debug_name);
	hr_print_debug(debug_name);
	log_debug("[STOP] %s", debug_name);

	return result;
}