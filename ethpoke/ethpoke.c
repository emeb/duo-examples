#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define ETH_BASE_ADDRESS 0x03009000  // Base address of ETH registers on Duo

// Offsets for ETH registers
#define ETH_070 0x070
#define ETH_074 0x074
#define ETH_078 0x078
#define ETH_07C 0x07C
#define ETH_800 0x800
#define ETH_804 0x804
#define ETH_808 0x806

// word access macro
#define word(x) (x / sizeof(unsigned int))

int main() {
    int mem_fd;
    void *reg_map;

    // Open /dev/mem
    if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
        perror("can't open /dev/mem");
        return 1;
    }

    // Map registers to user space memory
    reg_map = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, ETH_BASE_ADDRESS);

    if (reg_map == MAP_FAILED) {
        perror("mmap error");
        close(mem_fd);
        return 1;
    }

    // Access ETH registers
    volatile unsigned int *eth_reg = (volatile unsigned int *)reg_map;
    unsigned int temp;

    // rg_ephy_apb_rw_sel=1, use apb interface
    temp = eth_reg[word(ETH_804)];
    printf("READ 804: 0x%08X\n", temp);
    temp &= ~1;
    eth_reg[word(ETH_804)] = temp;
    printf("WRITE 804: 0x%08X\n", temp);

    // rg_ephy_pll_stable_cnt[4:0] = 5'd1 (10us)
    temp = eth_reg[word(ETH_808)];
    printf("READ 808: 0x%08X\n", temp);
    temp &= ~0x1F;
    temp |= 0x01;
    eth_reg[word(ETH_808)] = temp;
    printf("WRITE 808: 0x%08X\n", temp);

    // rg_ephy_dig_rst_n=1, reset release, other keep default
    temp = eth_reg[word(ETH_800)];
    printf("READ 800: 0x%08X\n", temp);
    temp = 0x0905;
    eth_reg[word(ETH_800)] = temp;
    printf("WRITE 808: 0x%08X\n", temp);
    
    // Delay 10us
    usleep(10);

    // page_sel_mode0 = page 5
    temp = eth_reg[word(ETH_07C)];
    printf("READ 07C: 0x%08X\n", temp);
    temp &= ~(0x1F << 8);
    temp |= (0x05 << 8);
    eth_reg[word(ETH_07C)] = temp;
    printf("WRITE 07C: 0x%08X\n", temp);
    
    // page_sel_mode0 = page 5
    temp = eth_reg[word(ETH_07C)];
    printf("READ 07C: 0x%08X\n", temp);
    temp &= ~(0x1F << 8);
    temp |= (0x05 << 8);
    eth_reg[word(ETH_07C)] = temp;
    printf("WRITE 07C: 0x%08X\n", temp);

    // Unmap memory
    munmap(reg_map, 4096);

    // Close /dev/mem
    close(mem_fd);

    return 0;
}
