#include "bitset.h"

#include "util.h"

void bitset_set_idx(Bitset* bitset, int idx, bool on)
{
    uint32_t i = idx / BITSET_BITS_PER_WORD;
    uint32_t b = idx % BITSET_BITS_PER_WORD;

    // Below are the "fast" forms of the above operations, respectively.
    // These are more efficient, but removed for readability
    // See: https://github.com/cellos51/balatro-gba/pull/132#discussion_r2365966071
    // Divide by 32 to get the word index
    // uint32_t i = idx >> 5;
    // Get last 5-bits, same as a modulo (% 32) operation on positive numbers
    // uint32_t b = idx & 0x1F;
    if (on)
    {
        bitset->w[i] |= (uint32_t)1 << b;
    }
    else
    {
        bitset->w[i] &= ~((uint32_t)1 << b);
    }
}

int bitset_set_next_free_idx(Bitset* bitset)
{
    for (uint32_t i = 0; i < bitset->nwords; i++)
    {
        uint32_t inv = ~bitset->w[i];

        // guard so we don't call `ctz` with 0, since __builtin_ctz(0) is undefined
        // https://gcc.gnu.org/onlinedocs/gcc/Bit-Operation-Builtins.html#index-_005f_005fbuiltin_005fctz
        //
        // By using the bitwise inverse of the word, you can skip words that are full
        // quickly (where the value is 0 or 'false' since all bits are '1', or 'in use').  Any value
        // greater than 0 indicates there is a free slot. Then, when counting the trailing 0's, you
        // can test very quickly where the first free slot is. This operation prevents looping
        // through every bit of filled flags, and will instead operate only on the first word with
        // free slots.
        if (inv)
        {
            int bit = __builtin_ctz(inv);
            bitset->w[i] |= ((uint32_t)1 << bit);
            int idx = i * BITSET_BITS_PER_WORD + bit;
            return (idx < bitset->cap) ? idx : UNDEFINED;
        }
    }

    return UNDEFINED;
}

void bitset_clear(Bitset* bitset)
{
    for (int i = 0; i < bitset->nwords; i++)
    {
        bitset->w[i] = 0;
    }
}

bool bitset_is_empty(Bitset* bitset)
{
    for (int i = 0; i < bitset->nwords; i++)
    {
        if (bitset->w[i])
            return false;
    }
    return true;
}

bool bitset_get_idx(Bitset* bitset, int idx)
{
    uint32_t i = idx / BITSET_BITS_PER_WORD;
    uint32_t b = idx % BITSET_BITS_PER_WORD;

    return bitset->w[i] & (uint32_t)1 << b;
}

int bitset_num_set_bits(Bitset* bitset)
{
    int sum = 0;

    for (int i = 0; i < bitset->nwords; i++)
    {
        sum += __builtin_popcount(bitset->w[i]);
    }

    return sum;
}

int bitset_find_idx_of_nth_set(const Bitset* bitset, int n)
{
    int tracker = 0;
    int prev_tracker = 0;

    for (int i = 0; i < bitset->nwords; i++)
    {
        tracker += __builtin_popcount(bitset->w[i]);

        if (tracker > n)
        {
            // The index is here somewhere
            // this one is to count the 1's not the offset, underflow to -1 is good for finding the
            // 0 index
            int base = prev_tracker - 1;
            // this one is for the actual offset we want to map the id to
            int offset = bitset->nbits * i;
            for (int j = 0; j < bitset->nbits; j++)
            {
                if (base == n)
                {
                    return offset - 1;
                }
                base += (bitset->w[i] >> j) & 0x01;
                offset++;
            }

            break;
        }

        prev_tracker = tracker;
    }

    return UNDEFINED;
}

BitsetItr bitset_itr_create(const Bitset* bitset)
{
    BitsetItr itr = {
        .bitset = bitset,
        .word = 0,
        .bit = 0,
        .itr = 0,
    };

    return itr;
}

int bitset_itr_next(BitsetItr* itr)
{
    // So, worst case scenario for this is one bit at the end of the last
    // word in the bitset. You would look (32 * 7) + 31 times!
    // This can be sped up with by checking if the word is empty first.
    // Then the worst enemy of this method would be something like a set bit at the end
    // of every word. In that case you would need to loop 31 times maximum.
    // So one last thing you could do is something like `bitset_allocate_idx` does with the
    // __builtin_ctz function as well.
    //
    // The point being, this can be very slow, but it's simple and can be much faster.
    for (; itr->word < itr->bitset->nwords; itr->word++)
    {
        for (; itr->bit < itr->bitset->nbits; itr->bit++)
        {
            itr->itr++;
            if (itr->bitset->w[itr->word] & (1 << itr->bit))
            {
                // if itr->bit == nbits on the next run, the for loop will handle it
                itr->bit++;
                // above we always make it one more than it is
                // it's so we can return without mutating the actual iterator
                // once it gets here. Just subtract one
                return itr->itr - 1;
            }
        }
        itr->bit = 0;
    }
    itr->word = 0;

    return UNDEFINED;
}
