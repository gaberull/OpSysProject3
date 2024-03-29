/**
 *  Project 3
 *  oufs_lib.c
 *
 *  Author: CS3113
 *
 */

#include "oufs_lib.h"
#include "oufs_lib_support.h"
#include "virtual_disk.h"

// Yes ... a global variable
int debug = 1;

// Translate inode types to descriptive strings
const char *INODE_TYPE_NAME[] = {"UNUSED", "DIRECTORY", "FILE"};

/**
 Read the OUFS_PWD, OUFS_DISK, OUFS_PIPE_NAME_BASE environment
 variables copy their values into cwd, disk_name an pipe_name_base.  If these
 environment variables are not set, then reasonable defaults are
 given.
 @param cwd String buffer in which to place the OUFS current working directory.
 @param disk_name String buffer in which to place file name of the virtual disk.
 @param pipe_name_base String buffer in which to place the base name of the
 named pipes for communication to the server.
 PROVIDED
 */
void oufs_get_environment(char *cwd, char *disk_name,
                          char *pipe_name_base)
{
    // Current working directory for the OUFS
    char *str = getenv("OUFS_PWD");
    if(str == NULL) {
        // Provide default
        strcpy(cwd, "/");
    }else{
        // Exists
        strncpy(cwd, str, MAX_PATH_LENGTH-1);
    }
    
    // Virtual disk location
    str = getenv("OUFS_DISK");
    if(str == NULL) {
        // Default
        strcpy(disk_name, "vdisk1");
    }else{
        // Exists: copy
        strncpy(disk_name, str, MAX_PATH_LENGTH-1);
    }
    
    // Pipe name base
    str = getenv("OUFS_PIPE_NAME_BASE");
    if(str == NULL) {
        // Default
        strcpy(pipe_name_base, "pipe");
    }else{
        // Exists: copy
        strncpy(pipe_name_base, str, MAX_PATH_LENGTH-1);
    }
    
}

/**
 * Completely format the virtual disk (including creation of the space).
 *
 * NOTE: this function attaches to the virtual disk at the beginning and
 *  detaches after the format is complete.
 *
 * - Zero out all blocks on the disk.
 * - Initialize the master block: mark inode 0 as allocated and initialize
 *    the linked list of free blocks
 * - Initialize root directory inode
 * - Initialize the root directory in block ROOT_DIRECTORY_BLOCK
 *
 * @return 0 if no errors
 *         -x if an error has occurred.
 *
 */

int oufs_format_disk(char  *virtual_disk_name, char *pipe_name_base)
{
    // Attach to the virtual disk
    if(virtual_disk_attach(virtual_disk_name, pipe_name_base) != 0) {
        return(-1);
    }
    
    BLOCK block;
    
    // Zero out the block
    memset(&block, 0, BLOCK_SIZE);
    for(BLOCK_REFERENCE i = 0; i < N_BLOCKS; ++i) {
        if(virtual_disk_write_block(i, &block) < 0) {
            return(-2);
        }
    }
    
    //////////////////////////////
    // Master block
    block.next_block = UNALLOCATED_BLOCK;
    block.content.master.inode_allocated_flag[0] = 0x80;
    // configure front and end references
    block.content.master.unallocated_front = N_INODE_BLOCKS+2; // this will be block #6
    block.content.master.unallocated_end = N_BLOCKS-1;    // will be block # 127
    // write master block to virtual disk
    if (virtual_disk_write_block(0, &block)<0)
    {
        return -2;
    }
    
    //  clear block again
    memset(&block, 0, BLOCK_SIZE);
    
    //////////////////////////////
    // Root directory inode / block
    INODE inode;
    // ROOT_DIRECTORY_BLOCK is block #5
    oufs_init_directory_structures(&inode, &block, ROOT_DIRECTORY_BLOCK,
                                   ROOT_DIRECTORY_INODE, ROOT_DIRECTORY_INODE);
    
    // Write the results to the disk
    if(oufs_write_inode_by_reference(0, &inode) != 0)
    {
        return(-3);
    }
    
    // write the directory block to disk
    if (virtual_disk_write_block(ROOT_DIRECTORY_BLOCK, &block)<0)
    {
        return -2;
    }
    
    //////////////////////////////
    // All other blocks are free blocks
    // for loop to initialize linked list of free blocks
    for (BLOCK_REFERENCE i=6; i<N_BLOCKS; i++)
    {
        memset(&block, 0, BLOCK_SIZE);
        if (i == 127)
        {
            block.next_block = UNALLOCATED_BLOCK;
        }
        else
            block.next_block = i+1;
        // Write each block to the virtual disk
        if (virtual_disk_write_block(i, &block)<0)
        {
            return -2;
        }
    }
    // Done
    virtual_disk_detach();
    
    return(0);
}

/*
 * Compare two inodes for sorting, handling the
 *  cases where the inodes are not valid
 *
 * @param e1 Pointer to a directory entry
 * @param e2 Pointer to a directory entry
 * @return -1 if e1 comes before e2 (or if e1 is the only valid one)
 * @return  0 if equal (or if both are invalid)
 * @return  1 if e1 comes after e2 (or if e2 is the only valid one)
 *
 * Note: this function is useful for qsort()
 */
static int inode_compare_to(const void *d1, const void *d2)
{
    // Type casting from generic to DIRECTORY_ENTRY*
    DIRECTORY_ENTRY* e1 = (DIRECTORY_ENTRY*) d1;
    DIRECTORY_ENTRY* e2 = (DIRECTORY_ENTRY*) d2;
    
    // TODO: complete implementation
    // if e1 comes before e2 or if e1 is only valid one
    int e1valid = 1;
    int e2valid = 1;
    if (e1->inode_reference == UNALLOCATED_INODE)
        e1valid = 0;
    if (e2->inode_reference == UNALLOCATED_INODE)
        e2valid = 0;
    
    if (e1valid)
    {
        if (e2valid)
        {
            // both valid. compare e1 and e2
            if (strcmp(e1->name, e2->name) < 0)
            {
                return -1;
            }
            else if (strcmp(e1->name, e2->name) > 0)
            {
                return 1;
            }
            else return 0;
        }
        else    // e1 is valid, e2 is invalid
        {
            return -1;
        }
    }
    else    // e1 is invalid
    {
        if (e2valid)    // e1 invalid, e2 valid
        {
            return 1;
        }
        else        // both invalid
        {
            return 0;
        }
    }
}


/**
 * Print out the specified file (if it exists) or the contents of the
 *   specified directory (if it exists)
 *
 * If a directory is listed, then the valid contents are printed in sorted order
 *   (as defined by strcmp()), one per line.  We know that a directory entry is
 *   valid if the inode_reference is not UNALLOCATED_INODE.
 *   Hint: qsort() will do the sort for you.  You just have to provide a compareTo()
 *   function (just like in Java!)
 *   Note: if an entry is a directory itself, then its name must be followed by "/"
 *
 * @param cwd Absolute path representing the current working directory
 * @param path Absolute or relative path to the file/directory
 * @return 0 if success
 *         -x if error
 *
 */

int oufs_list(char *cwd, char *path)
{
    INODE_REFERENCE parent;
    INODE_REFERENCE child;
    
    // Look up the inodes for the parent and child
    int ret = oufs_find_file(cwd, path, &parent, &child, NULL);
    
    // Did we find the specified file?
    if(ret == 0 && child != UNALLOCATED_INODE) {
        // Element found: read the inode
        INODE inode;
        if(oufs_read_inode_by_reference(child, &inode) != 0) {
            return(-1);
        }
        if(debug)
            fprintf(stderr, "\tDEBUG: Child found (type=%s).\n",  INODE_TYPE_NAME[inode.type]);
        
        // TODO: complete implementation
        BLOCK b;
        memset(&b, 0, sizeof(BLOCK));
        virtual_disk_read_block(inode.content, &b);
        // Have the child inode
        // check if it is a directory or a file inode
        if (inode.type == DIRECTORY_TYPE)
        {
            // Don't need pointer below?? According to TA
            qsort(b.content.directory.entry, N_DIRECTORY_ENTRIES_PER_BLOCK, sizeof(b.content.directory.entry[0]), inode_compare_to);
            for (int i = 0; i < N_DIRECTORY_ENTRIES_PER_BLOCK; i++)
            {
                // May be holes in the directory. So must check whole list. Not just inode.size many
                if (b.content.directory.entry[i].inode_reference != UNALLOCATED_INODE)
                {
                    // TODO: STill need to check if the entry is a directory and if so, add a / to the end
                    // TODO: check if I can write over the inode at this point
                    // check to see if inode_reference of the entry is a directory or not
                    oufs_read_inode_by_reference(b.content.directory.entry[i].inode_reference, &inode);
                    if (inode.type == DIRECTORY_TYPE)
                    {
                        // add a slash to the directory name
                        printf("%s/\n", b.content.directory.entry[i].name);
                    }
                    else
                    {
                        printf("%s\n", b.content.directory.entry[i].name);
                    }
                    
                }
            }
        }
        else if (inode.type == FILE_TYPE)
        {
            for (int n = 0; n<DATA_BLOCK_SIZE; n++)
            {
                // data[n] is unsigned char. So i think a printf with %c should do it
                printf("%c\n", b.content.data.data[n]);
            }
        }
        else
            return (-2);
    }
    else
    {
        // Did not find the specified file/directory
        fprintf(stderr, "Not found\n");
        if(debug)
            fprintf(stderr, "\tDEBUG: (%d)\n", ret);
    }
    // Done: return the status from the search
    return(ret);
}




///////////////////////////////////
/**
 * Make a new directory
 *
 * To be successful:
 *  - the parent must exist and be a directory
 *  - the parent must have space for the new directory
 *  - the child must not exist
 *
 * @param cwd Absolute path representing the current working directory
 * @param path Absolute or relative path to the file/directory
 * @return 0 if success
 *         -x if error
 *
 */
int oufs_mkdir(char *cwd, char *path)
{
    INODE_REFERENCE parent;
    INODE_REFERENCE child;
    
    // Name of a directory within another directory
    char local_name[MAX_PATH_LENGTH];
    int ret;
    
    // Attempt to find the specified directory
    if((ret = oufs_find_file(cwd, path, &parent, &child, local_name)) < -1) {
        if(debug)
            fprintf(stderr, "oufs_mkdir(): ret = %d\n", ret);
        return(-1);
    };
    // CHILD MUST NOT EXIST!??!
    // TODO: complete implementation
    
    fprintf(stderr, "\nlocal_name is  = %s\n", local_name);
    // parent inode and block
    INODE parentinode;
    oufs_read_inode_by_reference(parent, &parentinode);
    
    BLOCK pblock;
    // TODO: Is this read in right spot?
    virtual_disk_read_block(parentinode.content, &pblock);
    
    
    // add to parent directory and increment size
    
    // TODO: he says add it to first AVAILABLE ENTRY?????
    for (int i=2; i<N_DIRECTORY_ENTRIES_PER_BLOCK; i++)
    {
        if (pblock.content.directory.entry[i].inode_reference == UNALLOCATED_INODE)
        {
            fprintf(stderr, "allocating directory on inode: %d\n", parent);
            child = oufs_allocate_new_directory(parent);
            if (child == UNALLOCATED_INODE)
            {
                fprintf(stderr, "oufs_mkdir(): got UNALLOCATED_INODE calling allocate_new_dir");
                return (-3);
            }
            // TODO: local_name?????
            pblock.content.directory.entry[i].inode_reference =  child;// insert new inode ref
            // FIXME: this name is not copying correctly
            
            strcpy(pblock.content.directory.entry[i].name, local_name);
            parentinode.size++;
            // write parent directory block and inode back to disk
            virtual_disk_write_block(parentinode.content, &pblock);
            oufs_write_inode_by_reference(parent, &parentinode);
            return 0;
        }
    }
    // no space to store directory if it hits this point
    fprintf(stderr, "No space in directory to store new entry");
    return (-2);
}

/**
 * Remove a directory
 *
 * To be successul:
 *  - The directory must exist and must be empty
 *  - The directory must not be . or ..
 *  - The directory must not be /
 *
 * @param cwd Absolute path representing the current working directory
 * @param path Abslute or relative path to the file/directory
 * @return 0 if success
 *         -x if error
 *
 */
int oufs_rmdir(char *cwd, char *path)
{
    INODE_REFERENCE parent;
    INODE_REFERENCE child;
    char local_name[MAX_PATH_LENGTH];
    
    // Try to find the inode of the child
    int result = oufs_find_file(cwd, path, &parent, &child, local_name);
    if(result < -1) {
        return(-4);
    }
    else if (result==-1)    // could not find child
    {
        return -1;
    }
    // TODO: complete implementation
    // TODO: Will be error for: name does not exist, if its not a directory, if name is . or .., and if not an empty directory
    
    //TODO: Remove the entry from the parent's directory block
    INODE pnode;
    oufs_read_inode_by_reference(parent, &pnode);
    // error if type isn't directory type
    INODE cnode;
    oufs_read_inode_by_reference(child, &cnode);
    if (cnode.type != DIRECTORY_TYPE)
    {
        return -2;
    }
    
    BLOCK childdirectory;
    virtual_disk_read_block(cnode.content, &childdirectory);
    int count = 0;
    for (int i=0; i<N_DIRECTORY_ENTRIES_PER_BLOCK; i++)
    {
        if (childdirectory.content.directory.entry[i].inode_reference != UNALLOCATED_INODE)
        {
            count++;
        }
    }
    if (count > 2)
    {
        fprintf(stderr, "trying to remove non-empty directory\n");
        return -3;
    }
    // check to make sure name is not . or ..
    if (strcmp(local_name, ".") == 0)
        return -2;
    if (strcmp(local_name, "..") == 0)
        return -2;
    
        BLOCK directory;
    virtual_disk_read_block(pnode.content, &directory);
    
    // TODO: check this this. seems weird
    for (int i=0; i<N_DIRECTORY_ENTRIES_PER_BLOCK; i++)
    {
        fprintf(stderr, "checking directory entry %d:%s\n", i, directory.content.directory.entry[i].name);
        // TODO: check this name check NOT SURE ABOUT THIS
        if (strcmp(directory.content.directory.entry[i].name, local_name) == 0)
        {
            directory.content.directory.entry[i].inode_reference = UNALLOCATED_INODE;
            // TODO: do i need to remove the name??
            pnode.size--;
            break;
        }
    }
    BLOCK master;
    virtual_disk_read_block(MASTER_BLOCK_REFERENCE, &master);
    // change bit in master block's inode allocation table
    int byte = child / 8;
    int bit = 7 - (child % 8);
    fprintf(stderr, "byte %d bit %d\n", byte, bit);
    master.content.master.inode_allocated_flag[byte] = master.content.master.inode_allocated_flag[byte] ^ (1<<bit);
    
    
    //if(cnode.size==2)
    //    cnode.type= UNUSED_TYPE;

    //write blocks back to disk
    
    oufs_write_inode_by_reference(child, &cnode);
    virtual_disk_write_block(cnode.content, &childdirectory);
    oufs_deallocate_block(&master, cnode.content);
    virtual_disk_write_block(MASTER_BLOCK_REFERENCE, &master);
    virtual_disk_write_block(pnode.content, &directory);
    oufs_write_inode_by_reference(parent, &pnode);
    
    
    
    // Success
    return(0);
}
