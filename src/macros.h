/**
 * https://github.com/cmu-db/noisepage/blob/79276e68fe83322f1249e8a8be96bd63c583ae56/src/include/common/macros.h#L36-L45
 */

#ifdef NDEBUG
#define BTREE_ASSERT(expr, message) ((void)0)
#else
#define BTREE_ASSERT(expr, message) ((expr) && (message))
#endif
