# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


CFLAGS += -DLIST_NO_DUPLICATES
CFLAGS += -DMAP_USE_RBTREE
CFLAGS += -UDELIVERY_BIGTX

# In lib/ MACRO
CFLAGS += -DSERVER_MACRO

# E.T. + PREEMPTION
CFLAGS += -DEXTRA_TICK
CFLAGS += -DPREEMPTION

# For M.M.
CFLAGS += -DGC_PERIOD=1
CFLAGS += -DSINGLE_POOL
#CFLAGS += -DNBLIST_TASK
#CFLAGS += -DNBLIST_STATE

# For P.Q.
CFLAGS += -DBITMAP
#CFLAGS += -DTRY_REMOVE

# For different POLICIES
CFLAGS += -DMAX_SUSPENSIONS=2
CFLAGS += -DSCHED_POLICY
#CFLAGS += -DALWAYS_INC_SCHED_POLICY

# To display PROGRESS
CFLAGS += -DPRINT_PROGRESS

# To collect EXEC TIMEs
#CFLAGS += -DTXS_STATS
#CFLAGS += -DDROP_CONT_TIME

# To collect NO HIGHER PRIO avail
#CFLAGS += -DSCHED_MISS_COUNT


PROG := tpcc

SRCS += \
	arch/atomic.c \
	arch/jmp.S \
	arch/ult.c \
	mm/statepool.c \
	mm/taskpool.c \
	core/nblist.c \
	core/priority.c \
	scheduler/scheduler.c \
	scheduler/preempt_callback.S \
	scheduler/preempt.c \
	stats/stats.c \
	manager.c \
	stm_threadpool.c \
	Server.c \
	dataTable/customer.c \
	dataTable/district.c \
	dataTable/history.c \
	dataTable/item.c \
	dataTable/neworder.c \
	dataTable/order.c \
	dataTable/orderline.c \
	dataTable/stocktable.c \
	dataTable/warehouse.c \
	$(LIB)/list.c \
	$(LIB)/pair.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/random.c \
	$(LIB)/rbtree.c \
	$(LIB)/hashtable.c \
	$(LIB)/thread.c \
#
OBJS := ${SRCS:.c=.o}


# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
