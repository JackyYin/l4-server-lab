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

static Gelf_Sym * load_symbols(Elf_Data *sym_data, size_t *symbol_cnt)
{
    if (!sym_data || !symbol_cnt)
        return NULL;

    Gelf_Sym *sym = malloc(sym_data->d_size);
    if (sym) {
        for (int i = 0; i < sym_data->d_size/sizeof(Gelf_Sym); i++) {
            gelf_getsym(sym_data, i, &sym[i]);
        }
        *symbol_cnt = sym_data->d_size/sizeof(Gelf_Sym);
    }
    return sym;
}

static void get_router_symbols(Elf *elf, GElf_Ehdr *ehdr, int routerndx, GElf_Sym *symbols, size_t symbol_cnt)
{
    GElf_Shdr shdr;
    Elf_Scn *scn = elf_getscn(elf, routerndx);
    if (!scn)
        goto err;

    Elf_Data *data = elf_getdata(scn, NULL);
    if (!data)
        goto err;

    for (size_t i = 0; i < symbol_cnt; i++) {
        GElf_Sym symbol = symbols[i];

        if (symbol.st_shndx != routerndx)
            continue;

        char *symname = elf_strptr(elf, ehdr.shstrndx, symbol.st_name);
        LOG_INFO("symbol name: %s, symbol info: %d, symbol value: %lu, symbol size: %lu\n",symname, symbol.st_info, symbol.st_value, symbol.st_size);

        struct router *r = (struct router *)(data->d_buf + symbol.st_value);
        LOG_INFO("router path: %s, filepath: %s, method: %d\n", r->path, r->filepath, r->method);
    }
err:
}

static void load_router_via_elf()
{
    Elf *elf;
    GElf_Ehdr ehdr;
    GElf_Shdr shdr;
    GElf_Sym *symbols;
    int routerndx = -1;
    size_t symbol_cnt;

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
        if (!scn)
            continue;

        if (!gelf_getshdr(scn, &shdr))
            continue;

        if (shdr.sh_type == SHT_SYMTAB) {
            Elf_Data *data = elf_getdata(scn, NULL);
            symbols = load_symbols(data, symbol_cnt);
        } else {
            char *secname = elf_strptr(elf, ehdr.shstrndx, shdr.sh_name);
            if (secname && !strncmp(secname, "ROUTER", sizeof("ROUTER")))
                routerndx = i;
        }
    }

    if (routerndx >= 0 && symbols) {
        get_router_symbols(elf, &ehdr, routerndx, symbols, symbol_cnt);
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
