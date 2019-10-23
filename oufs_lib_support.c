/**
 *  Project 3
 *  oufs_lib_support.c
 *
 *  Author: CS3113
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "virtual_disk.h"
#include "oufs_lib_support.h"

extern int debug;

/**
 * Deallocate a single block.
 * - Modify the in-memory copy of the master block
 * - Add the specified block to THE END of the free block linked list
 * - Modify the disk copy of the deallocated block: next_block points to
 *     UNALLOCATED_BLOCK
 *   
 *
 * @param master_block Pointer to a loaded master block.  Changes to the MB will
 *           be made here, but not written to disk
 *
 * @param block_reference Reference to the block that is being deallocated
 *
 */
int oufs_deallocate_block(BLOCK *master_block, BLOCK_REFERENCE block_reference)
{
  BLOCK b;

  if(master_block->content.master.unallocated_front == UNALLOCATED_BLOCK) {
    // No blocks on the free list.  Both pointers point to this block now
    master_block->content.master.unallocated_front = master_block->content.master.unallocated_end =
      block_reference;

  }else{
    // TODO
  }

  // Update the new end block
  if(virtual_disk_read_block(block_reference, &b) != 0) {
    fprintf(stderr, "deallocate_block: error reading new end block\n");
    return(-1);
  }

  // Change the new end block to point to nowhere
  b.next_block = UNALLOCATED_BLOCK;

  // Write the block back
  if(virtual_disk_write_block(block_reference, &b) != 0) {
    fprintf(stderr, "deallocate_block: error writing new end block\n");
    return(-1);
  }

  return(0);
};


/**
 *  Initialize an inode and a directory block structure as a new directory.
 *  - Inode points to directory block (self_block_reference)
 *  - Inode size = 2 (for . and ..)
 *  - Direcory block: add entries . (self_inode_reference and .. (parent_inode_reference)
 *  -- Set all other entries to UNALLOCATED_BLOCK
 *
 * @param inode Pointer to inode structure to initialize
 * @param block Pointer to block structure to initialize as a directory
 * @param self_block_reference The block reference to the new directory block
 * @param self_inode_reference The inode reference to the new inode
 * @param parent_inode_reference The inode reference to the parent inode
 */
void oufs_init_directory_structures(INODE *inode, BLOCK *block,
				    BLOCK_REFERENCE self_block_reference,
				    INODE_REFERENCE self_inode_reference,
				    INODE_REFERENCE parent_inode_reference)
{
  // TODO
}


/**
 *  Given an inode reference, read the inode from the virtual disk.
 *
 *  @param i Inode reference (index into the inode list)
 *  @param inode Pointer to an inode memory structure.  This structure will be
 *                filled in before return)
 *  @return 0 = successfully loaded the inode
 *         -1 = an error has occurred
 *
 */
int oufs_read_inode_by_reference(INODE_REFERENCE i, INODE *inode)
{
  if(debug)
    fprintf(stderr, "\tDEBUG: Fetching inode %d\n", i);

  // Find the address of the inode block and the inode within the block
  BLOCK_REFERENCE block = i / N_INODES_PER_BLOCK + 1;
  int element = (i % N_INODES_PER_BLOCK);

  // Load the block that contains the inode
  BLOCK b;
  if(virtual_disk_read_block(block, &b) == 0) {
    // Successfully loaded the block: copy just this inode
    *inode = b.content.inodes.inode[element];
    return(0);
  }
  // Error case
  return(-1);
}


/**
 * Write a single inode to the disk
 *
 * @param i Inode reference index
 * @param inode Pointer to an inode structure
 * @return 0 if success
 *         -x if error
 *
 */
int oufs_write_inode_by_reference(INODE_REFERENCE i, INODE *inode)
{
  if(debug)
    fprintf(stderr, "\tDEBUG: Writing inode %d\n", i);

  // TODO

  // Success
  return(0);
}

/**
 * Set all of the properties of an inode
 *
 * @param inode Pointer to the inode structure to be initialized
 * @param type Type of inode
 * @param n_references Number of references to this inode
 *          (when first created, will always be 1)
 * @param content Block reference to the block that contains the information within this inode
 * @param size Size of the inode (# of directory entries or size of file in bytes)
 *
 */

void oufs_set_inode(INODE *inode, INODE_TYPE type, int n_references,
		    BLOCK_REFERENCE content, int size)
{
  inode->type = type;
  inode->n_references = n_references;
  inode->content = content;
  inode->size = size;
}


/*
 * Given a valid directory inode, return the inode reference for the sub-item
 * that matches <element_name>
 *
 * @param inode Pointer to a loaded inode structure.  Must be a directory inode
 * @param element_name Name of the directory element to look up
 *
 * @return = INODE_REFERENCE for the sub-item if found; UNALLOCATED_INODE if not found
 */

int oufs_find_directory_element(INODE *inode, char *element_name)
{
  if(debug)
    fprintf(stderr,"\tDEBUG: oufs_find_directory_element: %s\n", element_name);

  // TODO
}

/**
 *  Given a current working directory and either an absolute or relative path, find both the inode of the
 * file or directory and the inode of the parent directory.  If one or both are not found, then they are
 * set to UNALLOCATED_INODE.
 *
 *  This implementation handles a variety of strange cases, such as consecutive /'s and /'s at the end of
 * of the path (we have to maintain some extra state to make this work properly).
 *
 * @param cwd Absolute path for the current working directory
 * @param path Absolute or relative path of the file/directory to be found
 * @param parent Pointer to the found inode reference for the parent directory
 * @param child ointer to the found node reference for the file or directory specified by path
 * @param local_name String name of the file or directory without any path information
 *             (i.e., name relative to the parent)
 * @return 0 if no errors
 *         -1 if child not found
 *         -x if an error
 *
 */
int oufs_find_file(char *cwd, char * path, INODE_REFERENCE *parent, INODE_REFERENCE *child,
		   char *local_name)
{
  INODE_REFERENCE grandparent;
  char full_path[MAX_PATH_LENGTH];

  // Construct an absolute path the file/directory in question
  if(path[0] == '/') {
    strncpy(full_path, path, MAX_PATH_LENGTH-1);
  }else{
    if(strlen(cwd) > 1) {
      strncpy(full_path, cwd, MAX_PATH_LENGTH-1);
      strncat(full_path, "/", 2);
      strncat(full_path, path, MAX_PATH_LENGTH-1-strnlen(full_path, MAX_PATH_LENGTH));
    }else{
      strncpy(full_path, "/", 2);
      strncat(full_path, path, MAX_PATH_LENGTH-2);
    }
  }

  if(debug) {
    fprintf(stderr, "\tDEBUG: Full path: %s\n", full_path);
  };

  // Start scanning from the root directory
  // Root directory inode
  grandparent = *parent = *child = 0;
  if(debug)
    fprintf(stderr, "\tDEBUG: Start search: %d\n", *parent);

  // Parse the full path
  char *directory_name;
  directory_name = strtok(full_path, "/");
  while(directory_name != NULL) {
    if(strlen(directory_name) >= FILE_NAME_SIZE-1) 
      // Truncate the name
      directory_name[FILE_NAME_SIZE - 1] = 0;
    if(debug){
      fprintf(stderr, "\tDEBUG: Directory: %s\n", directory_name);
    }

    // TODO

  };

  // Item found.
  if(*child == UNALLOCATED_INODE) {
    // We went too far - roll back one step ***
    *child = *parent;
    *parent = grandparent;
  }
  if(debug) {
    fprintf(stderr, "\tDEBUG: Found: %d, %d\n", *parent, *child);
  }
  // Success!
  return(0);
} 


/**
 * Return the bit index for the first 0 bit in a byte (starting from 7th bit
 *   and scanning to the right)
 *
 * @param value: a byte
 * @return The bit number of the first 0 in value (starting from the 7th bit
 *         -1 if no zero bit is found
 */

int oufs_find_open_bit(unsigned char value)
{
  // TODO

  // Not found
  return(-1);
}

/**
 *  Allocate a new directory (an inode and block to contain the directory).  This
 *  includes initialization of the new directory.
 *
 * @param parent_reference The inode of the parent directory
 * @return The inode reference of the new directory
 *         UNALLOCATED_INODE if we cannot allocate the directory
 */
int oufs_allocate_new_directory(INODE_REFERENCE parent_reference)
{
  BLOCK block;
  BLOCK block2;
  // Read the master block
  if(virtual_disk_read_block(MASTER_BLOCK_REFERENCE, &block) != 0) {
    // Read error
    return(UNALLOCATED_INODE);
  }

  // TODO
};
