/*
 * Copyright (c) 2017 The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __HIF_EXEC_H__
#define __HIF_EXEC_H__

#include <hif.h>


struct hif_exec_context;

struct hif_execution_ops {
	char *context_type;
	void (*schedule)(struct hif_exec_context *);
	void (*reschedule)(struct hif_exec_context *);
	void (*kill)(struct hif_exec_context *);
};

/**
 * hif_exec_context: only ever allocated as a subtype eg.
 *					hif_tasklet_exec_context
 *
 * @context: context for the handler function to use.
 * @context_name: a pointer to a const string for debugging.
 *		this should help whenever there could be ambiguity
 *		in what type of context the void* context points to
 * @irq: irq handle coresponding to hw block
 * @os_irq: irq handle for irq_afinity
 * @cpu: the cpu this context should be affined to
 * @work_complete: Function call called when leaving the execution context to
 *	determine if this context should reschedule or wait for an interrupt.
 *	This function may be used as a hook for post processing.
 *
 * @irq_disable: called before scheduling the context.
 * @irq_enable: called when the context leaves polling mode
 */
struct hif_exec_context {
	struct hif_execution_ops *sched_ops;
	struct hif_opaque_softc *hif;
	uint32_t numirq;
	uint32_t irq[HIF_MAX_GRP_IRQ];
	uint32_t os_irq[HIF_MAX_GRP_IRQ];
	uint32_t grp_id;
	const char *context_name;
	void *context;
	ext_intr_handler handler;

	bool (*work_complete)(struct hif_exec_context *, int work_done);
	void (*irq_enable)(struct hif_exec_context *);
	void (*irq_disable)(struct hif_exec_context *);

	uint8_t cpu;
	struct qca_napi_stat stats[NR_CPUS];
	bool inited;
	bool configured;
	bool irq_requested;
	bool irq_enabled;
	qdf_spinlock_t irq_lock;
};

/**
 * struct hif_tasklet_exec_context - exec_context for tasklets
 * @exec_ctx: inherited data type
 * @tasklet: tasklet structure for scheduling
 */
struct hif_tasklet_exec_context {
	struct hif_exec_context exec_ctx;
	struct tasklet_struct tasklet;
};

/**
 * struct hif_napi_exec_context - exec_context for tasklets
 * @exec_ctx: inherited data type
 * @netdev: dummy net device associated with the napi context
 * @napi: napi structure used in scheduling
 */
struct hif_napi_exec_context {
	struct hif_exec_context exec_ctx;
	struct net_device    netdev; /* dummy net_dev */
	struct napi_struct   napi;
};

static inline struct hif_napi_exec_context*
	hif_exec_get_napi(struct hif_exec_context *ctx)
{
	return (struct hif_napi_exec_context *) ctx;
}

static inline struct hif_tasklet_exec_context*
	hif_exec_get_tasklet(struct hif_exec_context *ctx)
{
	return (struct hif_tasklet_exec_context *) ctx;
}

struct hif_exec_context *hif_exec_create(enum hif_exec_type type);
void hif_exec_destroy(struct hif_exec_context *ctx);

int hif_grp_irq_configure(struct hif_softc *scn,
			  struct hif_exec_context *hif_exec);
irqreturn_t hif_ext_group_interrupt_handler(int irq, void *context);

struct hif_exec_context *hif_exec_get_ctx(struct hif_opaque_softc *hif,
					  uint8_t id);
void hif_exec_kill(struct hif_opaque_softc *scn);

#endif
