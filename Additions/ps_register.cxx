#include "ps_global.hxx"
#include "ps_defines.hxx"
#include "ps_rule_handlers.hxx"
#include "ps_action_handlers.hxx"
#include "ps_register.hxx"
#include "ps_name_rules.hxx"
#include "ps_find_referencers.hxx"
#include "ps_query_folders.hxx"


using namespace ps;

void ps::ps_register_handlers()
{
	// Action handlers
	itk(EPM_register_action_handler(AH_CREATE_DISPATCHER_REQUEST, "Create dispatcher request.", (EPM_action_handler_t) ps_create_dispatcher_request_ah));
	itk(EPM_register_action_handler(AH_COPY_TASK_PROPERTIES, "Copy properties from task to process target.", (EPM_action_handler_t) ps_copy_task_properties_ah));
	itk(EPM_register_action_handler(AH_MOVE_ATTACHMENTS, "Move attachments from target to reference or vice versa.", (EPM_action_handler_t) ps_move_attachments_ah));
	itk(EPM_register_action_handler(AH_TIMER_START, "Starts a performance timer.", (EPM_action_handler_t) ps_timer_start_ah));
	itk(EPM_register_action_handler(AH_TIMER_STOP, "Stops a performance timer.", (EPM_action_handler_t) ps_timer_stop_ah));
	itk(EPM_register_action_handler(AH_SET_STATUS, "Creates and assigns release statuses.", (EPM_action_handler_t) ps_set_statuses_ah));
	itk(EPM_register_action_handler(AH_LOG_DECISION, "Logs decisions from review tasks.", (EPM_action_handler_t) ps_log_signoff_ah));

	// Rule handlers
	itk(EPM_register_rule_handler(AH_CHECK_IN_PROCESS, "Verify that no targets are in other processes.", (EPM_rule_handler_t) ps_check_in_process_rh));
	itk(EPM_register_rule_handler(AH_CHECK_INITIATOR, "Verify that initiating user is valid.", (EPM_rule_handler_t) ps_check_initiator_rh));
	itk(EPM_register_rule_handler(AH_CHECK_PRIVILEGES, "Verify that initiating user has correct privileges.", (EPM_rule_handler_t) ps_check_privileges_rh));
	itk(EPM_register_rule_handler(AH_CHECK_PROPERTIES, "Verify that mandatory properties are set.", (EPM_rule_handler_t) ps_check_properties_rh));
	itk(EPM_register_rule_handler(AH_CHECK_CHILD_STRUCTURE, "Verify that child structure have correct status.", (EPM_rule_handler_t) ps_check_child_structure_rh));
	itk(EPM_register_rule_handler(AH_CHECK_WHERE_USED, "Verify that target does not have an invalid structure parent.", (EPM_rule_handler_t) ps_check_where_used_rh));
}

void ps::ps_register_name_rules()
{
	c_pptr<char>	nameRuleLines;
	TC_preference_search_scope_t oldScope;

	nameRuleLines.set_free_container_only(true);
	itk(PREF_ask_search_scope(&oldScope));
	itk(PREF_set_search_scope(TC_preference_site));
	try
	{
		itk(PREF_ask_char_values(PREF_NAME_RULES, nameRuleLines.plen(), nameRuleLines.pptr()));
	}
	catch (tcexception& e)
	{
		if (e.getstat() == PF_NOTFOUND)
		{
			return;
		}
	}
	itk(PREF_set_search_scope(oldScope));

	for (int i = 0; i < nameRuleLines.len(); i++)
	{
		const char			*nameRuleLine = nameRuleLines.val(i);
		string				propertyTypeMsg;
		vector<string>		nameRuleLinesSplitted; //!< 0 = Object Type, 1 = Property Type, 2 = Property Name, 3 = RegEx., 4 = Length, 5 = Error message
		METHOD_id_t 		method;
		c_ptr<TC_argument_list_t>	argument;

		split_string(nameRuleLine, ":", true, nameRuleLinesSplitted);

		// Don't register if incorrect setting
		if (nameRuleLinesSplitted.size() != 6)
			continue;

		if (nameRuleLinesSplitted[1] == "int")
		{
			propertyTypeMsg = PROP_set_value_int_msg;
		}
		else if (nameRuleLinesSplitted[1] == "float")
		{
			propertyTypeMsg = PROP_set_value_double_msg;
		}
		else if (nameRuleLinesSplitted[1] == "double")
		{
			propertyTypeMsg = PROP_set_value_double_msg;
		}
		else if (nameRuleLinesSplitted[1] == "string")
		{
			propertyTypeMsg = PROP_set_value_string_msg;
		}
		else
			throw psexception("Unsupported property type for naming rule validation.");

		// Allocate all memory on the heap
		argument.set_free(false);
		argument.alloc(1);
		argument.ptr()->number_of_arguments = 4;
		sm_alloc(argument.ptr()->arguments, TC_argument_t, argument.ptr()->number_of_arguments);
		argument.ptr()->arguments[0].type = POM_string;
		argument.ptr()->arguments[0].array_size = 1;
		sm_alloc(argument.ptr()->arguments[0].val_union.str_value, char, nameRuleLinesSplitted[1].length() + 1);
		tc_strcpy(argument.ptr()->arguments[0].val_union.str_value, nameRuleLinesSplitted[1].c_str());
		argument.ptr()->arguments[1].type = POM_string;
		argument.ptr()->arguments[1].array_size = 1;
		sm_alloc(argument.ptr()->arguments[1].val_union.str_value, char, nameRuleLinesSplitted[3].length() + 1);
		tc_strcpy(argument.ptr()->arguments[1].val_union.str_value, nameRuleLinesSplitted[3].c_str());
		argument.ptr()->arguments[2].type = POM_int;
		argument.ptr()->arguments[2].array_size = 1;
		argument.ptr()->arguments[2].val_union.int_value = atoi(nameRuleLinesSplitted[4].c_str());
		argument.ptr()->arguments[3].type = POM_string;
		argument.ptr()->arguments[3].array_size = 1;
		sm_alloc(argument.ptr()->arguments[3].val_union.str_value, char, nameRuleLinesSplitted[5].length() + 1);
		tc_strcpy(argument.ptr()->arguments[3].val_union.str_value, nameRuleLinesSplitted[5].c_str());

		itk(METHOD_find_prop_method(nameRuleLinesSplitted[0].c_str(), nameRuleLinesSplitted[2].c_str(), propertyTypeMsg.c_str(), &method));
		itk(METHOD_add_action(method, METHOD_pre_action_type, ps_validate_name_rule, argument.ptr()));
	}
}

void ps::ps_register_referencers()
{
	c_pptr<char>	referencerLines;

	//char ch;
	//ch = getchar();

	referencerLines.set_free_container_only(true);
	if (!get_preference(PS2_REFERENCER_PROPERTIES_PREF,  referencerLines.plen(), referencerLines.pptr()))
		return;

	for (int i = 0; i < referencerLines.len(); i++)
	{
		const char			*referencerLine = referencerLines.val(i);
		vector<string>		referencerLinesSplitted; //!< 0 = Source Type, 1 = Source Property, 2 = Target Relation, 3 = Filter types
		string				sourceType;
		string				sourceProp;
		string				targetRelation;
		string				filterTypes;
		c_ptr<char>			modifiable;
		bool				filterDefined = false;
		METHOD_id_t 		method;
		c_ptr<TC_argument_list_t>	argument;

		split_string(referencerLine, ":", true, referencerLinesSplitted);

		// Don't register if incorrect settings
		if (referencerLinesSplitted.size() != 4)
			continue;
		if (referencerLinesSplitted[0].empty() || referencerLinesSplitted[1].empty() ||
			referencerLinesSplitted[2].empty() || referencerLinesSplitted[3].empty())
			continue;

		sourceType = referencerLinesSplitted[0];
		sourceProp = referencerLinesSplitted[1];
		targetRelation = referencerLinesSplitted[2];
		filterTypes = referencerLinesSplitted[3];

		// Allocate all memory on the heap
		argument.set_free(false);
		argument.alloc(1);
		argument.ptr()->number_of_arguments = 2;
		sm_alloc(argument.ptr()->arguments, TC_argument_t, argument.ptr()->number_of_arguments);
		// Target relation
		argument.ptr()->arguments[0].type = POM_string;
		argument.ptr()->arguments[0].array_size = 1;
		sm_alloc(argument.ptr()->arguments[0].val_union.str_value, char, targetRelation.length() + 1);
		tc_strcpy(argument.ptr()->arguments[0].val_union.str_value, targetRelation.c_str());
		// Filter types
		argument.ptr()->arguments[1].type = POM_string;
		argument.ptr()->arguments[1].array_size = 1;
		sm_alloc(argument.ptr()->arguments[1].val_union.str_value, char, filterTypes.length() + 1);
		tc_strcpy(argument.ptr()->arguments[1].val_union.str_value, filterTypes.c_str());

		itk(METHOD__register_prop_operationFn(sourceType.c_str(), sourceProp.c_str(), PROP_ask_value_tags_msg, &ps_ask_referencers, argument.ptr(), &method));

		itk(CONSTANTS_get_property_constant_value("Modifiable", sourceType.c_str(), sourceProp.c_str(), modifiable.pptr()));

		if (strcmp(modifiable.ptr(), "Write") == 0)
		{
			itk(METHOD__register_prop_operationFn(referencerLinesSplitted[0].c_str(), referencerLinesSplitted[1].c_str(), PROP_set_value_tags_msg, &ps_set_referencers, argument.ptr(), &method));
		}
	}
}

void ps::ps_register_query_folders()
{
	METHOD_id_t 		method;
	logical				isEnabled;

	//char ch;
	//ch = getchar();

	if (get_preference(PS2_QUERY_FOLDER_ENABLED_PREF, &isEnabled))
	{
		if (isEnabled)
		{
			printf("Dynamic folders enabled.\n");

			itk(METHOD__register_prop_operationFn(PS2_QUERY_FOLDER_TYPE, PS2_QUERY_FOLDER_RUNTIME_PROP,
				PROP_ask_value_tags_msg, &ps_query_folders, NULL, &method));
		}
	}
}

int ps::libps_oninit(int *decision, va_list args)
{
	try
	{
		// Initialize logging mechanism
		if (initialize_logging())
			printf("Logging module enabled.\n");
		// If enabled, initialize HRTimer class
		if (hr_init())
			printf("HRTimer module enabled.\n");

		// Register handlers
		try
		{
			ps_register_handlers();
		}
		catch (exception &e)
		{
			TC_write_syslog("%s\n", e.what());
		}

		// Register naming rules
		try
		{
			ps_register_name_rules();
		}
		catch (exception &e)
		{
			TC_write_syslog("%s\n", e.what());
		}

		// Register referencer properties
		try
		{
			ps_register_referencers();
		}
		catch (exception &e)
		{
			TC_write_syslog("%s\n", e.what());
		}
		// Register dynamic folders
		try
		{
			ps_register_query_folders();
		}
		catch (exception &e)
		{
			TC_write_syslog("%s\n", e.what());
		}
	}
	catch (tcexception& e)
	{
		TC_write_syslog("%s\n", e.what());

		return e.getstat();
	}
	catch (exception& e)
	{
		TC_write_syslog("%s\n", e.what());

		return 1;
	}

	return ITK_ok;
}

int ps::libps_onexit(int *decision, va_list args)
{
	try
	{
		// Print performance statistics when logging out
		hr_print_all();
	}
	catch (tcexception& e)
	{
		TC_write_syslog("%s\n", e.what());

		return e.getstat();
	}
	catch (exception& e)
	{
		TC_write_syslog("%s\n", e.what());

		return 1;
	}

	return ITK_ok;
}

int libpsbva_register_callbacks()
{ 
	printf("Installing PS User Exits Library - libpsadds, "__DATE__" "__TIME__".\n");

	try
	{
		itk(CUSTOM_register_exit("libpsbva", "USER_init_module", (CUSTOM_EXIT_ftn_t) libps_oninit));
		itk(CUSTOM_register_exit("libpsbva", "USER_exit_module", (CUSTOM_EXIT_ftn_t) libps_onexit));
	}
	catch (tcexception& e)
	{
		TC_write_syslog(e.what());

		return e.getstat();
	}
	catch (exception& e)
	{
		TC_write_syslog(e.what());

		return 1;
	}

	return ITK_ok;
}