/* =========================================================================
 *  Copyright 2021-2025 NXP
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
    oal_mutex_t lock;
} pfe_mirror_db_t;

struct pfe_mirror_tag
{
    char *name;           /* String identifier */
    addr_t phys_addr;     /* Address of the DMEM representation */
    pfe_mirror_db_t *db;  /* Database reference */
    LLIST_t this;         /* Link in database */
    pfe_ct_mirror_t phys; /* Physical representation */
    int8_t ref_counter;   /* Summary count of all leased references (pointers) to this mirror instance and to its DMEM representation. */
};

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_VAR_INIT_32
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static pfe_mirror_db_t *pfe_mirror_db = NULL;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_VAR_INIT_32
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */


#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

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
        db = NULL;
    }
    else
#endif
    {
        db = oal_mm_malloc(sizeof(pfe_mirror_db_t));
        if (NULL == db)
        {
            NXP_LOG_ERROR("Unable to allocate memory\n");
        }
        else
        {
            (void)memset(db, 0, sizeof(pfe_mirror_db_t));
            db->class = class;
            LLIST_Init(&db->mirrors);
            
            if (EOK != oal_mutex_init(&db->lock))
            {
                NXP_LOG_ERROR("Mutex initialization failed\n");
                oal_mm_free(db);
                db = NULL;
            }
        }
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
		LLIST_t *item, *aux;
		pfe_mirror_t *entry;
		LLIST_ForEachRemovable(item, aux, &db->mirrors)
		{
			entry = LLIST_Data(item, pfe_mirror_t, this);
			pfe_mirror_destroy(entry);
		}
        /* Check whether the database is empty */
        if(!LLIST_IsEmpty(&db->mirrors))
        {   /* Not empty */
            NXP_LOG_ERROR("There are still entries in the database, leaking memory\n");
        }

        if (EOK != oal_mutex_destroy(&db->lock))
        {
            NXP_LOG_ERROR("Could not destroy mutex\n");
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
    bool_t match = FALSE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == db))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        mirror = NULL;
    }
    else
#endif
    {
        /* Is there something to search */
        if(LLIST_IsEmpty(&db->mirrors))
        {   /* Nothing to search */
            mirror = NULL;
        }
        /* Special criterion - return the 1st in the database */
        else if(MIRROR_ANY == crit)
        {
            db->curr = db->mirrors.prNext->prNext;  /* HEAD.prNext --> Item0.prNext --> Item1. --> ... */
            mirror = LLIST_DataFirst(&db->mirrors, pfe_mirror_t, this);
        }
        else
        {
            /* Real search */
            LLIST_ForEach(curr, &db->mirrors)
            {
                mirror = LLIST_Data(curr, pfe_mirror_t, this);
                switch(crit)
                {
                    case MIRROR_BY_NAME:
                        if(0 == strcmp(mirror->name, (const char *)arg))
                        {   /* Match */
                            match = TRUE;
                        }
                        break;
                    case MIRROR_BY_PHYS_ADDR:
                        if(mirror->phys_addr == (addr_t) arg)
                        {   /* Match */
                            match = TRUE;
                        }
                        break;
                    default :
                        NXP_LOG_WARNING("Wrong criterion %u\n", crit);
                        break;
                }
                if(TRUE == match)
                {
                    break;
                }
                else
                {
                    mirror = NULL;
                }
            }
        }
    }

    return mirror;
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
        mirror = NULL;
    }
    else
#endif
    {
        /* Is there something to search */
        if(LLIST_IsEmpty(&db->mirrors))
        {   /* Nothing to search */
            mirror = NULL;
        }
        else
        {
            if(db->curr != &db->mirrors)
            {   /* Not the last item */
                mirror = LLIST_Data(db->curr, pfe_mirror_t, this);
                db->curr = db->curr->prNext;
            }
        }
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
    errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == class))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        ret = EINVAL;
    }
    else
#endif
    {
        ret = EOK;
        if(NULL != pfe_mirror_db)
        {
            NXP_LOG_ERROR("Already initialized\n");
            ret = EPERM;
        }
        else
        {
            pfe_mirror_db = pfe_mirror_create_db(class);
            if(NULL == pfe_mirror_db)
            {
                ret = ENOMEM;
            }
        }
    }

    return ret;
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
 *
 * @note Protected by mutex, because it accesses the mirror database.
 *
 * @note When execution thread which called this function finishes working with the provided instance,
 *       it must call pfe_mirror_ref_release() for the given instance to "release" it.
 */
pfe_mirror_t *pfe_mirror_get_first(pfe_mirror_db_crit_t crit, const void *arg)
{
    pfe_mirror_t *mirror = NULL;
    if(NULL != pfe_mirror_db)
    {
        /*	Protect against concurrent access */
        if (unlikely(EOK != oal_mutex_lock(&pfe_mirror_db->lock)))
        {
            NXP_LOG_ERROR("Mutex lock failed\n");
        }
        
        mirror = pfe_mirror_db_get_by_crit(pfe_mirror_db, crit, arg);
        if (NULL != mirror)
        {
            mirror->ref_counter++;
        }

        if (unlikely(EOK != oal_mutex_unlock(&pfe_mirror_db->lock)))
        {
            NXP_LOG_ERROR("Mutex unlock failed\n");
        }
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
 *
 * @note Protected by mutex, because it accesses the mirror database.
 *
 * @note When execution thread which called this function finishes working with the provided instance,
 *       it must call pfe_mirror_ref_release() for the given instance to "release" it.
 */
pfe_mirror_t *pfe_mirror_get_next(void)
{
    pfe_mirror_t *mirror = NULL;

    if(NULL != pfe_mirror_db)
    {
        /*	Protect against concurrent access */
        if (unlikely(EOK != oal_mutex_lock(&pfe_mirror_db->lock)))
        {
            NXP_LOG_ERROR("Mutex lock failed\n");
        }

        /* We do not support any other criteria than MIRROR_ANY, rework the function
           pfe_mirror_db_get_next() if you need to add other criteria. */
        mirror = pfe_mirror_db_get_next(pfe_mirror_db);
        if (NULL != mirror)
        {
            mirror->ref_counter++;
        }

        if (unlikely(EOK != oal_mutex_unlock(&pfe_mirror_db->lock)))
        {
            NXP_LOG_ERROR("Mutex unlock failed\n");
        }
    }
    return mirror;
}

/**
 * @brief Creates a new mirror instance
 * @param[in] name Unique name (identifier)
 * @return Mirror instance or NULL in case of failure
 *
 * @note Protected by mutex, because it modifies the mirror database.
 *
 * @note When execution thread which called this function finishes working with the provided instance,
 *       it must call pfe_mirror_ref_release() for the given instance to "release" it.
 */
pfe_mirror_t *pfe_mirror_create(const char *name)
{
    pfe_mirror_t *mirror = NULL;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == name))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        mirror = NULL;
    }
    else
#endif
    {
        if(NULL != pfe_mirror_db)
        {
            /*	Protect against concurrent access */
            if (unlikely(EOK != oal_mutex_lock(&pfe_mirror_db->lock)))
            {
                NXP_LOG_ERROR("Mutex lock failed\n");
            }

            /* Do not allow duplicates */
            if(NULL == pfe_mirror_db_get_by_crit(pfe_mirror_db, MIRROR_BY_NAME, (void *)name))
            {   /* No such entry in the database, we may add a new one */
                mirror = oal_mm_malloc(sizeof(pfe_mirror_t) + strlen(name) + 1U);
                if (NULL == mirror)
                {
                    NXP_LOG_ERROR("Unable to allocate memory\n");
                }
                else
                {   /* Memory available */
                    (void)memset(mirror, 0, sizeof(pfe_mirror_t));
                    /* Remember input data */
                    mirror->db = pfe_mirror_db;
                    mirror->name = (char *)&mirror[1];
                    mirror->ref_counter = 1;  /* Init to 1, because this function already returns a pointer (reference) of the newly created mirror. */
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

            if (unlikely(EOK != oal_mutex_unlock(&pfe_mirror_db->lock)))
            {
                NXP_LOG_ERROR("Mutex unlock failed\n");
            }
        }
    }

    return mirror;
}

/**
 * @brief Destroys the selected mirror
 * @param[in] mirror Mirror instance. Can be NULL.
 * @warning Make sure the mirror is not in use.
 * @retval OK Success
 * @retval EINVAL Internal error such as failed mutex lock.
 * @retval EBUSY Mirror instance is currently utilized. It must not be destroyed now.
 *
 * @note Protected by mutex, because it modifies the mirror database.
 */
errno_t pfe_mirror_destroy(pfe_mirror_t *mirror)
{
    errno_t ret = EOK;

    if(likely(NULL != mirror))
    {
        /*	Protect against concurrent access */
        if (unlikely(EOK != oal_mutex_lock(&pfe_mirror_db->lock)))
        {
            NXP_LOG_ERROR("Mutex lock failed\n");
            ret = EINVAL;
        }
        else
        {
            /* Count '1' or lower is considered OK for deletion.
             * Count '1' means only one existing reference - the reference in the thread which is calling this destroy function. */
            if (1 >= mirror->ref_counter) 
            {
                pfe_class_dmem_heap_free(mirror->db->class, mirror->phys_addr);
                LLIST_Remove(&mirror->this);
                oal_mm_free(mirror);
                ret = EOK;
            }
            else
            {
                ret = EBUSY;
            }

            if (unlikely(EOK != oal_mutex_unlock(&pfe_mirror_db->lock)))
            {
                NXP_LOG_ERROR("Mutex unlock failed\n");
                ret = EINVAL;
            }
        }
    }

    return ret;
}

/**
 * @brief Decrements reference counter of a mirror instance.
 * @param[in] mirror Mirror instance
 * @important When a code outside of this module obtains pointer to some mirror instance
 *            via _get_first()/_get_next(), then call this function in that code when
 *            the code is done working with the instance.
 * @note It is assumed this function is complementary to _get_first() / _get_next().
 *       This assumption ensures the target mirror instance stays valid (cannot be concurrently deleted)
 *       thanks to its high reference count.
 */
void pfe_mirror_put(pfe_mirror_t *mirror)
{
    if (likely(NULL != mirror))
    {
        if (unlikely(EOK != oal_mutex_lock(&mirror->db->lock)))
        {
            NXP_LOG_ERROR("Mutex lock failed\n");
        }

        mirror->ref_counter--;

        if (unlikely(EOK != oal_mutex_unlock(&mirror->db->lock)))
        {
            NXP_LOG_ERROR("Mutex unlock failed\n");
        }
    }
}

/**
 * @brief Decrements reference counter of a mirror instance. Finds the mirror instance by address of its DMEM representation.
 * @param[in] address Address of mirror instance DMEM representation.
 * @note See notes of pfe_mirror_put()
 */
void pfe_mirror_put_by_address(addr_t address)
{
    pfe_mirror_t *mirror = NULL;
    
    mirror = pfe_mirror_get_first(MIRROR_BY_PHYS_ADDR, (void *)address);
    pfe_mirror_put(mirror);  /* Decrement reference counter. This is what this function does. */
    pfe_mirror_put(mirror);  /* Notify mirror module we are done working with the mirror instance. This complements the previous _get_first(). */
}

/**
 * @brief Retrieves DMEM address used by the mirror instance
 * @param[in] mirror Mirror instance
 * @return DMEM address used by the mirror
 */
uint32_t pfe_mirror_get_address(const pfe_mirror_t *mirror)
{
    uint32_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mirror))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        ret = 0U;
    }
    else
#endif
    {
        ret = mirror->phys_addr;
    }
    return ret;
}

/**
 * @brief Retrieves mirror name
 * @param[in] mirror Mirror instance
 * @return Mirror name - this string shall not be modified outside; NULL in case of failure
 */
const char *pfe_mirror_get_name(const pfe_mirror_t *mirror)
{
    const char *str;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mirror))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        str = NULL;
    }
    else
#endif
    {
        str = mirror->name;
    }
    return str;
}

/**
 * @brief Configures egress port for mirrored frames
 * @param[in] mirror Mirror instance
 * @param[in] egress Egress port for mirrored frames
 * @return EOK when success or error code otherwise
 */
errno_t pfe_mirror_set_egress_port(pfe_mirror_t *mirror, pfe_ct_phy_if_id_t egress)
{
    errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mirror))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        ret = EINVAL;
    }
    else
#endif
    {
        /* No endian conversion is needed since the size is 8-bits */
        mirror->phys.e_phy_if = egress;
        ret = pfe_class_write_dmem(mirror->db->class, -1, mirror->phys_addr, &mirror->phys, sizeof(pfe_ct_mirror_t));
    }

    return ret;
}

/**
 * @brief Retrieves egress port for mirrored frames
 * @param[in] mirror Mirror instance
 * @return The egress port
 */
pfe_ct_phy_if_id_t pfe_mirror_get_egress_port(const pfe_mirror_t *mirror)
{
    pfe_ct_phy_if_id_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mirror))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        ret = PFE_PHY_IF_ID_INVALID;
    }
    else
#endif
    {
        /* No endian conversion is needed since the size is 8-bits */
        ret = mirror->phys.e_phy_if;
    }

    return ret;
}

/**
 * @brief Configures flexible filter to select mirrored frames
 * @param[in] mirror Mirror instance
 * @param[in] filter_adress Address of flexible filter to select mirrored frames (0 to disable the filter)
 * @return EOK when success or error code otherwise
 */
errno_t pfe_mirror_set_filter(pfe_mirror_t *mirror, uint32_t filter_address)
{
    errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mirror))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        ret = EINVAL;
    }
    else
#endif
    {
        /* Set the address of the filter table (convert endian) */
        mirror->phys.flexible_filter = oal_htonl(filter_address);
        ret = pfe_class_write_dmem(mirror->db->class, -1, mirror->phys_addr, &mirror->phys, sizeof(pfe_ct_mirror_t));
    }

    return ret;
}

/**
 * @brief Retrieves flexible filter to select mirrored frames
 * @param[in] mirror Mirror instance
 * @return Address of flexible filter to select mirrored frames (0 = disabled the filter)
 */
uint32_t pfe_mirror_get_filter(const pfe_mirror_t *mirror)
{
    uint32_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mirror))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        ret = 0U;
    }
    else
#endif
    {
        ret = oal_ntohl(mirror->phys.flexible_filter);
    }
    /* Set the address of the filter table (convert endian) */

    return ret;
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
    errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mirror))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        ret = EINVAL;
    }
    else
#endif
    {
        mirror->phys.actions = actions;
        if(RT_ACT_NONE != actions)
        {
            (void)memcpy(&mirror->phys.args, args, sizeof(pfe_ct_route_actions_args_t));
        }
        ret = pfe_class_write_dmem(mirror->db->class, -1, mirror->phys_addr, &mirror->phys, sizeof(pfe_ct_mirror_t));
    }

    return ret;
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
    errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == mirror)||(NULL == args)))
	{
        NXP_LOG_ERROR("NULL argument received\n");
        ret = EINVAL;
    }
    else
#endif
    {
        ret = EOK;
        *actions = mirror->phys.actions;
        if(RT_ACT_NONE != mirror->phys.actions)
        {   /* Arguments are needed */
            (void)memcpy(args, &mirror->phys.args, sizeof(pfe_ct_route_actions_args_t));
        }
    }

    return ret;
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

