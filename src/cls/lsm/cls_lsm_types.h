// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#ifndef CEPH_CLS_LSM_TYPES_H
#define CEPH_CLS_LSM_TYPES_H

#include <errno.h>
#include "include/types.h"
#include "objclass/objclass.h"

constexpr unsigned int LSM_NODE_START = 0xDEAD;
constexpr unsigned int LSM_ENTRY_OVERHEAD = sizeof(uint16_t) + sizeof(uint64_t);

// key range
struct cls_lsm_key_range
{
    uint64_t low_bound;
    uint64_t high_bound;

    void encode(ceph::buffer::list& bl) const {
        ENCODE_START(1, 1, bl);
        encode(low_bound, bl);
        encode(high_bound, bl);
        ENCODE_FINISH(bl);
    }

    void decode(ceph::buffer::list::const_iterator& bl) {
        DECODE_START(1, bl);
        decode(low_bound, bl);
        decode(high_bound, bl)
        DECODE_FINISH(bl);
    }
};
WRITE_CLASS_ENCODER(cls_lsm_key_range)

// column group
struct cls_lsm_column_group
{
    std::set<std::string> columns;

    void encode(ceph::buffer::list& bl) const {
        ENCODE_START(1, 1, bl);
        encode(columns, bl);
        ENCODE_FINISH(bl);
    }

    void decode(ceph::buffer::list::const_iterator& bl) {
        DECODE_START(1, bl);
        decode(columns, bl);
        DECODE_FINISH(bl);
    }    
};
WRITE_CLASS_ENCODER(cls_lsm_column_group)

// naming scheme rules
struct cls_lsm_object_naming_scheme
{
    std::map<cls_lsm_key_range, std::string>    key_range_parts;
    std::map<cls_lsm_column_group, std::string> clm_group_parts;

    void encode(ceph::buffer::list& bl) const {
        ENCODE_START(1, 1, bl);
        encode(key_range_parts, bl);
        encode(clm_group_parts, bl);
        ENCODE_FINISH(bl);
    }

    void decode(ceph::buffer::list::const_iterator& bl) {
        DECODE_START(1, bl);
        decode(key_range_parts, bl);
        decode(clm_group_parts, bl);
        DECODE_FINISH(bl);
    }
};
WRITE_CLASS_ENCODER(cls_lsm_object_naming_scheme)

// application data stored in the lsm node
// key-value format; value is "column name -> bufferlist" format
struct cls_lsm_data
{
    std::string key;
    std::map<std::string, ceph::buffer::list> value;
    
    void encode(ceph::buffer::list& bl) const {
        ENCODE_START(1, 1, bl);
        encode(key, bl);
        encode(value, bl);
        ENCODE_FINISH(bl);
    }

    void decode(ceph::buffer::list::const_iterator& bl) {
        DECODE_START(1, bl);
        decode(key, bl);
        decode(value, bl);
        DECODE_FINISH(bl);
    }
};
WRITE_CLASS_ENCODER(cls_lsm_data)

// lsm tree node
struct cls_lsm_node
{
    std::string object_name;                     // "file path" of the object                
    uint8_t level;                               // level of the tree that the node is on
    cls_lsm_key_range key_range;                 // range of keys stored in this object
    cls_lsm_object_naming_scheme naming_scheme;  // child node naming map
    uint64_t max_bloomfilter_data_size{0};       // max allowed space for bloom filter
    ceph::buffer::list bl_bloomfilter_data;      // special data known to apps using lsm

    void encode(ceph::buffer::list& bl) const {
        ENCODE_START(1, 1, bl);
        encode(object_name, bl);
        encode(level, bl);
        encode(key_range, bl);
        encode(naming, bl);
        encode(max_bloomfilter_data_size, bl);
        encode(bl_bloomfilter_data, bl);
        ENCODE_FINISH(bl);
    }

    void decode(ceph::buffer::list::const_iterator& bl) {
        DECODE_START(1, bl);
        decode(object_name, bl);
        decode(level, bl);
        decode(key_range, bl);
        decode(naming, bl);
        decode(max_bloomfilter_data_size, bl);
        decode(bl_bloomfilter_data, bl);
        DECODE_FINISH(bl);
    }
};
WRITE_CLASS_ENCODER(cls_lsm_node)

#endif /* CEPH_CLS_LSM_TYPES_H */
