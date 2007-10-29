
#define NUM_INTEREST_BITS 320 //number of bits per userid

#if 1
//32 bit
#define INTEREST_WORD_SIZE 32
#define INTEREST_TYPE uint32_t
#define INTEREST_SHIFT 5
#define INTEREST_MASK 31

#else
//64 bit
#define INTEREST_WORD_SIZE 64
#define INTEREST_TYPE uint64_t
#define INTEREST_SHIFT 6
#define INTEREST_MASK 63

#endif

#define INTEREST_WORDS NUM_INTEREST_BITS / INTEREST_WORD_SIZE //number of words of interests per user


#define CHECK_INTEREST(userid, interestid) \
	(( *(userinterestlist + (userid * INTEREST_WORDS) + (interestid >> INTEREST_SHIFT))) & (1 << (interestid & INTEREST_MASK)))

#define SET_INTEREST(userid, interestid) \
	( *(userinterestlist + ((userid) * INTEREST_WORDS) + ((interestid) >> INTEREST_SHIFT))) |= (1 << ((interestid) & INTEREST_MASK))

#define UNSET_INTEREST(userid, interestid) \
	( *(userinterestlist + ((userid) * INTEREST_WORDS) + ((interestid) >> INTEREST_SHIFT))) &= ~(1 << ((interestid) & INTEREST_MASK))

#define CLEAR_INTEREST(userid) \
	int clear_interest_counter; \
	for(clear_interest_counter = 0; clear_interest_counter < INTEREST_WORDS; clear_interest_counter++) \
		( *(userinterestlist + ((userid) * INTEREST_WORDS) + clear_interest_counter) = 0;




