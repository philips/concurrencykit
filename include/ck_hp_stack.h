/*
 * Copyright 2010-2012 Samy Al Bahra.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _CK_HP_STACK_H
#define _CK_HP_STACK_H

#include <ck_backoff.h>
#include <ck_cc.h>
#include <ck_hp.h>
#include <ck_pr.h>
#include <ck_stack.h>
#include <stddef.h>

#define CK_HP_STACK_SLOTS_COUNT 1
#define CK_HP_STACK_SLOTS_SIZE  sizeof(void *)

CK_CC_INLINE static void
ck_hp_stack_push_mpmc(struct ck_stack *target, struct ck_stack_entry *entry)
{

	ck_stack_push_upmc(target, entry);
	return;
}

CK_CC_INLINE static void *
ck_hp_stack_pop_mpmc(ck_hp_record_t *record, struct ck_stack *target)
{
	struct ck_stack_entry *entry, *update;
	ck_backoff_t backoff = CK_BACKOFF_INITIALIZER;

	do {
		entry = ck_pr_load_ptr(&target->head);
		if (entry == NULL)
			return (NULL);

		ck_hp_set(record, 0, entry);
		ck_pr_fence_memory();
	} while (entry != ck_pr_load_ptr(&target->head));

	while (ck_pr_cas_ptr_value(&target->head, entry, entry->next, &entry) == false) {
		if (entry == NULL)
			return (NULL);

		ck_hp_set(record, 0, entry);
		ck_pr_fence_memory();
		update = ck_pr_load_ptr(&target->head);
		while (entry != update) {
			ck_hp_set(record, 0, update);
			ck_pr_fence_memory();
			entry = update;
			update = ck_pr_load_ptr(&target->head);
			if (update == NULL)
				return (NULL);
		}

		ck_backoff_eb(&backoff);
	}

	return (entry);
}

#endif /* _CK_HP_STACK_H */
