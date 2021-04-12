import random
import collections
import functools

# I hardcoded DIM into sparse_ccl_debug, be careful here
DIM = 65535

POINTS = 1500

# at home : /home/sylvain/Desktop/traccc-local
PATH_CSV_OUTPUT = "your-local-path-here/traccc/data/ccl_test_gen_data.csv"

if __name__ == "__main__":
    points = collections.defaultdict(float)

    for _ in range(POINTS):
        x, y = random.randint(0, DIM), random.randint(0, DIM)
        val = random.uniform(5.0, 10.0)
        points[(x, y)] += val
        point_set = {(x, y)}

        for __ in range(random.randint(1, 10)):
            p = random.choice([
                (x, y)
                for (x, y) in functools.reduce(
                    set.union,
                    (
                        {(x, y + 1), (x, y - 1), (x + 1, y), (x - 1, y)}
                        for (x, y) in point_set
                    )
                )
                if (x, y) not in point_set and x > 0 and x < DIM and y > 0 and y < DIM
            ])

            points[p] += random.uniform(0.05, val)
            point_set.add(p)

    with open(PATH_CSV_OUTPUT, "w") as f:
        f.write("geometry_id,hit_id,channel0,channel1,timestamp,value\n")
        for ((x, y), v) in points.items():
            f.write("0, 0, %d, %d, 0, %f\n" % (x, y, v))