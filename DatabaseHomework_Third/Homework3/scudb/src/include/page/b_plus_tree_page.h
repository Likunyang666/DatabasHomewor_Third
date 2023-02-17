#pragma once

#include <cassert>
#include <climits>
#include <cstdlib>
#include <string>


#include "buffer/buffer_pool_manager.h"
#include "index/generic_key.h"

namespace scudb {

#define MappingType std::pair<KeyType, ValueType>

#define INDEX_TEMPLATE_ARGUMENTS                                               \
  template <typename KeyType, typename ValueType, typename KeyComparator>

// define page type enum
    enum class IndexPageType { INVALID_INDEX_PAGE = 0, LEAF_PAGE, INTERNAL_PAGE };
    enum class OpType { READ = 0, INSERT, DELETE };

// Abstract class.
    class BPlusTreePage {
    public:
        bool IsLeafPage() const;
        bool IsRootPage() const;
        void SetPageType(IndexPageType page_type);

        int GetSize() const;
        void SetSize(int size);
        void IncreaseSize(int amount);

        int GetMaxSize() const;
        void SetMaxSize(int max_size);
        int GetMinSize() const;

        page_id_t GetParentPageId() const;
        void SetParentPageId(page_id_t parent_page_id);

        page_id_t GetPageId() const;
        void SetPageId(page_id_t page_id);

        void SetLSN(lsn_t lsn = INVALID_LSN);

        bool IsSafe(OpType op);
    private:
        // 内部页和叶页的成员变量及属性 
        IndexPageType page_type_;
        lsn_t lsn_;
        int size_;
        int max_size_;
        page_id_t parent_page_id_;
        page_id_t page_id_;

    };

} // namespace scudb
