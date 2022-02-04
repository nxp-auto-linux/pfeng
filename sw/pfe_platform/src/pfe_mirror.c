/* =========================================================================
 *  Copyright 2021-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */
#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"
#include "pfe_class.h"
#include "pfe_mirror.h"
#include "linked_list.h"

typedef struct
{
    pfe_class_t *class;
    LLIST_t mirrors;
    LLIST_t *curr;
} pfe_mirror_db_t;

struct pfe_mirror_tag
{
    char *name;           /* String identifier */
    addr_t phys_addr;     /* Address of the DMEM representation */
    pfe_mirror_db_t *db;  /* Database reference */
    LLIST_t this;         /* Link in database */
    pfe_ct_mirror_t phys; /* Physical representation */
};

static pfe_mirror_db_t *pfe_mirror_db = NULL;


/**
 * @brief Creates a database for mirrors management
 * @note Practically there will be only a single database
 * @param[in] class Classifier which physical interfaces will use the mirrors
 * @return Database instance or NULL in case of failure
 */
static pfe_mirror_db_t *pfe_mirror_create_db(pfe_class_t *class)
{
    pfe_mirror_db_t *db;
    #if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == class))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        return NULL;
    }
    #endif

    db = oal_mm_malloc(sizeof(pfe_mirror_db_t));
    if(NULL != db)
    {
        (void)memset(db, 0, sizeof(pfe_mirror_db_t));
        db->class = class;
        LLIST_Init(&db->mirrors);
    }
    return db;
}

/**
 * @brief Destroys the selected mirrors database
 * @param[in] db Database to be destroyed
 */
static void pfe_mirror_destroy_db(pfe_mirror_db_t *db)
{
    if(NULL != db)
    {
        /* Check whether the database is empty */
        if(!LLIST_IsEmpty(&db->mirrors))
        {   /* Not empty */
            NXP_LOG_ERROR("There are still entries in the database, leaking memory\n");
        }
        oal_mm_free(db);
    }
}

/**
 * @brief Queries mirrors database for the mirror instance corresponding to the search criterion
 * @param[in] db Database to query
 * @param[in] crit Criterion to be used (MIRROR_ANY is used to get 1st entry)
 * @param[in] arg Criterion argument (data)
 * @return The matching mirror instance or NULL if there is no matching mirror in the database
 */
static pfe_mirror_t *pfe_mirror_db_get_by_crit(pfe_mirror_db_t *db, pfe_mirror_db_crit_t crit, const void *arg)
{
    LLIST_t *curr;
    pfe_mirror_t *mirror;

    #if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == db))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        return NULL;
    }
    #endif

    /* Is there something to search */
    if(LLIST_IsEmpty(&db->mirrors))
    {   /* Nothing to search */
        return NULL;
    }
    /* Special criterion - return the 1st in the database */
    if(MIRROR_ANY == crit)
    {
        db->curr = db->mirrors.prNext->prNext;  /* HEAD.prNext --> Item0.prNext --> Item1. --> ... */
        return LLIST_DataFirst(&db->mirrors, pfe_mirror_t, this);
    }
    /* Real search */
    LLIST_ForEach(curr, &db->mirrors)
    {
        mirror = LLIST_Data(curr, pfe_mirror_t, this);
        switch(crit)
        {
            case MIRROR_BY_NAME:
                if(0 == strcmp(mirror->name, (const char *)arg))
                {   /* Match */
                    return mirror;
                }
                break;
            case MIRROR_BY_PHYS_ADDR:
                if(mirror->phys_addr == (addr_t) arg)
                {   /* Match */
                    return mirror;
                }
                break;
            default :
                NXP_LOG_ERROR("Wrong criterion %u\n", crit);
				break;
        }
    }
    /* Not found */
    return NULL;
}

/**
 * @brief Continues reading entries as started by pfe_mirror_db_get_by_crit() with MIRROR_ANY as criterion
 * @param[in] db Mirrors database
 * @details This function is used to walk through all mirrors (used by fci client to print all existing mirrors).
 * @return Either found mirror or NULL if there are no more mirrors.
 */
/*
* Maintenance note:
* The pfe_mirror_db_get_by_crit() supports all criteria which means that the 1st instance can be found
* for any of them however the pfe_mirror_db_get_next() supports only MIRROR_ANY which means that the
* search can be continued only for this criterion. This is because there can be only single instance of
* mirror matching other criteria than MIRROR_ANY.
*/
static pfe_mirror_t *pfe_mirror_db_get_next(pfe_mirror_db_t *db)
{
    pfe_mirror_t *mirror = NULL;

    #if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == db))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        return NULL;
    }
    #endif

    /* Is there something to search */
    if(LLIST_IsEmpty(&db->mirrors))
    {   /* Nothing to search */
        return NULL;
    }
    if(db->curr != &db->mirrors)
    {   /* Not the last item */
        mirror = LLIST_Data(db->curr, pfe_mirror_t, this);
        db->curr = db->curr->prNext;
    }
    return mirror;
}

/**
 * @brief Initialize the module
 * @param[in] class Reference to the classifier instance
 * @note Can be called only once unless pfe_mirror_deinit() is called.
 * @return Either EOK or error code in case of failure
 * @retval EPERM Already called, cannot be called more than once.
 * @retval EINVAL Invalid input argument (NULL).
 * @retval ENOMEM Could not allocate the needed memory.
 */
errno_t pfe_mirror_init(pfe_class_t *class)
{
    #if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == class))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        return EINVAL;
    }
    #endif

    if(NULL != pfe_mirror_db)
    {
        NXP_LOG_ERROR("Already initialized\n");
        return EPERM;
    }
    pfe_mirror_db = pfe_mirror_create_db(class);
    if(NULL == pfe_mirror_db)
    {
        return ENOMEM;
    }
    return EOK;
}

/**
 * @brief Deinitialize the module - free all internally used resources
 */
void pfe_mirror_deinit(void)
{
    if(NULL != pfe_mirror_db)
    {
        pfe_mirror_destroy_db(pfe_mirror_db);
        pfe_mirror_db = NULL;
    }
}

/**
 * @brief Obtain the 1st mirror matching the specified criteria
 * @param[in] crit Matching criterion for the mirrors
 * @param[in] arg Criterion specific argument (value)
 * @return Either the 1st found mirror instance or NULL if there is no matching mirror
 */
pfe_mirror_t *pfe_mirror_get_first(pfe_mirror_db_crit_t crit, const void *arg)
{
    pfe_mirror_t *mirror = NULL;
    if(NULL != pfe_mirror_db)
    {
        mirror = pfe_mirror_db_get_by_crit(pfe_mirror_db, crit, arg);
    }
    return mirror;
}

/**
 * @brief Returns the next mirror matching the criterion passed to pfe_mirror_db_get_by_crit()
 * @note  Only the MIRROR_ANY criterion is supported because mirrors are forced to have
 *        unique name and address and there are no other criteria to match. It is expected
 *        that the pfe_mirror_get_first(MIRROR_ANY, NULL) is used to obtain the 1st mirror
 *        and pfe_mirror_get_next() is used to get list of all mirrors.
 * @return Either next mirror or NULL if there are no more mirrors.
 */
pfe_mirror_t *pfe_mirror_get_next(void)
{
    pfe_mirror_t *mirror = NULL;

    if(NULL != pfe_mirror_db)
    {
        /* We do not support any other criteria than MIRROR_ANY, rework the function
           pfe_mirror_db_get_next() if you need to add other criteria. */
        mirror = pfe_mirror_db_get_next(pfe_mirror_db);
    }
    return mirror;
}

/**
 * @brief Creates a new mirror instance
 * @param[in] name Unique name (identifier)
 * @return Mirror instance or NULL in case of failure
 */
pfe_mirror_t *pfe_mirror_create(const char *name)
{
    pfe_mirror_t *mirror = NULL;
    #if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == name))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        return NULL;
    }
    #endif

    if(NULL != pfe_mirror_db)
    {
        /* Do not allow duplicates */
        if(NULL == pfe_mirror_db_get_by_crit(pfe_mirror_db, MIRROR_BY_NAME, (void *)name))
        {   /* No such entry in the database, we may add a new one */
            mirror = oal_mm_malloc(sizeof(pfe_mirror_t) + strlen(name));
            if(NULL != mirror)
            {   /* Memory available */
                (void)memset(mirror, 0, sizeof(pfe_mirror_t));
                /* Remember input data */
                mirror->db = pfe_mirror_db;
                mirror->name = (char *)&mirror[1];
                (void)strcpy(mirror->name, name);
                /* Allocate DMEM */
                mirror->phys_addr = pfe_class_dmem_heap_alloc(mirror->db->class, sizeof(pfe_ct_mirror_t));
                if(0U == mirror->phys_addr)
                {   /* No DMEM */
                    NXP_LOG_ERROR("Not enough DMEM for mirror\n");
                    oal_mm_free(mirror);
                    mirror = NULL;
                }
                else
                {
                    /* Add the new mirror into the internal database */
                    LLIST_AddAtEnd(&mirror->this, &pfe_mirror_db->mirrors);
                }
            }
        }
    }
    return mirror;
}

/**
 * @brief Destroys the selected mirror
 * @param[in] mirror Mirror instance
 * @warning Make sure the mirror is not in use.
 */
void pfe_mirror_destroy(pfe_mirror_t *mirror)
{
    if(NULL != mirror)
    {
        pfe_class_dmem_heap_free(mirror->db->class, mirror->phys_addr);
        LLIST_Remove(&mirror->this);
        oal_mm_free(mirror);
    }
}

/**
 * @brief Retrieves DMEM address used by the mirror instance
 * @param[in] mirror Mirror instance
 * @return DMEM address used by the mirror
 */
uint32_t pfe_mirror_get_address(const pfe_mirror_t *mirror)
{
    #if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mirror))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        return 0;
    }
    #endif

    return mirror->phys_addr;
}

/**
 * @brief Retrieves mirror name
 * @param[in] mirror Mirror instance
 * @return Mirror name - this string shall not be modified outside; NULL in case of failure
 */
const char *pfe_mirror_get_name(const pfe_mirror_t *mirror)
{
    #if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mirror))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        return NULL;
    }
    #endif

    return mirror->name;
}

/**
 * @brief Configures egress port for mirrored frames
 * @param[in] mirror Mirror instance
 * @param[in] egress Egress port for mirrored frames
 * @return EOK when success or error code otherwise
 */
errno_t pfe_mirror_set_egress_port(pfe_mirror_t *mirror, pfe_ct_phy_if_id_t egress)
{
    #if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mirror))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        return EINVAL;
    }
    #endif

    /* No endian conversion is needed since the size is 8-bits */
    mirror->phys.e_phy_if = egress;
    return pfe_class_write_dmem(mirror->db->class, -1, mirror->phys_addr, &mirror->phys, sizeof(pfe_ct_mirror_t));
}

/**
 * @brief Retrieves egress port for mirrored frames
 * @param[in] mirror Mirror instance
 * @return The egress port
 */
pfe_ct_phy_if_id_t pfe_mirror_get_egress_port(const pfe_mirror_t *mirror)
{
    #if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mirror))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        return PFE_PHY_IF_ID_INVALID;
    }
    #endif

    /* No endian conversion is needed since the size is 8-bits */
    return mirror->phys.e_phy_if;
}

/**
 * @brief Configures flexible filter to select mirrored frames
 * @param[in] mirror Mirror instance
 * @param[in] filter_adress Address of flexible filter to select mirrored frames (0 to disable the filter)
 * @return EOK when success or error code otherwise
 */
errno_t pfe_mirror_set_filter(pfe_mirror_t *mirror, uint32_t filter_address)
{
    #if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mirror))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        return EINVAL;
    }
    #endif

    /* Set the address of the filter table (convert endian) */
    mirror->phys.flexible_filter = oal_htonl(filter_address);
    return pfe_class_write_dmem(mirror->db->class, -1, mirror->phys_addr, &mirror->phys, sizeof(pfe_ct_mirror_t));
}

/**
 * @brief Retrieves flexible filter to select mirrored frames
 * @param[in] mirror Mirror instance
 * @return Address of flexible filter to select mirrored frames (0 = disabled the filter)
 */
uint32_t pfe_mirror_get_filter(const pfe_mirror_t *mirror)
{
    #if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mirror))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        return 0U;
    }
    #endif

    /* Set the address of the filter table (convert endian) */
    return oal_ntohl(mirror->phys.flexible_filter);
}

/**
 * @brief Configures mirrored frame modifications
 * @param[in] mirror Mirror instance
 * @param[in] actions Actions to be done on mirrored frame (network endian)
 * @param[in] args Arguments for actions (all fields in network endian)
 * @return EOK when success or error code otherwise
 */
errno_t pfe_mirror_set_actions(pfe_mirror_t *mirror, pfe_ct_route_actions_t actions, const pfe_ct_route_actions_args_t *args)
{
    #if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mirror))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        return EINVAL;
    }
    #endif

    mirror->phys.actions = actions;
    if(RT_ACT_NONE != actions)
    {
        (void)memcpy(&mirror->phys.args, args, sizeof(pfe_ct_route_actions_args_t));
    }
    return pfe_class_write_dmem(mirror->db->class, -1, mirror->phys_addr, &mirror->phys, sizeof(pfe_ct_mirror_t));
}

/**
 * @brief Queries mirrored frame modifications
 * @param[in] mirror Mirror instance
 * @param[out] actions Actions to be done on mirrored frame (network endian)
 * @param[out] args Arguments for actions (all fields in network endian)
 * @return EOK when success or error code otherwise
 */
errno_t pfe_mirror_get_actions(const pfe_mirror_t *mirror, pfe_ct_route_actions_t *actions, pfe_ct_route_actions_args_t *args)
{
    #if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == mirror)||(NULL == args)))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        return EINVAL;
    }
    #endif

    *actions = mirror->phys.actions;
    if(RT_ACT_NONE != mirror->phys.actions)
    {   /* Arguments are needed */
        (void)memcpy(args, &mirror->phys.args, sizeof(pfe_ct_route_actions_args_t));
    }
    return EOK;
}
