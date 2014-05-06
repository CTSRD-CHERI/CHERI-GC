
#ifndef CHERI_EPHEMERAL_H
#define CHERI_EPHEMERAL_H

#include "gc_config.h"
#ifdef GC_EPHEMERAL_TRAP_SUPPORT

// By David Chisnall

/**
 * A structure containing the source and destination for a capability store.
 */
struct capstore
{
	/**
	 * The capability that is being stored.
	 */
	__capability void *src;
	/**
	 * The address (within the process's global address space) where the
	 * capability is being stored.
	 */
	__capability void **dst;
};
/**
 * Handler for capability stores.  Note that this will be called from a signal
 * context and so *must not* call any async signal unsafe functions.
 */
typedef void (*cheri_store_handler)(struct capstore);
/**
 * Registers a handler for unauthorised ephemeral capability stores. 
 */
int setupEphemeralEmulation(cheri_store_handler handler);

#endif // GC_EPHEMERAL_TRAP_SUPPORT

#endif
