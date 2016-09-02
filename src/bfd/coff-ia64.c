/* BFD back-end for HP/Intel IA-64 COFF files.
   Copyright 1999 Free Software Foundation, Inc.
   Contributed by David Mosberger <davidm@hpl.hp.com>

This file is part of BFD, the Binary File Descriptor library.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "bfd.h"
#include "sysdep.h"
#include "libbfd.h"

#include "coff/ia64.h"

#include "coff/internal.h"

#include "coff/pe.h"

#include "libcoff.h"

#define COFF_DEFAULT_SECTION_ALIGNMENT_POWER (2)
/* The page size is a guess based on ELF.  */

#define COFF_PAGE_SIZE 0x1000

static reloc_howto_type howto_table[] =
{
  {0},
};

#define BADMAG(x) IA64BADMAG(x)
#define IA64 1			/* Customize coffcode.h */

#ifdef COFF_WITH_PEP64
# undef AOUTSZ
# define AOUTSZ		PEP64AOUTSZ
# define PEAOUTHDR	PEP64AOUTHDR
#endif

#define RTYPE2HOWTO(cache_ptr, dst) \
	    (cache_ptr)->howto = howto_table + (dst)->r_type;

#ifdef COFF_WITH_PE
/* Return true if this relocation should
   appear in the output .reloc section.  */

static boolean
in_reloc_p(abfd, howto)
     bfd * abfd;
     reloc_howto_type *howto;
{
  return 0;			/* We don't do relocs for now...  */
}
#endif

#include "coffcode.h"

static const bfd_target *
ia64coff_object_p (abfd)
     bfd *abfd;
{
#ifdef COFF_IMAGE_WITH_PE
  /* We need to hack badly to handle a PE image correctly.  In PE
     images created by the GNU linker, the offset to the COFF header
     is always the size.  However, this is not the case in images
     generated by other PE linkers.  The PE format stores a four byte
     offset to the PE signature just before the COFF header at
     location 0x3c of the file.  We pick up that offset, verify that
     the PE signature is there, and then set ourselves up to read in
     the COFF header.  */
  {
    bfd_byte ext_offset[4];
    file_ptr offset;
    bfd_byte ext_signature[4];
    unsigned long signature;

    if (bfd_seek (abfd, 0x3c, SEEK_SET) != 0
	|| bfd_read (ext_offset, 1, 4, abfd) != 4)
      {
	if (bfd_get_error () != bfd_error_system_call)
	  bfd_set_error (bfd_error_wrong_format);
	return NULL;
      }
    offset = bfd_h_get_32 (abfd, ext_offset);
    if (bfd_seek (abfd, offset, SEEK_SET) != 0
	|| bfd_read (ext_signature, 1, 4, abfd) != 4)
      {
	/* Most likely we failed because of a bogus (huge) offset */
	bfd_set_error (bfd_error_wrong_format);
	return NULL;
      }
    signature = bfd_h_get_32 (abfd, ext_signature);

    if (signature != 0x4550)
      {
	bfd_set_error (bfd_error_wrong_format);
	return NULL;
      }

    /* Here is the hack.  coff_object_p wants to read filhsz bytes to
       pick up the COFF header.  We adjust so that that will work.  20
       is the size of the COFF filehdr.  */

    if (bfd_seek (abfd,
		  (bfd_tell (abfd)
		   - bfd_coff_filhsz (abfd)
		   + 20),
		  SEEK_SET)
	!= 0)
      {
	if (bfd_get_error () != bfd_error_system_call)
	  bfd_set_error (bfd_error_wrong_format);
	return NULL;
      }
  }
#endif

  return coff_object_p (abfd);
}

const bfd_target
#ifdef TARGET_SYM
  TARGET_SYM =
#else
  ia64coff_vec =
#endif
{
#ifdef TARGET_NAME
  TARGET_NAME,
#else
  "coff-ia64",			/* name */
#endif
  bfd_target_coff_flavour,
  BFD_ENDIAN_LITTLE,		/* data byte order is little */
  BFD_ENDIAN_LITTLE,		/* header byte order is little */

  (HAS_RELOC | EXEC_P |		/* object flags */
   HAS_LINENO | HAS_DEBUG |
   HAS_SYMS | HAS_LOCALS | WP_TEXT | D_PAGED),

#ifndef COFF_WITH_PE
  (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD | SEC_RELOC /* section flags */
   | SEC_CODE | SEC_DATA),
#else
  (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD | SEC_RELOC /* section flags */
   | SEC_CODE | SEC_DATA
   | SEC_LINK_ONCE | SEC_LINK_DUPLICATES),
#endif

#ifdef TARGET_UNDERSCORE
  TARGET_UNDERSCORE,		/* leading underscore */
#else
  0,				/* leading underscore */
#endif
  '/',				/* ar_pad_char */
  15,				/* ar_max_namelen */

  bfd_getl64, bfd_getl_signed_64, bfd_putl64,
     bfd_getl32, bfd_getl_signed_32, bfd_putl32,
     bfd_getl16, bfd_getl_signed_16, bfd_putl16, /* data */
  bfd_getl64, bfd_getl_signed_64, bfd_putl64,
     bfd_getl32, bfd_getl_signed_32, bfd_putl32,
     bfd_getl16, bfd_getl_signed_16, bfd_putl16, /* hdrs */

/* Note that we allow an object file to be treated as a core file as well.  */
    {_bfd_dummy_target, ia64coff_object_p, /* bfd_check_format */
       bfd_generic_archive_p, ia64coff_object_p},
    {bfd_false, coff_mkobject, _bfd_generic_mkarchive, /* bfd_set_format */
       bfd_false},
    {bfd_false, coff_write_object_contents, /* bfd_write_contents */
       _bfd_write_archive_contents, bfd_false},

     BFD_JUMP_TABLE_GENERIC (coff),
     BFD_JUMP_TABLE_COPY (coff),
     BFD_JUMP_TABLE_CORE (_bfd_nocore),
     BFD_JUMP_TABLE_ARCHIVE (_bfd_archive_coff),
     BFD_JUMP_TABLE_SYMBOLS (coff),
     BFD_JUMP_TABLE_RELOCS (coff),
     BFD_JUMP_TABLE_WRITE (coff),
     BFD_JUMP_TABLE_LINK (coff),
     BFD_JUMP_TABLE_DYNAMIC (_bfd_nodynamic),

  NULL,

  COFF_SWAP_TABLE
};