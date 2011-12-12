// neighbor should favor low energy changes

// cooling schedule

// restarting to sbest/ebest periodically?  if too far from best?

#ifndef __ANNEAL_H
#define __ANNEAL_H

#include <limits>
#include <math.h>
#include <stdlib.h>

template <typename Solution, int RateI = 958>
class Anneal
{
public:
    Solution operator () (Solution &s) __attribute__((__flatten__))
    {
        double Rate = RateI / 1000.0;
        int per_temp = s.iterations();
        double temp = 2000.0;
        Solution best(s);
        double ebest = best.e();

        double e = s.e();
        while (temp >= 0.01) {
            for (int i = 0; i < per_temp; ++i) {
                s.neighbor();
                double enew = s.e();
                if (enew < ebest) {
                    best = s;
                    ebest = enew;
                }
                if (enew > e && exp((e - enew) / temp) * RAND_MAX < random())
                    s.undo();
                else
                    e = enew;
            }
            temp *= Rate;
        }
        return best;
    }
};

#endif /* __ANNEAL_H */
