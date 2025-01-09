/* =========================================================================
 *  Copyright (C) 2007 Mindspeed Technologies, Inc.
 *  Copyright 2017-2023,2025 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <sys/socket.h>
#include <linux/netlink.h>

#include "pfe_cfg.h"
#include "fpp.h"
#include "fci_msg.h"
#include "libfci.h"

#ifndef EOK
#define EOK 0
#endif

#ifndef NETLINK_FF
#define NETLINK_FF 30
#endif
#define NETLINK_TEST 31
#ifndef NETLINK_KEY
#define NETLINK_KEY 32
#endif

#define NETLINK_TYPE_CUSTOM_FCI 17

/*
* Debug macros
*/
#define FCILIB_PRINT	0
#define FCILIB_ERR	0
#define	FCILIB_INIT	0
#define	FCILIB_OPEN	0
#define	FCILIB_CLOSE	0
#define	FCILIB_WRITE	0
#define	FCILIB_READ	0
#define FCILIB_DUMP	0
#define FCILIB_CATCH	0
#define FCILIB_REG_CB	0

#ifdef FCILIB_PRINT
#define FCILIB_PRINTF(type, ...) do {if(type) printf(__VA_ARGS__);} while(0);
#else
#define FCILIB_PRINTF(type, info, args...) do {} while(0);
#endif

#define FCI_PAYLOAD(n)	NLMSG_PAYLOAD((n), sizeof(struct fci_hdr))
#define FCI_DATA(f)	((unsigned short *)((char *)(f) + sizeof(struct fci_hdr)))

struct fci_hdr
{
	uint16_t fcode;
	uint16_t len;
} __attribute__((packed));

struct __fci_client_tag
{
	int cmd_sock_fd;
	int back_sock_fd;
	int group;
	uint32_t cmd_port_id;
	uint32_t back_port_id;
	fci_cb_retval_t (*event_cb)(unsigned short fcode, unsigned short len, unsigned short *payload);
};


static FCI_CLIENT *fci_create_client(int nl_type, unsigned long group);
static int fci_destroy_client(FCI_CLIENT *client);
static int __fci_cmd(FCI_CLIENT *client, unsigned short fcode, unsigned short *cmd_buf, unsigned short cmd_len, unsigned short *rep_buf, unsigned short *rep_len);

/****************************** PUBLICS FUNCTIONS ********************************/
FCI_CLIENT *fci_open(fci_client_type_t client_type, fci_mcast_groups_t group)
{
	FCI_CLIENT *new_client = NULL;

	/* Create client according to the requested socket type */
	switch(client_type)
	{
		case FCI_CLIENT_DEFAULT:
		{
			FCILIB_PRINTF(FCILIB_OPEN, "fci_open:%d client type FCILIB_FF_CLIENT with group %d\n", __LINE__, group);
			new_client = fci_create_client(NETLINK_FF, group);
			break;
		}

		default:
		{
			FCILIB_PRINTF(FCILIB_ERR, "LIB_FCI: fci_open():%d client type %d not supported\n", __LINE__, client_type);
			new_client = NULL;
		}

		break;
	}

	/* Unique ID used to identify this client */
	return new_client;
}

int fci_register_cb(FCI_CLIENT *client, fci_cb_retval_t (*event_cb)(unsigned short fcode, unsigned short len, unsigned short *payload))
{
	fci_msg_t msg;
	fci_msg_t *reply_msg;
	int err = EOK;
	struct msghdr msg_hdr;
	struct iovec iov;
	struct nlmsghdr *nlh = NULL;
	struct sockaddr_nl dest_addr;
	struct sockaddr_nl temp_addr;
	socklen_t temp_size = sizeof(temp_addr);
	struct sockaddr_nl src_addr_back;

	FCILIB_PRINTF(FCILIB_REG_CB, "fci_register_cb()\n")
	if (client != NULL)
	{
		client->event_cb = event_cb;

		if (NULL != event_cb)
		{
			nlh=(struct nlmsghdr *)malloc(NLMSG_SPACE(sizeof(fci_msg_t)));
			if (NULL == nlh)
			{
				FCILIB_PRINTF(FCILIB_ERR, "LIBFCI: nlh not allocated\n");
				return -ENOBUFS;
			}
			/* Create the back channel socket */
			client->back_sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_TYPE_CUSTOM_FCI);

			if(-1 == client->back_sock_fd)	
			{
				FCILIB_PRINTF(FCILIB_ERR, "LIBFCI: socket() failed with errno %d\n",errno);
				err = -errno;
				goto exit;
			}
			memset(&src_addr_back, 0, sizeof(src_addr_back));
			memset(&msg_hdr, 0, sizeof(msg_hdr));
			src_addr_back.nl_family = AF_NETLINK;
			src_addr_back.nl_pid = 0;  /* let kernel fill the pid */
			src_addr_back.nl_groups = 0;  /* not in mcast groups */
	
			if(-1 == bind(client->back_sock_fd, (struct sockaddr*)&src_addr_back, sizeof(src_addr_back)))	
			{
				FCILIB_PRINTF(FCILIB_ERR, "LIBFCI: bind() failed with errno %d\n",errno);
				err = -errno;
				goto close_and_exit;
			}
			/* Read the port_id assigned by kernel */
			if(-1 == getsockname(client->back_sock_fd, (struct sockaddr *)&temp_addr, &temp_size)){
				FCILIB_PRINTF(FCILIB_ERR, "getsockname failed with %d\n",errno);
				err = -errno;
				goto close_and_exit;
			}
			client->back_port_id = temp_addr.nl_pid;

			/* Let endpoint know that we're here */
			msg.type = FCI_MSG_CLIENT_REGISTER;
			msg.msg_client_register.port_id = client->back_port_id;
			memset(&dest_addr, 0, sizeof(dest_addr));
			dest_addr.nl_family = AF_NETLINK;
			dest_addr.nl_pid = 0;   /* For Linux Kernel */
			dest_addr.nl_groups = client->group;

			/* Fill the netlink message header */
			nlh->nlmsg_len = NLMSG_SPACE(sizeof(fci_msg_t));
			nlh->nlmsg_pid = client->cmd_port_id;  /* self port id */
			nlh->nlmsg_flags = 0;
			memcpy(NLMSG_DATA(nlh), &msg,sizeof(fci_msg_t));

			iov.iov_base = (void *)nlh;
			iov.iov_len = nlh->nlmsg_len;
			msg_hdr.msg_name = (void *)&dest_addr;
			msg_hdr.msg_namelen = sizeof(dest_addr);
			msg_hdr.msg_iov = &iov;
			msg_hdr.msg_iovlen = 1;

			if (-1 == sendmsg(client->cmd_sock_fd, &msg_hdr, 0))
			{
				/*	Transport failure */
				FCILIB_PRINTF(FCILIB_ERR, "LIBFCI: sendmsg() failed with %d\n", errno);
				err = -errno;
				goto close_and_exit;
			}
			else
			{
				/* Read the response */
				memset(nlh,0,NLMSG_SPACE(sizeof(fci_msg_t)));

				if(-1 == recvmsg(client->cmd_sock_fd, &msg_hdr, 0))
				{
					switch(errno)
					{
						case EAGAIN:
							FCILIB_PRINTF(FCILIB_ERR, "recvmsg() failed: %d - timeout\n", errno);
							break;
						default:
							FCILIB_PRINTF(FCILIB_ERR, "recvmsg() failed: %d\n", errno);
							break;
					}
					err = -errno;
					goto close_and_exit;
				}

				if (sizeof(fci_msg_t) <= (nlh->nlmsg_len - NLMSG_LENGTH(0)))
				{
					/*
					 * WARNING:
					 * In reply_msg, only ret_code member is initialized.
					 * All other members are uninitialized.
					 */
					reply_msg = NLMSG_DATA(nlh);
					err = reply_msg->ret_code;

					if (err != EOK) {
						/* Registration failed */
						FCILIB_PRINTF(FCILIB_ERR,
									"Registration failed. %d received.\n", err);
						goto close_and_exit;
					} else {
						FCILIB_PRINTF(FCILIB_PRINT,
									"Client registered successfully.\n");
					}
				}
				else
				{
					FCILIB_PRINTF(FCILIB_ERR, "Incorrect response length.\n");
					err = -ENOBUFS;
					goto close_and_exit;
				}
			}
		}
		else if(-1 !=client->back_sock_fd)
		{
			/* Unregister the client from server and close the socket */
			nlh=(struct nlmsghdr *)malloc(NLMSG_SPACE(sizeof(fci_msg_t)));
			if (NULL == nlh)
			{
				FCILIB_PRINTF(FCILIB_ERR, "LIBFCI: nlh not allocated\n");
				err = -ENOBUFS;
				goto close_and_exit;
			}

			client->event_cb = NULL;

			memset(&dest_addr, 0, sizeof(dest_addr));
			memset(&msg_hdr, 0, sizeof(msg_hdr));

			/*	Unregister from server */
			msg.type = FCI_MSG_CLIENT_UNREGISTER;
			msg.msg_client_register.port_id = client->back_port_id;
			dest_addr.nl_family = AF_NETLINK;
			dest_addr.nl_pid = 0;   /* For Linux Kernel */
			dest_addr.nl_groups = client->group;

			/* Fill the netlink message header */
			nlh->nlmsg_len = NLMSG_SPACE(sizeof(fci_msg_t));
			nlh->nlmsg_pid = client->cmd_port_id;  /* command port_id */
			nlh->nlmsg_flags = 0;
			memcpy(NLMSG_DATA(nlh), &msg, sizeof(fci_msg_t));

			iov.iov_base = (void *)nlh;
			iov.iov_len = nlh->nlmsg_len;
			msg_hdr.msg_name = (void *)&dest_addr;
			msg_hdr.msg_namelen = sizeof(dest_addr);
			msg_hdr.msg_iov = &iov;
			msg_hdr.msg_iovlen = 1;

			if (-1 == sendmsg(client->cmd_sock_fd, &msg_hdr, 0))
			{
				err = -errno;
				goto close_and_exit;
			}
			else
			{
				/* Read the response */
				memset(nlh,0,NLMSG_SPACE(sizeof(fci_msg_t)));
				if(-1 == recvmsg(client->cmd_sock_fd, &msg_hdr, 0))
				{
					switch(errno)
					{
						case EAGAIN:
							FCILIB_PRINTF(FCILIB_ERR, "recvmsg() failed: %d - timeout\n", errno);
							break;
						default:
							FCILIB_PRINTF(FCILIB_ERR, "recvmsg() failed: %d\n", errno);
							break;
					}
					err = -errno;
					goto close_and_exit;
				}

				if (sizeof(fci_msg_t) <= (nlh->nlmsg_len - NLMSG_LENGTH(0)))
				{
					/*
					 * WARNING:
					 * In reply_msg, only ret_code member is initialized.
					 * All other members are uninitialized.
					 */
					reply_msg = NLMSG_DATA(nlh);
					err = reply_msg->ret_code;

					if (err != FPP_ERR_OK) {
						/* Registration failed */
						FCILIB_PRINTF(FCILIB_ERR,
									"Unregistration failed. %d received.\n",
									err);
						goto close_and_exit;
					} else {
						FCILIB_PRINTF(FCILIB_PRINT,
									"Client unregistered successfully.\n");
						close(client->back_sock_fd);
						client->back_sock_fd = -1;
					}
				}
				else
				{
					FCILIB_PRINTF(FCILIB_ERR, "Incorrect response length.\n");
					err = -ENOBUFS;
					goto close_and_exit;
				}
			}

		}
		err = EOK;
		goto exit;
	}
	else
	{
		return -EINVAL;
	}
close_and_exit:
	close(client->back_sock_fd);
	client->back_sock_fd = -1;
	if (NULL != nlh)
	{
		free(nlh);
	}
	return err;
exit:
	free(nlh);
	return err;
}

int fci_close(FCI_CLIENT *client)
{
	int rc;

	FCILIB_PRINTF(FCILIB_CLOSE, "fci_close()\n");

	/* Unregister FCI client */
	if (client == NULL)
	{
		return -1;
	}
	if(EOK != (rc = fci_register_cb(client, NULL)))
	{
		FCILIB_PRINTF(FCILIB_ERR, "fci_close: fci_register_cb failed with %d!\n",rc);
	}
	if ((rc = fci_destroy_client(client)) < 0)
	{
		FCILIB_PRINTF(FCILIB_ERR, "fci_close: fci_destroy_client failed !\n");

		return rc;
	}
	return 0;
}

int fci_cmd(FCI_CLIENT *client, unsigned short fcode, unsigned short *cmd_buf, unsigned short cmd_len, unsigned short *rep_buf, unsigned short *rep_len)
{
	FCILIB_PRINTF(FCILIB_WRITE, "fci_cmd: send fcode %#x length %d\n", fcode, cmd_len);

	return __fci_cmd(client, fcode, cmd_buf, cmd_len, rep_buf, rep_len);
}

int fci_write(FCI_CLIENT *client, unsigned short fcode, unsigned short cmd_len, unsigned short *cmd_buf)
{
	FCILIB_PRINTF(FCILIB_WRITE, "fci_write: send fcode %#x length %d\n", fcode, cmd_len);

	return __fci_cmd(client, fcode, cmd_buf, cmd_len, NULL, NULL);
}

int fci_query(FCI_CLIENT *client, unsigned short fcode, unsigned short cmd_len, unsigned short *cmd_buf, unsigned short *rep_len, unsigned short *rep_buf)
{
	FCILIB_PRINTF(FCILIB_WRITE, "fci_query: send fcode %#x length %d\n", fcode, cmd_len);

	return __fci_cmd(client, fcode, cmd_buf, cmd_len, rep_buf, rep_len);
}

int fci_catch(FCI_CLIENT *client)
{
	fci_msg_t msg;
	int ret;
	bool shall_quit = false;
	struct msghdr msg_hdr;
	struct iovec iov;
	struct nlmsghdr *nlh = NULL;
	struct sockaddr_nl dest_addr;

	FCILIB_PRINTF(FCILIB_CATCH, "fci_catch()\n")
	if (-1 == client->back_sock_fd)
	{
		FCILIB_PRINTF(FCILIB_ERR, "LIBFCI: socket not initialized \n");
		return -ENOTSOCK;
	}

	memset(&msg_hdr, 0, sizeof(msg_hdr));

	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0;   /* For Linux Kernel */
	dest_addr.nl_groups = client->group;

	nlh=(struct nlmsghdr *)malloc(NLMSG_SPACE(sizeof(fci_msg_t)));
	if (NULL == nlh)
	{
		FCILIB_PRINTF(FCILIB_ERR, "LIBFCI: nlh not allocated\n");
		return -ENOBUFS;
	}

	/* Fill the netlink message header */
	nlh->nlmsg_len = NLMSG_SPACE(sizeof(fci_msg_t));
	nlh->nlmsg_pid = client->back_port_id;  /* self port id */
	nlh->nlmsg_flags = 0;

	iov.iov_base = (void *)nlh;
	iov.iov_len = nlh->nlmsg_len;
	msg_hdr.msg_name = (void *)&dest_addr;
	msg_hdr.msg_namelen = sizeof(dest_addr);
	msg_hdr.msg_iov = &iov;
	msg_hdr.msg_iovlen = 1;	

	while (false == shall_quit)
	{
		/* Read message from kernel */
		memset(nlh, 0, NLMSG_SPACE(sizeof(fci_msg_t)));
		if (-1 == recvmsg(client->back_sock_fd, &msg_hdr, 0))
		{
			FCILIB_PRINTF(FCILIB_ERR, "recvmsg() failed: %d\n", errno);
		}
		else
		{
			memcpy(&msg, NLMSG_DATA(nlh),sizeof(fci_msg_t));
			/*	Message */
			FCILIB_PRINTF(FCILIB_PRINT," Received message payload: %s\n",(char*)NLMSG_DATA(nlh));
			if (FCI_MSG_CMD == msg.type)
			{
				/*	Call registered callback */
				if (NULL != client->event_cb)
				{
					ret = client->event_cb((unsigned short)msg.msg_cmd.code, (unsigned short)msg.msg_cmd.length,
										(unsigned short *)msg.msg_cmd.payload);

					if (FCI_CB_CONTINUE == ret)
					{
						/*	Continue */
						;
					}
					else
					{
						/*	Terminate */
						shall_quit = true;
					}
				}
			}
			else
			{
				FCILIB_PRINTF(FCILIB_ERR, "Unknown message received (type = 0x%x)\n", msg.type);
			}

		}
	}

	free(nlh);
	return 0;
}

/*
 * @brief		Return file descriptor of a socket for FCI events from driver.
 * @param[in]	client The FCI client instance
 * @return		File descriptor or -1 if some error.
 */
int fci_fd(FCI_CLIENT *client)
{
	if (NULL == client)
	{
		return -1;
	}
	else
	{
		return client->back_sock_fd;
	}
}


/****************************** PRIVATE FUNCTIONS ********************************/
static int __fci_cmd(FCI_CLIENT *client, unsigned short fcode, unsigned short *cmd_buf, unsigned short cmd_len, unsigned short *rep_buf, unsigned short *rep_len)
{
	fci_msg_t msg;
	fci_msg_t *reply_msg;
	unsigned short cmd_ret = EOK;
	struct sockaddr_nl dest_addr;
	struct nlmsghdr *nlh = NULL;
	struct msghdr msg_hdr = {0};
	struct iovec iov;

	msg.type = FCI_MSG_CMD;
	msg.msg_cmd.code = fcode;

	if (cmd_len > 0)
	{
		msg.msg_cmd.length = cmd_len;
		memcpy(&msg.msg_cmd.payload, cmd_buf, cmd_len);
	}

	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0;   /* For Linux Kernel */
	dest_addr.nl_groups = client->group;

	nlh=(struct nlmsghdr *)malloc(NLMSG_SPACE(sizeof(fci_msg_t)));
	if (NULL == nlh)
	{
		FCILIB_PRINTF(FCILIB_ERR, "LIBFCI: nlh not allocated\n");
		return -ENOBUFS;
	}

	/* Fill the netlink message header */
	nlh->nlmsg_len = NLMSG_SPACE(sizeof(fci_msg_t));
	nlh->nlmsg_pid = client->cmd_port_id;  /* self port id */
	nlh->nlmsg_flags = 0;
	/* Fill in the netlink message payload */
	memcpy(NLMSG_DATA(nlh), &msg,sizeof(msg));

	iov.iov_base = (void *)nlh;
	iov.iov_len = nlh->nlmsg_len;
	msg_hdr.msg_name = (void *)&dest_addr;
	msg_hdr.msg_namelen = sizeof(dest_addr);
	msg_hdr.msg_iov = &iov;
	msg_hdr.msg_iovlen = 1;

	if (-1 == sendmsg(client->cmd_sock_fd, &msg_hdr, 0))
	{
		/*	Transport failure */
		FCILIB_PRINTF(FCILIB_ERR, "LIBFCI: sendmsg() failed with %d\n", errno);
		free(nlh);
		return -errno;
	}
	else
	{
		memset(nlh,0,NLMSG_SPACE(sizeof(fci_msg_t)));
		if(-1 == recvmsg(client->cmd_sock_fd, &msg_hdr, 0))
		{
			switch(errno)
			{
				case EAGAIN:
					FCILIB_PRINTF(FCILIB_ERR, "recvmsg() failed3: %d - timeout\n", errno);
					break;
				default:
					FCILIB_PRINTF(FCILIB_ERR, "recvmsg() failed: %d\n", errno);
					break;
			}
			free(nlh);
			return -errno;
		}
		reply_msg = NLMSG_DATA(nlh);

		if(reply_msg->ret_code != EOK)
		{
			/*	Command failure */
			FCILIB_PRINTF(FCILIB_ERR, "LIBFCI: Command failed with %d\n", reply_msg->ret_code);

			free(nlh);
			return -reply_msg->ret_code;
		}
		else
		{
			/*	Success, pass reply data (if any) and its length to user */
			if ((NULL != rep_buf) && (NULL != rep_len) &&
				(4U <= reply_msg->msg_cmd.length) &&
				(0U != (nlh->nlmsg_len - NLMSG_LENGTH(0))))
			{
				memcpy(rep_buf, ((void *)reply_msg->msg_cmd.payload + 4U), reply_msg->msg_cmd.length - 4U);
				*rep_len = reply_msg->msg_cmd.length - 4U;
			}
			memcpy(&cmd_ret, reply_msg->msg_cmd.payload, sizeof(unsigned short));

			free(nlh);
			return cmd_ret;
		}
	}
}

static FCI_CLIENT *fci_create_client(int nl_type, unsigned long group)
{
	FCI_CLIENT *cl;
	struct sockaddr_nl src_addr_cmd;
	struct timeval time;
	struct sockaddr_nl temp_addr;
	socklen_t temp_size = sizeof(temp_addr);
	
	cl = malloc(sizeof(FCI_CLIENT));
	if (NULL == cl)
	{
		FCILIB_PRINTF(FCILIB_ERR, "LIBFCI: fci_create_client() failed\n");
		return NULL;
	}
	cl->back_sock_fd = -1;
	cl->group = group;
	/* Create the command socket first */	
	cl->cmd_sock_fd = socket(PF_NETLINK, SOCK_RAW,NETLINK_TYPE_CUSTOM_FCI);
	if(-1 == cl->cmd_sock_fd)	
	{
		FCILIB_PRINTF(FCILIB_ERR, "LIBFCI: socket() failed with errno %d\n",errno);
		goto free_and_exit;	
	}
	memset(&src_addr_cmd, 0, sizeof(src_addr_cmd));
	src_addr_cmd.nl_family = AF_NETLINK;
	src_addr_cmd.nl_pid = 0;  /* let kernel fill the port id */
	src_addr_cmd.nl_groups = 0;  /* not in mcast groups */

	if(-1 == bind(cl->cmd_sock_fd, (struct sockaddr*)&src_addr_cmd, sizeof(src_addr_cmd)))	
	{
		FCILIB_PRINTF(FCILIB_ERR, "LIBFCI: bind() failed with errno %d\n",errno);
		goto free_and_exit;
	}

	/* Set the RX timeout for the command socket */
	time.tv_sec = 5;
	time.tv_usec = 0;
	setsockopt(cl->cmd_sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time, sizeof time);

	/* Read the port_id assigned by kernel */
	if(-1 == getsockname(cl->cmd_sock_fd,(struct sockaddr *)&temp_addr,&temp_size)){
		FCILIB_PRINTF(FCILIB_ERR, "getsockname failed with %d\n",errno);
		goto free_and_exit;
	}
	cl->cmd_port_id = temp_addr.nl_pid;
	
	return cl;

free_and_exit:
	if (-1 != cl->cmd_sock_fd)
	{
		close(cl->cmd_sock_fd);
		cl->cmd_sock_fd = -1;
	}

	if (-1 != cl->back_sock_fd)
	{
		close(cl->back_sock_fd);
		cl->back_sock_fd = -1;
	}

	free(cl);
	return NULL;
}

static int fci_destroy_client(FCI_CLIENT *client)
{
	FCILIB_PRINTF(FCILIB_CLOSE, "fci_destroy_client()\n");
	
	if ((-1 == close(client->cmd_sock_fd)))
	{
		FCILIB_PRINTF(FCILIB_ERR, "LIBFCI: close() failed: %d\n", errno);
		return -1;
	}

	free(client);
	client = NULL;

	return 0;
}
