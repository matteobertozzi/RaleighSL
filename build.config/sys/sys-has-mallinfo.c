#include <malloc.h>
#include <stdint.h>

int main (int argc, char **argv) {
	struct mallinfo mminfo = mallinfo();
	uint64_t arena    = mminfo.arena;        /* Non-mmapped space allocated (bytes) */
  uint64_t ordblks  = mminfo.ordblks;      /* Number of free chunks */
  uint64_t smblks   = mminfo.smblks;       /* Number of free fastbin blocks */
  uint64_t hblks    = mminfo.hblks;        /* Number of mmapped regions */
  uint64_t hblkhd   = mminfo.hblkhd;       /* Space allocated in mmapped regions (bytes) */
  uint64_t usmblks  = mminfo.usmblks;      /* Maximum total allocated space (bytes) */
  uint64_t fsmblks  = mminfo.fsmblks;      /* Space in freed fastbin blocks (bytes) */
  uint64_t uordblks = mminfo.uordblks;     /* Total allocated space (bytes) */
  uint64_t fordblks = mminfo.fordblks;     /* Total free space (bytes) */
  uint64_t keepcost = mminfo.keepcost;     /* Top-most, releasable space (bytes) */
	return(0);
}