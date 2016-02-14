#include "ps_global.hxx"
#include "ps_defines.hxx"
#include "ps_find_referencers.hxx"

using namespace ps;

int ps::ps_find_referencers(tag_t tSource, string relationProp, string filterTypes, vector<tag_t>& v_referencers, vector<string>& v_relations)
{
	const char		*debug_name = "PS2 Find Referencers";
	int				result = ITK_ok;
	c_ptr<tag_t>	c_referencers;
	c_ptr<int>		c_levels;
	c_pptr<char>	c_relations;
	int				activeSeq;
	bool			applyTypeFilter = false;
	bool			applyRelationFilter = false;
	vector<string>	v_filterTypes;
	vector<string>	v_relationNames;

	log_debug("[START] %s", debug_name);
	hr_start_debug(debug_name);

	if (relationProp != "*")
	{
		applyRelationFilter = true;
		split_string(relationProp, ";", true, v_relationNames);
	}

	if (filterTypes != "*")
	{
		applyTypeFilter = true;
		split_string(filterTypes, ";", true, v_filterTypes);
	}

	itk(WSOM_where_referenced(tSource, 1, c_referencers.plen(), c_levels.pptr(), c_referencers.pptr(), c_relations.pptr()));

	for (int i = 0; i < c_referencers.len(); i++)
	{
		tag_t	otherObj = c_referencers.val(i);
		char	*relation = c_relations.val(i);

		// Only get the last version of the wso
		itk(AOM_ask_value_int(otherObj, "active_seq", &activeSeq));
		if (activeSeq == 0)
			continue;

		if (applyRelationFilter)
		{
			if (!find_string(relation, v_relationNames))
				continue;
		}
		if (applyTypeFilter)
		{
			c_ptr<char>		objectType;

			itk(AOM_ask_value_string(otherObj, "object_type", objectType.pptr()));

			if (!find_string(objectType.ptr(), v_filterTypes))
				continue;
		}
		v_referencers.push_back(otherObj);
		v_relations.push_back(string(relation));
	}

	hr_stop_debug(debug_name);
	hr_print_debug(debug_name);
	log_debug("[STOP] %s", debug_name);

	return result;
}

int ps::ps_ask_referencers(METHOD_message_t *m, va_list  args)
{
	const char		*debug_name = "PS2 Ask Referencers";
	int				result = ITK_ok;
	vector<tag_t>	v_tmpRefs;
	vector<string>	v_tmpRels;
	string			relationProp = m->user_args->arguments[0].val_union.str_value;
	string			filterTypes = m->user_args->arguments[1].val_union.str_value;
	tag_t			tSource = m->object_tag;
	tag_t			tProp = va_arg(args, tag_t);
	int				*numReferencers = va_arg(args, int*);
	tag_t			**referencers = va_arg(args, tag_t**);

	log_debug("[START] %s", debug_name);
	hr_start_debug(debug_name);

	try
	{
		ps_find_referencers(tSource, relationProp, filterTypes, v_tmpRefs, v_tmpRels);

		if (v_tmpRefs.size() > 0)
		{
			*numReferencers = v_tmpRefs.size();
			sm_alloc(*referencers, tag_t, *numReferencers);
			memcpy(*referencers, &v_tmpRefs[0], *numReferencers * sizeof(tag_t));
		}
	}
	catch (tcexception& e)
	{
		result = e.getstat();
		EMH_store_error_s1(EMH_severity_error, FIND_REFERENCERS_DEFAULT_IFAIL, e.what());
		log_error(e.what());
	}
	catch (psexception& e)
	{
		result = FIND_REFERENCERS_DEFAULT_IFAIL;
		EMH_store_error_s1(EMH_severity_error, FIND_REFERENCERS_DEFAULT_IFAIL, e.what());
		log_error(e.what());
	}

	hr_stop_debug(debug_name);
	hr_print_debug(debug_name);
	log_debug("[STOP] %s", debug_name);

	return result;
}

int ps::ps_set_referencers(METHOD_message_t *m, va_list  args)
{
	const char		*debug_name = "PS2 Set Referencers";
	int				result = ITK_ok;
	vector<tag_t>	v_tmpRefs;
	vector<string>	v_tmpRels;
	c_ptr<char>		currentUserName;
	static tag_t	tUser = 0;
	logical			haveWriteAccess;
	logical			isCheckedOut;
	string			relationProp = m->user_args->arguments[0].val_union.str_value;
	string			filterTypes = m->user_args->arguments[1].val_union.str_value;
	tag_t			tSource = m->object_tag;
	tag_t			tProp = va_arg(args, tag_t);
	int				numReferencers = va_arg(args, int);
	tag_t			*referencers = va_arg(args, tag_t*);

	log_debug("[START] %s", debug_name);
	hr_start_debug(debug_name);

	try
	{
		if (tUser == 0)
		{
			itk(POM_get_user(currentUserName.pptr(), &tUser));
		}

		ps_find_referencers(tSource, relationProp, filterTypes, v_tmpRefs, v_tmpRels);

		// Add
		for (int i = 0; i < numReferencers; i++)
		{
			tag_t	newTag = referencers[i];

			// If set newTag not in v_tmpRefs, add tSource to relationProp on newTag
			if (!find_tag(newTag, v_tmpRefs))
			{
				c_ptr<tag_t>	tagsToSet;
				tag_t			tCheckOutUser = 0;
				tag_t			tCheckOutGroup = 0;

				itk(AM_check_privilege(newTag, "WRITE", &haveWriteAccess));
				itk(RES_is_checked_out(newTag, &isCheckedOut));
				if (isCheckedOut)
					itk(RES_who_checked_object_out(newTag, &tCheckOutUser, &tCheckOutGroup));
				if (haveWriteAccess && (tCheckOutUser == 0 || tCheckOutUser == tUser))
				{
					itk(AOM_ask_value_tags(newTag, relationProp.c_str(), tagsToSet.plen(), tagsToSet.pptr()));
					tagsToSet.add(tSource);
					itk(AOM_refresh(newTag, true));
					itk(AOM_set_value_tags(newTag, relationProp.c_str(), tagsToSet.len(), tagsToSet.ptr()));
					itk(AOM_refresh(newTag, false));
				}
			}
		}
		// Remove
		for (auto it = v_tmpRefs.begin(); it != v_tmpRefs.end(); ++it)
		{
			tag_t	actualTag = *it;
			
			// If actualTag cannot be found in referencers, remove tSource from actualTag
			if (!find_tag(actualTag, numReferencers, referencers))
			{
				c_ptr<tag_t>	tagsToSet;
				tag_t			tCheckOutUser = 0;
				tag_t			tCheckOutGroup = 0;

				itk(AM_check_privilege(actualTag, "WRITE", &haveWriteAccess));
				itk(RES_is_checked_out(actualTag, &isCheckedOut));
				if (isCheckedOut)
					itk(RES_who_checked_object_out(actualTag, &tCheckOutUser, &tCheckOutGroup));
				if (haveWriteAccess && (tCheckOutUser == 0 || tCheckOutUser == tUser))
				{
					itk(AOM_ask_value_tags(actualTag, relationProp.c_str(), tagsToSet.plen(), tagsToSet.pptr()));
					tagsToSet.remove(tSource);
					itk(AOM_refresh(actualTag, true));
					itk(AOM_set_value_tags(actualTag, relationProp.c_str(), tagsToSet.len(), tagsToSet.ptr()));
					itk(AOM_refresh(actualTag, false));
				}
			}
		}
	}
	catch (tcexception& e)
	{
		result = e.getstat();
		EMH_store_error_s1(EMH_severity_error, FIND_REFERENCERS_DEFAULT_IFAIL, e.what());
		log_error(e.what());
	}
	catch (psexception& e)
	{
		result = FIND_REFERENCERS_DEFAULT_IFAIL;
		EMH_store_error_s1(EMH_severity_error, FIND_REFERENCERS_DEFAULT_IFAIL, e.what());
		log_error(e.what());
	}

	hr_stop_debug(debug_name);
	hr_print_debug(debug_name);
	log_debug("[STOP] %s", debug_name);

	return result;
}