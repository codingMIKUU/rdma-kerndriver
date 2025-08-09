#include "mlx5_avl_tree.h"
#include <linux/slab.h>   // 内核内存分配（kmalloc, kfree）

// 内核中获取节点高度（空节点返回-1）
static inline s32 avl_get_height(struct avl_node *node)
{
    return (node == NULL) ? -1 : node->height;
}

// 计算平衡因子（左高 - 右高）
static inline s32 avl_get_balance(struct avl_node *node)
{
    return (node == NULL) ? 0 : avl_get_height(node->left) - avl_get_height(node->right);
}

// 更新节点高度
static inline void avl_update_height(struct avl_node *node)
{
    if (node) {
        s32 left_h = avl_get_height(node->left);
        s32 right_h = avl_get_height(node->right);
        node->height = (left_h > right_h ? left_h : right_h) + 1;
    }
}

// 右旋转（内核中指针操作与用户态一致）
static struct avl_node *avl_right_rotate(struct avl_node *y)
{
    struct avl_node *x = y->left;
    struct avl_node *T2 = x->right;

    x->right = y;
    y->left = T2;

    avl_update_height(y);
    avl_update_height(x);

    return x;
}

// 左旋转
static struct avl_node *avl_left_rotate(struct avl_node *x)
{
    struct avl_node *y = x->right;
    struct avl_node *T2 = y->left;

    y->left = x;
    x->right = T2;

    avl_update_height(x);
    avl_update_height(y);

    return y;
}

// 递归插入节点（内核中递归深度有限制，建议复杂场景用迭代实现）
static struct avl_node *__avl_insert(struct avl_node *node, int val, int *err)
{
    if (!node) {
        // 分配新节点（GFP_KERNEL：可睡眠分配，适合进程上下文）
        struct avl_node *new_node = kmalloc(sizeof(*new_node), GFP_KERNEL);
        if (!new_node) {
            *err = -ENOMEM;  // 内存分配失败
            return NULL;
        }
        new_node->val = val;
        new_node->count = 1;
        new_node->left = new_node->right = NULL;
        new_node->height = 0;
        *err = 0;
        return new_node;
    }

    if (val < node->val) {
        node->left = __avl_insert(node->left, val, err);
    } else if (val > node->val) {
        node->right = __avl_insert(node->right, val, err);
    } else {
        // 重复值：递增计数
        node->count++;
        *err = 0;
        return node;
    }

    if (*err)  // 子树插入失败，直接返回
        return node;

    // 更新高度并平衡
    avl_update_height(node);
    s32 balance = avl_get_balance(node);

    // LL型
    if (balance > 1 && val < node->left->val)
        return avl_right_rotate(node);
    // RR型
    if (balance < -1 && val > node->right->val)
        return avl_left_rotate(node);
    // LR型
    if (balance > 1 && val > node->left->val) {
        node->left = avl_left_rotate(node->left);
        return avl_right_rotate(node);
    }
    // RL型
    if (balance < -1 && val < node->right->val) {
        node->right = avl_right_rotate(node->right);
        return avl_left_rotate(node);
    }

    return node;
}

// 对外接口：插入值
int avl_insert(struct avl_tree *tree, int val)
{
    int err;
    if (!tree)
        return -EINVAL;

    // 若加锁：mutex_lock(&tree->lock);
    struct avl_node *new_root = __avl_insert(tree->root, val, &err);
    if (!err)
        tree->root = new_root;
    // 若加锁：mutex_unlock(&tree->lock);

    return err;
}

// 查找最小值节点（用于删除操作）
static struct avl_node *avl_find_min(struct avl_node *node)
{
    struct avl_node *curr = node;
    while (curr && curr->left)
        curr = curr->left;
    return curr;
}

// 递归删除节点
static struct avl_node *__avl_delete(struct avl_node *node, int val, int *err)
{
    if (!node) {
        *err = -ENOENT;  // 未找到值
        return NULL;
    }

    if (val < node->val) {
        node->left = __avl_delete(node->left, val, err);
    } else if (val > node->val) {
        node->right = __avl_delete(node->right, val, err);
    } else {
        // 找到目标值：先减少计数
        node->count--;
        if (node->count > 0) {
            *err = 0;  // 计数未到0，不删除节点
            return node;
        }

        // 计数为0，执行节点删除
        struct avl_node *temp;
        if (!node->left || !node->right) {
            // 叶子节点或单子树
            temp = node->left ? node->left : node->right;
            if (!temp) {  // 叶子节点
                temp = node;
                node = NULL;
            } else {  // 单子树：复制子树内容
                *node = *temp;  // 注意：内核中结构体赋值安全
            }
            kfree(temp);  // 释放原节点
        } else {
            // 双子树：用中序后继（右子树最小值）替换
            temp = avl_find_min(node->right);
            node->val = temp->val;
            node->count = temp->count;
            // 删除后继节点（将其计数设为1，确保只删除一次）
            temp->count = 1;
            node->right = __avl_delete(node->right, temp->val, err);
            if (*err)
                return node;
        }
        *err = 0;
    }

    if (!node)  // 树已空
        return NULL;

    // 更新高度并平衡
    avl_update_height(node);
    s32 balance = avl_get_balance(node);

    // LL型
    if (balance > 1 && avl_get_balance(node->left) >= 0)
        return avl_right_rotate(node);
    // LR型
    if (balance > 1 && avl_get_balance(node->left) < 0) {
        node->left = avl_left_rotate(node->left);
        return avl_right_rotate(node);
    }
    // RR型
    if (balance < -1 && avl_get_balance(node->right) <= 0)
        return avl_left_rotate(node);
    // RL型
    if (balance < -1 && avl_get_balance(node->right) > 0) {
        node->right = avl_right_rotate(node->right);
        return avl_left_rotate(node);
    }

    return node;
}

// 对外接口：删除值
int avl_delete(struct avl_tree *tree, int val)
{
    int err;
    if (!tree)
        return -EINVAL;

    // 若加锁：mutex_lock(&tree->lock);
    struct avl_node *new_root = __avl_delete(tree->root, val, &err);
    if (!err)
        tree->root = new_root;
    // 若加锁：mutex_unlock(&tree->lock);

    return err;
}

// 对外接口：获取最小值
int avl_get_min(struct avl_tree *tree, int *out_val)
{
    if (!tree || !out_val)
        return -EINVAL;

    // 若加锁：mutex_lock(&tree->lock);
    struct avl_node *curr = tree->root;
    if (!curr) {
        // 若加锁：mutex_unlock(&tree->lock);
        return -EINVAL;  // 树为空
    }

    // 遍历至最左节点
    while (curr->left)
        curr = curr->left;

    *out_val = curr->val;
    // 若加锁：mutex_unlock(&tree->lock);
    return 0;
}

// 递归销毁树（释放所有节点）
static void __avl_destroy(struct avl_node *node)
{
    if (node) {
        __avl_destroy(node->left);
        __avl_destroy(node->right);
        kfree(node);  // 内核中释放内存
    }
}

// 对外接口：销毁树
void avl_destroy(struct avl_tree *tree)
{
    if (tree) {
        // 若加锁：mutex_lock(&tree->lock);
        __avl_destroy(tree->root);
        tree->root = NULL;
        // 若加锁：mutex_unlock(&tree->lock);
        // 若加锁：mutex_destroy(&tree->lock);
    }
}