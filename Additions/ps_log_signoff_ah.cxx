#include "ps_global.hxx"
#include "ps_defines.hxx"
#include "ps_action_handlers.hxx"

using namespace ps;

// Return true if a form with SignOff UID already exist
bool form_exist(tag_t tPrimary, tag_t tRelationType, string& formUidProp, const char *signOffUid) {
	c_ptr<tag_t>		tAttachedForms;
	logical				formExist = false;

	itk(GRM_list_secondary_objects_only(tPrimary, tRelationType, tAttachedForms.plen(), tAttachedForms.pptr()));

	for (int j = 0; j < tAttachedForms.len(); j++) {
		c_ptr<char>			formUid;

		itk(AOM_ask_value_string(tAttachedForms.val(j), formUidProp.c_str(), formUid.pptr()));

		if (match_string(signOffUid, formUid.ptr())) {
			return true;
		}
	}

	return false;
}

// Get values from SignOff, create form, set values on form
tag_t create_form(tag_t tSignOff, const char *attachUid, tag_t tFormType, vector<string>& vLogFormPropMap, const char *reviewTaskName) {
	tag_t			tFormTypeCreInput;
	tag_t			tAssignee;
	tag_t			tForm = 0;
	tag_t			tRelation;
	tag_t			tAttachClass;
	c_ptr<char>		decision;
	c_ptr<char>		comments;
	date_t			decisionDate;

	// Get values from SignOff
	itk(AOM_ask_value_tag(tSignOff, "fnd0Assignee", &tAssignee));
	itk(AOM_ask_value_string(tSignOff, "fnd0Status", decision.pptr()));
	itk(AOM_ask_value_date(tSignOff, "decision_date", &decisionDate));
	itk(AOM_ask_value_string(tSignOff, "comments", comments.pptr()));

	// Construct form and set all values from SignOff
	itk(TCTYPE_construct_create_input(tFormType, &tFormTypeCreInput));

	itk(AOM_set_value_string(tFormTypeCreInput, "object_name", attachUid));
	itk(AOM_set_value_string(tFormTypeCreInput, vLogFormPropMap[0].c_str(), attachUid));
	itk(AOM_set_value_string(tFormTypeCreInput, vLogFormPropMap[1].c_str(), reviewTaskName));
	itk(AOM_set_value_tag(tFormTypeCreInput, vLogFormPropMap[2].c_str(), tAssignee));
	itk(AOM_set_value_string(tFormTypeCreInput, vLogFormPropMap[3].c_str(), decision.ptr()));
	itk(AOM_set_value_date(tFormTypeCreInput, vLogFormPropMap[4].c_str(), decisionDate));
	itk(AOM_set_value_string(tFormTypeCreInput, vLogFormPropMap[5].c_str(), comments.ptr()));

	itk(TCTYPE_create_object(tFormTypeCreInput, &tForm));
	itk(AOM_save_with_extensions(tForm));

	return tForm;
}

int ps_log_signoff_ah(EPM_action_message_t msg)
{
	const char			*debug_name = AH_LOG_DECISION;
	int					x;
	int					result = ITK_ok;
	string				pLogFormType;
	string				pLogAttachType;
	string				pLogAttachRelation;
	tag_t				tRootTask = 0;
	static tag_t		tFormType = 0;
	static tag_t		tRelationType = 0;
	static tag_t		tSignOff = 0;
	c_ptr<tag_t>		tSignOffs;
	c_ptr<tag_t>		tTargets;
	c_ptr<char>			reviewTaskName;
	vector<string>		vLogFormPropMap;
	vector<tag_t>		vTargets;
	h_args				args(msg.arguments);

	// Skip all messages except Approve or Reject
	if (msg.action != EPM_approve_action && msg.action != EPM_reject_action)
		return ITK_ok;

	log_debug("[START] %s", debug_name);
	hr_start_debug(debug_name);

	try
	{
		// Parse all arguments
		if (args.size() == 0)
			throw psexception("Missing mandatory arguments.");

		if (!args.getStr("log_form_type", pLogFormType))
			throw psexception("Missing mandatory argument 'log_form_type'.");
		if (!args.getVec("log_form_prop_map", vLogFormPropMap))
			throw psexception("Missing mandatory argument 'log_form_prop_map'.");
		if (!args.getStr("attach_type", pLogAttachType))
			throw psexception("Missing mandatory argument 'attach_type'.");
		if (!args.getStr("attach_relation", pLogAttachRelation))
			throw psexception("Missing mandatory argument 'attach_relation'.");

		// Check that correct number of values passed in log_form_prop_map
		if (vLogFormPropMap.size() != 6)
			throw psexception("Illegal number of values in parameter 'log_form_prop_map'");

		itk(EPM_ask_root_task(msg.task, &tRootTask));

		// If not adding forms to root task, collect valid objects from target list and pre-populate vector
		if (pLogAttachType == "<RootTask>") {
			vTargets.push_back(tRootTask);
		}
		else {
			itk(EPM_ask_attachments(tRootTask, EPM_target_attachment, tTargets.plen(), tTargets.pptr()));

			for (int j = 0; j < tTargets.len(); j++) {
				c_ptr<char>			targetType;

				itk(WSOM_ask_object_type2(tTargets.val(j), targetType.pptr()));

				if (match_string(targetType.ptr(), pLogAttachType)) {
					vTargets.push_back(tTargets.val(j));
				}
			}

		}

		// If no targets found, exit function
		if (vTargets.size() == 0)
			return ITK_ok;

		// Preset constant variables
		if (tSignOff == 0)
			itk(POM_class_id_of_class("SignOff", &tSignOff));
		if (tFormType == 0)
			itk(TCTYPE_find_type(pLogFormType.c_str(), NULL, &tFormType));
		if (tRelationType == 0)
			itk(GRM_find_relation_type(pLogAttachRelation.c_str(), &tRelationType));

		// Get information from current task
		itk(EPM_ask_review_task_name2(msg.task, reviewTaskName.pptr()));
		itk(AOM_ask_value_tags(msg.task, "signoff_attachments", tSignOffs.plen(), tSignOffs.pptr()));

		// Loop over all task attachments
		for (int i = 0; i < tSignOffs.len(); i++) {
			tag_t				tSignOff = tSignOffs.val(i);
			tag_t				tForm = 0;
			tag_t				tRelation;
			c_ptr<char>			decision;
			logical				isDesc;

			// Get decision from SignOff
			itk(AOM_ask_value_string(tSignOff, "fnd0Status", decision.pptr()));

			// Only proceed if action equals action registered on SignOff
			if (msg.action == EPM_approve_action && match_string("Approved", decision.ptr()) ||
				msg.action == EPM_reject_action && match_string("Rejected", decision.ptr())) {
				
				// Loop over valid targets
				for (int j = 0; j < vTargets.size(); j++) {
					tag_t				tTarget = vTargets[j];
					c_ptr<char>			signOffUid;

					// Check so that SignOff info has not already been added
					itk(POM_tag_to_uid(tSignOff, signOffUid.pptr()));
					if (!form_exist(tTarget, tRelationType, vLogFormPropMap[0], signOffUid.ptr())) {
						begin_trans(x);

						if (tForm == 0)
							tForm = create_form(tSignOff, signOffUid.ptr(), tFormType, vLogFormPropMap, reviewTaskName.ptr());
						itk(GRM_create_relation(tTarget, tForm, tRelationType, 0, &tRelation));
						itk(GRM_save_relation(tRelation));
						
						commit_trans(x);
					}
				}

			}
		}
	}
	catch (tcexception& e)
	{
		rollback_trans(x);
		result = e.getstat();
		EMH_store_error_s1(EMH_severity_error, ACTION_HANDLER_DEFAULT_IFAIL, e.what());
		log_error(e.what());
	}
	catch (psexception& e)
	{
		rollback_trans(x);
		result = ACTION_HANDLER_DEFAULT_IFAIL;
		EMH_store_error_s1(EMH_severity_error, ACTION_HANDLER_DEFAULT_IFAIL, e.what());
		log_error(e.what());
	}

	hr_stop_debug(debug_name);
	hr_print_debug(debug_name);
	log_debug("[STOP] %s", debug_name);

	return result;
}