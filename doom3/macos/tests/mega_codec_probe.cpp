#include <cstdio>
#include <vector>
std::vector<byte> tile(FILE *f, int level, int x, int y) {
    int base = 0, axis = 256;
    for (int i = 0; i < level; i++) {
        base += axis * axis;
        axis /= 2;
    }
    int offset, size;
    fseek(f, 56 + (base + y * axis + x) * 8, SEEK_SET);
    fread(&offset, 4, 1, f);
    fread(&size, 4, 1, f);
    std::vector<byte> out(size + 3);
    fseek(f, offset, SEEK_SET);
    fread(out.data(), 1, out.size(), f);
    return out;
}
int main(int argc, char **argv) {
    FILE *f = fopen(argv[1], "rb");
    if (!f)
        return 1;
    idBareDctDecoder d;
    int count = 0;
    double largest = 0;
    for (int y = 16; y < 48; y += 8)
        for (int x = 16; x < 56; x += 8) {
            auto p = tile(f, 3, x / 2, y / 2);
            auto c = tile(f, 2, x, y);
            byte parent[65536], child[65536];
            d.SetQuality(p[0], p[1], p[2]);
            bool ok = d.DecompressImageYCoCg_Generic(p.data() + 3, parent, 128, 128, p.size() - 3);
            MegaTextureUpscale2xBicubic(parent, 64, 64, 128 * 4, child);
            double before = 0, after = 0;
            for (int i = 0; i < 16384; i++)
                before += child[i * 4];
            d.SetQuality(c[0], c[1], c[2]);
            ok &= d.DecompressLuminanceEnhancement_Generic(c.data() + 3, child, 128, 128, c.size() - 3);
            for (int i = 0; i < 16384; i++)
                after += child[i * 4];
            double drift = fabs(after - before) / 16384;
            largest = std::max(largest, drift);
            if (!ok || drift > 15) {
                printf("FAIL tile %d,%d: mean Y %.2f -> %.2f\n", x, y, before / 16384, after / 16384);
                return 2;
            }
            count++;
        }
    printf("PASS %d real Valley luminance tiles; maximum mean drift %.3f/255\n", count, largest);
    fclose(f);
}
