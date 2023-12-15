#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "directory_tree.h"
#include "fat16.h"

const size_t MASTER_BOOT_RECORD_SIZE = 0x20B;

void follow(FILE *disk, directory_node_t *node, bios_parameter_block_t bpb) {
    directory_entry_t d;
    assert(fread(&d, sizeof(directory_entry_t), 1, disk) == 1);
    while (d.filename[0] != '\0') {
        size_t prev_loc = ftell(disk);
        fseek(disk, get_offset_from_cluster(d.first_cluster, bpb), SEEK_SET);
        if (!is_hidden(d) && is_directory(d)) {
            directory_node_t *dnode = init_directory_node(get_file_name(d));
            follow(disk, dnode, bpb);
            add_child_directory_tree(node, (node_t *) dnode);
        }
        else if (!is_hidden(d)) { // file type
            uint8_t *contents = malloc(sizeof(uint8_t) * d.file_size);
            assert(fread(contents, d.file_size, 1, disk) == 1);
            file_node_t *fnode = init_file_node(get_file_name(d), d.file_size, contents);
            add_child_directory_tree(node, (node_t *) fnode);
        }
        fseek(disk, prev_loc, SEEK_SET);
        assert(fread(&d, sizeof(directory_entry_t), 1, disk) == 1);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <image filename>\n", argv[0]);
        return 1;
    }

    FILE *disk = fopen(argv[1], "r");
    if (disk == NULL) {
        fprintf(stderr, "No such image file: %s\n", argv[1]);
        return 1;
    }

    bios_parameter_block_t bpb;

    fseek(disk, MASTER_BOOT_RECORD_SIZE, SEEK_SET);
    assert(fread(&bpb, sizeof(bios_parameter_block_t), 1, disk) == 1);
    fseek(disk, get_root_directory_location(bpb), SEEK_SET);

    directory_node_t *root = init_directory_node(NULL);
    follow(disk, root, bpb);
    print_directory_tree((node_t *) root);
    create_directory_tree((node_t *) root);
    free_directory_tree((node_t *) root);

    int result = fclose(disk);
    assert(result == 0);
}
