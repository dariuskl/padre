// Sets the options for nuklear in a single place as they have to be set in the
// same way whenever "nuklear.h" is included.
#ifndef LADER_NUKLEAR_IMPL_H_INCLUDED
#define LADER_NUKLEAR_IMPL_H_INCLUDED

#define NK_INCLUDE_STANDARD_BOOL
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"
#include "nuklear_sdl_renderer.h"

#endif // LADER_NUKLEAR_IMPL_H_INCLUDED
