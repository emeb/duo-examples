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

#define GPIO_MUX_BASE_ADDRESS 0x03001000

// Offsets for GPIO MUX registers
#define PIN_47 0xc0
#define PIN_48 0xc4
#define PIN_49 0xc8
#define PIN_50 0xcc

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
	
	printf("Switching ETH pins to GPIO\n");

    // Map ETH registers to user space memory
    reg_map = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, ETH_BASE_ADDRESS);

    if (reg_map == MAP_FAILED) {
        perror("mmap error");
        close(mem_fd);
        return 1;
    }

    // Access ETH registers
    volatile unsigned int *eth_reg = (volatile unsigned int *)reg_map;
    unsigned int temp;

    // 0x03009804[0] = 1'b1 (rg_ephy_apb_rw_sel=1, use apb interface)
    temp = eth_reg[word(ETH_804)];
    printf("READ 804: 0x%08X\n", temp);
    temp |= 0x1;
    eth_reg[word(ETH_804)] = temp;
    printf("WRITE 804: 0x%08X\n", temp);

    // 0x03009808[4:0] = 5'b00001 (rg_ephy_pll_stable_cnt[4:0] = 5'd1 (10us))
    temp = eth_reg[word(ETH_808)];
    printf("READ 808: 0x%08X\n", temp);
    temp &= ~0x1F;
    temp |= 0x01;
    eth_reg[word(ETH_808)] = temp;
    printf("WRITE 808: 0x%08X\n", temp);

    // 0x03009800[2] = 0x0905 (rg_ephy_dig_rst_n=1, reset release, other keep default)
    temp = eth_reg[word(ETH_800)];
    printf("READ 800: 0x%08X\n", temp);
    temp = 0x0905;
    eth_reg[word(ETH_800)] = temp;
    printf("WRITE 808: 0x%08X\n", temp);
    
    // Delay 10us
    usleep(10);

    // 0x0300907C[12:8]= 5'b00101 (page_sel_mode0 = page 5)
    temp = eth_reg[word(ETH_07C)];
    printf("READ 07C: 0x%08X\n", temp);
    temp &= ~(0x1F << 8);
    temp |= (0x05 << 8);
    eth_reg[word(ETH_07C)] = temp;
    printf("WRITE 07C: 0x%08X\n", temp);
    
    // 0x03009078[11:0] = 0xF00 (set to gpio from top)
    temp = eth_reg[word(ETH_078)];
    printf("READ 078: 0x%08X\n", temp);
    temp &= ~0xFFF;
    temp |= 0xF00;
    eth_reg[word(ETH_078)] = temp;
    printf("WRITE 078: 0x%08X\n", temp);

    // 0x03009074[10:9 2:1]= 0x606 (set ephy rxp&rxm input&output enable)
    temp = eth_reg[word(ETH_074)];
    printf("READ 074: 0x%08X\n", temp);
    temp |= 0x606;
    eth_reg[word(ETH_074)] = temp;
    printf("WRITE 074: 0x%08X\n", temp);

    // 0x03009070[10:9 2:1]= 0x606 (set ephy rxp&rxm input&output enable)
    temp = eth_reg[word(ETH_070)];
    printf("READ 070: 0x%08X\n", temp);
    temp |= 0x606;
    eth_reg[word(ETH_070)] = temp;
    printf("WRITE 070: 0x%08X\n", temp);

    // Unmap memory
    munmap(reg_map, 4096);
	
	printf("Switching GPIO to I2S\n");

    // Map GPIO MUX registers to user space memory
    reg_map = mmap(NULL, 512, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_MUX_BASE_ADDRESS);

    if (reg_map == MAP_FAILED) {
        perror("mmap error on GPIO MUX");
        close(mem_fd);
        return 1;
    }

    // Access GPIO MUX registers
    volatile unsigned int *mux_reg = (volatile unsigned int *)reg_map;
	
    // set 0x030010c0 to 7 for I2S_LRCK
    temp = mux_reg[word(PIN_47)];
    printf("READ PIN 47: 0x%08X\n", temp);
    temp = 0x7;
    eth_reg[word(PIN_47)] = temp;
    printf("WRITE PIN 47: 0x%08X\n", temp);

    // set 0x030010c4 to 7 for I2S_BCLK
    temp = mux_reg[word(PIN_48)];
    printf("READ PIN 48: 0x%08X\n", temp);
    temp = 0x7;
    eth_reg[word(PIN_48)] = temp;
    printf("WRITE PIN 48: 0x%08X\n", temp);

    // set 0x030010c8 to 7 for I2S_SDI
    temp = mux_reg[word(PIN_49)];
    printf("READ PIN 49: 0x%08X\n", temp);
    temp = 0x7;
    eth_reg[word(PIN_49)] = temp;
    printf("WRITE PIN 49: 0x%08X\n", temp);

    // set 0x030010cc to 7 for I2S_SDO
    temp = mux_reg[word(PIN_50)];
    printf("READ PIN 50: 0x%08X\n", temp);
    temp = 0x7;
    eth_reg[word(PIN_50)] = temp;
    printf("WRITE PIN 50: 0x%08X\n", temp);

    // Unmap memory
    munmap(reg_map, 512);
	
    // Close /dev/mem
    close(mem_fd);

    return 0;
}