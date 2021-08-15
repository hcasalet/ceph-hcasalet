#include "include/types.h"

#include "objclass/objclass.h"
#include "cls/lsm/cls_lsm_types.h"
#include "cls/lsm/cls_lsm_ops.h"
#include "cls/lsm/cls_lsm_src.h"

using ceph::bufferlist;
using ceph::decode;
using ceph::encode;

// should we define the max size in cls_lsm_node?
int lsm_write_node(cls_method_context_t hctx, cls_lsm_node& node)
{
    bufferlist bl;
    uint16_t entry_start = LSM_NODE_START;
    encode(entry_start, bl);

    bufferlist bl_node;
    encode(node, bl_node);

    uint64_t encoded_len = bl_node.length();
    encode(encoded_len, bl);
    
    bl.claim_append(bl_node);

    int ret = cls_cxx_write2(hctx, 0, bl.length(), &bl, CEPH_OSD_OP_FLAG_FADVISE_WILLNEED);
    if (ret < 0) {
        CLS_LOG(5, "ERROR: lsm_write_node: failed to write lsm node");
        return ret;
    }

    return 0;
}

int lsm_read_node(cls_method_context_t hctx, cls_lsm_node& node)
{
    uint64_t chunk_size = 1024, start_offset = 0;

    bufferlist bl_node;
    const auto ret = cls_cxx_read(hctx, start_offset, chunk_size, &bl_node);
    if (ret < 0) {
        CLS_LOG(5, "ERROR: lsm_read_node: failed read lsm node");
        return ret;
    }
    if (ret == 0) {
        CLS_LOG(20, "INFO: lsm_read_node: empty node, not initialized yet");
        return -EINVAL;
    }

    // Process the chunk of data read
    auto it = bl_node.cbegin();
    
    // Check node start
    uint16_t node_start;
    try {
        decode(node_start, it);
    } catch (const ceph::buffer::error& err) {
        CLS_LOG(0, "ERROR: lsm_read_node: failed to decode node start: %s", err.what());
        return -EINVAL;
    }
    if (node_start != LSM_NODE_START) {
        CLS_LOG(0, "ERROR: lsm_read_node: invalid node start");
        return -EINVAL;
    }

    uint64_t encoded_len;
    try {
        decode(encoded_len, it);
    } catch (const ceph::buffer::error& err) {
        CLS_LOG(0, "ERROR: lsm_read_node: failed to decode encoded size", err.what());
        return -EINVAL;
    }

    if (encoded_len > (chunk_size - LSM_ENTRY_OVERHEAD)) {
        start_offset = chunk_size;
        chunk_size = encoded_len - (chunk_size - LSM_ENTRY_OVERHEAD);
        bufferlist bl_remaining_node;
        const auto ret = cls_cxx_read2(hctx, start_offset, chunk_size, &bl_remaining_node, CEPH_OSD_OP_FLAG_FADVISE_SEQUENTIAL);
        if (ret < 0) {
            CLS_LOG(5, "ERROR: lsm_read_node: failed to read the remaining part of the node");
            return ret;
        }
        bl_node.claim_append(bl_remaining_node);
    }

    try {
        decode(node, it);
    } catch (const ceph::buffer::error& err) {
        CLS_LOG(0, "ERROR: lsm_read_node: failed to decode node: %s", err.what());
        return -EINVAL;
    }

    return 0;
}

int lsm_init(cls_method_context_t hctx, const cls_lsm_init_op& op)
{
    cls_lsm_node node;
    int ret = lsm_read_node(hctx, node);

    // if node has already been initialized, return
    if (ret == 0) {
        return -EEXIST;
    }

    // bail out for other errors
    if (ret < 0 && ret != -EINVAL) {
        return ret;
    }

    node.object_name = op.object_name;
    node.level = op.level;
    node.key_range = op.key_range;
    node.naming = op.naming_map;
    node.max_bloomfilter_data_size = op.max_bloomfilter_data_size;
    node.bl_bloomfilter_data = op.bl_bloomfilter_data;

    CLS_LOG(20, "INFO: lsm_init object name %s", node.object_name);
    CLS_LOG(20, "INFO: lsm_init level %u", node.level);
    CLS_LOG(20, "INFO: lsm_init key range (%lu, %lu)", node.key_range.low_bound, node.key_range.high_bound);
    CLS_LOG(20, "INFO: lsm_init max bloom filter size %lu", node.max_bloomfilter_data_size);

    return lsm_write_node(hctx, node);
}

int lsm_get_child_object_names(cls_method_context_t hctx, cls_lsm_get_child_object_names_ret& op_ret)
{
    // get the node
    cls_lsm_node node;
    int ret = lsm_read_node(hctx, node);
    if (ret < 0) {
        return ret;
    }
    
    std::set<std::string> children_objects;
    for (auto it = node.naming_scheme.key_range_parts.begin(); it != node.naming_scheme.key_range_parts.end(); it++) {
        for (auto ot = node.naming_scheme.clm_group_parts.begin(); ot != node.naming_scheme.clm_group.parts.end(); ot++) {
            std::stringstream ss;
            ss << node.object_name << "/lv" << node.level+1 << "-" << it->second << "-" << ot->second;
            children_objects.insert(ss.str())
        }
    }

    opt_ret.child_object_names = children_objects;

    CLS_LOG(20, "INFO: lsm_get_child_object_names: size of child objects: %lu", opt_ret.child_object_names.size());
    
    return 0;
}
