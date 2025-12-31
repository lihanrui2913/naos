#include <syscall/syscall.h>
#include <task/task.h>
#include <kcall/handle.h>

void send_request(request_t *req) {
    lane_t *self = current_task->posix_lane->lane.lane;
    size_t recv_pos =
        (self->peer && self->peer->peer_connected) ? self->peer->recv_pos : 0;
    size_t len = offsetof(request_t, data) + req->data_len;
    spin_lock(&self->peer->lock);
    memcpy(self->peer->recv_buff + recv_pos, req, len);
    recv_pos += len;
    if (self->peer && self->peer->peer_connected)
        self->peer->recv_pos = recv_pos;
    spin_unlock(&self->peer->lock);
}

response_t *recv_response(void) {
    response_t *resp = malloc(sizeof(response_t));
    lane_t *self = current_task->posix_lane->lane.lane;
    while (self->recv_pos < sizeof(response_t))
        schedule(SCHED_YIELD);
    size_t recv_pos = self->recv_pos;
    spin_lock(&self->lock);
    memcpy(resp, self->recv_buff, sizeof(response_t));
    memmove(self->recv_buff, &self->recv_buff[sizeof(response_t)],
            sizeof(response_t));
    recv_pos -= sizeof(response_t);
    spin_unlock(&self->lock);
    if (resp->magic != RESPONSE_MAGIC) {
        free(resp);
        return NULL;
    }
    resp = realloc(resp, sizeof(response_t) + resp->data_len);
    while (self->recv_pos < resp->data_len)
        schedule(SCHED_YIELD);
    spin_lock(&self->lock);
    memcpy((void *)(resp + 1), self->recv_buff, resp->data_len);
    memmove(self->recv_buff, &self->recv_buff[resp->data_len], resp->data_len);
    recv_pos -= resp->data_len;
    self->recv_pos = recv_pos;
    spin_unlock(&self->lock);
    return resp;
}
