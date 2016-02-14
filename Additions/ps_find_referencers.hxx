/*!
 * \file ps_find_referencers.hxx
 * \date 2014/05/04 19:12
 *
 * \author Mattias Jonsson (jonssonm)
 * Contact: jonsson.mattias@siemens.com
 *
 * \brief Runtime property returning referencers to the object and relation
 * it is configured for.
 *
*/

#pragma once

#include "ps_global.hxx"

//! Preference definitions
#define PS2_REFERENCER_PROPERTIES_PREF "PS2_Referencer_Properties"

namespace ps
{
	//! Returns referencing objects to configured object and relation. Type filter can be optionally set.
	int ps_find_referencers(tag_t tSource, string relationProp, string filterTypes, vector<tag_t>& v_referencers, vector<string>& v_relations);
	int ps_ask_referencers(METHOD_message_t *m, va_list  args);
	int ps_set_referencers(METHOD_message_t *m, va_list  args);
}