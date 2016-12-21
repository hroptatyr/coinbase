/*** btree.c -- simple btree impl
 *
 * Copyright (C) 2016 Sebastian Freundt
 *
 * Author:  Sebastian Freundt <freundt@ga-group.nl>
 *
 * This file is part of coinbase.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the author nor the names of any contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **/
#if defined HAVE_CONFIG_H
# include "config.h"
#endif	/* HAVE_CONFIG_H */
#if defined HAVE_DFP754_H
# include <dfp754.h>
#endif	/* HAVE_DFP754_H */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "btree.h"
#include "nifty.h"

typedef union {
	VAL_T v;
	btree_t t;
} btree_val_t;

struct btree_s {
	size_t innerp:1;
	size_t n:63;
	KEY_T key[63U + 1U/*spare*/];
	btree_val_t val[64U];
	btree_t next;
};


static bool
leaf_add(btree_t t, KEY_T k, VAL_T *v[static 1U])
{
	size_t i;

	for (i = 0U; i < t->n && k > t->key[i]; i++);
	/* so k is <= t->key[i] or t->key[i] is nan */

	if (k == t->key[i]) {
		/* got him */
		goto out;
	} else if (i < t->n) {
		/* have to open a new node, isn't it? */
		memmove(t->key + i + 1U, t->key + i, (countof(t->key) - (i + 1U)) * sizeof(*t->key));
		memmove(t->val + i + 1U, t->val + i, (countof(t->key) - (i + 1U)) * sizeof(*t->val));
	}
	t->n++;
	t->key[i] = k;
	t->val[i].v = 0.dd;
out:
	*v = &t->val[i].v;
	return t->n >= countof(t->key) - 1U;
}

static bool
twig_add(btree_t t, KEY_T k, VAL_T *v[static 1U])
{
	bool splitp;
	btree_t c;
	size_t i;

	for (i = 0U; i < t->n && k > t->key[i]; i++);
	/* so k is <= t->key[i] or t->key[i] is nan */

	/* descent */
	c = t->val[i].t;

	if (!c->innerp) {
		/* oh, we're in the leaves again */
		splitp = leaf_add(c, k, v);
	} else {
		/* got to go deeper, isn't it? */
		splitp = twig_add(c, k, v);
	}

	if (splitp) {
		/* C needs splitting, not again */
		btree_t rght = make_btree();
		const size_t piv = countof(c->key) / 2U - 1U;

		/* shift things to RGHT */
		memcpy(rght->key, c->key + piv + 1U, (piv + 0U) * sizeof(*c->key));
		memcpy(rght->val, c->val + piv + 1U, (piv + 1U) * sizeof(*c->val));
		rght->innerp = c->innerp;
		rght->n = piv;
		rght->next = NULL;

		if (i < t->n) {
			/* make some room then */
			memmove(t->key + i + 1U, t->key + i, (countof(t->key) - (i + 1U)) * sizeof(*t->key));
			memmove(t->val + i + 1U, t->val + i, (countof(t->key) - (i + 1U)) * sizeof(*t->val));
		}
		/* and now massage LEFT which is C and T */
		t->key[i] = c->key[piv];
		t->n++;
		t->val[i + 1U].t = rght;
		c->n = piv + !c->innerp;
		memset(c->key + c->n, -1, sizeof(c->key) - c->n * sizeof(*c->key));
		c->next = rght;
	}
	return t->n >= countof(t->key) - 1U;
}


btree_t
make_btree(void)
{
	btree_t r = calloc(1U, sizeof(*r));

	memset(r->key, -1, sizeof(r->key));
	return r;
}

void
free_btree(btree_t t)
{
	if (!t->innerp) {
		return;
	}
	/* descend and free */
	for (size_t i = 0U; i <= t->n; i++) {
		/* descend */
		free_btree(t->val[i].t);
	}
	free(t);
	return;
}

VAL_T
btree_add(btree_t t, KEY_T k, VAL_T v)
{
	VAL_T *vp;
	bool splitp;

	/* check if root has leaves */
	if (!t->innerp) {
		splitp = leaf_add(t, k, &vp);
	} else {
		splitp = twig_add(t, k, &vp);
	}
	/* do the maths */
	v = *vp += v;

	if (UNLIKELY(splitp)) {
		/* root got split, bollocks */
		btree_t left = make_btree();
		btree_t rght = make_btree();
		const size_t piv = countof(t->key) / 2U - 1U;

		/* T will become the new root so push stuff to LEFT ... */
		memcpy(left->key, t->key, (piv + 1U) * sizeof(*t->key));
		memcpy(left->val, t->val, (piv + 1U) * sizeof(*t->val));
		left->innerp = t->innerp;
		left->n = piv + !t->innerp;
		left->next = rght;
		/* ... and RGHT */
		memcpy(rght->key, t->key + piv + 1U, (piv + 0U) * sizeof(*t->key));
		memcpy(rght->val, t->val + piv + 1U, (piv + 1U) * sizeof(*t->val));
		rght->innerp = t->innerp;
		rght->n = piv;
		rght->next = NULL;
		/* and now massage T */
		t->key[0U] = t->key[piv];
		memset(t->key + 1U, -1, sizeof(t->key) - sizeof(*t->key));
		t->val[0U].t = left;
		t->val[1U].t = rght;
		t->n = 1U;
		t->innerp = 1U;
		t->next = NULL;
	}
	return v;
}

#if defined STANDALONE
# include <stdio.h>

static void
btree_prnt(btree_t t, size_t lvl)
{
	printf("%p %zu %d  L%zu", t, t->n, t->innerp, lvl);
	for (size_t i = 0U; i < t->n; i++) {
		printf("\t%f", (double)t->key[i]);
	}
	putchar('\n');
	if (t->innerp) {
		for (size_t i = 0U; i <= t->n; i++) {
			btree_prnt(t->val[i].t, lvl + 1U);
		}
	}
	return;
}

int
main(void)
{
	btree_t x;

	x = make_btree();
	btree_add(x, 1.23228df, 0.5dd);
	btree_add(x, 1.23226df, 0.5dd);
	btree_add(x, 1.23225df, 0.5dd);
	btree_add(x, 1.23230df, 0.5dd);
	btree_add(x, 1.23229df, 0.5dd);
	btree_add(x, 1.23232df, 0.5dd);
	btree_add(x, 1.23227df, 0.5dd);

	btree_prnt(x, 0U);
	free_btree(x);
	return 0;
}
#endif	/* STANDALONE */

/* btree.c ends here */
