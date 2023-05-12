// The drawback of using perceus reference counting is that all FFI called
// functions must be very careful with memory usage. In particular, the
// invariant that MUST be maintained is that all args must either be dropped
// or returned to the caller. They cannot be modified other than with the drop fn.
// To simplify, we always drop all args and return newly created objects(with refcount=1)

#include "runtime.h"
#include <stdio.h>

qdbp_object_ptr qdbp_abort() {
    printf("aborting\n");
    exit(0);
}