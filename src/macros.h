#ifndef BTREE_MACROS_H
#define BTREE_MACROS_H

#include <cassert>

/**
 * Always Assert
 */
#ifdef NDEBUG
#define BPLUSTREE_ASSERT(expr, message) ((void)0)
#else
#define BPLUSTREE_ASSERT(expr, message) assert((expr) && (message))
#endif /* NDEBUG */

#endif //BTREE_MACROS_H
