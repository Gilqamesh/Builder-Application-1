# `m03gt0l0q3l4b1k27eab5k7py1_texture`

## Purpose

Keep this module responsible for owning CPU-side 2D texture data, sampler descriptions, and backend-independent sampling behavior. The module also owns validated, non-owning pixel views shared by sampling and CPU
framebuffer attachments. See [pixel views](pixel_view.h) and [texture ownership](texture.h).

Renderer and backend modules own GPU resource creation, uploads, binding, attachment
eligibility, and renderer-facing resource lifetimes. Applications allocate owning
textures, keep borrowed attachments alive, and sequence rendering and sampling.

Sampling leaves alpha association to shaders and blend equations. Its transfer and
filtering contract is owned by [sampler.h](sampler.h).

- Textures allocate a fixed contiguous prefix of mip levels at construction.
  Explicit generation updates lower-level contents in place; writable edits never
  trigger regeneration. Applications sequence rendering, regeneration and sampling.
  Pixel-view lifetimes and sampling rules are owned by the public headers.
