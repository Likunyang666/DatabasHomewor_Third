
/*
B+������Ӧ�ȸ��ݼ���������������һҳһҳ�Ĳ��룬����������и��ڵ�ķ��� 
*/ 

#include "page/b_plus_tree_page.h"

namespace scudb {

    bool BPlusTreePage::IsLeafPage() const
    {
        return page_type_ == IndexPageType::LEAF_PAGE;
    }

    bool BPlusTreePage::IsRootPage() const
    {
        // -1
        return parent_page_id_ == INVALID_PAGE_ID;
    }

    void BPlusTreePage::SetPageType(IndexPageType page_type)
    {
        page_type_ = page_type;
    }

//��á����á��������Ĵ�С 
    int BPlusTreePage::GetSize() const
    {
        return size_;
    }

    void BPlusTreePage::SetSize(int size)
    {
        size_ = size;
    }

    void BPlusTreePage::IncreaseSize(int amount)
    {
        size_ += amount;
    }

//��ȡ������ҳ������С 
    int BPlusTreePage::GetMaxSize() const
    {
        return max_size_;
    }

    void BPlusTreePage::SetMaxSize(int size)
    {
        max_size_ = size;
    }


    int BPlusTreePage::GetMinSize() const
    {
        if(!IsRootPage())
        {
            return (max_size_ ) / 2;//��Сҳ��С=���ҳ��С/2 

        }
        else
        {
            if(IsLeafPage())
            {// root, leaf, just point to only a page according to its value
                return 1;
            }
            else
            {// root, not leaf, it has at least one leaf page
                return 2;
            }
        }
    }

/*
 ��ȡ�����ø�ҳ��ID 
 */
    page_id_t BPlusTreePage::GetParentPageId() const
    {
        return parent_page_id_;
    }

    void BPlusTreePage::SetParentPageId(page_id_t parent_page_id)
    {
        parent_page_id_ = parent_page_id;
    }

/*
 * Helper methods to get/set self page id
 */
    page_id_t BPlusTreePage::GetPageId() const
    {
        return page_id_;
    }

    void BPlusTreePage::SetPageId(page_id_t page_id)
    {
        page_id_ = page_id;
    }

/*
 * Helper methods to set lsn
 */
    void BPlusTreePage::SetLSN(lsn_t lsn)
    {
        lsn_ = lsn;
    }

/* for concurrent index */
    bool BPlusTreePage::IsSafe(OpType op)
    {
        int size = GetSize();
        if (op == OpType::INSERT)
        {
            if(size < GetMaxSize())
            {
                return true;
            }
            return false;
        }
        int minSize = GetMinSize() + 1;
        if (op == OpType::DELETE)
        {
            if(IsLeafPage())
            {
                if(size >= minSize)
                {
                    return true;
                }
                return false;
            }
            else
            {
                if(size > minSize)
                {
                    return true;
                }
                return false;
            }
        }
        assert(false);
    }

} // namespace scudb
