#pragma once

#include <3ds/types.h>

/**
 * @brief Executes a function in supervisor mode, using the supervisor-mode stack.
 * @param func Function to execute.
 * @param ... Function parameters, up to 3.
*/
Result svcCustomBackdoor(void *func, ...);

/**
 * @brief Gives the physical address corresponding to a virtual address.
 * @param VA Virtual address.
 * @return The corresponding physical address, or NULL.
*/
void *svcConvertVAToPA(const void *VA);

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
