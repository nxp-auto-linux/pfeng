/* =========================================================================
 *  Copyright 2019-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_FCI
 * @{
 *
 * @file		fci_core_linux.c
 * @brief		The Linux-specific FCI core component. Full description can be
 * 				found within the fci_core.h.
 *
 */

#include <linux/module.h>
#include "pfe_cfg.h"
#include "oal.h"
#include "fci.h"
#include "fci_internal.h"
#include "fci_core.h"

#include <linux/sched.h>
#include <linux/netlink.h>
#include <linux/moduleparam.h>
#include <net/sock.h>
#include <net/net_namespace.h>

#define NETLINK_TYPE_CUSTOM_FCI 17

#include <linux/rtnetlink.h>

/* The command line parameter is defined in pfeng-drv.c or pfeng-slave-drv.c */
extern bool disable_netlink;

static errno_t fci_netlink_send(uint32_t port_id, fci_msg_t *msg);
static errno_t fci_handle_msg(fci_msg_t *msg, fci_msg_t *rep_msg, uint32_t port_id);

/*
 *	LINUX-specific FCI client representation type (fci_core_client_t)
 */
struct __fci_core_client_tag
{
	uint32_t back_port_id;		/*	Client's back channel connection ID */
	uint32_t cmd_port_id;		/*	Client's command channel connection ID */
	bool_t connected;
};

/*
 *	LINUX-specific FCI core representation type (fci_core_t)
 */
struct __fci_core_tag
{
	fci_t *context;				/*	Associated FCI instance */
	struct sock *handle;
	struct mutex lock;
	struct mutex clients_lock;
	fci_core_client_t clients[FCI_CFG_MAX_CLIENTS];
};

static void fci_recv_msg(struct sk_buff *skb);
static void fci_client_unbind(struct net *net, int group);

struct netlink_kernel_cfg fci_netlink_cfg = {
	.input = fci_recv_msg,
	.unbind = fci_client_unbind,
};

#define GET_FCI_CORE()	__context.core
#define PUT_FCI_CORE(c)	__context.core = c

/*
	Get client by connection ID
*/
static fci_core_client_t *fci_core_get_client(fci_core_t *core, uint32_t port_id)
{
	int ii;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == core))
	{
		NXP_LOG_RAW_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/* The core is secured by lock in the caller */
	for (ii=0U; ii<FCI_CFG_MAX_CLIENTS; ii++)
	{
		if ((core->clients[ii].connected == TRUE) && (port_id == core->clients[ii].cmd_port_id))
		{
			return &core->clients[ii];
		}
	}

	return NULL;
}

/*
	Get count of registered clients.
*/
static uint8_t fci_core_get_count_of_clients(fci_core_t *core)
{
	int ii;
	uint8_t count = 0;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == core))
	{
		NXP_LOG_RAW_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/* The core is secured by lock in the caller */
	for (ii=0U; ii<FCI_CFG_MAX_CLIENTS; ii++)
	{
		if (core->clients[ii].connected == TRUE)
		{
			count++;
		}
	}

	return count;
}

/*
 	 Create FCI core instance
*/
errno_t fci_core_init(const char_t *const id)
{
	fci_core_t *core;
	uint32_t ii;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == id))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if(NULL != GET_FCI_CORE())
	{
		NXP_LOG_ERROR("FCI_CORE has already been initialized\n");
		return EINVAL;
	}

	core = oal_mm_malloc(sizeof(fci_core_t));
	if (NULL == core)
	{
		NXP_LOG_ERROR("Core not allocated\n");
		return ENOMEM;
	}

	memset(core, 0, sizeof(fci_core_t));
	mutex_init(&core->lock);
	mutex_init(&core->clients_lock);

	PUT_FCI_CORE(core);

	/*	Initialize event listeners */
	for (ii=0; ii<FCI_CFG_MAX_CLIENTS; ii++)
	{
		core->clients[ii].connected = FALSE;
	}

	/*  Initialize netlink */
	if (false == disable_netlink)
	{
		NXP_LOG_DEBUG("Do netlink initialization\n");
		core->handle = netlink_kernel_create(&init_net, NETLINK_TYPE_CUSTOM_FCI, &fci_netlink_cfg);
		if (NULL == core->handle)
		{
			NXP_LOG_ERROR("Error creating netlink\n");
			goto free_and_fail;
		}
	}
	else
	{
		NXP_LOG_DEBUG("Skip netlink initialization\n");
	}

	return EOK;

free_and_fail:
	fci_core_fini();
	return EINVAL;
}

/*
 	 Destroy FCI core
*/
void fci_core_fini(void)
{
	uint32_t ii;
	fci_msg_t msg = {0};

	msg.type = FCI_MSG_CMD;
	msg.msg_cmd.code = FPP_CMD_ENDPOINT_SHUTDOWN;

	if (NULL != GET_FCI_CORE())
	{
		if(mutex_lock_interruptible(&GET_FCI_CORE()->lock))
		{
			NXP_LOG_WARNING("FCI lock failed\n");
			return;
		}
		if(mutex_lock_interruptible(&GET_FCI_CORE()->clients_lock))
		{
			NXP_LOG_RAW_WARNING("FCI clients lock failed\n");
			return;
		}
		for(ii = 0; ii < FCI_CFG_MAX_CLIENTS; ii++)
		{
			
			if (TRUE == GET_FCI_CORE()->clients[ii].connected)
			{
				if (EOK != fci_netlink_send(GET_FCI_CORE()->clients[ii].back_port_id, &msg))
				{
					NXP_LOG_RAW_ERROR("fci_netlink_send failed\n");
				}

				GET_FCI_CORE()->clients[ii].connected = FALSE;
			}
		}

		if (NULL != GET_FCI_CORE()->handle)
		{
			sock_release(GET_FCI_CORE()->handle->sk_socket);
			GET_FCI_CORE()->handle = NULL;
		}

		mutex_unlock(&GET_FCI_CORE()->clients_lock);
		mutex_destroy(&GET_FCI_CORE()->clients_lock);

		mutex_unlock(&GET_FCI_CORE()->lock);
		mutex_destroy(&GET_FCI_CORE()->lock);

		oal_mm_free(GET_FCI_CORE());

		PUT_FCI_CORE(NULL);
	}
}

static void fci_client_unbind(struct net *net, int group)
{

	/* TODO: invoked when client died */

	NXP_LOG_INFO("FCI: client died!\n");
}

static void fci_recv_msg(struct sk_buff *skb)
{
	struct nlmsghdr *nlh;
	int port_id;
	fci_msg_t *msg, rep_msg;

	if (unlikely(NULL == skb))
	{
		NXP_LOG_WARNING("no skb received\n");
		return;
	}

	if (unlikely(NULL == GET_FCI_CORE()))
	{
		NXP_LOG_ERROR("FCI context is missing\n");
		return;
	}

	if (unlikely(FALSE == __context.fci_initialized))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		return;
	}

	nlh = (struct nlmsghdr *)skb->data;
	if (unlikely(!nlh))
	{
		NXP_LOG_WARNING("Message error: no netlink data\n");
		return;
	}

	port_id = nlh->nlmsg_pid; /*pid of sending process */
	msg = (fci_msg_t *)nlmsg_data(nlh);
	if (unlikely(!msg))
	{
		NXP_LOG_WARNING("Message error: payload is NULL\n");
		return;
	}

	/* Parse the received message and send the reply back */
	fci_handle_msg(msg, &rep_msg, port_id);

	/* If the message is not from kernel send the reply */
	if(0 != port_id){
		if(mutex_lock_interruptible(&GET_FCI_CORE()->lock))
		{
			NXP_LOG_ERROR("FCI lock failed\n");
			return;
		}

		if (EOK != fci_netlink_send(port_id,&rep_msg))
		{
			NXP_LOG_WARNING("fci_netlink_send failed\n");
		}
		mutex_unlock(&GET_FCI_CORE()->lock);
	}

	return;
}

/*
	Handle the received message
*/
static errno_t fci_handle_msg(fci_msg_t *msg, fci_msg_t *rep_msg, uint32_t port_id)
{
	int ii;
	errno_t ret = EOK;
	fci_core_client_t *client;
	fci_core_t *core = GET_FCI_CORE();
	bool first_client_connected = FALSE;
	
	NXP_LOG_DEBUG("FCI received msg of type %u from port_id 0x%x\n", msg->type, port_id);
	
	if(mutex_lock_interruptible(&core->lock))
	{
		NXP_LOG_ERROR("FCI lock failed\n");
		return EAGAIN;
	}
	
	/*	Message */
	switch (msg->type)
	{
		case FCI_MSG_CLIENT_REGISTER:
		{
			if(mutex_lock_interruptible(&core->clients_lock))
			{
				NXP_LOG_RAW_ERROR("FCI clients lock failed\n");
				return EAGAIN;
			}

			/*	Add FCI client */
			for (ii=0; ii<FCI_CFG_MAX_CLIENTS; ii++)
			{
				/* Check if the connection is not already registered */	
				if (TRUE == core->clients[ii].connected)
				{
					if(msg->msg_client_register.port_id == core->clients[ii].back_port_id)
					{
						NXP_LOG_RAW_ERROR("Client already registered\n");
						ret = EPERM;
						break;
					}
				}
				else
				{
					core->clients[ii].connected = TRUE;
					core->clients[ii].cmd_port_id = port_id;
					core->clients[ii].back_port_id = msg->msg_client_register.port_id;
					break;
				}
			}

			if (EOK == ret)
			{
				if (FCI_CFG_MAX_CLIENTS == ii)
				{
					NXP_LOG_RAW_ERROR("Can't register new event listener, storage is full\n");
					ret = ENOSPC;
				}
				else
				{
					NXP_LOG_RAW_INFO("Listener with port id cmd 0x%x, back 0x%x registered to pos %d\n", core->clients[ii].cmd_port_id,core->clients[ii].back_port_id,ii);
					ret = EOK;
				}
			}

			if (EOK == ret)
			{
				__context.is_some_client = TRUE;
				if (1U == fci_core_get_count_of_clients(core))
				{
					first_client_connected = TRUE;
				}
			}

			mutex_unlock(&GET_FCI_CORE()->clients_lock);

			/* Health Monitor FCI events ; this must be checked/called after unlock, because FCI event sending manipulates the clients_lock too. */
			if (TRUE == first_client_connected)
			{
				fci_hm_send_events();
			}

			break;
		}

		case FCI_MSG_CLIENT_UNREGISTER:
		{
			if(mutex_lock_interruptible(&core->clients_lock))
			{
				NXP_LOG_RAW_ERROR("FCI clients lock failed\n");
				return EAGAIN;
			}

			/*	Remove FCI client */
			for (ii=0; ii<FCI_CFG_MAX_CLIENTS; ii++)
			{
				if (TRUE == core->clients[ii].connected)
				{
					if(core->clients[ii].cmd_port_id == port_id)
					{
						core->clients[ii].connected = FALSE;
						core->clients[ii].cmd_port_id = 0;
						core->clients[ii].back_port_id = 0;
						break;
					}
				}
			}

			if (FCI_CFG_MAX_CLIENTS == ii)
			{
				NXP_LOG_RAW_ERROR("Requested connection ID not found\n");
				ret = ENOENT;
			}
			else
			{
				NXP_LOG_RAW_INFO("Listener with port id cmd 0x%x unregistered from pos %d\n",port_id, ii);
				ret = EOK;
			}

			/* Health Monitor FCI events */
			if (EOK == ret)
			{
				if (0U == fci_core_get_count_of_clients(core))
				{
					__context.is_some_client = FALSE;
				}
			}

			mutex_unlock(&GET_FCI_CORE()->clients_lock);
			break;
		}

		case FCI_MSG_CMD:
		{
			/*	Get and bind client instance with the message */
			/*	We need to find the client based on cmd port id to be able to pass the client to the lower layers */
			if(mutex_lock_interruptible(&core->clients_lock))
			{
				NXP_LOG_RAW_ERROR("FCI clients lock failed\n");
				return EAGAIN;
			}
			client = fci_core_get_client(core, port_id);
			msg->client = (void *)client;
			mutex_unlock(&GET_FCI_CORE()->clients_lock);

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
			/*	Local sender identification */
			msg->msg_cmd.sender = OAL_PFE_CFG_MASTER_IF;
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
			memset(rep_msg, 0, sizeof(fci_msg_t));

			/*	Here we call the OS-independent FCI message processor */
			ret = fci_process_ipc_message(msg, rep_msg);

			break;
		}

		default:
		{
			NXP_LOG_WARNING("Unknown FCI message: %d\n", msg->type);
			ret = EINVAL;

			break;
		}
	} /* switch */

	mutex_unlock(&core->lock);

	rep_msg->ret_code = ret;

	return ret;
}

/*
	Send message via netlink, function does not lock the mutex, this has to be done outside
*/
static errno_t fci_netlink_send(uint32_t port_id, fci_msg_t *msg)
{
	struct nlmsghdr *nlh;
	struct sk_buff *skb_out;
	uint32_t msg_size = sizeof(fci_msg_t);
	int32_t res;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == msg))
	{
		NXP_LOG_RAW_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	skb_out = nlmsg_new(msg_size, 0);
	if (!skb_out) {
		NXP_LOG_RAW_ERROR("Failed to allocate new skb\n");
		return ENOMEM;
	}

	NXP_LOG_RAW_DEBUG("FCI send netlink message to port_id 0x%x\n",port_id);
	nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, GFP_ATOMIC);
	nlh->nlmsg_flags = NLM_F_REQUEST;
	NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
	memcpy(nlmsg_data(nlh), msg, msg_size);
	
	res = nlmsg_unicast(GET_FCI_CORE()->handle, skb_out, port_id);

	if (res < 0) {
		NXP_LOG_RAW_ERROR("Error while sending: %d\n", res);
	}
	else
	{
		res = EOK;
	}

	return res;
}

/*
	Send message to the FCI core
*/
errno_t fci_core_send(fci_msg_t *msg, fci_msg_t *rep)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == GET_FCI_CORE()))
	{
		NXP_LOG_ERROR("FCI context is missing\n");
		return EINVAL;
	}

	if (unlikely(FALSE == __context.fci_initialized))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		return EPERM;
	}

	if (unlikely(NULL == msg) || unlikely(NULL == rep))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return fci_handle_msg(msg, rep, 0);
}

/*
	Send message to FCI client
*/
errno_t fci_core_client_send(fci_core_client_t *client, fci_msg_t *msg, fci_msg_t *rep)
{
	errno_t ret = ENOENT;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(FALSE == __context.fci_initialized))
	{
		NXP_LOG_RAW_ERROR("Context not initialized\n");
		return EPERM;
	}

	if (unlikely(NULL == msg) || unlikely(NULL == rep))
	{
		NXP_LOG_RAW_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	this function could be called from a callback even after the FCI module was destroyed */
	if (NULL == GET_FCI_CORE())
	{
		NXP_LOG_RAW_WARNING("Cannot send FCI message (FCI core is NULL)\n");
		return EINVAL;
	}

	if(mutex_lock_interruptible(&GET_FCI_CORE()->clients_lock))
	{
		NXP_LOG_RAW_WARNING("FCI clients lock failed\n");
		return EAGAIN;
	}
	
	if(client->connected == TRUE)
	{
		if (EOK != (ret = fci_netlink_send(client->back_port_id, msg)))
		{
			NXP_LOG_RAW_ERROR("fci_netlink_send() failed: %d\n", ret);
		}
	}

	(void)mutex_unlock(&GET_FCI_CORE()->clients_lock);
	return ret;
}

/*
	Send message to all registered FCI clients
*/
errno_t fci_core_client_send_broadcast(fci_msg_t *msg, fci_msg_t *rep)
{
	fci_core_t *core = GET_FCI_CORE();
	uint32_t ii;
	errno_t ret = ENOENT;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(FALSE == __context.fci_initialized))
	{
		NXP_LOG_RAW_ERROR("Context not initialized\n");
		return EPERM;
	}

	if (unlikely(NULL == msg))
	{
		NXP_LOG_RAW_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	this function could be called from a callback even after the FCI module was destroyed */
	if (NULL == GET_FCI_CORE())
	{
		NXP_LOG_RAW_WARNING("Cannot send FCI message (FCI core is NULL)\n");
		return EINVAL;
	}

	if(mutex_lock_interruptible(&core->clients_lock))
	{
		NXP_LOG_RAW_WARNING("FCI clients lock failed\n");
		return EAGAIN;
	}

	for (ii=0; ii<FCI_CFG_MAX_CLIENTS; ii++)
	{
		if (TRUE == core->clients[ii].connected)
		{
			if (EOK != (ret = fci_netlink_send(core->clients[ii].back_port_id, msg)))
			{
				NXP_LOG_RAW_ERROR("fci_netlink_send() failed: %d\n", ret);
			}
		}
	}

	(void)mutex_unlock(&core->clients_lock);
	return ret;
}

/** @}*/
