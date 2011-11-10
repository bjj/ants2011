// neighbor should favor low energy changes

// cooling schedule

// restarting to sbest/ebest periodically?  if too far from best?

#include <limits>
#include <math.h>
#include <stdlib.h>

template <typename Solution>
class Anneal
{
public:
    Solution operator () (Solution &s)
    {
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
                if (enew > e && exp((e - enew) / temp) < drand48())
                    s.undo();
                else
                    e = enew;
            }
            temp *= 0.96;
        }
        return best;
    }
};
