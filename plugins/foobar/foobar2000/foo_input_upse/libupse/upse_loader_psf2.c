/*
 * UPSE: the unix playstation sound emulator.
 *
 * Filename: upse_loader_psf2.c
 * Purpose: libupse: PSF2/MiniPSF2 loader.
 *
 * Copyright (c) 2008 William Pitcock <nenolod@dereferenced.org>
 *
 * UPSE is free software, released under the GNU General Public License,
 * version 2.
 *
 * A copy of the GNU General Public License, version 2, is included in
 * the UPSE source kit as COPYING.
 *
 * UPSE is offered without any warranty of any kind, explicit or implicit.
 */

#include "upse-internal.h"

/* kang'd from binutils... *sigh* */
#define ELF32_R_SYM(val)  ((val) >> 8)
#define ELF32_R_TYPE(val) ((val) & 0xff)

static u32 initialPC, initialSP;
static u32 loadAddr;

static char *_upse_resolve_path(const char *f, const char *newfile)
{
    static char *ret;
    char *tp1;

#if PSS_STYLE==1
    tp1 = ((char *) strrchr(f, '/'));
#else
    tp1 = ((char *) strrchr(f, '\\'));
#if PSS_STYLE!=3
    {
        char *tp3;

        tp3 = ((char *) strrchr(f, '/'));
        if (tp1 < tp3)
            tp1 = tp3;
    }
#endif
#endif
    if (!tp1)
    {
        ret = malloc(strlen(newfile) + 1);
        strcpy(ret, newfile);
    }
    else
    {
        ret = malloc(tp1 - f + 2 + strlen(newfile));    // 1(NULL), 1(/).
        memcpy(ret, f, tp1 - f);
        ret[tp1 - f] = '/';
        ret[tp1 - f + 1] = 0;
        strcat(ret, newfile);
    }
    return (ret);
}

static u32
upse_psf2_parse_filesystem(upse_filesystem_t *ret, char *curdir, u8 *filesys, u8 *start, u32 len)
{
    int numfiles, i, j;
    u8 *cptr;
    u32 offs, uncomp, bsize, cofs, uofs;
    u32 X;
    uLongf dlength;
    int uerr;
    u32 tlen;
    char tfn[4096];
    u32 buflen = (16 * 1024 * 1024);
    u8 *buf = calloc(sizeof(u8), buflen);

    tlen = len;
    cptr = start + 4;
    numfiles = start[0] | start[1]<<8 | start[2]<<16 | start[3]<<24;

    _DEBUG("beginning parsing of filesystem @%p", filesys);

    for (i = 0; i < numfiles; i++)
    {
         offs = cptr[36] | cptr[37]<<8 | cptr[38]<<16 | cptr[39]<<24;
         uncomp = cptr[40] | cptr[41]<<8 | cptr[42]<<16 | cptr[43]<<24;
         bsize = cptr[44] | cptr[45]<<8 | cptr[46]<<16 | cptr[47]<<24;

         if (bsize > 0)
         {
              X = (uncomp + bsize - 1) / bsize;

              _DEBUG("file %s [filesystem @%p], %d block(s) (size %d, uncompressed %d)", cptr, filesys, X, bsize, uncomp);

              cofs = (offs + (X*4));
              uofs = 0;
              for (j = 0; j < X; j++)
              {
                  u32 usize = filesys[offs+(j*4)] | filesys[offs+1+(j*4)]<<8 | filesys[offs+2+(j*4)]<<16 | filesys[offs+3+(j*4)]<<24;
                  dlength = buflen - uofs;

                  _DEBUG("uncompressing block %d of %d (dlength %d, start %d, size %d)", j + 1, X, dlength, cofs, usize);
                  uerr = uncompress(&buf[uofs], &dlength, &filesys[cofs], usize);
                  if (uerr != Z_OK)
                  {
                      _WARN("uncompress failed, uerr:%d, buf:%s", uerr, &buf[uofs]);
                      return 0;
                  }

                  cofs += usize;
                  uofs += dlength;
              }

#ifndef WIN32_MSC
              snprintf(tfn, 4096, "%s/%s", curdir, cptr);
#else
			  _snprintf_s(tfn, 4095, 4095, "%s/%s", curdir, cptr);
#endif
              upse_filesystem_attach_path(ret, tfn, buf, uncomp);
         }
         else
         {
              char lcurdir[4096];
              _DEBUG("new subdirectory [filesystem @%p]: %s %d %d %d", filesys, cptr, offs, uncomp, bsize);

              strncpy(lcurdir, curdir, 4096);
              strncat(lcurdir, "/", 4096);
              strncat(lcurdir, (char *) cptr, 4096);

              _DEBUG("parsing directory %s", lcurdir);
              upse_psf2_parse_filesystem(ret, lcurdir, filesys, &filesys[offs], len);
         }

         cptr += 48;
    }

    return (len - (cptr - filesys));
}

/* this is sure going to be fun...
 * parse a PS2 elf image and return the initial PC addr.
 */
u32
upse_parse_psf2_elf(upse_module_instance_t *ins, u8 *start, u32 len)
{
    u32 entry, phoff, shoff, phentsize, shentsize, phnum, shnum, shstrndx;
    u32 name, type, flags, addr, offset, size, shent;
    u32 totallen;
    int i, rec;

    /*
     * Make sure the load address is valid.  The rest of this will be messed
     * up otherwise... and bad rips can cause bad load addresses...
     */
    if (loadAddr & 3)
    {
        loadAddr &= ~3;
        loadAddr += 4;
    }

    if (memcmp("ELF", start + 1, 3))
    {
        _DEBUG("Not an ELF file!");
        return 0xffffffff;
    }

    /*
     * Ah, ELF.  One of USL's most vile inventions, later enhanced by SCO and
     * Sun has been holding UNIX back for years.  And then SONY goes and uses
     * it on the Playstation 2 and Playstation 3.  Boy, what were they thinking?
     *
     * ELF is the inspircd of binary formats.  It's everything COFF isn't.
     * Bloated, complex and poorly documented with lots of undocumented side
     * effects.  Oh, and guess what!  SONY added more nonsense ontop like the 
     * .iopmod section which is some sort of codesigning verification that we
     * don't actually care about (and hey, neither does the console.)
     *
     * Leave it to SONY to move from something sensible like COFF to a full blown
     * executable format that causes 800% overhead on a SGI IRIX box.
     */
    entry = start[24] | start[25] << 8 | start[26] << 16 | start[27] << 24;
    phoff = start[28] | start[29] << 8 | start[30] << 16 | start[31] << 24;
    shoff = start[32] | start[33] << 8 | start[34] << 16 | start[35] << 24;

    _DEBUG("entry: %08x phoff %08x shoff %08x", entry, phoff, shoff);

    phentsize = start[42] | start[43] << 8;
    phnum = start[44] | start[45] << 8;
    shentsize = start[46] | start[47] << 8;
    shnum = start[48] | start[49] << 8;
    shstrndx = start[50] | start[51] << 8;

    _DEBUG("phentsize %08x phnum %d shentsize %08x shnum %d shstrndx %d", phentsize, phnum, shentsize, shnum, shstrndx);

    shent = shoff;
    totallen = 0;

    for (i = 0; i < shnum; i++)
    {
        name = start[shent] | start[shent+1] << 8 | start[shent+2] << 16 | start[shent+3] << 24;
        type = start[shent+4] | start[shent+5] << 8 | start[shent+6] << 16 | start[shent+7] << 24;
        flags = start[shent+8] | start[shent+9] << 8 | start[shent+10] << 16 | start[shent+11] << 24;
        addr = start[shent+12] | start[shent+13] << 8 | start[shent+14] << 16 | start[shent+15] << 24;
        offset = start[shent+16] | start[shent+17] << 8 | start[shent+18] << 16 | start[shent+19] << 24;
        size = start[shent+20] | start[shent+21] << 8 | start[shent+22] << 16 | start[shent+23] << 24;

        switch (type)
        {
        case 0:
            break;

        case 1:
            upse_ps1_memory_load(ins, loadAddr + addr, size, start + offset);
            totallen += size;
            break;

        case 2:
        case 3:
            break;

        case 8:
            upse_ps1_memory_clear(ins, loadAddr + addr, size);
            totallen += size;
            break;

        case 9:
            for (rec = 0; rec < (size/8); rec++)
            {
                u32 offs, info, target, temp, val, vallo;
                static u32 hi16offs = 0, hi16target = 0;

                offs = start[offset+(rec*8)] | start[offset+1+(rec*8)]<<8 | start[offset+2+(rec*8)]<<16 | start[offset+3+(rec*8)]<<24;
                info = start[offset+4+(rec*8)] | start[offset+5+(rec*8)]<<8 | start[offset+6+(rec*8)]<<16 | start[offset+7+(rec*8)]<<24;

                target = loadAddr + offs;

                _DEBUG("[%04d] offs %08x type %02x info %08x => %08x", rec, offs, ELF32_R_TYPE(info), ELF32_R_SYM(info), target);

                /*
                 * MIPS relocation types include some very exotic ones:
                 *
                 * 2 - standard 32-bit ELF relocation; just bump us to loadAddr * 2.
                 * 4 - 26-bit ELF relocation (yeah... I know...) = fun stuff.
                 * 5 - highest 16 bits only.
                 * 6 - lowest 16 bits only.
                 */
                switch (ELF32_R_TYPE(info))
                {
                case 2:
                    target += loadAddr;
                    break;

                case 4:
                    temp = (target & 0x03ffffff);
                    target &= 0xfc000000;
                    temp += (loadAddr>>2);
                    target |= temp;
                    break;

                case 5:
                    hi16offs = offs;
                    hi16target = target;
                    break;

                case 6:
                    vallo = ((target & 0xffff) ^ 0x8000) - 0x8000;

                    val = ((hi16target & 0xffff) << 16) + vallo;
                    val += loadAddr;
                    val = ((val >> 16) + ((val & 0x8000) != 0)) & 0xffff;

                    hi16target = (hi16target & ~0xffff) | val;

                    val = loadAddr + vallo;
                    target = (target & ~0xffff) | (val & 0xffff);

                    PSXMu32(ins, loadAddr + hi16offs) = BFLIP32(hi16target);
                    break;

                default:
                    _ERROR("unknown PS2-ELF relocation type: %d.", ELF32_R_TYPE(info));
                    return 0xffffffff;
                }
            }
            break;

            /* XXX: .iopmod section: we may want to validate the modules here. */
            case 0x70000080:
                 break;

            default:
                 _DEBUG("unknown PS2-ELF section type: %d.", type);
                 break;
        }

        shent += shentsize;
    }

    entry += loadAddr;
    entry |= 0x80000000;
    loadAddr += totallen;

    _DEBUG("entrypoint is %08x", entry);

    return entry;
}

upse_module_t *
upse_load_psf2(void *fp, const char *path, const upse_iofuncs_t *iofuncs)
{
    upse_psf_t *psfi;
    upse_xsf_t *xsf;
    u8 *in, *out, *buf = NULL;
    u32 inlen, buflen;
    u64 outlen;
    upse_filesystem_t *fs;
    upse_module_t *ret;
    char curdir[4096] = "";
    upse_module_instance_t *ins;

    ret = calloc(sizeof(upse_module_t), 1);
    ins = &ret->instance;

    /* XXX: this is a magic value in HE.  who knows where Neill got it from... */
    loadAddr = 0x23f00;

    /* get and decode the data. */
    in = upse_get_buffer(fp, iofuncs, &inlen);
    xsf = upse_xsf_decode(in, inlen, &out, &outlen);

    if (outlen > 0)
        return NULL;

    /* parse the reserved section into a upse_filesystem object. */
    _DEBUG("filesystem length %d bytes", xsf->res_size);
    fs = upse_filesystem_new();
    upse_psf2_parse_filesystem(fs, curdir, (u8 *) xsf->res_section, (u8 *) xsf->res_section, xsf->res_size);
    _DEBUG("lib: %s, libaux[0]: %s", xsf->lib, xsf->libaux[0]);
    if (*xsf->lib != '\0')
    {
        upse_xsf_t *lib;
        void *fplib;
        u8 *inlib, *outlib;
        u32 inliblen;
        u64 outliblen;
        char *tmpfn;
        char lcurdir[4096] = "";

        tmpfn = _upse_resolve_path(path, xsf->lib);
        _DEBUG("opening %s", tmpfn);
        fplib = iofuncs->open_impl(tmpfn, "rb");

        inlib = upse_get_buffer(fplib, iofuncs, &inliblen);
        lib = upse_xsf_decode(inlib, inliblen, &outlib, &outliblen);

        _DEBUG("subfilesystem length %d bytes", lib->res_size);
        upse_psf2_parse_filesystem(fs, lcurdir, (u8 *) lib->res_section, (u8 *) lib->res_section, lib->res_size);

        free(inlib);
        free(outlib);
        free(lib);
    }

    free(in);
    free(out);

    /* find and load psf2.irx. */
    upse_filesystem_get_path(fs, "/psf2.irx", &buf, &buflen);
    if (buf == NULL)
        return NULL;

    upse_ps1_init(ins);
    upse_ps1_reset(ins, UPSE_PSX_REV_PS2_IOP);

    initialPC = upse_parse_psf2_elf(&ret->instance, buf, buflen);
    initialSP = 0x801ffff0;

    ins->cpustate.pc = initialPC;
    ins->cpustate.GPR.n.sp = initialSP;
    ins->cpustate.GPR.n.ra = 0x80000000;

    /* set up argc and argv */
    ins->cpustate.GPR.n.a0 = 2;
    ins->cpustate.GPR.n.a1 = 0x80000004;

    /* we're running upse:/psf2.irx, so our device prefix is upse:/. */
    strcpy((char *) PSXM(ins, ins->cpustate.GPR.n.a1), "upse:/psf2.irx");

    /* fill out our metadata struct. */
    psfi = calloc(sizeof(upse_psf_t), 1);
    psfi->xsf = xsf;
    psfi->volume = upse_strtof(xsf->inf_volume) * 32;
    psfi->fade = upse_time_to_ms(xsf->inf_fade);
    psfi->stop = upse_time_to_ms(xsf->inf_length);
    psfi->title = xsf->inf_title;
    psfi->artist = xsf->inf_artist;
    psfi->copyright = xsf->inf_copy;
    psfi->game = xsf->inf_game;
    psfi->year = xsf->inf_year;

    upse_ps1_spu_setvolume(ret->instance.spu, psfi->volume);
    upse_ps1_spu_setlength(ret->instance.spu, psfi->stop, psfi->fade);
    psfi->length = psfi->stop + psfi->fade;
    psfi->rate = 44100;

    ret->metadata = psfi;
    ret->opaque = fs;
    ret->evloop_run = upse_r3000_cpu_execute;
    ret->evloop_stop = upse_ps1_spu_stop;
    ret->evloop_render = upse_r3000_cpu_execute_render;
    ret->evloop_setcb = upse_ps1_spu_set_audio_callback;
    ret->evloop_seek = upse_ps1_spu_seek;

    return ret;
}
