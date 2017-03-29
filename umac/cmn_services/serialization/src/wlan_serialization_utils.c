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
/**
 * DOC: wlan_serialization_utils.c
 * This file defines the utility helper functions for serialization component.
 */

#include "wlan_serialization_utils_i.h"
#include "wlan_serialization_main_i.h"
#include "wlan_serialization_api.h"
#include "wlan_objmgr_vdev_obj.h"
#include "wlan_objmgr_pdev_obj.h"
#include "qdf_mc_timer.h"
#include "wlan_utility.h"

QDF_STATUS
wlan_serialization_put_back_to_global_list(qdf_list_t *queue,
		struct wlan_serialization_pdev_priv_obj *ser_pdev_obj,
		struct wlan_serialization_command_list *cmd_list)
{
	QDF_STATUS status;

	if (!queue || !ser_pdev_obj || !cmd_list) {
		serialization_err("input parameters are invalid");
		return QDF_STATUS_E_FAILURE;
	}
	status = qdf_list_remove_node(queue, &cmd_list->node);
	if (QDF_STATUS_SUCCESS != status) {
		serialization_err("can't remove cmd from queue");
		/* assert to catch any leaks */
		QDF_ASSERT(0);
		return status;
	}
	serialization_info("cmd_id-%d, cmd_type-%d", cmd_list->cmd.cmd_id,
				cmd_list->cmd.cmd_type);
	qdf_mem_zero(&cmd_list->cmd, sizeof(struct wlan_serialization_command));
	status = qdf_list_insert_back(&ser_pdev_obj->global_cmd_pool_list,
					&cmd_list->node);
	if (QDF_STATUS_SUCCESS != status) {
		serialization_err("can't put command back to global pool");
		QDF_ASSERT(0);
		return status;
	}

	return status;
}

struct wlan_objmgr_pdev*
wlan_serialization_get_pdev_from_cmd(struct wlan_serialization_command *cmd)
{
	struct wlan_objmgr_pdev *pdev = NULL;

	if (!cmd) {
		serialization_err("invalid cmd");
		return pdev;
	}
	pdev = wlan_vdev_get_pdev(cmd->vdev);

	return pdev;
}

/**
 * wlan_serialization_get_cmd_from_queue() - to extract command from given queue
 * @queue: pointer to queue
 * @nnode: next node to extract
 *
 * This API will try to extract node from queue which is next to prev node. If
 * no previous node is given then take out the front node of the queue.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_serialization_get_cmd_from_queue(qdf_list_t *queue,
						 qdf_list_node_t **nnode)
{
	QDF_STATUS status;
	qdf_list_node_t *pnode;

	pnode = *nnode;
	if (!pnode)
		status = qdf_list_peek_front(queue, nnode);
	else
		status = qdf_list_peek_next(queue, pnode, nnode);

	if (status != QDF_STATUS_SUCCESS) {
		serialization_err("can't get next node from queue");
		return status;
	}
	return QDF_STATUS_SUCCESS;
}

/**
 * wlan_serialization_timer_destroy() - destroys the timer
 * @ser_timer: pointer to particular timer
 *
 * This API destroys the memory allocated by timer and assigns cmd member of
 * that timer structure to NULL
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS wlan_serialization_timer_destroy(
		struct wlan_serialization_timer *ser_timer)
{
	QDF_STATUS status = QDF_STATUS_E_FAILURE;

	if (!ser_timer || !ser_timer->cmd) {
		serialization_debug("Invalid ser_timer");
		return status;
	}
	status = qdf_mc_timer_destroy(&ser_timer->timer);
	if (!QDF_IS_STATUS_SUCCESS(status)) {
		serialization_err("Failed to destroy timer for cmd_id[%d]",
				ser_timer->cmd->cmd_id);
		QDF_ASSERT(0);
		return status;
	}
	ser_timer->cmd = NULL;

	return status;
}

/**
 * wlan_serialization_generic_timer_callback() - timer callback when timer fire
 * @arg: argument that timer passes to this callback
 *
 * All the timers in serialization module calls this callback when they fire,
 * and this API in turn calls command specific timeout callback and remove
 * timed-out command from active queue and move any pending command to active
 * queue of same cmd_type.
 *
 * Return: none
 */
static void wlan_serialization_generic_timer_callback(void *arg)
{
	struct wlan_serialization_timer *timer = arg;
	struct wlan_serialization_command *cmd = timer->cmd;

	if (!cmd) {
		serialization_err("command not found");
		QDF_ASSERT(0);
		return;
	}

	serialization_err("active command timeout for cmd_id[%d]", cmd->cmd_id);
	if (cmd->cmd_cb) {
		cmd->cmd_cb(cmd, WLAN_SER_CB_ACTIVE_CMD_TIMEOUT);
		cmd->cmd_cb(cmd, WLAN_SER_CB_RELEASE_MEM_CMD);
	}

	serialization_err("active command timeout for cmd_id[%d]", cmd->cmd_id);
	if (cmd->cmd_type >= WLAN_SER_CMD_NONSCAN)
		QDF_BUG(0);
	/*
	 * dequeue cmd API will cleanup and destroy the timer. If it fails to
	 * dequeue command then we have to destroy the timer.
	 */
	if (WLAN_SER_CMD_NOT_FOUND == wlan_serialization_dequeue_cmd(cmd, true))
		wlan_serialization_timer_destroy(timer);
}

/**
 * wlan_serialization_stop_timer() - to stop particular timer
 * @ser_timer: pointer to serialization timer
 *
 * This API stops the particular timer
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS
wlan_serialization_stop_timer(struct wlan_serialization_timer *ser_timer)
{
	QDF_TIMER_STATE state;
	QDF_STATUS status;

	state = qdf_mc_timer_get_current_state(&ser_timer->timer);
	if (QDF_TIMER_STATE_RUNNING != state ||
			QDF_TIMER_STATE_STARTING != state) {
		serialization_debug("nothing to stop");
		wlan_serialization_timer_destroy(ser_timer);
		return QDF_STATUS_SUCCESS;
	}
	status = qdf_mc_timer_stop(&ser_timer->timer);
	if (!QDF_IS_STATUS_SUCCESS(status)) {
		serialization_err("Failed to stop timer");
		/* to catch the bug */
		QDF_ASSERT(0);
		return status;
	}
	wlan_serialization_timer_destroy(ser_timer);
	status = QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_serialization_cleanup_all_timers(
			struct wlan_serialization_psoc_priv_obj *psoc_ser_obj)
{
	struct wlan_serialization_timer *ser_timer;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	uint32_t i = 0;

	if (!psoc_ser_obj) {
		serialization_err("Invalid psoc_ser_obj");
		return QDF_STATUS_E_FAILURE;
	}

	for (i = 0; psoc_ser_obj->max_active_cmds > i; i++) {
		ser_timer = &psoc_ser_obj->timers[i];
		if (!ser_timer->cmd)
			continue;
		status = wlan_serialization_stop_timer(ser_timer);
		if (QDF_STATUS_SUCCESS != status) {
			/* lets not break the loop but report error */
			serialization_err("some error in stopping timer");
		}
	}

	return status;
}

QDF_STATUS
wlan_serialization_find_and_stop_timer(struct wlan_objmgr_psoc *psoc,
				       struct wlan_serialization_command *cmd)
{
	struct wlan_serialization_psoc_priv_obj *psoc_ser_obj;
	struct wlan_serialization_timer *ser_timer;
	QDF_STATUS status = QDF_STATUS_E_FAILURE;
	int i = 0;

	if (!psoc || !cmd) {
		serialization_err("invalid param");
		return status;
	}

	if ((cmd->cmd_timeout_duration == 0) &&
		(wlan_is_emulation_platform(wlan_psoc_get_nif_phy_version(psoc)
	))) {
		serialization_err("[SCAN-EMULATION]: Not performing timer functions\n");
		return QDF_STATUS_SUCCESS;
	}

	psoc_ser_obj = wlan_serialization_get_psoc_priv_obj(psoc);
	/*
	 * Here cmd_id and cmd_type are used to locate the timer being
	 * associated with command. For scan command, cmd_id is expected to
	 * be unique and For non-scan command, there should be only one active
	 * command per pdev
	 */
	for (i = 0; psoc_ser_obj->max_active_cmds > i; i++) {
		ser_timer = &psoc_ser_obj->timers[i];
		if (!(ser_timer->cmd) ||
				(ser_timer->cmd->cmd_id != cmd->cmd_id) ||
				(ser_timer->cmd->cmd_type != cmd->cmd_type) ||
				(ser_timer->cmd->vdev != cmd->vdev))
			continue;
		status = wlan_serialization_stop_timer(ser_timer);
		if (QDF_STATUS_SUCCESS != status) {
			serialization_err("Failed to stop timer for cmd_id[%d]",
					cmd->cmd_id);
		}
		break;
	}

	if (QDF_STATUS_SUCCESS != status) {
		serialization_err("can't find timer for cmd_type[%d]",
				cmd->cmd_type);
	}
	return status;
}

QDF_STATUS
wlan_serialization_find_and_start_timer(struct wlan_objmgr_psoc *psoc,
					struct wlan_serialization_command *cmd)
{
	QDF_STATUS status = QDF_STATUS_E_FAILURE;
	struct wlan_serialization_psoc_priv_obj *psoc_ser_obj;
	struct wlan_serialization_timer *ser_timer;
	int i = 0;

	if (!psoc || !cmd) {
		serialization_err("invalid param");
		return status;
	}

	if ((cmd->cmd_timeout_duration == 0) &&
		(wlan_is_emulation_platform(wlan_psoc_get_nif_phy_version(psoc)
	))) {
		serialization_err("[SCAN-EMULATION]: Not performing timer functions\n");
		return QDF_STATUS_SUCCESS;
	}


	psoc_ser_obj = wlan_serialization_get_psoc_priv_obj(psoc);
	for (i = 0; psoc_ser_obj->max_active_cmds > i; i++) {
		/* Keep trying timer */
		ser_timer = &psoc_ser_obj->timers[i];
		if (ser_timer->cmd)
			continue;
		/* Remember timer is pointing to command */
		ser_timer->cmd = cmd;
		if (!QDF_IS_STATUS_SUCCESS(qdf_mc_timer_init(&ser_timer->timer,
				QDF_TIMER_TYPE_SW,
				wlan_serialization_generic_timer_callback,
				ser_timer))) {
			serialization_err("Failed to init timer cmdid [%d]",
					cmd->cmd_id);
			QDF_ASSERT(0);
			continue;
		}
		if (!QDF_IS_STATUS_SUCCESS(qdf_mc_timer_start(&ser_timer->timer,
						cmd->cmd_timeout_duration))) {
			serialization_err("Failed to start timer cmdid [%d]",
					cmd->cmd_id);
			wlan_serialization_timer_destroy(ser_timer);
			QDF_ASSERT(0);
			continue;
		}
		status = QDF_STATUS_SUCCESS;
		break;
	}

	return status;
}

/**
 * wlan_serialization_active_scan_cmd_count_handler() - count active scan cmds
 * @psoc: pointer to soc strucutre
 * @obj : pointer to pdev object
 * @arg: pointer to argument
 *
 * This API will be called while iterating each pdev object and it will count
 * number of scan commands present in that pdev object's active queue. count
 * will be updated in *arg
 *
 * Return: none
 */
static void
wlan_serialization_active_scan_cmd_count_handler(struct wlan_objmgr_psoc *psoc,
						 void *obj, void *arg)
{
	struct wlan_objmgr_pdev *pdev = obj;
	struct wlan_serialization_pdev_priv_obj *ser_pdev_obj;
	uint32_t *count = arg;

	if (!pdev) {
		serialization_err("invalid pdev");
		return;
	}

	ser_pdev_obj = wlan_objmgr_pdev_get_comp_private_obj(
			pdev, WLAN_UMAC_COMP_SERIALIZATION);
	*count += qdf_list_size(&ser_pdev_obj->active_scan_list);
}

/**
 * wlan_serialization_is_active_scan_cmd_allowed() - find if scan cmd allowed
 * @pdev: pointer to pdev object
 *
 * This API will be called to find out if active scan cmd is allowed. It has
 * to iterate through all pdev to find out total number of active scan cmds.
 * If total number of active scan cmds reach to allowed threshold then don't
 * allow more scan cmd.
 *
 * Return: true or false
 */
static bool
wlan_serialization_is_active_scan_cmd_allowed(struct wlan_objmgr_pdev *pdev)
{
	uint32_t count = 0;
	struct wlan_objmgr_psoc *psoc;

	if (!pdev) {
		serialization_err("invalid pdev");
		return false;
	}

	psoc = wlan_pdev_get_psoc(pdev);

	if (!psoc) {
		serialization_err("invalid psoc");
		return false;
	}

	wlan_objmgr_iterate_obj_list(psoc, WLAN_PDEV_OP,
			wlan_serialization_active_scan_cmd_count_handler,
			&count, 1, WLAN_SERIALIZATION_ID);
	if (count < WLAN_SERIALIZATION_MAX_ACTIVE_SCAN_CMDS) {
		serialization_notice("count is [%d]", count);
		return true;
	}

	return false;
}

/**
 * wlan_serialization_is_active_nonscan_cmd_allowed() - find if cmd allowed
 * @pdev: pointer to pdev object
 *
 * This API will be called to find out if non scan cmd is allowed.
 *
 * Return: true or false
 */
static bool
wlan_serialization_is_active_nonscan_cmd_allowed(struct wlan_objmgr_pdev *pdev)
{
	struct wlan_serialization_pdev_priv_obj *ser_pdev_obj;

	if (!pdev) {
		serialization_err("invalid pdev");
		return false;
	}

	ser_pdev_obj = wlan_objmgr_pdev_get_comp_private_obj(
			pdev, WLAN_UMAC_COMP_SERIALIZATION);

	if (!ser_pdev_obj) {
		serialization_err("invalid ser_pdev_obj");
		return false;
	}

	if (qdf_list_empty(&ser_pdev_obj->active_list))
		return true;

	return false;
}

bool
wlan_serialization_is_active_cmd_allowed(struct wlan_serialization_command *cmd)
{
	struct wlan_objmgr_pdev *pdev;

	pdev = wlan_serialization_get_pdev_from_cmd(cmd);
	if (!pdev) {
		serialization_err("NULL pdev");
		return false;
	}

	if (cmd->cmd_type < WLAN_SER_CMD_NONSCAN)
		return wlan_serialization_is_active_scan_cmd_allowed(pdev);
	else
		return wlan_serialization_is_active_nonscan_cmd_allowed(pdev);
}

QDF_STATUS wlan_serialization_validate_cmdtype(
		 enum wlan_serialization_cmd_type cmd_type)
{
	serialization_info("validate cmd_type:%d", cmd_type);

	if (cmd_type < 0 || cmd_type >= WLAN_SER_CMD_MAX) {
		serialization_err("Invalid cmd or comp passed");
		return QDF_STATUS_E_INVAL;
	}

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_serialization_validate_cmd(
		 enum wlan_umac_comp_id comp_id,
		 enum wlan_serialization_cmd_type cmd_type)
{
	serialization_info("validate cmd_type:%d, comp_id:%d",
			cmd_type, comp_id);
	if (cmd_type < 0 || comp_id < 0 ||
			cmd_type >= WLAN_SER_CMD_MAX ||
			comp_id >= WLAN_UMAC_COMP_ID_MAX) {
		serialization_err("Invalid cmd or comp passed");
		return QDF_STATUS_E_INVAL;
	}

	return QDF_STATUS_SUCCESS;
}

void wlan_serialization_release_list_cmds(
		struct wlan_serialization_pdev_priv_obj *ser_pdev_obj,
		qdf_list_t *list)
{
	qdf_list_node_t *node = NULL;

	while (!qdf_list_empty(list)) {
		qdf_list_remove_front(list, &node);
		qdf_list_insert_back(&ser_pdev_obj->global_cmd_pool_list, node);
	}

	return;
}

void wlan_serialization_destroy_list(
		struct wlan_serialization_pdev_priv_obj *ser_pdev_obj,
		qdf_list_t *list)
{
	wlan_serialization_release_list_cmds(ser_pdev_obj, list);
	qdf_list_destroy(list);
}

struct wlan_serialization_psoc_priv_obj *wlan_serialization_get_psoc_priv_obj(
		struct wlan_objmgr_psoc *psoc)
{
	struct wlan_serialization_psoc_priv_obj *ser_soc_obj;
	wlan_psoc_obj_lock(psoc);
	ser_soc_obj = wlan_objmgr_psoc_get_comp_private_obj(psoc,
					WLAN_UMAC_COMP_SERIALIZATION);
	wlan_psoc_obj_unlock(psoc);

	return ser_soc_obj;
}

struct wlan_serialization_pdev_priv_obj *wlan_serialization_get_pdev_priv_obj(
		struct wlan_objmgr_pdev *pdev)
{
	struct wlan_serialization_pdev_priv_obj *obj;
	wlan_pdev_obj_lock(pdev);
	obj = wlan_objmgr_pdev_get_comp_private_obj(pdev,
					WLAN_UMAC_COMP_SERIALIZATION);
	wlan_pdev_obj_unlock(pdev);

	return obj;
}

struct wlan_serialization_psoc_priv_obj *
wlan_serialization_get_obj(struct wlan_serialization_command *cmd)
{
	struct wlan_serialization_psoc_priv_obj *ser_soc_obj;
	struct wlan_objmgr_psoc *psoc;

	psoc = wlan_vdev_get_psoc(cmd->vdev);
	ser_soc_obj = wlan_serialization_get_psoc_priv_obj(psoc);

	return ser_soc_obj;
}

bool wlan_serialization_is_cmd_in_vdev_list(struct wlan_objmgr_vdev *vdev,
					     qdf_list_t *queue)
{
	uint32_t queuelen;
	qdf_list_node_t *nnode = NULL;
	struct wlan_serialization_command_list *cmd_list = NULL;
	QDF_STATUS status;

	queuelen = qdf_list_size(queue);
	if (!queuelen) {
		serialization_err("invalid queue length");
		return false;
	}

	while (queuelen--) {
		status = wlan_serialization_get_cmd_from_queue(queue, &nnode);
		if (status != QDF_STATUS_SUCCESS)
			break;
		cmd_list = qdf_container_of(nnode,
				struct wlan_serialization_command_list, node);
		if (cmd_list->cmd.vdev == vdev)
			return true;
	};

	return false;
}

bool wlan_serialization_is_cmd_in_pdev_list(struct wlan_objmgr_pdev *pdev,
					     qdf_list_t *queue)
{
	uint32_t queuelen;
	qdf_list_node_t *nnode = NULL;
	struct wlan_objmgr_pdev *node_pdev = NULL;
	struct wlan_serialization_command_list *cmd_list = NULL;
	QDF_STATUS status;

	queuelen = qdf_list_size(queue);
	if (!queuelen) {
		serialization_err("invalid queue length");
		return false;
	}

	while (queuelen--) {
		status = wlan_serialization_get_cmd_from_queue(queue, &nnode);
		if (status != QDF_STATUS_SUCCESS)
			break;
		cmd_list = qdf_container_of(nnode,
				struct wlan_serialization_command_list, node);
		node_pdev = wlan_vdev_get_pdev(cmd_list->cmd.vdev);
		if (node_pdev == pdev)
			return true;
	}

	return false;
}

enum wlan_serialization_cmd_status
wlan_serialization_is_cmd_in_active_pending(bool cmd_in_active,
					    bool cmd_in_pending)
{
	if (cmd_in_active && cmd_in_pending)
		return WLAN_SER_CMDS_IN_ALL_LISTS;
	else if (cmd_in_active)
		return WLAN_SER_CMD_IN_ACTIVE_LIST;
	else if (cmd_in_pending)
		return WLAN_SER_CMD_IN_PENDING_LIST;
	else
		return WLAN_SER_CMD_NOT_FOUND;
}

static bool wlan_serialization_is_cmd_present_in_given_queue(qdf_list_t *queue,
				struct wlan_serialization_command *cmd)
{
	uint32_t qsize;
	QDF_STATUS status;
	struct wlan_serialization_command_list *cmd_list = NULL;
	qdf_list_node_t *nnode = NULL, *pnode = NULL;
	bool found = false;

	qsize = qdf_list_size(queue);
	while (qsize--) {
		if (!cmd_list)
			status = qdf_list_peek_front(queue, &nnode);
		else
			status = qdf_list_peek_next(queue, pnode,
					&nnode);

		if (status != QDF_STATUS_SUCCESS)
			break;

		pnode = nnode;
		cmd_list = qdf_container_of(nnode,
				struct wlan_serialization_command_list, node);
		if ((cmd_list->cmd.cmd_id == cmd->cmd_id) &&
				(cmd_list->cmd.cmd_type == cmd->cmd_type) &&
				(cmd_list->cmd.vdev == cmd->vdev)) {
			found = true;
			break;
		}
		nnode = NULL;
	}
	return found;
}

bool wlan_serialization_is_cmd_present_queue(
			struct wlan_serialization_command *cmd,
			uint8_t is_active_queue)
{
	qdf_list_t *queue;
	struct wlan_objmgr_pdev *pdev;
	struct wlan_serialization_pdev_priv_obj *ser_pdev_obj;

	if (!cmd) {
		serialization_err("invalid params");
		return false;
	}
	pdev = wlan_serialization_get_pdev_from_cmd(cmd);
	if (!pdev) {
		serialization_err("invalid pdev");
		return false;
	}
	ser_pdev_obj = wlan_objmgr_pdev_get_comp_private_obj(pdev,
					WLAN_UMAC_COMP_SERIALIZATION);
	if (!ser_pdev_obj) {
		serialization_err("invalid ser_pdev_obj");
		return false;
	}
	if (!is_active_queue) {
		if (cmd->cmd_type < WLAN_SER_CMD_NONSCAN)
			queue = &ser_pdev_obj->pending_scan_list;
		else
			queue = &ser_pdev_obj->pending_list;
	} else {
		if (cmd->cmd_type < WLAN_SER_CMD_NONSCAN)
			queue = &ser_pdev_obj->active_scan_list;
		else
			queue = &ser_pdev_obj->active_list;
	}

	return wlan_serialization_is_cmd_present_in_given_queue(queue, cmd);
}