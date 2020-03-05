/* This file is part of Lwt, released under the MIT license. See LICENSE.md for
   details, or visit https://github.com/ocsigen/lwt/blob/master/LICENSE.md. */



#include "lwt_config.h"

#if defined(LWT_ON_WINDOWS)

#include <caml/bigarray.h>
#include <caml/memory.h>
#include <caml/mlvalues.h>
#include <caml/unixsupport.h>

#include "lwt_unix.h"

struct job_bytes_write {
    struct lwt_unix_job job;
    union {
        HANDLE handle;
        SOCKET socket;
    } fd;
    int kind;
    char *buffer;
    DWORD length;
    int is_pwrite;
    DWORD Offset;
    DWORD OffsetHigh;
    DWORD result;
    DWORD error_code;
    value ocaml_buffer;
};

static void worker_bytes_write(struct job_bytes_write *job)
{
    if (job->kind == KIND_SOCKET) {
        int ret;
        ret = send(job->fd.socket, job->buffer, job->length, 0);
        if (ret == SOCKET_ERROR) job->error_code = WSAGetLastError();
        job->result = ret;
    } else {
        OVERLAPPED overlapped, *overlapped_ptr;
        if (Is_long(val_file_offset)) {
            overlapped_ptr = NULL;
        } else {
            file_offset = Field(val_file_offset, 0);
            memset( &overlapped, 0, sizeof(overlapped));
            overlapped.OffsetHigh = job->OffsetHigh;
            overlapped.Offset = job->Offset;
            overlapped_ptr = &overlapped;
        }
        if (!WriteFile(job->fd.handle, job->buffer, job->length, &(job->result),
                       overlapped_ptr))
            job->error_code = GetLastError();
    }
}

CAMLprim value result_bytes_write(struct job_bytes_write *job)
{
    value result;
    DWORD error = job->error_code;
    caml_remove_generational_global_root(&job->ocaml_buffer);
    if (error) {
        lwt_unix_free_job(&job->job);
        win32_maperr(error);
        uerror("bytes_write", Nothing);
    }
    result = Val_long(job->result);
    lwt_unix_free_job(&job->job);
    return result;
}

CAMLprim value lwt_unix_bytes_write_job(value val_fd, value val_buffer,
                                        value val_file_offset,
                                        value val_offset, value val_length)
{
    struct filedescr *fd = (struct filedescr *)Data_custom_val(val_fd);
    LWT_UNIX_INIT_JOB(job, bytes_write, 0);
    job->job.worker = (lwt_unix_job_worker)worker_bytes_write;
    job->kind = fd->kind;
    if (fd->kind == KIND_HANDLE)
        job->fd.handle = fd->fd.handle;
    else
        job->fd.socket = fd->fd.socket;
    if (Is_long(val_file_offset)) {
      job->is_pwrite = 0;
    } else {
      DWORDLONG file_offset = Long_val(Field(val_file_offset, 0));
      if (fd->kind != KIND_HANDLE) {
        caml_invalid_argument("Lwt_unix.pwrite");
      }
      job->is_pwrite = 1;
      job->OffsetHigh = (DWORD)(file_offset >> 32);
      job->Offset = (DWORD)(file_offset & 0xFFFFFFFFLL);
    }
    job->buffer = (char *)Caml_ba_data_val(val_buffer) + Long_val(val_offset);
    job->length = Long_val(val_length);
    job->error_code = 0;
    job->ocaml_buffer = val_buffer;
    caml_register_generational_global_root(&job->ocaml_buffer);
    return lwt_unix_alloc_job(&(job->job));
}
#endif
