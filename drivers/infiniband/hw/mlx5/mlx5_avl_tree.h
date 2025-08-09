#ifndef _MLX5_AVL_TREE_H
#define _MLX5_AVL_TREE_H

#include <linux/types.h>   // 内核基本类型（size_t, s32等） // 内核常用函数（printk等）
#include <linux/errno.h>   // 错误码（-ENOMEM等）
#include <linux/stddef.h> // NULL定义

// 内核态AVL树节点结构（支持重复值）
struct avl_node {
    int val;                  // 存储的值
    int count;                // 重复值计数
    struct avl_node *left;    // 左子树
    struct avl_node *right;   // 右子树
    s32 height;               // 节点高度（用s32避免溢出）
};

// AVL树根结构（封装节点，便于扩展锁/统计信息）
struct avl_tree {
    struct avl_node *root;    // 根节点
    // 若需多线程安全，可添加锁：struct mutex lock;
};

// 初始化AVL树
static inline void avl_tree_init(struct avl_tree *tree)
{
    tree->root = NULL;
    // 若加锁：mutex_init(&tree->lock);
}

// 插入值（返回0成功，-ENOMEM内存分配失败）
int avl_insert(struct avl_tree *tree, int val);

// 删除值（返回0成功，-ENOENT值不存在）
int avl_delete(struct avl_tree *tree, int val);

// 获取最小值（返回0成功，-EINVAL树为空；结果通过out_val输出）
int avl_get_min(struct avl_tree *tree, int *out_val);

// 销毁整棵树（释放所有节点）
void avl_destroy(struct avl_tree *tree);

#endif /* __AVL_TREE_H */