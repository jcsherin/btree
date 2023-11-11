#include <gtest/gtest.h>
#include <numeric>
#include "../src/bplustree.h"

namespace bplustree {
    class BPlusTreeTest : public testing::Test {
    protected:
        void SetUp() override {}

        BPlusTree index{4, 5};
    };

    TEST_F(BPlusTreeTest, IsEmptyInitially) {
        EXPECT_EQ(index.root_, nullptr);
    }

    TEST_F(BPlusTreeTest, RootIsLeafNode) {
        auto element = std::make_pair(1, 1);
        index.Insert(element);

        auto root_leaf_node = reinterpret_cast<ElasticNode<KeyValuePair> *>(index.root_);

        EXPECT_NE(root_leaf_node, nullptr);
        EXPECT_EQ(root_leaf_node->GetType(), NodeType::LeafType);
        EXPECT_EQ(root_leaf_node->GetMaxSize(), index.leaf_node_max_size_);
        EXPECT_EQ(root_leaf_node->GetCurrentSize(), 1);

        EXPECT_EQ(root_leaf_node->At(0).first, element.first);
        EXPECT_EQ(root_leaf_node->At(0).second, element.second);
    }

    TEST_F(BPlusTreeTest, FillRootLeafNode) {
        std::vector<int> keys(index.leaf_node_max_size_);
        std::iota(keys.begin(), keys.end(), 0);

        for (auto &key: keys) {
            auto element = std::make_pair(key, key);
            index.Insert(element);
        }

        auto root_leaf_node = reinterpret_cast<ElasticNode<KeyValuePair> *>(index.root_);

        EXPECT_NE(root_leaf_node, nullptr);
        EXPECT_EQ(root_leaf_node->GetType(), NodeType::LeafType);
        EXPECT_EQ(root_leaf_node->GetMaxSize(), index.leaf_node_max_size_);
        EXPECT_EQ(root_leaf_node->GetCurrentSize(), index.leaf_node_max_size_);

        for (auto &key: keys) {
            EXPECT_EQ(root_leaf_node->At(key).first, key);
            EXPECT_EQ(root_leaf_node->At(key).second, key);
        }
    }

    TEST_F(BPlusTreeTest, RootLeafNodeWillSplit) {
        std::vector<int> keys(index.leaf_node_max_size_);
        std::iota(keys.begin(), keys.end(), 0);

        for (auto &key: keys) {
            auto element = std::make_pair(key, key);
            index.Insert(element);
        }

        auto root_leaf_node = reinterpret_cast<ElasticNode<KeyValuePair> *>(index.root_);

        EXPECT_NE(root_leaf_node, nullptr);
        EXPECT_EQ(root_leaf_node->GetType(), NodeType::LeafType);
        EXPECT_EQ(root_leaf_node->GetMaxSize(), index.leaf_node_max_size_);
        EXPECT_EQ(root_leaf_node->GetCurrentSize(), index.leaf_node_max_size_);

        auto extra_element = std::make_pair(keys.back() + 1, keys.back() + 1);
        index.Insert(extra_element); // Root (leaf) node will split

        auto root_inner_node = reinterpret_cast<ElasticNode<KeyNodePointerPair> *>(index.root_);
        EXPECT_NE(root_inner_node, nullptr);
        EXPECT_EQ(root_inner_node->GetType(), NodeType::InnerType);
        EXPECT_EQ(root_inner_node->GetMaxSize(), index.inner_node_max_size_);
        EXPECT_EQ(root_inner_node->GetCurrentSize(), 1);

        auto low_key_leaf = reinterpret_cast<ElasticNode<KeyValuePair> *>(root_inner_node->GetLowKeyPair().second);
        auto first_key_leaf = reinterpret_cast<ElasticNode<KeyValuePair> *>(root_inner_node->At(0).second);

        EXPECT_NE(low_key_leaf, nullptr);
        EXPECT_EQ(low_key_leaf->GetType(), NodeType::LeafType);
        EXPECT_EQ(low_key_leaf->GetMaxSize(), index.leaf_node_max_size_);
        EXPECT_EQ(low_key_leaf->GetCurrentSize(), 3);

        EXPECT_NE(first_key_leaf, nullptr);
        EXPECT_EQ(first_key_leaf->GetType(), NodeType::LeafType);
        EXPECT_EQ(first_key_leaf->GetMaxSize(), index.leaf_node_max_size_);
        EXPECT_EQ(first_key_leaf->GetCurrentSize(), 3);

        for (int i = 0; i < 3; ++i) {
            EXPECT_EQ(low_key_leaf->At(i).first, i);
            EXPECT_EQ(low_key_leaf->At(i).second, i);

            EXPECT_EQ(first_key_leaf->At(i).first, i + 3);
            EXPECT_EQ(first_key_leaf->At(i).second, i + 3);
        }

        EXPECT_EQ(low_key_leaf->GetSiblingLeft(), nullptr);
        EXPECT_EQ(low_key_leaf->GetSiblingRight(), first_key_leaf);
        EXPECT_EQ(first_key_leaf->GetSiblingLeft(), low_key_leaf);
        EXPECT_EQ(first_key_leaf->GetSiblingRight(), nullptr);
    }
}
