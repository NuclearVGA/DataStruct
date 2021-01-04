#ifndef __RadixTree_HPP__
#include <cstdlib> // malloc
#include <cstring> // memset
#define LTLT(a, b, c) ((a) < (b) && (b) < (c))
#define U8AT(a, b) (((a) >> ((b) << 3)) & 255)

template <typename uintK = unsigned, typename ptrV = void, unsigned lastPrefix = 1>
class RadixTree // lastPrefix = 1 : 前缀匹配最低字节以上的所有字节，关键字局部性很强时使用
{
private:
    class rtNode
    {
    private:
        static rtNode *tinyMemPool(rtNode *p)
        {
            static rtNode *lst = nullptr;
            static unsigned need = 8;
            if (p) // 释放
            {
                p->next[0] = lst;
                lst = p;
            }
            else // 申请
            {
                if (lst) // 有可用对象
                {
                    p = lst;
                    lst = p->next[0];
                }
                else // 无可用对象
                {
                    need <<= 1;
                    p = (rtNode *)malloc(need * sizeof(rtNode));
                    for (unsigned i = 1; i < need; i++, lst = p++)
                        p->next[0] = lst;
                }
            }
            return p;
        }

    public:
        rtNode *next[256] = {};
        unsigned size = 1;
        void *operator new(size_t sz) { return tinyMemPool(nullptr); }
        void operator delete(void *p) { tinyMemPool((rtNode *)p); }
    };

    static void _delTree(rtNode *p, int depth)
    {
        if (p && depth--)
        {
            for (rtNode *i : p->next)
                _delTree(i, depth);
            delete p;
        }
    }

    static const unsigned KL = sizeof(uintK); // 关键字长度
    uintK oldKey;                             // 上一次访问的关键字
    unsigned level;                           // 当前节点高度
    rtNode *trace[KL + 1] = {};               // 节点访问路径，trace[0]为值，trace[KL]为根节点
    uintK treeSize = 0;                       // 关键字的总数

public:
    RadixTree() { trace[KL] = new rtNode(); }

    ~RadixTree() { _delTree(trace[KL], KL); }

    inline uintK size() { return treeSize; }

    void clear()
    {
        memset(trace, treeSize = 0, KL * sizeof(rtNode *));
        _delTree(trace[KL], KL);
        trace[KL] = new rtNode();
    }

    ptrV *search(uintK key) // search、insert、remove 返回相同结果 (key不存在时返回nullptr)
    {
        oldKey ^= key ^= oldKey;
        if (LTLT(0, lastPrefix, KL))
        {
            uintK mask = ((uintK)-1) << (lastPrefix * 8); // 低字节缓存命中率低，不匹配
            for (level = lastPrefix; (mask & key) || !trace[level]; level++, mask <<= 8)
                ; // 根据关键字前缀匹配从上次的节点访问路径中选择本次访问的起点，减少指针调用次数
        }
        else
            level = KL;
        trace[0] = nullptr;
        while (trace[level] && level--)
            trace[level] = trace[level + 1]->next[U8AT(oldKey, level)]; // 记录节点访问路径
        return (ptrV *)trace[0];
    }

    ptrV *insert(uintK key, ptrV *val, bool force = true) // Never insert nullptr !!!
    {
        if (search(key))
        {
            if (force) // 强制覆盖原值
                trace[1]->next[U8AT(oldKey, 0)] = (rtNode *)val;
        }
        else
        {
            treeSize++;
            for (trace[level + 1]->size++; level > 0; level--)
                trace[level + 1]->next[U8AT(oldKey, level)] = trace[level] = new rtNode();
            trace[1]->next[U8AT(oldKey, 0)] = (rtNode *)val;
        }
        return (ptrV *)trace[0];
    }

    ptrV *remove(uintK key)
    {
        if (search(key))
        {
            treeSize--;
            for (trace[level = 1]->next[U8AT(oldKey, 0)] = nullptr; !--(trace[level]->size); level++)
            {
                delete trace[level];
                trace[level + 1]->next[U8AT(oldKey, level)] = trace[level] = nullptr;
            }
        }
        return (ptrV *)trace[0];
    }
};
#define __RadixTree_HPP__
#endif
