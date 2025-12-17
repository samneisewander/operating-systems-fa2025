/* fs.c: SimpleFS file system */

#include "sfs/fs.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "sfs/logging.h"
#include "sfs/utils.h"

/* External Functions */

/**
 * Debug FileSystem by doing the following:
 *
 *  1. Read SuperBlock and report its information.
 *
 *  2. Read Inode Table and report information about each Inode.
 *
 * @param       disk        Pointer to Disk structure.
 **/
void fs_debug(Disk* disk) {
    Block block = {{0}};

    /* Read SuperBlock */
    if (disk_read(disk, 0, block.data) == DISK_FAILURE) {
        return;
    }

    printf("SuperBlock:\n");
    printf("    magic number is %s\n",
           (block.super.magic_number == MAGIC_NUMBER) ? "valid" : "invalid");
    printf("    %u blocks\n", block.super.blocks);
    printf("    %u inode blocks\n", block.super.inode_blocks);
    printf("    %u inodes\n", block.super.inodes);

    /* Read Inodes */
    Block inode_block = {{0}};
    for (int i = 0; i < block.super.inode_blocks; i++) {
        if (disk_read(disk, i + 1, inode_block.data) == DISK_FAILURE) {
            continue;
        }
        for (int j = 0; j < INODES_PER_BLOCK; j++) {
            Inode inode = inode_block.inodes[j];
            if (!inode.valid) {
                continue;
            }
            printf("Inode %d:\n", j);
            printf("    size: %d bytes\n", inode.size);
            printf("    direct blocks:");

            // print direct blocks
            int num_blocks = (inode.size + (BLOCK_SIZE - 1)) / BLOCK_SIZE;
            for (int k = 0; k < POINTERS_PER_INODE; k++) {
                if (k >= num_blocks) {
                    continue;
                }
                printf(" %d", inode.direct[k]);
            }
            printf("\n");
            if (num_blocks <= POINTERS_PER_INODE) {
                continue;
            }
            Block indirect_block = {{0}};
            if (disk_read(disk, inode.indirect, indirect_block.data) == DISK_FAILURE) {
                continue;
            }

            printf("    indirect block: %d\n", inode.indirect);
            printf("    indirect data blocks:");

            // print indirect blocks
            for (int k = 0; k < num_blocks - POINTERS_PER_INODE; k++) {
                printf(" %d", indirect_block.pointers[k]);
            }
            printf("\n");
        }
    }
}

/**
 * Format Disk by doing the following:
 *
 *  1. Write SuperBlock (with appropriate magic number, number of blocks,
 *  number of inode blocks, and number of inodes).
 *
 *  2. Clear all remaining blocks.
 *
 * Note: Do not format a mounted Disk!
 *
 * @param       fs      Pointer to FileSystem structure.
 * @param       disk    Pointer to Disk structure.
 * @return      Whether or not all disk operations were successful.
 **/
bool fs_format(FileSystem* fs, Disk* disk) {
    if (fs->disk) {
        fprintf(stderr, "Cannot format a mounted disk.\n");
        return false;
    }

    if (disk->blocks < 3) {
        fprintf(stderr, "Cannot format disk with fewer than 3 blocks.\n");
        return false;
    }

    fs->meta_data.magic_number = MAGIC_NUMBER;
    fs->meta_data.blocks = disk->blocks;

    float inode_blocks = (float)fs->meta_data.blocks / 10;
    fs->meta_data.inode_blocks = ceil(inode_blocks);
    fs->meta_data.inodes = fs->meta_data.inode_blocks * INODES_PER_BLOCK;

    Block zeros = {0};
    for (int i = 1; i < fs->meta_data.blocks; i++) {
        if (disk_write(disk, i, zeros.data) == DISK_FAILURE) {
            fprintf(stderr, "Failed to overwrite block %d during formatting.\n", i);
            return false;
        }
    }

    return true;
}

/**
 * Mount specified FileSystem to given Disk.
 *
 * @param       fs      Pointer to FileSystem structure.
 * @param       disk    Pointer to Disk structure.
 * @return      Whether or not the mount operation was successful.
 **/
bool fs_mount(FileSystem* fs, Disk* disk) {
    // Do not mount a Disk that has already been mounted!
    if (fs->disk) {
        fprintf(stderr, "You fool. You have already mounted this disk! Mount something else.\n");
        return false;
    }

    // 1. Read and check SuperBlock (verify attributes).
    Block superblock = {0};
    if (disk_read(disk, 0, superblock.data) == DISK_FAILURE) {
        return false;
    }

    if (superblock.super.magic_number != MAGIC_NUMBER) {
        fprintf(stderr, "The Wizard doesn't like your magic.\n");
        return false;
    }

    if (superblock.super.inode_blocks < 1 || superblock.super.blocks < 3 || superblock.super.inodes < 1) {
        fprintf(stderr, "YOUUUU SHALLLL NOOOOOT MOUUUUUUUUUUNT!!.\n");
        return false;
    }

    if (superblock.super.inodes != superblock.super.inode_blocks * INODES_PER_BLOCK) {
        fprintf(stderr, "YOUUUU SHALLLL NOOOOOT MOUUUUUUUUUUNT!!.\n");
        return false;
    }

    // 2. Record FileSystem disk attribute.
    fs->disk = disk;

    // 3. Copy SuperBlock to FileSystem meta data attribute
    fs->meta_data.blocks = superblock.super.blocks;
    fs->meta_data.inode_blocks = superblock.super.inode_blocks;
    fs->meta_data.inodes = superblock.super.inodes;
    fs->meta_data.magic_number = superblock.super.magic_number;

    // 4. Initialize FileSystem free blocks bitmap.
    fs->free_blocks = calloc(fs->meta_data.blocks, sizeof(bool));
    memset(fs->free_blocks, 1, fs->meta_data.blocks * sizeof(bool));

    // Remove superblock from freelist
    fs->free_blocks[0] = false;

    // Walk the inode blocks
    for (int i = 1; i < fs->meta_data.inode_blocks + 1; i++) {
        Block inode_block = {0};
        fs->free_blocks[i] = false;

        if (disk_read(disk, i, inode_block.data) == DISK_FAILURE) {
            return false;
        }
        // Walk the inodes in this block
        for (int j = 0; j < INODES_PER_BLOCK; j++) {
            Inode inode = inode_block.inodes[j];
            if (!inode.valid) {
                continue;
            }

            // Remove direct blocks in use by this inode from the free list
            for (int k = 0; k < POINTERS_PER_INODE; k++) {
                if (inode.direct[k] > 0) {
                    fs->free_blocks[inode.direct[k]] = false;
                }
            }

            // Similarly, remove indirect block and valid indirect data blocks from the free list
            if (inode.indirect < 1) {
                continue;
            }

            fs->free_blocks[inode.indirect] = false;

            Block indirect_block = {0};
            if (disk_read(disk, inode.indirect, indirect_block.data) == DISK_FAILURE) {
                return false;
            }

            for (int k = 0; k < POINTERS_PER_BLOCK; k++) {
                if (indirect_block.pointers[k] > 0) {
                    fs->free_blocks[indirect_block.pointers[k]] = false;
                }
            }
        }
    }

    return true;
}

/**
 * Unmount FileSystem from internal Disk by doing the following:
 *
 *  1. Set FileSystem disk attribute.
 *
 *  2. Release free blocks bitmap.
 *
 * @param       fs      Pointer to FileSystem structure.
 **/
void fs_unmount(FileSystem* fs) {
    fs->disk = NULL;
    free(fs->free_blocks);
    fs->free_blocks = NULL;
}

/**
 * Allocate an Inode in the FileSystem Inode table by doing the following:
 *
 *  1. Search Inode table for free inode.
 *
 *  2. Reserve free inode in Inode table.
 *
 * Note: Be sure to record updates to Inode table to Disk.
 *
 * @param       fs      Pointer to FileSystem structure.
 * @return      Inode number of allocated Inode.
 **/
ssize_t fs_create(FileSystem* fs) {
    Block inode_table = {0};
    for (int i = 1; i < fs->meta_data.inode_blocks + 1; i++) {
        if (disk_read(fs->disk, i, inode_table.data) == DISK_FAILURE) {
            fprintf(stderr, "uh oh\n");
            return -1;
        }

        for (int j = 0; j < INODES_PER_BLOCK; j++) {
            if (inode_table.inodes[j].valid == 0) {
                // Found an inode!
                inode_table.inodes[j].valid = 1;
                if (disk_write(fs->disk, i, inode_table.data) == DISK_FAILURE) {
                    fprintf(stderr, "uh oh\n");
                    return -1;
                }

                return j;
            }
        }
    }

    // Couldn't find an inode. Darn!
    return -1;
}

/**
 * Remove Inode and associated data from FileSystem by doing the following:
 *
 *  1. Load and check status of Inode.
 *
 *  2. Release any direct blocks.
 *
 *  3. Release any indirect blocks.
 *
 *  4. Mark Inode as free in Inode table.
 *
 * @param       fs              Pointer to FileSystem structure.
 * @param       inode_number    Inode to remove.
 * @return      Whether or not removing the specified Inode was successful.
 **/
bool fs_remove(FileSystem* fs, size_t inode_number) {
    Block zeros = {0};
    Inode inode = {0};

    if (!load_inode(&inode, inode_number, fs->disk)) {
        return false;
    }

    // Release indirect block and indirect data blocks from the free list
    if (inode.indirect > 0) {
        Block indirect_block = {0};
        if (disk_read(fs->disk, inode.indirect, indirect_block.data) == DISK_FAILURE) {
            return false;
        }

        for (int k = 0; k < POINTERS_PER_BLOCK; k++) {
            if (indirect_block.pointers[k] > 0) {
                if (disk_write(fs->disk, indirect_block.pointers[k], zeros.data) == DISK_FAILURE) {
                    return false;
                }
                fs->free_blocks[indirect_block.pointers[k]] = true;
            }
        }

        if (disk_write(fs->disk, inode.indirect, zeros.data) == DISK_FAILURE) {
            return false;
        }

        fs->free_blocks[inode.indirect] = true;
    }

    // Release direct blocks in use by this inode
    for (int k = 0; k < POINTERS_PER_INODE; k++) {
        if (inode.direct[k] > 0) {
            if (disk_write(fs->disk, inode.direct[k], zeros.data) == DISK_FAILURE) {
                return false;
            }
            fs->free_blocks[inode.direct[k]] = true;
        }
    }

    // Set inode invalid
    Inode new_inode = {0};
    save_inode(&new_inode, inode_number, fs->disk);

    return true;
}

/**
 * Return size of specified Inode.
 *
 * @param       fs              Pointer to FileSystem structure.
 * @param       inode_number    Inode to remove.
 * @return      Size of specified Inode (-1 if does not exist).
 **/
ssize_t fs_stat(FileSystem* fs, size_t inode_number) {
    Inode inode = {0};

    if (!load_inode(&inode, inode_number, fs->disk)) {
        return -1;
    }

    if (inode.valid) {
        return inode.size;
    } else {
        return -1;
    }
}

/**
 * Read from the specified Inode into the data buffer exactly length bytes
 * beginning from the specified offset by doing the following:
 *
 *  1. Load Inode information.
 *
 *  2. Continuously read blocks and copy data to buffer.
 *
 *  Note: Data is read from direct blocks first, and then from indirect blocks.
 *
 * @param       fs              Pointer to FileSystem structure.
 * @param       inode_number    Inode to read data from.
 * @param       data            Buffer to copy data to.
 * @param       length          Number of bytes to read.
 * @param       offset          Byte offset from which to begin reading.
 * @return      Number of bytes read (-1 on error).
 **/
ssize_t fs_read(FileSystem* fs, size_t inode_number, char* data, size_t length, size_t offset) {
    Inode inode = {0};
    if (!load_inode(&inode, inode_number, fs->disk)) {
        return -1;
    }

    // Return successful read of 0 if read is not possible.
    if (inode.size < 1 || offset >= inode.size) {
        return 0;
    }

    // Truncate requested length to the file size.
    length = min(inode.size - offset, length);

    // Determine which logical data block to begin the read at.
    uint32_t start_block = offset / BLOCK_SIZE;
    uint32_t offset_into_block = offset % BLOCK_SIZE;

    ssize_t bytes_read = 0;

    for (uint32_t i = start_block; length > 0; i++) {
        // Determine how many bytes to read from this data block.
        ssize_t length_to_read;
        if (i == 0) {
            length_to_read = min(BLOCK_SIZE - offset, length);
        } else {
            length_to_read = min(BLOCK_SIZE, length);
        }

        // Load the data block and read its contents.
        Block data_block = {0};
        if (i < POINTERS_PER_INODE) {
            // Read from direct block
            if (disk_read(fs->disk, inode.direct[i], data_block.data) == DISK_FAILURE) {
                return -1;
            }
            memcpy(data + bytes_read, data_block.data + offset_into_block, length_to_read);
        } else {
            // Read from indirect block
            Block indirect_block = {0};
            if (disk_read(fs->disk, inode.indirect, indirect_block.data) == DISK_FAILURE) {
                return -1;
            }
            if (disk_read(fs->disk, indirect_block.pointers[i - POINTERS_PER_INODE], data_block.data) == DISK_FAILURE) {
                return -1;
            }
            memcpy(data + bytes_read, data_block.data + offset_into_block, length_to_read);
        }

        // If we have reached the end of the length requested to read, break early.
        bytes_read += length_to_read;
        length -= length_to_read;
        offset_into_block = 0;
    }

    return bytes_read;
}

/**
 * Write to the specified Inode from the data buffer exactly length bytes
 * beginning from the specified offset by doing the following:
 *
 *  1. Load Inode information.
 *
 *  2. Continuously copy data from buffer to blocks.
 *
 *  Note: Data is read from direct blocks first, and then from indirect blocks.
 *
 * @param       fs              Pointer to FileSystem structure.
 * @param       inode_number    Inode to write data to.
 * @param       data            Buffer with data to copy
 * @param       length          Number of bytes to write.
 * @param       offset          Byte offset from which to begin writing.
 * @return      Number of bytes read (-1 on error).
 **/
ssize_t fs_write(FileSystem* fs, size_t inode_number, char* data, size_t length, size_t offset) {
    Inode inode = {0};
    if (!load_inode(&inode, inode_number, fs->disk)) {
        return -1;
    }

    ssize_t bytes_written = 0;

    // Calculate the start and end indices of the data blocks to be written.
    uint32_t start_block = offset / BLOCK_SIZE;
    uint32_t offset_into_block = offset % BLOCK_SIZE;
    uint32_t end_block = (length + offset) / BLOCK_SIZE;
    if ((length + offset) % BLOCK_SIZE > 0) {
        end_block++;
    }

    for (uint32_t i = start_block; i < end_block; i++) {
        /**
         * Determine the number of bytes to write to this data block.
         * Case 1: Data block 0. If there is an offset, we need to write starting at the offset to the end of the block, not exceeding the requested write length.
         * Case 2: Anything else. Write up to the requested length, not exceeding block size.
         */
        ssize_t length_to_write;
        if (i == 0) {
            length_to_write = min(BLOCK_SIZE - offset, length);
        } else {
            length_to_write = min(BLOCK_SIZE, length);
        }

        Block data_block = {0};
        memcpy(data_block.data + offset_into_block, data + bytes_written, length_to_write);

        if (i < POINTERS_PER_INODE) {
            // Write to direct block, allocate new data block if needed.
            if (inode.direct[i] == 0) {
                uint32_t allocated_block = allocate_free_block(fs);
                if (allocated_block == 0) {
                    fprintf(stderr, "Couldn't allocate direct block %d\n", inode.direct[i]);
                    goto fs_write_exit;
                }
                inode.direct[i] = allocated_block;
            }

            if (disk_write(fs->disk, inode.direct[i], data_block.data) == DISK_FAILURE) {
                fprintf(stderr, "Couldn't write to direct block %d\n", inode.direct[i]);
                return -1;
            }
        } else {
            // Write to indirect block, allocated indirct block if needed, allocated data block if needed.

            // 1. If indirect block doesn't already exist, allocate one.
            Block indirect_block = {0};
            if (inode.indirect == 0) {
                uint32_t allocated_block = allocate_free_block(fs);
                if (allocated_block == 0) {
                    fprintf(stderr, "Couldn't allocate indirect block.\n");
                    goto fs_write_exit;
                }
                inode.indirect = allocated_block;
            } else {
                // 2. Otherwise, read the existing one.
                if (disk_read(fs->disk, inode.indirect, indirect_block.data) == DISK_FAILURE) {
                    fprintf(stderr, "Couldn't read indirect data block.\n");
                    return -1;
                }
            }

            // 3. In the indirect block, allocate a data block if necessary..
            if (indirect_block.pointers[i - POINTERS_PER_INODE] == 0) {
                uint32_t allocated_block = allocate_free_block(fs);
                if (allocated_block == 0) {
                    fprintf(stderr, "Couldn't allocate indirect data block. Got %d\n", allocated_block);
                    goto fs_write_exit;
                }
                indirect_block.pointers[i - POINTERS_PER_INODE] = allocated_block;
                if (disk_write(fs->disk, inode.indirect, indirect_block.data) == DISK_FAILURE) {
                    fprintf(stderr, "Couldn't update indirect block %d.\n", indirect_block.pointers[i - POINTERS_PER_INODE]);
                    return -1;
                }
            }

            // 3. Write the data block.
            if (disk_write(fs->disk, indirect_block.pointers[i - POINTERS_PER_INODE], data_block.data) == DISK_FAILURE) {
                fprintf(stderr, "Couldn't write to indirect data block %d.\n", indirect_block.pointers[i - POINTERS_PER_INODE]);
                return -1;
            }
        }

        bytes_written += length_to_write;
        length -= length_to_write;
        offset_into_block = 0;
    }

    // Compute the new size of the inode and save it.
fs_write_exit:
    inode.size = max(offset + bytes_written, inode.size);
    if (!save_inode(&inode, inode_number, fs->disk)) {
        return -1;
    }

    return bytes_written;
}

/**
 * Helper function that reads the inode with number inumber from disk, saving into the passed inode structure.
 * @param inode Inode structure into which disk contents should be read.
 * @param inumber The logical inode number of the inode we wish to load from disk.
 * @param disk The disk structure representing the disk on which the desired inode resides.
 * @returns On success, returns true and places the desired inode into the passed inode structure. On failure, returns false.
 */
bool load_inode(Inode* inode, size_t inumber, Disk* disk) {
    /* On disk, inodes are saved in a series of blocks starting at logical block 1 (since the superblock resides in block 0) and continuing through to block N + 1, where N is the number of inode blocks. To read the correct block given a logical inode number, we need to compute the block in which the inode resides as well as the inode's offset within that block. -SN */
    int iblock = (inumber / INODES_PER_BLOCK) + 1;
    int ioffset = inumber % INODES_PER_BLOCK;

    Block inode_block = {0};

    if (disk_read(disk, iblock, inode_block.data) == DISK_FAILURE) {
        return false;
    }

    Inode disk_inode = inode_block.inodes[ioffset];

    if (!disk_inode.valid) {
        fprintf(stderr, "Cannot load invalid inode.\n");
        return false;
    }

    inode->valid = disk_inode.valid;
    inode->size = disk_inode.size;
    inode->indirect = disk_inode.indirect;

    for (int i = 0; i < POINTERS_PER_INODE; i++) {
        inode->direct[i] = disk_inode.direct[i];
    }
    // *inode = disk_inode;

    return true;
}

/**
 * Writes an structure inode to a specified inode number on a specified disk.
 *
 * @returns Whether or not the save completed successfully
 */
bool save_inode(Inode* inode, size_t inumber, Disk* disk) {
    int iblock = (inumber / INODES_PER_BLOCK) + 1;
    int ioffset = inumber % INODES_PER_BLOCK;

    Block inode_block = {0};

    if (disk_read(disk, iblock, inode_block.data) == DISK_FAILURE) {
        return false;
    }

    inode_block.inodes[ioffset].valid = inode->valid;
    inode_block.inodes[ioffset].size = inode->size;
    inode_block.inodes[ioffset].indirect = inode->indirect;

    for (int i = 0; i < POINTERS_PER_INODE; i++) {
        inode_block.inodes[ioffset].direct[i] = inode->direct[i];
    }

    if (disk_write(disk, iblock, inode_block.data) == DISK_FAILURE) {
        return false;
    }

    return true;
}

uint32_t allocate_free_block(FileSystem* fs) {
    for (uint32_t i = 1; i < fs->meta_data.blocks; i++) {
        if (fs->free_blocks[i]) {
            fs->free_blocks[i] = false;
            return i;
        }
    }

    return 0;
}
/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
