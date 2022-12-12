#include <fcntl.h>
#include <libelf.h>
#include <gelf.h>
#include "common.h"
#include "syscall.h"
#include "router.h"

static int get_self_fd()
{
    return __open("/proc/self/exe", O_RDONLY);
}

static void load_router_via_elf()
{
    Elf *elf;
    GElf_Ehdr ehdr;
    GElf_Shdr shdr;

    int fd = get_self_fd();
    if (fd < 0)
        goto err;

    elf = elf_begin(fd, ELF_C_READ, NULL);
    if (!elf)
        goto err;


    if (!gelf_getehdr(elf, &ehdr))
        goto err;

    for (size_t i = 0; i < ehdr.e_shnum; i++) {
        Elf_Scn *scn = elf_getscn(elf, i);
        Elf_Data *data = NULL;
        if (!scn)
            continue;

        if (!gelf_getshdr(scn, &shdr))
            continue;

        char *secname = elf_strptr(elf, ehdr.shstrndx, shdr.sh_name);
        if (secname && !strncmp(secname, "ROUTER", sizeof("ROUTER"))) {
            while ((data = elf_getdata(scn, data))) {
                if (!data->d_buf)
                    continue;

                for (int j = 0; j < data->d_size / sizeof(struct router); j++) {
                    struct router *base = (struct router *)data->d_buf + j;
                    fprintf(stdout, "base: %p\n", base);
                    fprintf(stdout, "path: %s, filepath: %s, method: %d\n", base->path,  base->filepath, base->method);
                }
            }
        }
    }

close:
    __close(fd);
err:
}

const struct router *find_router(const char *path, int method)
{
    const struct router *iter;

    load_router_via_elf();
    FOREACH_ROUTER(iter)
    {
        if ((method & iter->method) && strcmp(path, iter->path) == 0) {
            return iter;
        }
    }
    return NULL;
}
