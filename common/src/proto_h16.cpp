#include <errno.h>

#include "proto_h16.h"

void init_proto_h16_res(struct proto_h16_head *rsp, const struct proto_h16_head *req)
{
	rsp->len = sizeof(*rsp);
	rsp->cmd = req->cmd + 1;
	rsp->ver = 1;
	rsp->syn = req->ack;
	rsp->ack = req->syn;
}

