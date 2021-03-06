#ifndef CEPH_CLS_LSM_OPS_H
#define CEPH_CLS_LSM_OPS_H

#include "cls/lsm/cls_lsm_types.h"

struct cls_lsm_init_op {
    std::string object_name;
    uint8_t level{0};
    cls_lsm_key_range key_range;
    cls_lsm_object_naming_scheme naming_scheme;
    uint64_t max_bloomfilter_data_size;
    ceph::buffer::list bl_bloomfilter_data;

    cls_lsm_init_op() {}

    void encode(ceph::buffer::list& bl) const {
        ENCODE_START(1, 1, bl);
        encode(object_name, bl);
        encode(level, bl);
        encode(key_range, bl);
        encode(naming_scheme, bl);
        encode(max_bloomfilter_data_size, bl);
        encode(bl_bloomfilter_data, bl);
        ENCODE_FINISH(bl);
    }

    void decode(ceph::buffer::list::const_iterator& bl) {
        DECODE_START(1, bl);
        decode(object_name, bl);
        decode(level, bl);
        decode(key_range, bl);
        decode(naming_scheme, bl);
        decode(max_bloomfilter_data_size, bl);
        decode(bl_bloomfilter_data, bl);
        DECODE_FINISH(bl);
    }
};
WRITE_CLASS_ENCODER(cls_lsm_init_op)

struct cls_lsm_get_child_object_names_ret {
    std::vector<std::string> child_object_names;

    cls_lsm_get_child_object_names_ret() {}

    void encode (ceph::buffer::list& bl) const {
        ENCODE_START(1, 1, bl);
        encode(child_object_names, bl);
        ENCODE_FINISH(bl);
    }

    void decode(ceph::buffer::list::const_iterator& bl) {
        DECODE_START(1, bl);
        decode(child_object_names, bl);
        DECODE_FINISH(bl);
    }
};
WRITE_CLASS_ENCODER(cls_lsm_get_child_object_names_ret)

struct cls_lsm_persist_data_op {
    std::vector<cls_lsm_entry> bl_data_vec;

    cls_lsm_persist_data_op() {}

    void encode (ceph::buffer::list& bl) const {
        ENCODE_START(1, 1, bl);
        encode(bl_data_vec, bl);
        ENCODE_FINISH(bl);
    }

    void decode(ceph::buffer::list::const_iterator& bl) {
        DECODE_START(1, bl);
        decode(bl_data_vec, bl);
        DECODE_FINISH(bl);
    }
};
WRITE_CLASS_ENCODER(cls_lsm_persist_data_op)

struct cls_lsm_retrieve_data_op {
    uint64_t start_marker;

    cls_lsm_retrieve_data_op() {}

    void encode(ceph::buffer::list& bl) const {
        ENCODE_START(1, 1, bl);
        encode(start_marker, bl);
        ENCODE_FINISH(bl);
    }

    void decode(ceph::buffer::list::const_iterator& bl) {
        DECODE_START(1, 1, bl);
        decode(start_marker, bl);
        DECODE_FINISH(bl);
    }
};
WRITE_CLASS_ENCODER(cls_lsm_retrieve_data_op)

struct cls_lsm_retrieve_data_ret {
    std::vector<cls_lsm_entry> entries;

    cls_lsm_retrieve_data_ret() {}

    void encode(ceph::buffer::list& bl) const {
        ENCODE_START(1, 1, bl);
        encode(entries, bl);
        ENCODE_FINISH(bl);
    }

    void decode(ceph::buffer::list::const_iterator& bl) {
        DECODE_START(1, bl);
        decode(entries, bl);
        DECODE_FINISH(bl);
    }
};
WRITE_CLASS_ENCODER(cls_lsm_retrieve_data_ret)

struct cls_lsm_split_data_ret {
    std::vector<std::vector<cls_lsm_entry>> split_entries;

    cls_lsm_split_data_ret() {}

    void encode(ceph::buffer::list& bl) const {
        ENCODE_START(1, 1, bl);
        encode(split_entries, bl);
        ENCODE_FINISH(bl);
    }

    void decode(ceph::buffer::list::const_iterator& bl) {
        DECODE_START(1, bl);
        decode(split_entries, bl);
        DECODE_FINISH(bl);
    }
};
WRITE_CLASS_ENCODER(cls_lsm_split_data_ret)

struct cls_lsm_compact_op {
    std::vector<cls_lsm_entry> bl_data_vec;

    cls_lsm_compact_op() {}

    void encode(ceph::buffer::list& bl) const {
        ENCODE_START(1, 1, bl);
        encode(bl_data_vec, bl);
        ENCODE_FINISH(bl);
    }

    void decode(ceph::buffer::list::const_iterator& bl) {
        DECODE_START(1, bl);
        decode(bl_data_vec, bl);
        DECODE_FINISH(bl);
    }
};
WRITE_CLASS_ENCODER(cls_lsm_compact_op)

#endif /* CEPH_CLS_LSM_OPS_H */
