/*!
 * \file ps_global_tc.hxx
 * \date 2014/04/30 20:52
 *
 * \author Mattias Jonsson (jonssonm)
 * Contact: jonsson.mattias@siemens.com
 *
 * \brief Global include file that can be called from various ITK implementation headers.
 *
*/

#pragma once

#ifdef _TC_9
#include <tc/tc.h>
#include <sa/am.h>
#include <tc/folder.h>
#include <sa/user.h>
#include <pom/pom/pom.h>
#include <bom/bom.h>
#include <ae/ae.h>
#include <tccore/aom.h>
#include <tccore/aom_prop.h>
#include <constants/constants.h>
#include <tccore/item.h>
#include <cfm/cfm.h>
#include <tccore/workspaceobject.h>
#include <tc/emh.h>
#include <tc/preferences.h>
#include <res/res_itk.h>
#include <fclasses/tc_string.h>
#include <epm/epm.h>
#include <epm/epm_task_template_itk.h>
#include <epm/signoff.h>
#include <property/nr.h>
#include <pom/enq/enq.h>
#include <tccore/grm.h>
#include <tccore/project.h>
#include <tc/emh.h>
#include <textsrv/textserver.h>
#include <server_exits/user_server_exits.h>
#include <user_exits/user_exits.h>
#include <tccore/custom.h>
#include <schmgt/schmgt_itk.h>
#include <user_exits/epm_toolkit_utils.h>
#include <dispatcher/dispatcher.h>
#include <dispatcher/dispatcher_itk.h>
#include <ss/ss_errors.h>
#endif

#ifdef _TC_10
#include <tc/tc.h>
#include <sa/am.h>
#include <tc/folder.h>
#include <sa/user.h>
#include <pom/pom/pom.h>
#include <bom/bom.h>
#include <ae/ae.h>
#include <tccore/aom.h>
#include <tccore/aom_prop.h>
#include <constants/constants.h>
#include <tccore/item.h>
#include <cfm/cfm.h>
#include <tccore/workspaceobject.h>
#include <tc/emh.h>
#include <tc/preferences.h>
#include <res/res_itk.h>
#include <fclasses/tc_string.h>
#include <epm/epm.h>
#include <epm/epm_task_template_itk.h>
#include <epm/signoff.h>
#include <property/nr.h>
#include <pom/enq/enq.h>
#include <tccore/grm.h>
#include <tccore/project.h>
#include <tc/emh.h>
#include <textsrv/textserver.h>
#include <server_exits/user_server_exits.h>
#include <user_exits/user_exits.h>
#include <tccore/custom.h>
#include <schmgt/schmgt_itk.h>
#include <user_exits/epm_toolkit_utils.h>
#include <dispatcher/dispatcher.h>
#include <dispatcher/dispatcher_itk.h>
#include <ss/ss_errors.h>
#endif