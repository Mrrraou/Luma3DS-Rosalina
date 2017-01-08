// Taken from ctrulib

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/// The maximum value of a u64.
#define U64_MAX	UINT64_MAX

/// would be nice if newlib had this already
#ifndef SSIZE_MAX
#ifdef SIZE_MAX
#define SSIZE_MAX ((SIZE_MAX) >> 1)
#endif
#endif

typedef uint8_t u8;   ///<  8-bit unsigned integer
typedef uint16_t u16; ///< 16-bit unsigned integer
typedef uint32_t u32; ///< 32-bit unsigned integer
typedef uint64_t u64; ///< 64-bit unsigned integer

typedef int8_t s8;   ///<  8-bit signed integer
typedef int16_t s16; ///< 16-bit signed integer
typedef int32_t s32; ///< 32-bit signed integer
typedef int64_t s64; ///< 64-bit signed integer

typedef volatile u8 vu8;   ///<  8-bit volatile unsigned integer.
typedef volatile u16 vu16; ///< 16-bit volatile unsigned integer.
typedef volatile u32 vu32; ///< 32-bit volatile unsigned integer.
typedef volatile u64 vu64; ///< 64-bit volatile unsigned integer.

typedef volatile s8 vs8;   ///<  8-bit volatile signed integer.
typedef volatile s16 vs16; ///< 16-bit volatile signed integer.
typedef volatile s32 vs32; ///< 32-bit volatile signed integer.
typedef volatile s64 vs64; ///< 64-bit volatile signed integer.

typedef u32 Handle;                 ///< Resource handle.
typedef s32 Result;                 ///< Function result.

/// Creates a bitmask from a bit number.
#define BIT(n) (1U<<(n))

/// Aligns a struct (and other types?) to m, making sure that the size of the struct is a multiple of m.
#define ALIGN(m)   __attribute__((aligned(m)))
/// Packs a struct (and other types?) so it won't include padding bytes.
#define PACKED      __attribute__((packed))
#define USED        __attribute__((used))
#define UNUSED      __attribute__((unused))
/// Packs a system version from its components.
#define SYSTEM_VERSION(major, minor, revision) \
    (((major)<<24)|((minor)<<16)|((revision)<<8))

#define MPCORE_REGS_BASE        (0x17E00000 | (1u << 31))
#define MPCORE_SCU_CFG          (*(vu32 *)(MPCORE_REGS_BASE + 4))
#define MPCORE_INT_ACK          (*(vu32 *)(MPCORE_REGS_BASE + 0x10C))

#define MPCORE_GID_REGS_BASE    (MPCORE_REGS_BASE + 0x1000)
#define MPCORE_GID_SGI          (*(vu32 *)(MPCORE_GID_REGS_BASE + 0xF00))

#define PDN_REGS_BASE           (0x10140000 | (1u << 31))
#define PDN_MPCORE_CFG          (*(vu16 *)(PDN_REGS_BASE + 0xFFC))
#define PDN_MPCORE_CLKCNT       (*(vu16 *)(PDN_REGS_BASE + 0x1300))

#define L2C_REGS_BASE           (0x17E10000 | (1u << 31))
#define L2C_CTRL                (*(vu32 *)(L2C_REGS_BASE + 0x100))
