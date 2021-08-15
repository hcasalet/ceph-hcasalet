//
// Created by Holly Casaletto on 7/29/21.
//
#include "include/types.h"
//#include <errno.h>

#include "include/rados/objclass.h"

using ceph::bufferlist;

CLS_VER(1,0)
CLS_NAME(lsm)

cls_handle_t h_class;
cls_method_handle_t h_lsmnode_write;
cls_method_handle_t h_lsmnode_read;

/**
 * read data from a node
 */
static int lsmnode_read(cls_method_context_t hctx, ceph::buffer::list *in, ceph::buffer::list *out) {
    auto in_iter = in->cbegin();
    for (auto it = in->begin(); it != in->end(); it++) {


    }
    
}