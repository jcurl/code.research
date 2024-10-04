int main(int argc, char **argv) {
    asm volatile(
        "  cas w0, w1, [sp];"
        :
        :
        : "w0", "w1");
    return 0;
}
