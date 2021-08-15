#ifndef CEPH_CLS_LSM_SRC_H
#define CEPH_CLS_LSM_SRC_H

#include "objclass/objclass.h"
#include "cls/lsm/cls_lsm_types.h"
#include "cls/lsm/cls_lsm_ops.h"

int lsm_write_node(cls_method_context_t hctx, cls_lsm_node& node);
int lsm_read_node(cls_method_context_t hctx, cls_lsm_node& node);
int lsm_init(cls_method_context_t hctx, const cls_lsm_init_op& op);
int lsm_get_child_object_names(cls_method_context_t hctx, cls_lsm_get_child_object_names_ret& op_ret);
int lsm_get_apps_data(cls_method_contest_t hctx, cls_lsm_get_apps_data_ret& op_ret);
int lsm_compact(cls_method_context_t hctx, cls_lsm_compact_op& op, cls_lsm_node& node);


#endif /* CEPH_CLS_LSM_SRC_H */
