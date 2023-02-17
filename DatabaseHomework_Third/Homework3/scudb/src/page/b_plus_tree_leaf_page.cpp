#include <sstream>
#include <include/page/b_plus_tree_internal_page.h>

#include "common/exception.h"
#include "common/rid.h"
#include "page/b_plus_tree_leaf_page.h"

namespace scudb {

    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_LEAF_PAGE_TYPE::Init(page_id_t page_id, page_id_t parent_id) {
        SetPageType(IndexPageType::LEAF_PAGE);//设置当前页为叶页 
        SetSize(0);//设置当前页大小为0 
        assert(sizeof(BPlusTreeLeafPage) == 28);
        SetMaxSize((PAGE_SIZE - sizeof(BPlusTreeLeafPage))/sizeof(MappingType) - 1);//设置页最大大小 
        SetPageId(page_id);//设置页对应id 
        SetParentPageId(parent_id);//设置父页id 
        SetNextPageId(INVALID_PAGE_ID);//设置下一页id 
    }

/**
 * Helper methods to set/get next page id
 */
    INDEX_TEMPLATE_ARGUMENTS
            page_id_t B_PLUS_TREE_LEAF_PAGE_TYPE::GetNextPageId() const {return next_page_id_;}

    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_LEAF_PAGE_TYPE::SetNextPageId(page_id_t next_page_id){next_page_id_ = next_page_id;}

/**
 * Helper method to find the first index i so that array[i].first >= key
 * NOTE: This method is only used when generating index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
int B_PLUS_TREE_LEAF_PAGE_TYPE::KeyIndex(
        const KeyType &key, const KeyComparator &comparator) const {
    assert(GetSize() >= 0);
    int head = 0;
	int tail = GetSize() - 1;
    while (head <= tail) {
        int middle = (tail - head) / 2 + head;
        if (comparator(array[mid].first,key) >= 0){ tail = middle - 1;}
        else{head = middle + 1;}
    }
    return tail + 1;
}


INDEX_TEMPLATE_ARGUMENTS
        KeyType B_PLUS_TREE_LEAF_PAGE_TYPE::KeyAt(int index) const {
    assert(index >= 0 && index < GetSize());
    return array[index].first;//返回序列号所对应的键值 
}

/*
 * Helper method to find and return the key & value pair associated with input
 * "index"(a.k.a array offset)
 */
INDEX_TEMPLATE_ARGUMENTS
const MappingType &B_PLUS_TREE_LEAF_PAGE_TYPE::GetItem(int index) {
    assert(index >= 0 && index < GetSize());
    return array[index];
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/*
 * Insert key & value pair into leaf page ordered by key
 * @return  page size after insertion
 */
INDEX_TEMPLATE_ARGUMENTS
int B_PLUS_TREE_LEAF_PAGE_TYPE::Insert(const KeyType &key,
                                       const ValueType &value,
                                       const KeyComparator &comparator) {
    int _index = KeyIndex(key,comparator);
    assert(_index >= 0);
    IncreaseSize(1);
    int cur_Size = GetSize();
    for (int j = cur_Size - 1; j > _index; j--)
    {
        array[j].first = array[j- 1].first;
        array[j].second = array [j- 1].second;
    }
    array[_index].first = key;
    array[_index].second = value;
    return cur_Size;//返回插入后的页大小 
}

/*****************************************************************************
 * SPLIT
 *****************************************************************************/
/*
 * Remove half of key & value pairs from this page to "recipient" page
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveHalfTo(BPlusTreeLeafPage *recipient,__attribute__((unused)) BufferPoolManager *buffer_pool_manager) {
    assert(recipient != nullptr);
    int total_size = GetMaxSize() + 1;
    assert(GetSize() == total);
    int copy_Idx = (total_size)/2;
    for (int j = copy_Idx; j < total_size;j++)
    {
        recipient->array[j - copy_Idx].first = array[j].first;
        recipient->array[j - copy_Idx].second = array[j].second;
    }
    recipient->SetNextPageId(GetNextPageId());
    SetNextPageId(recipient->GetPageId());
    SetSize(copy_Idx);
    recipient->SetSize(total_size - copy_Idx);
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::CopyHalfFrom(MappingType *items, int size) {}

/*****************************************************************************
 * LOOKUP
 *****************************************************************************/
/*
 * For the given key, check to see whether it exists in the leaf page. If it
 * does, then store its corresponding value in input "value" and return true.
 * If the key does not exist, then return false
 */
INDEX_TEMPLATE_ARGUMENTS
bool B_PLUS_TREE_LEAF_PAGE_TYPE::Lookup(const KeyType &key, ValueType &value,const KeyComparator &comparator) const {
    int _index = KeyIndex(key,comparator);
    if (_index < GetSize() && comparator(array[_index].first, key) == 0)
    {
        value = array[_index].second;
        return true;
    }
    return false;
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * First look through leaf page to see whether delete key exist or not. If
 * exist, perform deletion, otherwise return immdiately.
 * NOTE: store key&value pair continuously after deletion
 * @return   page size after deletion
 */
INDEX_TEMPLATE_ARGUMENTS
int B_PLUS_TREE_LEAF_PAGE_TYPE::RemoveAndDeleteRecord(const KeyType &key, const KeyComparator &comparator) {
    int firIdxLargerEqualThanKey = KeyIndex(key,comparator);
    if (firIdxLargerEqualThanKey >= GetSize() || comparator(key,KeyAt(firIdxLargerEqualThanKey)) != 0)
    {
        return GetSize();
    }
    int target_Idx = firIdxLargerEqualThanKey;
    memmove(array + target_Idx, array + target_Idx + 1, static_cast<size_t>((GetSize() - target_Idx - 1)*sizeof(MappingType)));
    IncreaseSize(-1);
    return GetSize();//需要返回删除后页的大小 
}

/*****************************************************************************
 * MERGE
 *****************************************************************************/
/*
 * Remove all of key & value pairs from this page to "recipient" page, then
 * update next page id
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveAllTo(BPlusTreeLeafPage *recipient,int, BufferPoolManager *) {
    assert(recipient != nullptr);
    int start_Idx = recipient->GetSize();
    for (int j = 0; j < GetSize(); j++)
    {
        recipient->array[start_Idx + j].first = array[j].first;
        recipient->array[start_Idx + j].second = array[j].second;
    }
    recipient->SetNextPageId(GetNextPageId());
    recipient->IncreaseSize(GetSize());
    SetSize(0);//合并后相关页的大小设置为0 

}
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::CopyAllFrom(MappingType *items, int size) {}

/*****************************************************************************
 * REDISTRIBUTE
 *****************************************************************************/
/*
 * Remove the first key & value pair from this page to "recipient" page, then
 * update relavent key & value pair in its parent page.
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveFirstToEndOf(BPlusTreeLeafPage *recipient,BufferPoolManager *buffer_pool_manager) {
    MappingType pair = GetItem(0);
    IncreaseSize(-1);
    memmove(array, array + 1, static_cast<size_t>(GetSize()*sizeof(MappingType)));
    recipient->CopyLastFrom(pair);
    Page *page = buffer_pool_manager->FetchPage(GetParentPageId());
    B_PLUS_TREE_INTERNAL_PAGE *parent = reinterpret_cast<B_PLUS_TREE_INTERNAL_PAGE *>(page->GetData());
    parent->SetKeyAt(parent->ValueIndex(GetPageId()), array[0].first);
    buffer_pool_manager->UnpinPage(GetParentPageId(), true);
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::CopyLastFrom(const MappingType &item) {
    assert(GetSize() + 1 <= GetMaxSize());
    array[GetSize()] = item;
    IncreaseSize(1);
}
/*
 * Remove the last key & value pair from this page to "recipient" page, then
 * update relavent key & value pair in its parent page.
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveLastToFrontOf(BPlusTreeLeafPage *recipient, int parent_Index,BufferPoolManager *buffer_pool_manager) {
    MappingType pair = GetItem(GetSize() - 1);
    IncreaseSize(-1);
    recipient->CopyFirstFrom(pair, parent_Index, buffer_pool_manager);
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::CopyFirstFrom(const MappingType &item, int parent_Index,BufferPoolManager *buffer_pool_manager) {
    assert(GetSize() + 1 < GetMaxSize());
    memmove(array + 1, array, GetSize()*sizeof(MappingType));
    IncreaseSize(1);
    array[0] = item;
    Page *page = buffer_pool_manager->FetchPage(GetParentPageId());
    B_PLUS_TREE_INTERNAL_PAGE *parent = reinterpret_cast<B_PLUS_TREE_INTERNAL_PAGE *>(page->GetData());
    parent->SetKeyAt(parent_Index, array[0].first);
    buffer_pool_manager->UnpinPage(GetParentPageId(), true);
}

/*****************************************************************************
 * DEBUG
 *****************************************************************************/
INDEX_TEMPLATE_ARGUMENTS
        std::string B_PLUS_TREE_LEAF_PAGE_TYPE::ToString(bool verbose) const {
    if (GetSize() == 0) {
        return "";
    }
    std::ostringstream stream;
    if (verbose) {
        stream << "[pageId: " << GetPageId() << " parentId: " << GetParentPageId()<< "]<" << GetSize() << "> ";
    }
    int entry = 0;
    int end = GetSize();
    bool first = true;

    while (entry < end) {
        if (first) {
            first = false;
        } else {
            stream << " ";
        }
        stream << std::dec << array[entry].first;
        if (verbose) {
            stream << "(" << array[entry].second << ")";
        }
        ++entry;
    }
    return stream.str();
}

template class BPlusTreeLeafPage<GenericKey<4>, RID,
GenericComparator<4>>;
template class BPlusTreeLeafPage<GenericKey<8>, RID,
GenericComparator<8>>;
template class BPlusTreeLeafPage<GenericKey<16>, RID,
GenericComparator<16>>;
template class BPlusTreeLeafPage<GenericKey<32>, RID,
GenericComparator<32>>;
template class BPlusTreeLeafPage<GenericKey<64>, RID,
GenericComparator<64>>;
} // namespace scudb
