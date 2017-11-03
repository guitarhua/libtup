/* libtup
 * Copyright (C) 2017 Actronika SAS
 *     Author: Aurélien Zanelli <aurelien.zanelli@actronika.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * @file
 * \defgroup context Context
 */

#include "libtup.h"
#include <stdio.h>
#include "libtup-private.h"

static void on_sf_new_frame(uint8_t *frame, size_t size, void *userdata)
{
    TupContext *ctx = userdata;
    TupMessage msg;
    int ret;

    ret = smp_message_init_from_buffer(&msg, frame, size);
    if (ret < 0) {
        fprintf(stderr, "failed to init message from frame\n");
        return;
    }

    ctx->cbs.new_message(ctx, &msg, ctx->userdata);
}

static void on_sf_error(SmpSerialFrameError error, void *userdata)
{
    fprintf(stderr, "serial frame error: %d\n", error);
}

/* API */

/**
 * \ingroup context
 * Initialize a TupContext using the provided serial device.
 *
 * @param[in] ctx the TupContext to initialize
 * @param[in] device path to the serial device to use
 * @param[in] cbs pointer to a callback structure
 * @param[in] userdata userdata to pass in callbacks
 *
 * @return 0 on success, a negative errno value on error
 */
int tup_context_init(TupContext *ctx, const char *device, TupCallbacks *cbs,
        void *userdata)
{
    const SmpSerialFrameDecoderCallbacks sf_cbs = {
        .new_frame = on_sf_new_frame,
        .error = on_sf_error
    };

    ctx->cbs = *cbs;
    ctx->userdata = userdata;

    return smp_serial_frame_init(&ctx->sf_ctx, device, &sf_cbs, ctx);
}

/**
 * \ingroup context
 * Clear a TupContext object.
 *
 * @param[in] ctx the TupContext to clear
 */
void tup_context_clear(TupContext *ctx)
{
    smp_serial_frame_deinit(&ctx->sf_ctx);
}

/**
 * \ingroup context
 * Get the file descriptor of the opened serial device.
 *
 * @param[in] ctx the TupContext
 *
 * @return the fd on success, a negative errno value otherwise.
 */
int tup_context_get_fd(TupContext *ctx)
{
    return smp_serial_frame_get_fd(&ctx->sf_ctx);
}

/**
 * \ingroup context
 * Send a message to the peer
 *
 * @param[in] ctx the TupContext
 * @param[in] msg the TupMessage to send
 *
 * @return 0 on success, a negative errno value otherwise.
 */
int tup_context_send(TupContext *ctx, TupMessage *msg)
{
    uint8_t buf[512];
    int ret;

    ret = smp_message_encode(msg, buf, sizeof(buf));
    if (ret < 0)
        return ret;

    return smp_serial_frame_send(&ctx->sf_ctx, buf, ret);
}

/**
 * \ingroup context
 * Process incoming data on the serial file descriptor.
 * New message will be posted to the dedicated callback
 *
 * @param[in] ctx the TupContext
 *
 * @return 0 on success, a negative errno value otherwise.
 */
int tup_context_process_fd(TupContext *ctx)
{
    return smp_serial_frame_process_recv_fd(&ctx->sf_ctx);
}
