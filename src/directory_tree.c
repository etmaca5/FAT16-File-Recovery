#include "directory_tree.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

const mode_t MODE_PERMISSION = 0777;

void init_node(node_t *node, char *name, node_type_t type) {
    if (name == NULL) {
        name = strdup("ROOT");
        assert(name != NULL);
    }
    node->name = name;
    node->type = type;
}

file_node_t *init_file_node(char *name, size_t size, uint8_t *contents) {
    file_node_t *node = malloc(sizeof(file_node_t));
    assert(node != NULL);
    init_node((node_t *) node, name, FILE_TYPE);
    node->size = size;
    node->contents = contents;
    return node;
}

directory_node_t *init_directory_node(char *name) {
    directory_node_t *node = malloc(sizeof(directory_node_t));
    assert(node != NULL);
    init_node((node_t *) node, name, DIRECTORY_TYPE);
    node->num_children = 0;
    node->children = NULL;
    return node;
}

void add_child_directory_tree(directory_node_t *dnode, node_t *child) {
    dnode->children =
        realloc(dnode->children, sizeof(node_t *) * (dnode->num_children + 1));
    // int64 instead of size_t because idx can be negative
    int64_t idx = dnode->num_children - 1;
    dnode->children[idx + 1] = child;
    while (idx >= 0) {
        if (strcmp(child->name, dnode->children[idx]->name) < 0) {
            dnode->children[idx + 1] = dnode->children[idx];
            dnode->children[idx] = child;
            idx--;
        }
        else {
            break;
        }
    }
    dnode->num_children = dnode->num_children + 1;
}

void print_directory_tree_helper(node_t *node, size_t level) {
    for (size_t i = 0; i < level; i++) {
        printf("    ");
    }
    printf("%s\n", node->name);
    if (node->type == DIRECTORY_TYPE) {
        directory_node_t *dnode = (directory_node_t *) node;
        for (size_t i = 0; i < dnode->num_children; i++) {
            print_directory_tree_helper(dnode->children[i], level + 1);
        }
    }
}

void print_directory_tree(node_t *node) {
    print_directory_tree_helper(node, 0);
}

void create_directory_tree_helper(node_t *node, char *root_name) {
    char *file_path = malloc(sizeof(char) * (strlen(root_name) + strlen(node->name) + 2));
    strcpy(file_path, root_name);
    strcat(file_path, "/");
    strcat(file_path, node->name);
    // file path now created
    if (node->type == DIRECTORY_TYPE) {
        directory_node_t *dnode = (directory_node_t *) node;
        assert(mkdir(file_path, MODE_PERMISSION) == 0);
        for (size_t i = 0; i < dnode->num_children; i++) {
            create_directory_tree_helper(dnode->children[i], file_path);
        }
    }
    else {
        assert(node->type == FILE_TYPE);
        file_node_t *fnode = (file_node_t *) node;
        FILE *file = fopen(file_path, "w");
        assert(file != NULL);
        assert(fwrite(fnode->contents, sizeof(uint8_t), fnode->size, file) ==
               fnode->size);
        assert(fclose(file) == 0);
    }
    free(file_path);
}

void create_directory_tree(node_t *node) {
    assert(node->type == DIRECTORY_TYPE);
    create_directory_tree_helper(node, "."); // dot for current directory
}

void free_directory_tree(node_t *node) {
    if (node->type == FILE_TYPE) {
        file_node_t *fnode = (file_node_t *) node;
        free(fnode->contents);
    }
    else {
        assert(node->type == DIRECTORY_TYPE);
        directory_node_t *dnode = (directory_node_t *) node;
        for (size_t i = 0; i < dnode->num_children; i++) {
            free_directory_tree(dnode->children[i]);
        }
        free(dnode->children);
    }
    free(node->name);
    free(node);
}
