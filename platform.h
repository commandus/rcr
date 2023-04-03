#ifdef _MSC_VER
#define SWAP_BYTES_2(x) htons(x)
#define SWAP_BYTES_4(x) htonl(x)
#define SWAP_BYTES_8(x) htonll(x)
#else
#define SWAP_BYTES_2(x) be16toh(x)
#define SWAP_BYTES_4(x) be32toh(x)
#define SWAP_BYTES_8(x) be64toh(x)
#endif
