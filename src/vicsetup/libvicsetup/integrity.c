#include "integrity.h"

#include <stdio.h>
#include <string.h>

#include "raise.h"

// Layout: SB | JOURNAL | [ DATA | TAGS ]*
//
// SB is padded out to 4096
// JOURNAL is 88 sectors?
//
// Example: assuming 32 byte tags (16 tags per sector):
//     16 data sectors followed by one tag sector

static uint8_t _magic[8] = { 'i', 'n', 't', 'e', 'g', 'r', 't', '\0' };

static uint64_t _inverse_log2(uint8_t log)
{
    return 1 << (uint64_t)log;
}

vic_result_t vic_read_integrity_sb(
    vic_blockdev_t* device,
    uint64_t offset,
    vic_integrity_sb_t* sb)
{
    vic_result_t result = VIC_UNEXPECTED;
    const uint64_t blkno = offset / VIC_SECTOR_SIZE;
    uint8_t blk[VIC_SECTOR_SIZE];
    size_t block_size;

    CHECK(vic_blockdev_get_block_size(device, &block_size));

    if (block_size != VIC_SECTOR_SIZE)
        RAISE(VIC_BAD_BLOCK_SIZE);

    if (vic_blockdev_get(device, blkno, blk, 1) != 0)
        RAISE(VIC_FAILED);

    memcpy(sb, &blk, sizeof(vic_integrity_sb_t));

    if (memcmp(sb->magic, _magic, sizeof(sb->magic)) != 0)
        RAISE(VIC_NOT_FOUND);

    result = VIC_OK;

done:
    return result;
}

vic_result_t vic_dump_integrity_sb(const vic_integrity_sb_t* sb)
{
    vic_result_t result;

    if (!sb || memcmp(sb->magic, _magic, sizeof(sb->magic)) != 0)
        RAISE(VIC_BAD_PARAMETER);

    printf("vic_luks_integrity_sb\n");
    printf("{\n");
    printf("  magic=%s (%02x %02x %02x %02x %02x %02x %02x %02x)\n", sb->magic,
        sb->magic[0], sb->magic[1], sb->magic[2], sb->magic[3],
        sb->magic[4], sb->magic[5], sb->magic[6], sb->magic[7]);
    printf("  version=%u\n", sb->version);
    printf("  log2_interleave_sectors=%u (%lu)\n",
        sb->log2_interleave_sectors,
        _inverse_log2(sb->log2_interleave_sectors));
    printf("  integrity_tag_size=%u\n", sb->integrity_tag_size);
    printf("  journal_sections=%u\n", sb->journal_sections);
    printf("  provided_data_sectors=%lu\n", sb->provided_data_sectors);
    printf("  flags=%u\n", sb->flags);
    printf("  log2_sectors_per_block=%u (%lu)\n",
        sb->log2_sectors_per_block,
        _inverse_log2(sb->log2_sectors_per_block));
    printf("  log2_blocks_per_bitmap_bit=%u (%lu)\n",
        sb->log2_blocks_per_bitmap_bit,
        _inverse_log2(sb->log2_blocks_per_bitmap_bit));
    printf("  recalc_sector=%lu\n", sb->recalc_sector);
    printf("}\n");

    result = VIC_OK;

done:
    return result;
}

const char* vic_integrity_name(vic_integrity_t integrity)
{
    switch(integrity)
    {
        case VIC_INTEGRITY_NONE:
            return "none";
        case VIC_INTEGRITY_HMAC_AEAD:
            return "aead";
        case VIC_INTEGRITY_HMAC_SHA256:
            return "hmac(sha256)";
        case VIC_INTEGRITY_HMAC_SHA512:
            return "hmac(sha512)";
        case VIC_INTEGRITY_CMAC_AES:
            return "cmac(aes)";
        case VIC_INTEGRITY_POLY1305:
            return "poly1305";
    }

    return NULL;
}

size_t vic_integrity_tag_size(vic_integrity_t integrity)
{
    switch(integrity)
    {
        case VIC_INTEGRITY_NONE:
            return 0;
        case VIC_INTEGRITY_HMAC_AEAD:
            return 16;
        case VIC_INTEGRITY_HMAC_SHA256:
            return 32;
        case VIC_INTEGRITY_HMAC_SHA512:
            return 64;
        case VIC_INTEGRITY_CMAC_AES:
            return 16;
        case VIC_INTEGRITY_POLY1305:
            return 16;
    }

    return 0;
}

vic_integrity_t vic_integrity_enum(const char* str)
{
    if (strcmp(str, "aead") == 0)
        return VIC_INTEGRITY_HMAC_AEAD;
    else if (strcmp(str, "hmac(sha256)") == 0)
        return VIC_INTEGRITY_HMAC_SHA256;
    else if (strcmp(str, "hmac(sha512)") == 0)
        return VIC_INTEGRITY_HMAC_SHA512;
    else if (strcmp(str, "cmac(aes)") == 0)
        return VIC_INTEGRITY_CMAC_AES;
    else if (strcmp(str, "poly1305") == 0)
        return VIC_INTEGRITY_POLY1305;

    return VIC_INTEGRITY_NONE;
}

size_t vic_integrity_tag_size_from_str(const char* integrity)
{
    if (!integrity)
        return (size_t)-1;

    if (strcmp(integrity, "aead") == 0)
        return 16;
    else if (strcmp(integrity, "hmac(sha256)") == 0)
        return 32;
    else if (strcmp(integrity, "hmac(sha512)") == 0)
        return 64;
    else if (strcmp(integrity, "cmac(aes)") == 0)
        return 16;
    else if (strcmp(integrity, "poly1305") == 0)
        return 16;

    return (size_t)-1;
}

size_t vic_integrity_key_size(vic_integrity_t integrity)
{
    switch(integrity)
    {
        case VIC_INTEGRITY_NONE:
            return 0;
        case VIC_INTEGRITY_HMAC_AEAD:
            return 0;
        case VIC_INTEGRITY_HMAC_SHA256:
            return 32;
        case VIC_INTEGRITY_HMAC_SHA512:
            return 64;
        case VIC_INTEGRITY_CMAC_AES:
            return 0;
        case VIC_INTEGRITY_POLY1305:
            return 0;
    }

    return 0;
}

size_t vic_integrity_key_size_from_str(const char* integrity)
{
    if (!integrity)
        return 0;

    if (strcmp(integrity, "aead") == 0)
        return 0;
    else if (strcmp(integrity, "hmac(sha256)") == 0)
        return 32;
    else if (strcmp(integrity, "hmac(sha512)") == 0)
        return 64;
    else if (strcmp(integrity, "cmac(aes)") == 0)
        return 0;
    else if (strcmp(integrity, "poly1305") == 0)
        return 0;

    return 0;
}
