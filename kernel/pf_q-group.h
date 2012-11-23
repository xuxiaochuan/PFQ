/***************************************************************
 *                                                
 * (C) 2011-12 Nicola Bonelli <nicola.bonelli@cnit.it>   
 *             Andrea Di Pietro <andrea.dipietro@for.unipi.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 ****************************************************************/

#ifndef _PF_Q_GROUP_H_
#define _PF_Q_GROUP_H_ 

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/pf_q.h>
#include <linux/filter.h>


#include <pf_q-common.h>
#include <pf_q-sparse-counter.h>
#include <pf_q-steer.h>
#include <pf_q-bpf.h>

struct pfq_group
{
    int policy;                             /* policy for the group */
    int pid;	                            /* process id for restricted/private group */;

	atomic_long_t sock_mask[Q_CLASS_MAX];   /* for class: Q_GROUP_DATA, Q_GROUP_CONTROL, etc... */

	atomic_long_t steering;                 /* steering_function_t */ 
    atomic_long_t state;                    /* opaque state for the steering function */
	atomic_long_t filter; 					/* struct sk_filter pointer */

	sparse_counter_t recv;
	sparse_counter_t lost;
	sparse_counter_t drop;
};


extern struct pfq_group pfq_groups[Q_MAX_GROUP];


unsigned long __pfq_get_all_groups_mask(int gid);


static inline
void __pfq_set_steering_for_group(int gid, steering_function_t fun)
{
    atomic_long_set(&pfq_groups[gid].steering, (long)fun);
    
    msleep(GRACE_PERIOD);
}


static inline
void __pfq_set_state_for_group(int gid, void *state)
{
	void * old = (void *)atomic_long_xchg(& pfq_groups[gid].state, (long)state);

    msleep(GRACE_PERIOD);

	kfree(old);
}


static inline
void __pfq_set_filter_for_group(int gid, struct sk_filter *filter)
{
	struct sk_filter * old_filter = (void *)atomic_long_xchg(& pfq_groups[gid].filter, (long)filter);

    msleep(GRACE_PERIOD);
                             	
    pfq_free_sk_filter(old_filter); 
}


static inline
void __pfq_dismiss_steering_function(steering_function_t fun)
{
	int n;
	for(n = 0; n < Q_MAX_GROUP; n++)
	{
	    steering_function_t steer = (steering_function_t)atomic_long_read(&pfq_groups[n].steering);
    	if (steer == fun)
		{
      		__pfq_set_steering_for_group(n, NULL);
			__pfq_set_state_for_group(n, NULL);
            
			printk(KERN_INFO "[PFQ] steering function @%p dismissed.\n", fun);
		}
	}
}


static inline
bool __pfq_group_is_empty(int gid)
{
	return __pfq_get_all_groups_mask(gid) == 0;
}


static inline
bool __pfq_has_joined_group(int gid, int id)
{
	return __pfq_get_all_groups_mask(gid) & (1L << id);
}

bool __pfq_group_access(int gid, int id, int policy, bool join);

int pfq_join_free_group(int id, unsigned long class_mask, int policy);

int pfq_join_group(int gid, int id, unsigned long class_mask, int policy);

int pfq_leave_group(int gid, int id);

void pfq_leave_all_groups(int id);

unsigned long pfq_get_groups(int id);

#endif /* _PF_Q_GROUP_H_ */
