#pragma once

#include <3ds/types.h>

/**
 * @brief Executes a function in supervisor mode, using the supervisor-mode stack.
 * @param func Function to execute.
 * @param ... Function parameters, up to 3 registers.
*/
Result svcCustomBackdoor(void *func, ...);

/**
 * @brief Gives the physical address corresponding to a virtual address.
 * @param[out] attributes Memory attributes, see ARM documentation.
 * @param VA Virtual address.
 * @param writeCheck whether to check if the VA is writable in supervisor mode
 * @return The corresponding physical address, or NULL.
*/
void *svcConvertVAToPA(u32 *attributes, const void *VA, bool writeCheck);

/**
 * @brief Flushes a range of the data cache (L2C included).
 * @param addr Start address.
 * @param len Length of the range.
*/
void svcFlushDataCacheRange(void *addr, u32 len);

/**
 * @brief Flushes the data cache entirely (L2C included).
*/
void svcFlushEntireDataCache(void);

/**
 * @brief Invalidates a range of the instruction cache.
 * @param addr Start address.
 * @param len Length of the range.
*/
void svcInvalidateInstructionCacheRange(void *addr, u32 len);

/**
 * @brief Invalidates the data cache entirely.
*/
void svcInvalidateEntireInstructionCache(void);

/**
 * @brief Maps a block of process memory.
 * @param process Handle of the process.
 * @param destAddress Address of the mapped block in the current process.
 * @param srcAddress Address of the mapped block in the source process.
 * @param size Size of the block of the memory to map (truncated to a multiple of 0x1000 bytes).
*/
Result svcMapProcessMemoryWithSource(Handle process, u32 destAddr, u32 srcAddr, u32 size);

/**
 * @brief Controls memory mapping, with the choice to use region attributes or not
 * @param[out] addr_out The virtual address resulting from the operation. Usually the same as addr0.
 * @param addr0    The virtual address to be used for the operation.
 * @param addr1    The virtual address to be (un)mirrored by @p addr0 when using @ref MEMOP_MAP or @ref MEMOP_UNMAP.
 *                 It has to be pointing to a RW memory.
 *                 Use NULL if the operation is @ref MEMOP_FREE or @ref MEMOP_ALLOC.
 * @param size     The requested size for @ref MEMOP_ALLOC and @ref MEMOP_ALLOC_LINEAR.
 * @param op       Operation flags. See @ref MemOp.
 * @param perm     A combination of @ref MEMPERM_READ and @ref MEMPERM_WRITE. Using MEMPERM_EXECUTE will return an error.
 *                 Value 0 is used when unmapping memory.
 * @param isLoader Whether to use the region attributes
 * If a memory is mapped for two or more addresses, you have to use MEMOP_UNMAP before being able to MEMOP_FREE it.
 * MEMOP_MAP will fail if @p addr1 was already mapped to another address.
 *
 * @sa svcControlMemory
 */
Result svcControlMemoryEx(u32* addr_out, u32 addr0, u32 addr1, u32 size, MemOp op, MemPerm perm, bool isLoader);