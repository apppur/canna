#pragma once

class CaRandom
{
    public:
        CaRandom() {m_seed  = 0;}
        ~CaRandom() {}

        void SetSeed(unsigned int seed)
        {
            m_seed = seed;
        }

        unsigned int Random()
        {
            unsigned int next = m_seed;
            unsigned int result;

            next *= 1103515245;
            next += 12345;
            result = (unsigned int)(next >> 16) % 2048;

            next *= 1103515245;
            next += 12345;
            result <<= 10;
            result ^= (unsigned int)(next >> 16) % 1024;

            next *= 1103515245;
            next += 12345;
            result <<= 10;
            result ^= (unsigned int)(next >> 16) % 1024;

            m_seed = next;

            return result;
        }

        // return [0, range - 1]
        unsigned int Random(unsigned int range)
        {
            if (range == 0)
                return 0;
            return Random() % range;
        }

        // return [min, max - 1]
        unsigned int Random(unsigned int min, unsigned int max)
        {
            if (min == max)
                return max;
            return Random(max - min) + min;
        }
    private:
        unsigned int m_seed;
};
