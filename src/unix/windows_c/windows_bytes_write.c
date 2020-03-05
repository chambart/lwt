/* This file is part of Lwt, released under the MIT license. See LICENSE.md for
   details, or visit https://github.com/ocsigen/lwt/blob/master/LICENSE.md. */



#include "lwt_config.h"

#if defined(LWT_ON_WINDOWS)

#include <caml/bigarray.h>
#include <caml/memory.h>
#include <caml/mlvalues.h>
#include <caml/unixsupport.h>

CAMLprim value lwt_unix_bytes_write(value fd, value buf, value val_file_offset,
                                    value vofs, value vlen)
{
    intnat ofs, len, file_offset, written;
    DWORD numbytes, numwritten;
    DWORD err = 0;

    Begin_root(buf);
    ofs = Long_val(vofs);
    len = Long_val(vlen);
    written = 0;
    if (len > 0) {
        numbytes = len;
        if (Descr_kind_val(fd) == KIND_SOCKET) {
            int ret;
            SOCKET s = Socket_val(fd);
            ret = send(s, (char *)Caml_ba_array_val(buf)->data + ofs, numbytes,
                       0);
            if (ret == SOCKET_ERROR) err = WSAGetLastError();
            numwritten = ret;
        } else {
            HANDLE h = Handle_val(fd);
            OVERLAPPED overlapped, *overlapped_ptr;
            if (Is_long(val_file_offset)) {
                overlapped_ptr = NULL;
            } else {
              file_offset = Long_val(Field(val_file_offset, 0));
                memset( &overlapped, 0, sizeof(overlapped));
                overlapped.OffsetHigh = (DWORD)(file_offset >> 32);
                overlapped.Offset = (DWORD)(file_offset & 0xFFFFFFFFLL);
                overlapped_ptr = &overlapped;
            }
            if (!WriteFile(h, (char *)Caml_ba_array_val(buf)->data + ofs,
                           numbytes, &numwritten, overlapped_ptr))
                err = GetLastError();
        }
        if (err) {
            win32_maperr(err);
            uerror("write", Nothing);
        }
        written = numwritten;
    }
    End_roots();
    return Val_long(written);
}
#endif
