#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SETUP_SIZE 4096
#define SYSSIZE_OFFSET 0x1f4
#define PAYLOAD_LENGTH_OFFSET 0x24c
#define INIT_SIZE_OFFSET 0x260

static void put_le32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static uint8_t *read_file(const char *path, size_t *size) {
    FILE *file = fopen(path, "rb");
    uint8_t *data;
    long length;

    if (!file || fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0 ||
        fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "cannot read %s: %s\n", path, strerror(errno));
        exit(1);
    }
    data = malloc((size_t)length);
    if (!data || fread(data, 1, (size_t)length, file) != (size_t)length) {
        fprintf(stderr, "cannot read %s\n", path);
        exit(1);
    }
    fclose(file);
    *size = (size_t)length;
    return data;
}

int main(int argc, char **argv) {
    uint8_t *setup;
    uint8_t *payload;
    size_t setup_size;
    size_t payload_size;
    unsigned long init_size;
    FILE *output;

    if (argc != 5) {
        fprintf(stderr, "usage: %s SETUP PAYLOAD INIT_SIZE OUTPUT\n", argv[0]);
        return 2;
    }
    setup = read_file(argv[1], &setup_size);
    payload = read_file(argv[2], &payload_size);
    init_size = strtoul(argv[3], NULL, 0);
    if (setup_size != SETUP_SIZE || payload_size > UINT32_MAX ||
        init_size < payload_size || init_size > UINT32_MAX) {
        fprintf(stderr, "invalid Linux image sizes: setup=%zu payload=%zu init=%lu\n",
                setup_size, payload_size, init_size);
        return 1;
    }

    put_le32(setup + SYSSIZE_OFFSET,
             (uint32_t)((payload_size + 15) / 16));
    put_le32(setup + PAYLOAD_LENGTH_OFFSET, (uint32_t)payload_size);
    put_le32(setup + INIT_SIZE_OFFSET, (uint32_t)init_size);

    output = fopen(argv[4], "wb");
    if (!output || fwrite(setup, 1, setup_size, output) != setup_size ||
        fwrite(payload, 1, payload_size, output) != payload_size ||
        fclose(output) != 0) {
        fprintf(stderr, "cannot write %s: %s\n", argv[4], strerror(errno));
        return 1;
    }
    free(setup);
    free(payload);
    return 0;
}

