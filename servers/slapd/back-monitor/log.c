/* log.c - deal with log subsystem */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2003 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */
/* This is an altered version */
/*
 * Copyright 2001, Pierangelo Masarati, All rights reserved. <ando@sys-net.it>
 * 
 * This work has beed deveolped for the OpenLDAP Foundation 
 * in the hope that it may be useful to the Open Source community, 
 * but WITHOUT ANY WARRANTY.
 * 
 * Permission is granted to anyone to use this software for any purpose
 * on any computer system, and to alter it and redistribute it, subject
 * to the following restrictions:
 * 
 * 1. The author and SysNet s.n.c. are not responsible for the consequences
 *    of use of this software, no matter how awful, even if they arise from
 *    flaws in it.
 * 
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Since few users ever read sources,
 *    credits should appear in the documentation.
 * 
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.  Since few users
 *    ever read sources, credits should appear in the documentation.
 *    SysNet s.n.c. cannot be responsible for the consequences of the
 *    alterations.
 * 
 * 4. This notice may not be removed or altered.
 */

#ifndef _BACK_MONITOR_H_
#define _BACK_MONITOR_H_

#include <ldap_pvt.h>
#include <ldap_pvt_thread.h>
#include <avl.h>
#include <slap.h>

LDAP_BEGIN_DECL

/*
 * The cache maps DNs to Entries.
/*
 * Copyright 2001, Pierangelo Masarati, All rights reserved. <ando@sys-net.it>
 * 
 * This work has beed deveolped for the OpenLDAP Foundation 
 * in the hope that it may be useful to the Open Source community, 
 * but WITHOUT ANY WARRANTY.
 * 
 * Permission is granted to anyone to use this software for any purpose
 * on any computer system, and to alter it and redistribute it, subject
 * to the following restrictions:
 * 
 * 1. The author and SysNet s.n.c. are not responsible for the consequences
 *    of use of this software, no matter how awful, even if they arise from
 *    flaws in it.
 * 
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Since few users ever read sources,
 *    credits should appear in the documentation.
 * 
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.  Since few users
 *    ever read sources, credits should appear in the documentation.
 *    SysNet s.n.c. cannot be responsible for the consequences of the
 *    alterations.
 * 
 * 4. This notice may not be removed or altered.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>

#include "slap.h"
#include <lber_pvt.h>
#include "lutil.h"
#include "ldif.h"
#include "back-monitor.h"

/*
 * log mutex
 */
ldap_pvt_thread_mutex_t		monitor_log_mutex;

static struct {
	int i;
	struct berval s;
	struct berval n;
} int_2_level[] = {
	{ LDAP_DEBUG_TRACE,	BER_BVC("Trace"),	{ 0, NULL } },
	{ LDAP_DEBUG_PACKETS,	BER_BVC("Packets"),	{ 0, NULL } },
	{ LDAP_DEBUG_ARGS,	BER_BVC("Args"),	{ 0, NULL } },
	{ LDAP_DEBUG_CONNS,	BER_BVC("Conns"),	{ 0, NULL } },
	{ LDAP_DEBUG_BER,	BER_BVC("BER"),	{ 0, NULL } },
	{ LDAP_DEBUG_FILTER,	BER_BVC("Filter"),	{ 0, NULL } },
	{ LDAP_DEBUG_CONFIG,	BER_BVC("Config"),	{ 0, NULL } },	/* useless */
	{ LDAP_DEBUG_ACL,	BER_BVC("ACL"),	{ 0, NULL } },
	{ LDAP_DEBUG_STATS,	BER_BVC("Stats"),	{ 0, NULL } },
	{ LDAP_DEBUG_STATS2,	BER_BVC("Stats2"),	{ 0, NULL } },
	{ LDAP_DEBUG_SHELL,	BER_BVC("Shell"),	{ 0, NULL } },
	{ LDAP_DEBUG_PARSE,	BER_BVC("Parse"),	{ 0, NULL } },
	{ LDAP_DEBUG_CACHE,	BER_BVC("Cache"),	{ 0, NULL } },
	{ LDAP_DEBUG_INDEX,	BER_BVC("Index"),	{ 0, NULL } },
	{ 0,			{ 0, NULL },	{ 0, NULL } }
};

static int loglevel2int( struct berval *l );
static int int2loglevel( int n );

static int add_values( Entry *e, Modification *mod, int *newlevel );
static int delete_values( Entry *e, Modification *mod, int *newlevel );
static int replace_values( Entry *e, Modification *mod, int *newlevel );

/*
 * initializes log subentry
 */
int
monitor_subsys_log_init(
	BackendDB	*be
)
{
	struct monitorinfo	*mi;
	Entry			*e;
	int			i;
	struct berval		desc[] = {
		BER_BVC("This entry allows to set the log level runtime."),
		BER_BVC("Set the attribute 'managedInfo' to the desired log levels."),
		{ 0, NULL }
	};

	ldap_pvt_thread_mutex_init( &monitor_log_mutex );

	mi = ( struct monitorinfo * )be->be_private;

	if ( monitor_cache_get( mi, &monitor_subsys[SLAPD_MONITOR_LOG].mss_ndn, 
				&e ) ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, CRIT,
			"monitor_subsys_log_init: "
			"unable to get entry '%s'\n",
			monitor_subsys[SLAPD_MONITOR_LOG].mss_ndn.bv_val, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_log_init: "
			"unable to get entry '%s'\n%s%s",
			monitor_subsys[SLAPD_MONITOR_LOG].mss_ndn.bv_val, 
			"", "" );
#endif
		return( -1 );
	}

	/* initialize the debug level(s) */
	for ( i = 0; int_2_level[ i ].i != 0; i++ ) {

		if ( mi->mi_ad_managedInfo->ad_type->sat_equality->smr_normalize ) {
			int	rc;

			rc = (*mi->mi_ad_managedInfo->ad_type->sat_equality->smr_normalize)(
					SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
					mi->mi_ad_managedInfo->ad_type->sat_syntax,
					mi->mi_ad_managedInfo->ad_type->sat_equality,
					&int_2_level[ i ].s,
					&int_2_level[ i ].n, NULL );
			if ( rc ) {
				return( -1 );
			}
		}

		if ( int_2_level[ i ].i & ldap_syslog ) {
			attr_merge_one( e, mi->mi_ad_managedInfo,
					&int_2_level[ i ].s,
					&int_2_level[ i ].n );
		}
	}

	attr_merge( e, mi->mi_ad_description, desc, NULL );

	monitor_cache_release( mi, e );

	return( 0 );
}

int 
monitor_subsys_log_modify( 
	Operation		*op,
	Entry 			*e
)
{
	struct monitorinfo *mi = (struct monitorinfo *)op->o_bd->be_private;
	int		rc = LDAP_OTHER;
	int		newlevel = ldap_syslog;
	Attribute	*save_attrs;
	Modifications	*modlist = op->oq_modify.rs_modlist;
	Modifications	*ml;

	ldap_pvt_thread_mutex_lock( &monitor_log_mutex );

	save_attrs = e->e_attrs;
	e->e_attrs = attrs_dup( e->e_attrs );

	for ( ml = modlist; ml != NULL; ml = ml->sml_next ) {
		Modification	*mod = &ml->sml_mod;

		/*
		 * accept all operational attributes
		 */
		if ( is_at_operational( mod->sm_desc->ad_type ) ) {
			( void ) attr_delete( &e->e_attrs, mod->sm_desc );
			rc = attr_merge( e, mod->sm_desc, mod->sm_bvalues,
					mod->sm_nvalues );
			if ( rc != 0 ) {
				rc = LDAP_OTHER;
				break;
			}
			continue;

		/*
		 * only the monitor description attribute can be modified
		 */
		} else if ( mod->sm_desc != mi->mi_ad_managedInfo) {
			rc = LDAP_UNWILLING_TO_PERFORM;
			break;
		}

		switch ( mod->sm_op ) {
		case LDAP_MOD_ADD:
			rc = add_values( e, mod, &newlevel );
			break;
			
		case LDAP_MOD_DELETE:
			rc = delete_values( e, mod, &newlevel );
			break;

		case LDAP_MOD_REPLACE:
			rc = replace_values( e, mod, &newlevel );
			break;

		default:
			rc = LDAP_OTHER;
			break;
		}

		if ( rc != LDAP_SUCCESS ) {
			break;
		}
	}

	/* set the new debug level */
	if ( rc == LDAP_SUCCESS ) {
		const char	*text;
		static char	textbuf[ BACKMONITOR_BUFSIZE ];

		/* check for abandon */
		if ( op->o_abandon ) {
			rc = SLAPD_ABANDON;

			goto cleanup;
		}

		/* check that the entry still obeys the schema */
		rc = entry_schema_check( be_monitor, e, save_attrs, 
				&text, textbuf, sizeof( textbuf ) );
		if ( rc != LDAP_SUCCESS ) {
			goto cleanup;
		}

		/*
		 * Do we need to protect this with a mutex?
		 */
		ldap_syslog = newlevel;

#if 0	/* debug rather than log */
		slap_debug = newlevel;
		lutil_set_debug_level( "slapd", slap_debug );
		ber_set_option(NULL, LBER_OPT_DEBUG_LEVEL, &slap_debug);
		ldap_set_option(NULL, LDAP_OPT_DEBUG_LEVEL, &slap_debug);
		ldif_debug = slap_debug;
#endif
	}

cleanup:;
	if ( rc == LDAP_SUCCESS ) {
		attrs_free( save_attrs );

	} else {
		attrs_free( e->e_attrs );
		e->e_attrs = save_attrs;
	}
	
	ldap_pvt_thread_mutex_unlock( &monitor_log_mutex );

	return( rc );
}

static int
loglevel2int( struct berval *l )
{
	int		i;
	
	for ( i = 0; int_2_level[ i ].i != 0; i++ ) {
		if ( l->bv_len != int_2_level[ i ].s.bv_len ) {
			continue;
		}

		if ( strcasecmp( l->bv_val, int_2_level[ i ].s.bv_val ) == 0 ) {
			return int_2_level[ i ].i;
		}
	}

	return 0;
}

static int
int2loglevel( int n )
{
	int		i;
	
	for ( i = 0; int_2_level[ i ].i != 0; i++ ) {
		if ( int_2_level[ i ].i == n ) {
			return i;
		}
	}

	return -1;
}

static int
check_constraints( Modification *mod, int *newlevel )
{
	int		i;

	for ( i = 0; mod->sm_bvalues && mod->sm_bvalues[i].bv_val != NULL; i++ ) {
		int		l;
		
		l = loglevel2int( &mod->sm_bvalues[i] );
		if ( !l ) {
			return LDAP_CONSTRAINT_VIOLATION;
		}

		if ( ( l = int2loglevel( l ) ) == -1 ) {
			return LDAP_OTHER;
		}

		assert( int_2_level[ l ].s.bv_len
				== mod->sm_bvalues[i].bv_len );
		
		AC_MEMCPY( mod->sm_bvalues[i].bv_val,
				int_2_level[ l ].s.bv_val,
				int_2_level[ l ].s.bv_len );

		AC_MEMCPY( mod->sm_nvalues[i].bv_val,
				int_2_level[ l ].n.bv_val,
				int_2_level[ l ].n.bv_len );

		*newlevel |= l;
	}

	return LDAP_SUCCESS;
}	

static int 
add_values( Entry *e, Modification *mod, int *newlevel )
{
	Attribute	*a;
	int		i, rc;
	MatchingRule 	*mr = mod->sm_desc->ad_type->sat_equality;

	rc = check_constraints( mod, newlevel );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	a = attr_find( e->e_attrs, mod->sm_desc );

	if ( a != NULL ) {
		/* "description" SHOULD have appropriate rules ... */
		if ( mr == NULL || !mr->smr_match ) {
			return LDAP_INAPPROPRIATE_MATCHING;
		}

		for ( i = 0; mod->sm_bvalues[i].bv_val != NULL; i++ ) {
			int rc;
			int j;
			const char *text = NULL;
			struct berval asserted;

			rc = asserted_value_validate_normalize(
				mod->sm_desc, mr, SLAP_MR_EQUALITY,
				&mod->sm_bvalues[i], &asserted, &text, NULL );

			if ( rc != LDAP_SUCCESS ) {
				return rc;
			}

			for ( j = 0; a->a_vals[j].bv_val != NULL; j++ ) {
				int match;
				int rc = value_match( &match, mod->sm_desc, mr,
					0,
					&a->a_vals[j], &asserted, &text );

				if ( rc == LDAP_SUCCESS && match == 0 ) {
					free( asserted.bv_val );
					return LDAP_TYPE_OR_VALUE_EXISTS;
				}
			}

			free( asserted.bv_val );
		}
	}

	/* no - add them */
	rc = attr_merge( e, mod->sm_desc, mod->sm_bvalues, mod->sm_nvalues );
	if ( rc != LDAP_SUCCESS ) {
		/* this should return result of attr_mergeit */
		return rc;
	}

	return LDAP_SUCCESS;
}

static int
delete_values( Entry *e, Modification *mod, int *newlevel )
{
	int             i, j, k, found, rc, nl = 0;
	Attribute       *a;
	MatchingRule 	*mr = mod->sm_desc->ad_type->sat_equality;

	rc = check_constraints( mod, &nl );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	*newlevel &= ~nl;

	/* delete the entire attribute */
	if ( mod->sm_bvalues == NULL ) {
		int rc = attr_delete( &e->e_attrs, mod->sm_desc );

		if ( rc ) {
			rc = LDAP_NO_SUCH_ATTRIBUTE;
		} else {
			*newlevel = 0;
			rc = LDAP_SUCCESS;
		}
		return rc;
	}

	if ( mr == NULL || !mr->smr_match ) {
		/* disallow specific attributes from being deleted if
		 * no equality rule */
		return LDAP_INAPPROPRIATE_MATCHING;
	}

	/* delete specific values - find the attribute first */
	if ( (a = attr_find( e->e_attrs, mod->sm_desc )) == NULL ) {
		return( LDAP_NO_SUCH_ATTRIBUTE );
	}

	/* find each value to delete */
	for ( i = 0; mod->sm_bvalues[i].bv_val != NULL; i++ ) {
		int rc;
		const char *text = NULL;

		struct berval asserted;

		rc = asserted_value_validate_normalize(
				mod->sm_desc, mr, SLAP_MR_EQUALITY,
				&mod->sm_bvalues[i], &asserted, &text, NULL );

		if( rc != LDAP_SUCCESS ) return rc;

		found = 0;
		for ( j = 0; a->a_vals[j].bv_val != NULL; j++ ) {
			int match;
			int rc = value_match( &match, mod->sm_desc, mr,
				0,
				&a->a_vals[j], &asserted, &text );

			if( rc == LDAP_SUCCESS && match != 0 ) {
				continue;
			}

			/* found a matching value */
			found = 1;

			/* delete it */
			free( a->a_vals[j].bv_val );
			for ( k = j + 1; a->a_vals[k].bv_val != NULL; k++ ) {
				a->a_vals[k - 1] = a->a_vals[k];
			}
			a->a_vals[k - 1].bv_val = NULL;

			break;
		}

		free( asserted.bv_val );

		/* looked through them all w/o finding it */
		if ( ! found ) {
			return LDAP_NO_SUCH_ATTRIBUTE;
		}
	}

	/* if no values remain, delete the entire attribute */
	if ( a->a_vals[0].bv_val == NULL ) {
		/* should already be zero */
		*newlevel = 0;
		
		if ( attr_delete( &e->e_attrs, mod->sm_desc ) ) {
			return LDAP_NO_SUCH_ATTRIBUTE;
		}
	}

	return LDAP_SUCCESS;
}

static int
replace_values( Entry *e, Modification *mod, int *newlevel )
{
	int rc;

	*newlevel = 0;
	rc = check_constraints( mod, newlevel );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	rc = attr_delete( &e->e_attrs, mod->sm_desc );

	if ( rc != LDAP_SUCCESS && rc != LDAP_NO_SUCH_ATTRIBUTE ) {
		return rc;
	}

	if ( mod->sm_bvalues != NULL ) {
		rc = attr_merge( e, mod->sm_desc, mod->sm_bvalues,
		       mod->sm_nvalues );
		if ( rc != LDAP_SUCCESS ) {
			return rc;
		}
	}

	return LDAP_SUCCESS;
}

